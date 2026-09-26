//! Standalone Rage Racer multiplayer server (see ../docs/multiplayer.md).
//!
//! First working milestone: exactly two human seats, one hardcoded course,
//! stepped by the real retail simulation (`rage-sim`) from network input.
//! There is no lobby/room list/SQLite yet (docs/multiplayer.md step 2's
//! full scope); this proves step 3, "two humans in one room", end to end.
//!
//! Usage: rage-racer-server <path to Rage Racer.cue> [port]

mod sim;

use sim::{Race, RaceArchive, SeatPlan};
use std::io::{ErrorKind, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::atomic::{AtomicI32, AtomicI64, AtomicU8, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

const SEAT_COUNT: usize = 2;
const TICK_HZ: u64 = sim::SIM_TICK_RATE as u64;
const COUNTDOWN_TICKS: u32 = 3 * sim::SIM_TICK_RATE as u32;
const LAPS: i32 = 3;
const CAR_MODEL_VARIANT: i32 = 0; // Grade-0 Erriso for both seats, for now.

// Wire protocol (see docs/multiplayer.md, "Wire messages"). Small and
// binary, one TCP connection per client, little-endian throughout.
mod wire {
    pub const C2S_HELLO: u8 = 0x01;
    pub const C2S_INPUT: u8 = 0x02;

    pub const S2C_WELCOME: u8 = 0x81;
    pub const S2C_START: u8 = 0x82;
    pub const S2C_SNAPSHOT: u8 = 0x83;
    pub const S2C_RESULT: u8 = 0x84;
}

/// Latest input received from one client, applied at the next physics tick.
/// Steering angle/throttle/brake are stored raw; gear edges are separate
/// atomics so a rising edge received between ticks is not lost the way a
/// plain "latest value" would lose it (matching RaceSim's own edge queue,
/// duplicated here only for the network hop).
struct SharedInput {
    steering_mode: AtomicU8,
    steering_left: AtomicU8,
    steering_right: AtomicU8,
    steering_angle: AtomicI32,
    throttle: AtomicI32,
    brake: AtomicI32,
    shift_up: AtomicU8,
    shift_down: AtomicU8,
    connected: AtomicU8,
    last_seen: AtomicI64,
}

impl Default for SharedInput {
    fn default() -> Self {
        SharedInput {
            steering_mode: AtomicU8::new(0),
            steering_left: AtomicU8::new(0),
            steering_right: AtomicU8::new(0),
            steering_angle: AtomicI32::new(0),
            throttle: AtomicI32::new(0),
            brake: AtomicI32::new(0),
            shift_up: AtomicU8::new(0),
            shift_down: AtomicU8::new(0),
            connected: AtomicU8::new(0),
            last_seen: AtomicI64::new(0),
        }
    }
}

fn read_exact_or_none(stream: &mut TcpStream, buf: &mut [u8]) -> Option<()> {
    match stream.read_exact(buf) {
        Ok(()) => Some(()),
        Err(e) if e.kind() == ErrorKind::UnexpectedEof => None,
        Err(e) => {
            eprintln!("rage-racer-server: read error: {e}");
            None
        }
    }
}

/// Reads one client's Hello, then its Input messages forever, updating the
/// shared latest-input state. Runs on its own thread per connection so the
/// race-step loop is never blocked on network I/O.
fn client_reader(mut stream: TcpStream, input: Arc<SharedInput>) {
    let mut header = [0u8; 1];
    loop {
        if read_exact_or_none(&mut stream, &mut header).is_none() {
            input.connected.store(0, Ordering::Relaxed);
            return;
        }
        match header[0] {
            wire::C2S_HELLO => {
                let mut len_buf = [0u8; 1];
                if read_exact_or_none(&mut stream, &mut len_buf).is_none() {
                    return;
                }
                let mut name = vec![0u8; len_buf[0] as usize];
                if read_exact_or_none(&mut stream, &mut name).is_none() {
                    return;
                }
                let name = String::from_utf8_lossy(&name).to_string();
                println!("rage-racer-server: hello from '{name}'");
                input.connected.store(1, Ordering::Relaxed);
            }
            wire::C2S_INPUT => {
                let mut body = [0u8; 12];
                if read_exact_or_none(&mut stream, &mut body).is_none() {
                    input.connected.store(0, Ordering::Relaxed);
                    return;
                }
                input.steering_mode.store(body[0], Ordering::Relaxed);
                input.steering_left.store(body[1], Ordering::Relaxed);
                input.steering_right.store(body[2], Ordering::Relaxed);
                let angle = i16::from_le_bytes([body[3], body[4]]) as i32;
                input.steering_angle.store(angle, Ordering::Relaxed);
                let throttle = i16::from_le_bytes([body[5], body[6]]) as i32;
                input.throttle.store(throttle, Ordering::Relaxed);
                let brake = i16::from_le_bytes([body[7], body[8]]) as i32;
                input.brake.store(brake, Ordering::Relaxed);
                input.shift_up.store(body[9], Ordering::Relaxed);
                input.shift_down.store(body[10], Ordering::Relaxed);
                input.last_seen.store(now_millis(), Ordering::Relaxed);
            }
            other => {
                eprintln!("rage-racer-server: unknown message type {other:#x}, dropping client");
                return;
            }
        }
    }
}

fn now_millis() -> i64 {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_millis() as i64)
        .unwrap_or(0)
}

fn write_all_or_drop(stream: &mut TcpStream, buf: &[u8]) -> bool {
    match stream.write_all(buf) {
        Ok(()) => true,
        Err(e) => {
            eprintln!("rage-racer-server: write error: {e}");
            false
        }
    }
}

fn accept_seat(listener: &TcpListener, seat: u8) -> TcpStream {
    loop {
        match listener.accept() {
            Ok((mut stream, addr)) => {
                stream.set_nodelay(true).ok();
                println!("rage-racer-server: seat {seat} connected from {addr}");
                let mut welcome = [0u8; 2];
                welcome[0] = wire::S2C_WELCOME;
                welcome[1] = seat;
                if write_all_or_drop(&mut stream, &welcome) {
                    return stream;
                }
                println!("rage-racer-server: seat {seat} dropped before welcome, waiting again");
            }
            Err(e) => eprintln!("rage-racer-server: accept error: {e}"),
        }
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("usage: {} <path to Rage Racer.cue> [port]", args[0]);
        std::process::exit(1);
    }
    let disc_path = &args[1];
    let port: u16 = args.get(2).and_then(|p| p.parse().ok()).unwrap_or(7878);

    println!("rage-racer-server: loading {disc_path}");
    let archive = RaceArchive::load_disc(disc_path)
        .unwrap_or_else(|| panic!("rage-racer-server: failed to load disc/archive at {disc_path}"));
    println!("rage-racer-server: archive loaded");

    let listener = TcpListener::bind(("0.0.0.0", port)).expect("failed to bind TCP listener");
    println!("rage-racer-server: listening on port {port}, waiting for {SEAT_COUNT} players");

    let inputs: Vec<Arc<SharedInput>> = (0..SEAT_COUNT).map(|_| Arc::new(SharedInput::default())).collect();
    let mut streams = Vec::with_capacity(SEAT_COUNT);
    for seat in 0..SEAT_COUNT {
        let stream = accept_seat(&listener, seat as u8);
        let reader_stream = stream.try_clone().expect("failed to clone client stream");
        let input = inputs[seat].clone();
        std::thread::spawn(move || client_reader(reader_stream, input));
        streams.push(Mutex::new(stream));
    }
    println!("rage-racer-server: all seats connected, starting race");

    let track = archive
        .copy_track(0, 0)
        .expect("failed to copy track physics for class 0 course 0");
    let seats: Vec<SeatPlan> = (0..SEAT_COUNT)
        .map(|i| SeatPlan::Human {
            model: CAR_MODEL_VARIANT,
            manual: false,
            seed: 0x1234_5670 + i as u32,
        })
        .collect();
    let mut race = Race::new(&archive, track, &seats, LAPS, false)
        .expect("failed to initialize race grid");
    race.start(COUNTDOWN_TICKS);
    println!("rage-racer-server: race started, {LAPS} laps");

    for (seat, stream) in streams.iter().enumerate() {
        let mut guard = stream.lock().unwrap();
        let mut msg = [0u8; 7];
        msg[0] = wire::S2C_START;
        msg[1] = LAPS as u8;
        msg[2] = 0; // reverse
        msg[3..7].copy_from_slice(&COUNTDOWN_TICKS.to_le_bytes());
        if !write_all_or_drop(&mut guard, &msg) {
            eprintln!("rage-racer-server: seat {seat} failed to receive start");
        }
    }

    let tick_duration = Duration::from_millis(1000 / TICK_HZ);
    let mut next_tick = Instant::now();
    let mut seat_alive = [true; SEAT_COUNT];
    loop {
        for (seat, input) in inputs.iter().enumerate() {
            race.set_input(
                seat as i32,
                input.steering_mode.load(Ordering::Relaxed) as u32,
                input.steering_left.load(Ordering::Relaxed) != 0,
                input.steering_right.load(Ordering::Relaxed) != 0,
                input.steering_angle.load(Ordering::Relaxed),
                input.throttle.load(Ordering::Relaxed) as i16,
                input.brake.load(Ordering::Relaxed) as i16,
                input.shift_up.swap(0, Ordering::Relaxed) != 0,
                input.shift_down.swap(0, Ordering::Relaxed) != 0,
            );
        }
        race.step();

        let mut snapshot = Vec::with_capacity(1 + 4 + 1 + SEAT_COUNT * 17);
        snapshot.push(wire::S2C_SNAPSHOT);
        snapshot.extend_from_slice(&race.tick().to_le_bytes());
        snapshot.push(race.phase() as u8);
        for seat in 0..SEAT_COUNT {
            let pose = race.pose(seat);
            snapshot.push(pose.active as u8);
            snapshot.extend_from_slice(&pose.x.to_le_bytes());
            snapshot.extend_from_slice(&pose.y.to_le_bytes());
            snapshot.extend_from_slice(&pose.z.to_le_bytes());
            snapshot.extend_from_slice(&pose.yaw.to_le_bytes());
        }
        for (seat, stream) in streams.iter().enumerate() {
            if !seat_alive[seat] {
                continue;
            }
            let mut guard = stream.lock().unwrap();
            if !write_all_or_drop(&mut guard, &snapshot) {
                seat_alive[seat] = false;
                eprintln!("rage-racer-server: seat {seat} disconnected, no longer sending it snapshots");
            }
        }
        if !seat_alive.iter().any(|alive| *alive) {
            println!("rage-racer-server: every seat disconnected, stopping");
            break;
        }

        if race.is_finished() {
            println!("rage-racer-server: race finished at tick {}", race.tick());
            for (seat, stream) in streams.iter().enumerate() {
                if !seat_alive[seat] {
                    continue;
                }
                let mut guard = stream.lock().unwrap();
                write_all_or_drop(&mut guard, &[wire::S2C_RESULT]);
            }
            break;
        }

        next_tick += tick_duration;
        let now = Instant::now();
        if next_tick > now {
            std::thread::sleep(next_tick - now);
        } else {
            next_tick = now;
        }
    }
}
