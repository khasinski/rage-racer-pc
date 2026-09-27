//! Standalone Rage Racer multiplayer server (see ../docs/multiplayer.md).
//!
//! Prototype milestone: exactly two human seats, one hardcoded course,
//! stepped by the real retail simulation (`rage-sim`) from network input.
//! There is no lobby/room list/SQLite yet (docs/multiplayer.md step 2's
//! full scope). Playable client integration and end-to-end verification remain pending.
//!
//! Usage: rage-racer-server <CUE, Track 01 BIN or cache directory> [port] [--reimport]

mod sim;
mod cache;

use sim::{Race, RaceArchive, SeatPlan};
use std::io::{ErrorKind, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

const PROTOCOL_VERSION: u8 = 9;
const SEAT_COUNT: usize = 2;
const TICK_HZ: u64 = sim::SIM_TICK_RATE as u64;

#[derive(Clone, Copy)]
struct Seat { model: i32, manual: bool, seed: u32 }

#[derive(Clone, Copy)]
struct Plan {
    class: i32, course: i32, laps: i32, reverse: bool,
    countdown: u32,
    seats: [Seat; SEAT_COUNT],
}

impl Default for Plan {
    fn default() -> Self {
        Self { class: 0, course: 0, laps: 3, reverse: false,
            countdown: 3 * sim::SIM_TICK_RATE as u32,
            seats: std::array::from_fn(|seat| Seat {
                model: 0, manual: false, seed: 0x1234_5670 + seat as u32,
            }) }
    }
}

impl Plan {
    fn valid(&self) -> bool {
        (0..6).contains(&self.class) && (0..4).contains(&self.course) &&
            (1..=sim::PLAYER_LAP_TIME_CAPACITY as i32).contains(&self.laps) &&
            self.seats.iter().all(|seat| (0..sim::CAR_MODEL_VARIANT_COUNT as i32).contains(&seat.model))
    }
}

// Wire protocol (see docs/multiplayer.md, "Wire messages"). Small and
// binary, one TCP connection per client, little-endian throughout.
mod wire {
    pub const C2S_HELLO: u8 = 0x01;
    pub const C2S_INPUT: u8 = 0x02;
    pub const C2S_LOADED: u8 = 0x03;

    pub const S2C_WELCOME: u8 = 0x81;
    pub const S2C_START: u8 = 0x82;
    pub const S2C_SNAPSHOT: u8 = 0x83;
    pub const S2C_RESULT: u8 = 0x84;
}

/* Keep the race on its original time axis after a delayed step. Moving the
 * deadline to now on every overrun permanently loses simulation time. */
fn advance_deadline(next: &mut Instant, now: Instant) -> Duration {
    *next += Duration::from_nanos(1_000_000_000 / TICK_HZ);
    next.saturating_duration_since(now)
}

fn start_message(plan: &Plan, boot: [u8; 16], fingerprint: u64, executable: u64) -> Option<Vec<u8>> {
    if !plan.valid() { return None; }
    let mut message = vec![wire::S2C_START, PROTOCOL_VERSION, plan.course as u8,
        plan.class as u8, plan.laps as u8, plan.reverse as u8];
    message.extend_from_slice(&plan.countdown.to_le_bytes());
    message.push(SEAT_COUNT as u8);
    message.extend_from_slice(&boot);
    for seat in &plan.seats {
        message.extend_from_slice(&[seat.model as u8, seat.manual as u8]);
        message.extend_from_slice(&seat.seed.to_le_bytes());
    }
    message.extend_from_slice(&fingerprint.to_le_bytes());
    message.extend_from_slice(&executable.to_le_bytes());
    Some(message)
}

fn append_pose(message: &mut Vec<u8>, pose: sim::CarPose) {
    message.push(pose.status);
    for value in [pose.x, pose.y, pose.z, pose.yaw, pose.pitch, pose.roll,
                  pose.steering, pose.wheels, pose.brake, pose.progress, pose.rpm, pose.throttle, pose.clutch, pose.gear, pose.ground, pose.roll_speed] {
        message.extend_from_slice(&value.to_le_bytes());
    }
}

fn append_result(message: &mut Vec<u8>, finished: bool, place: u8, milliseconds: i32) {
    message.extend_from_slice(&[finished as u8, place]);
    message.extend_from_slice(&milliseconds.to_le_bytes());
}

fn result_message(race: &Race) -> Vec<u8> {
    let mut message = vec![wire::S2C_RESULT];
    for seat in 0..SEAT_COUNT {
        let (finished, place, time) = race.result(seat);
        append_result(&mut message, finished, place, time);
    }
    message
}

/// One coherent packet plus pending gear edges. The network reader publishes
/// all controls under the same lock; the tick consumes both edges together.
#[derive(Default)]
struct Input {
    packet: [u8; 12],
    greeted: bool,
    loading: bool,
    loaded: bool,
    closed: bool,
}

#[derive(Default)]
struct SharedInput {
    state: Mutex<Input>,
    ready: std::sync::Condvar,
}

impl SharedInput {
    fn greet(&self) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.closed || current.greeted { return false; }
        current.greeted = true;
        self.ready.notify_all();
        true
    }

    fn wait_hello(&self, timeout: Duration) -> bool {
        let current = self.state.lock().unwrap();
        let (current, _) = self.ready.wait_timeout_while(current, timeout,
            |state| !state.greeted && !state.closed).unwrap();
        current.greeted && !current.closed
    }

    fn begin_load(&self) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.closed || !current.greeted || current.loading { return false; }
        current.loading = true;
        true
    }

    fn loaded(&self) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.closed || !current.loading || current.loaded { return false; }
        current.loaded = true;
        self.ready.notify_all();
        true
    }

    fn wait_loaded(&self, timeout: Duration) -> bool {
        let current = self.state.lock().unwrap();
        let (current, _) = self.ready.wait_timeout_while(current, timeout,
            |state| !state.loaded && !state.closed).unwrap();
        current.loaded && !current.closed
    }

    fn publish(&self, mut packet: [u8; 12]) -> bool {
        if sim::decode_input(&packet).is_none() {
            return false;
        }
        let mut current = self.state.lock().unwrap();
        if current.closed {
            return false;
        }
        packet[9] |= current.packet[9];
        packet[10] |= current.packet[10];
        current.packet = packet;
        true
    }

    fn take(&self) -> Option<[u8; 12]> {
        let mut current = self.state.lock().unwrap();
        if current.closed {
            return None;
        }
        let packet = current.packet;
        current.packet[9] = 0;
        current.packet[10] = 0;
        Some(packet)
    }

    fn close(&self) {
        let mut current = self.state.lock().unwrap();
        current.closed = true;
        current.packet = [0; 12];
        self.ready.notify_all();
    }
}

/// At most one unsent snapshot per connection. Tick publication never waits
/// for socket I/O; a finish marker follows the final retained snapshot.
#[derive(Default)]
struct Output {
    snapshot: Option<Arc<[u8]>>,
    result: Option<Arc<[u8]>>,
    closed: bool,
}

#[derive(Default)]
struct Outbox {
    state: Mutex<Output>,
    ready: std::sync::Condvar,
}

impl Outbox {
    fn publish(&self, snapshot: Arc<[u8]>) {
        let mut state = self.state.lock().unwrap();
        if state.closed || state.result.is_some() { return; }
        state.snapshot = Some(snapshot);
        self.ready.notify_one();
    }

    fn finish(&self, result: Arc<[u8]>) {
        let mut state = self.state.lock().unwrap();
        if state.closed || state.result.is_some() { return; }
        state.result = Some(result);
        self.ready.notify_one();
    }

    fn close(&self) {
        let mut state = self.state.lock().unwrap();
        state.closed = true;
        state.snapshot = None;
        state.result = None;
        self.ready.notify_one();
    }

    fn take(&self) -> Option<Arc<[u8]>> {
        let mut state = self.state.lock().unwrap();
        loop {
            if state.closed { return None; }
            if let Some(snapshot) = state.snapshot.take() { return Some(snapshot); }
            if let Some(result) = state.result.take() {
                state.closed = true;
                return Some(result);
            }
            state = self.ready.wait(state).unwrap();
        }
    }
}

fn client_writer(mut stream: TcpStream, input: Arc<SharedInput>, output: Arc<Outbox>) {
    write_client(&mut stream, &output);
    output.close();
    input.close();
    let _ = stream.shutdown(std::net::Shutdown::Both);
}

fn write_client<W: Write>(stream: &mut W, output: &Outbox) {
    while let Some(message) = output.take() {
        if !write_all_or_drop(stream, &message) { break; }
    }
}

fn read_exact_or_none<R: Read>(stream: &mut R, buf: &mut [u8]) -> Option<()> {
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
fn client_reader(mut stream: TcpStream, input: Arc<SharedInput>, output: Arc<Outbox>) {
    read_client(&mut stream, &input);
    input.close();
    output.close();
    let _ = stream.shutdown(std::net::Shutdown::Both);
}

fn read_client<R: Read>(stream: &mut R, input: &SharedInput) {
    let mut greeted = false;
    let mut header = [0u8; 1];
    loop {
        if read_exact_or_none(stream, &mut header).is_none() {
            return;
        }
        match header[0] {
            wire::C2S_HELLO if !greeted => {
                let mut len_buf = [0u8; 1];
                if read_exact_or_none(stream, &mut len_buf).is_none() {
                    return;
                }
                let length = len_buf[0] as usize;
                let mut name = [0u8; 15];
                if length > name.len() || read_exact_or_none(stream, &mut name[..length]).is_none() {
                    return;
                }
                let name = String::from_utf8_lossy(&name[..length]);
                if !input.greet() { return; }
                greeted = true;
                println!("rage-racer-server: hello from '{name}'");
            }
            wire::C2S_LOADED if greeted => {
                if !input.loaded() { return; }
            }
            wire::C2S_INPUT if greeted && input.wait_loaded(Duration::ZERO) => {
                let mut body = [0u8; 12];
                if read_exact_or_none(stream, &mut body).is_none() {
                    return;
                }
                if !input.publish(body) {
                    eprintln!("rage-racer-server: invalid driver input, dropping client");
                    return;
                }
            }
            other => {
                eprintln!("rage-racer-server: unexpected message type {other:#x}, dropping client");
                return;
            }
        }
    }
}

fn write_all_or_drop<W: Write>(stream: &mut W, buf: &[u8]) -> bool {
    match stream.write_all(buf) {
        Ok(()) => true,
        Err(e) => {
            eprintln!("rage-racer-server: write error: {e}");
            false
        }
    }
}

fn close_session(streams: &[TcpStream], inputs: &[Arc<SharedInput>], outputs: &[Arc<Outbox>]) {
    for input in inputs { input.close(); }
    for output in outputs { output.close(); }
    for stream in streams { let _ = stream.shutdown(std::net::Shutdown::Both); }
}

/* Own the connection lifetime, including partially started sessions. Drop
 * closes sockets before joining readers, so blocked reads cannot outlive it. */
struct Session {
    streams: Vec<TcpStream>,
    inputs: Vec<Arc<SharedInput>>,
    outputs: Vec<Arc<Outbox>>,
    readers: Vec<std::thread::JoinHandle<()>>,
    writers: Vec<std::thread::JoinHandle<()>>,
}

impl Session {
    fn new() -> Self {
        Self {
            streams: Vec::with_capacity(SEAT_COUNT),
            inputs: (0..SEAT_COUNT).map(|_| Arc::new(SharedInput::default())).collect(),
            outputs: (0..SEAT_COUNT).map(|_| Arc::new(Outbox::default())).collect(),
            readers: Vec::with_capacity(SEAT_COUNT),
            writers: Vec::with_capacity(SEAT_COUNT),
        }
    }

    fn finish_writers(&mut self) -> bool {
        let mut success = true;
        for writer in self.writers.drain(..) {
            if writer.join().is_err() {
                eprintln!("rage-racer-server: snapshot writer failed");
                success = false;
            }
        }
        success
    }
}

impl Drop for Session {
    fn drop(&mut self) {
        close_session(&self.streams, &self.inputs, &self.outputs);
        for worker in self.readers.drain(..).chain(self.writers.drain(..)) {
            if worker.join().is_err() { eprintln!("rage-racer-server: session worker failed"); }
        }
    }
}

fn accept_seat(listener: &TcpListener, seat: u8) -> std::io::Result<TcpStream> {
    loop {
        match listener.accept() {
            Ok((mut stream, addr)) => {
                stream.set_nodelay(true).ok();
                stream.set_write_timeout(Some(Duration::from_millis(100)))?;
                println!("rage-racer-server: seat {seat} connected from {addr}");
                let welcome = [wire::S2C_WELCOME, PROTOCOL_VERSION, seat];
                if write_all_or_drop(&mut stream, &welcome) {
                    return Ok(stream);
                }
                println!("rage-racer-server: seat {seat} dropped before welcome, waiting again");
            }
            Err(e) if e.kind() == ErrorKind::Interrupted => continue,
            Err(e) => return Err(e),
        }
    }
}

struct Start<'a> { source: &'a str, port: u16, reimport: bool }

fn parse_start(args: &[String]) -> Result<Start<'_>, &'static str> {
    let usage = "usage: rage-racer-server <CUE, Track 01 BIN or cache directory> [port] [--reimport]";
    let reimport = args.last().is_some_and(|arg| arg == "--reimport");
    let options = if reimport { &args[..args.len() - 1] } else { args };
    if options.is_empty() || options.len() > 2 || options[0].is_empty() { return Err(usage); }
    let port = match options.get(1) {
        None => 7878,
        Some(text) => {
            if text.is_empty() || !text.bytes().all(|byte| byte.is_ascii_digit()) {
                return Err("port must be an integer from 1 to 65535");
            }
            text.parse::<u16>().ok().filter(|port| *port != 0)
                .ok_or("port must be an integer from 1 to 65535")?
        }
    };
    Ok(Start { source: &options[0], port, reimport })
}

fn prepare_race(archive: &RaceArchive, plan: &Plan) -> Option<Race> {
    if !plan.valid() { return None; }
    if !archive.boot().contains(&0) || archive.boot()[0] == 0 || archive.executable() == 0 { return None; }
    let track = archive.copy_track(plan.class, plan.course)?;
    let seats: Vec<SeatPlan> = plan.seats.iter()
        .map(|seat| SeatPlan::Human {
            model: seat.model, manual: seat.manual, seed: seat.seed,
        }).collect();
    Race::new(archive, track, &seats, plan.laps, plan.reverse)
}

fn load_race(source: &str, reimport: bool, plan: &Plan) -> Result<(RaceArchive, Race), String> {
    if !plan.valid() { return Err("invalid race plan".into()); }
    let path = std::path::Path::new(source);
    let direct_cache = path.is_dir();
    if direct_cache && reimport {
        return Err("--reimport requires a CUE or Track 01 BIN source, not a cache directory".into());
    }
    let mut cache_name = path.as_os_str().to_os_string();
    cache_name.push(".server-cache");
    let destination = if direct_cache { path.to_path_buf() } else { std::path::PathBuf::from(cache_name) };
    if !reimport {
        if let Some(archive) = RaceArchive::load_cache(&destination) {
            if let Some(race) = prepare_race(&archive, plan) {
                println!("rage-racer-server: using validated cache {}", destination.display());
                return Ok((archive, race));
            }
        }
    }
    if direct_cache {
        return Err("invalid cache; select a CUE or Track 01 BIN to rebuild it".into());
    }
    println!("rage-racer-server: importing {source}");
    let archive = RaceArchive::load_disc(source)
        .ok_or_else(|| format!("failed to load CUE or Track 01 BIN at {source}"))?;
    let race = prepare_race(&archive, plan).ok_or("failed to prepare selected race from source data")?;
    archive.save_cache(&destination)
        .map_err(|error| format!("could not save cache {}: {error}", destination.display()))?;
    println!("rage-racer-server: data cached at {}", destination.display());
    Ok((archive, race))
}

fn main() -> std::process::ExitCode {
    let args: Vec<String> = std::env::args().collect();
    let start = match parse_start(&args[1..]) {
        Ok(options) => options,
        Err(error) => {
            eprintln!("rage-racer-server: {error}");
            return std::process::ExitCode::FAILURE;
        }
    };
    let plan = Plan::default();
    let (archive, mut race) = match load_race(start.source, start.reimport, &plan) {
        Ok(prepared) => prepared,
        Err(error) => {
            eprintln!("rage-racer-server: {error}");
            return std::process::ExitCode::FAILURE;
        }
    };
    let port = start.port;

    let listener = match TcpListener::bind(("0.0.0.0", port)) {
        Ok(listener) => listener,
        Err(error) => {
            eprintln!("rage-racer-server: cannot listen on port {port}: {error}");
            return std::process::ExitCode::FAILURE;
        }
    };
    println!("rage-racer-server: listening on port {port}, waiting for {SEAT_COUNT} players");

    let mut session = Session::new();
    for seat in 0..SEAT_COUNT {
        let stream = match accept_seat(&listener, seat as u8) {
            Ok(stream) => stream,
            Err(error) => {
                eprintln!("rage-racer-server: cannot accept seat {seat}: {error}");
                return std::process::ExitCode::FAILURE;
            }
        };
        let reader_stream = match stream.try_clone() {
            Ok(reader) => reader,
            Err(error) => {
                eprintln!("rage-racer-server: cannot clone seat {seat}: {error}");
                return std::process::ExitCode::FAILURE;
            }
        };
        let input = session.inputs[seat].clone();
        let output = session.outputs[seat].clone();
        session.streams.push(stream);
        let spawned = std::thread::Builder::new()
            .name(format!("seat-{seat}-reader"))
            .spawn(move || client_reader(reader_stream, input, output));
        match spawned {
            Ok(reader) => session.readers.push(reader),
            Err(error) => {
                eprintln!("rage-racer-server: cannot start seat {seat} reader: {error}");
                return std::process::ExitCode::FAILURE;
            }
        }
        /* Bound each hello before waiting for the next connection. A peer
         * that never greets must not occupy the first seat indefinitely. */
        if !session.inputs[seat].wait_hello(Duration::from_secs(5)) {
            eprintln!("rage-racer-server: seat {seat} did not complete hello, cancelling race");
            return std::process::ExitCode::FAILURE;
        }
    }
    println!("rage-racer-server: all seats greeted, starting race");

    let Some(msg) = start_message(&plan, archive.boot(), archive.fingerprint(), archive.executable()) else {
        eprintln!("rage-racer-server: invalid race plan");
        return std::process::ExitCode::FAILURE;
    };
    for (seat, stream) in session.streams.iter_mut().enumerate() {
        if !session.inputs[seat].begin_load() || !write_all_or_drop(stream, &msg) {
            session.inputs[seat].close();
            session.outputs[seat].close();
            let _ = stream.shutdown(std::net::Shutdown::Both);
            eprintln!("rage-racer-server: seat {seat} failed to receive start");
        }
    }

    let load_deadline = Instant::now() + Duration::from_secs(60);
    for input in &session.inputs {
        if !input.wait_loaded(load_deadline.saturating_duration_since(Instant::now())) {
            eprintln!("rage-racer-server: clients did not finish loading");
            return std::process::ExitCode::FAILURE;
        }
    }
    race.start(plan.countdown);
    println!("rage-racer-server: race started, {} laps", plan.laps);

    for (seat, stream) in session.streams.iter().enumerate() {
        let input = session.inputs[seat].clone();
        let output = session.outputs[seat].clone();
        let spawned = stream.try_clone().and_then(|stream| {
            std::thread::Builder::new().name(format!("seat-{seat}-writer"))
                .spawn(move || client_writer(stream, input, output))
        });
        match spawned {
            Ok(writer) => session.writers.push(writer),
            Err(error) => {
                eprintln!("rage-racer-server: cannot start seat {seat} writer: {error}");
                return std::process::ExitCode::FAILURE;
            }
        }
    }

    let mut next_tick = Instant::now();
    let mut seat_alive = [true; SEAT_COUNT];
    loop {
        for (seat, input) in session.inputs.iter().enumerate() {
            let Some(controls) = input.take().and_then(|packet| sim::decode_input(&packet)) else {
                if seat_alive[seat] {
                    seat_alive[seat] = false;
                    race.retire(seat as i32);
                }
                continue;
            };
            race.set_input(seat as i32, &controls);
        }
        race.step();

        let mut snapshot = Vec::with_capacity(1 + 4 + 1 + SEAT_COUNT * 65);
        snapshot.push(wire::S2C_SNAPSHOT);
        snapshot.extend_from_slice(&race.tick().to_le_bytes());
        snapshot.push(race.phase() as u8);
        for seat in 0..SEAT_COUNT {
            let pose = race.pose(seat);
            append_pose(&mut snapshot, pose);
        }
        let snapshot: Arc<[u8]> = snapshot.into();
        for (seat, output) in session.outputs.iter().enumerate() {
            if seat_alive[seat] { output.publish(snapshot.clone()); }
        }
        if !seat_alive.iter().any(|alive| *alive) {
            println!("rage-racer-server: every seat disconnected, stopping");
            break;
        }

        if race.is_finished() {
            println!("rage-racer-server: race finished at tick {}", race.tick());
            break;
        }

        let wait = advance_deadline(&mut next_tick, Instant::now());
        if !wait.is_zero() { std::thread::sleep(wait); }
    }
    for output in &session.outputs {
        if race.is_finished() { output.finish(result_message(&race).into()); } else { output.close(); }
    }
    let success = session.finish_writers();
    if success { std::process::ExitCode::SUCCESS } else { std::process::ExitCode::FAILURE }
}

#[cfg(test)]

mod tests {
    use super::{SharedInput, Outbox, wire};
    use std::sync::Arc;

    #[test]
    fn custom_plan_controls_start_metadata_and_rejects_invalid_selectors() {
        let mut plan = super::Plan { class: 4, course: 3, laps: 6, reverse: true,
            countdown: 25, ..Default::default() };
        plan.seats[0] = super::Seat { model: 9, manual: true, seed: 42 };
        plan.seats[1] = super::Seat { model: 31, manual: false, seed: 123 };
        let boot = *b"SCES_006.96\0\0\0\0\0";
        let bytes = super::start_message(&plan, boot, 1, 2).unwrap();
        assert_eq!(&bytes[2..6], &[3, 4, 6, 1]);
        assert_eq!(&bytes[6..10], &25u32.to_le_bytes());
        assert_eq!(&bytes[27..33], &[9, 1, 42, 0, 0, 0]);
        assert_eq!(&bytes[33..39], &[31, 0, 123, 0, 0, 0]);
        for invalid in [
            super::Plan { class: -1, ..plan }, super::Plan { class: 6, ..plan },
            super::Plan { course: -1, ..plan }, super::Plan { course: 4, ..plan },
            super::Plan { laps: 0, ..plan }, super::Plan { laps: 7, ..plan },
        ] {
            assert!(super::start_message(&invalid, boot, 1, 2).is_none());
            assert!(super::load_race("unused-source", false, &invalid).err().unwrap().contains("invalid race plan"));
        }
        for model in [-1, super::sim::CAR_MODEL_VARIANT_COUNT as i32] {
            plan.seats[1].model = model;
            assert!(!plan.valid());
            assert!(super::start_message(&plan, boot, 1, 2).is_none());
        }
    }

    #[test]
    fn session_drop_joins_waiting_workers_without_closing_another_session() {
        use std::sync::atomic::{AtomicUsize, Ordering};
        let mut first = super::Session::new();
        let second = super::Session::new();
        let observer = first.inputs[0].clone();
        let completed = Arc::new(AtomicUsize::new(0));
        let input = first.inputs[0].clone();
        let done = completed.clone();
        first.readers.push(std::thread::spawn(move || {
            assert!(!input.wait_loaded(std::time::Duration::from_secs(3)));
            done.fetch_add(1, Ordering::SeqCst);
        }));
        let output = first.outputs[0].clone();
        let done = completed.clone();
        first.writers.push(std::thread::spawn(move || {
            assert!(output.take().is_none());
            done.fetch_add(1, Ordering::SeqCst);
        }));
        drop(first);
        assert_eq!(completed.load(Ordering::SeqCst), 2);
        assert!(observer.take().is_none());
        assert!(second.inputs[0].publish([0; 12]));
        assert_eq!(second.inputs[0].take(), Some([0; 12]));
    }

    #[test]
    fn input_decoder_preserves_signed_angle_and_pedal_boundaries() {
        let mut packet = [0; 12];
        packet[0] = 1;
        packet[1] = 1;
        packet[3..5].copy_from_slice(&(-123i16).to_le_bytes());
        packet[5..7].copy_from_slice(&256i16.to_le_bytes());
        packet[9] = 1;
        let input = super::sim::decode_input(&packet).unwrap();
        assert_eq!(input.steering.mode, 1);
        assert_eq!(input.steering.left, 1);
        assert_eq!(input.steering.right, 0);
        assert_eq!(input.steering.angle, -123);
        assert_eq!(input.throttle, 256);
        assert_eq!(input.brake, 0);
        assert_eq!(input.shiftUp, 1);
        assert_eq!(input.shiftDown, 0);
        for pedal in [-1i16, 257, i16::MAX, i16::MIN] {
            packet[5..7].copy_from_slice(&pedal.to_le_bytes());
            assert!(super::sim::decode_input(&packet).is_none());
        }
        packet[5..7].copy_from_slice(&256i16.to_le_bytes());
        for reserved in 1..=255 {
            packet[11] = reserved;
            assert!(super::sim::decode_input(&packet).is_none());
        }
    }

    #[test]
    fn incomplete_hello_expires_without_starting_load() {
        let input = SharedInput::default();
        assert!(!input.wait_hello(std::time::Duration::from_millis(1)));
        assert!(!input.begin_load());
        input.close(); /* The session cancels after its per-seat deadline. */
        assert!(!input.greet());
        assert!(!input.begin_load());
        assert!(!input.wait_loaded(std::time::Duration::ZERO));
    }

    #[test]
    fn cancelling_session_clears_every_seat_and_pending_message() {
        let inputs: Vec<_> = (0..super::SEAT_COUNT)
            .map(|_| Arc::new(SharedInput::default())).collect();
        let outputs: Vec<_> = (0..super::SEAT_COUNT)
            .map(|_| Arc::new(Outbox::default())).collect();
        for (input, output) in inputs.iter().zip(&outputs) {
            let mut packet = [0; 12];
            packet[5] = 100;
            assert!(input.publish(packet));
            output.publish(Arc::from([1u8, 2, 3]));
        }
        super::close_session(&[], &inputs, &outputs);
        super::close_session(&[], &inputs, &outputs); /* Idempotent cancellation. */
        for (input, output) in inputs.iter().zip(&outputs) {
            assert_eq!(input.take(), None);
            assert!(!input.publish([0; 12]));
            assert!(!input.wait_hello(std::time::Duration::ZERO));
            assert!(!input.wait_loaded(std::time::Duration::ZERO));
            assert!(output.take().is_none());
        }
    }

    #[test]
    fn delayed_steps_catch_up_without_clock_drift() {
        use std::time::{Duration, Instant};
        let origin = Instant::now();
        let step = Duration::from_nanos(1_000_000_000 / super::TICK_HZ);
        let mut next = origin;
        assert_eq!(super::advance_deadline(&mut next, origin), step);
        let delayed = origin + step * 3 + Duration::from_millis(1);
        assert_eq!(super::advance_deadline(&mut next, delayed), Duration::ZERO);
        assert_eq!(super::advance_deadline(&mut next, delayed), Duration::ZERO);
        assert_eq!(super::advance_deadline(&mut next, delayed), step - Duration::from_millis(1));
        assert_eq!(next, origin + step * 4);
        for tick in 5..=1000 {
            let work_finished = origin + step * (tick - 1) + Duration::from_millis(3);
            assert_eq!(super::advance_deadline(&mut next, work_finished), step - Duration::from_millis(3));
        }
        assert_eq!(next, origin + step * 1000);
    }

    #[test]
    fn latest_controls_retain_unconsumed_shift_edges() {
        let input = SharedInput::default();
        assert_eq!(input.take(), Some([0; 12]));
        let mut first = [0; 12];
        first[5] = 100;
        first[9] = 1;
        input.publish(first);
        let mut second = [0; 12];
        second[5] = 200;
        second[10] = 1;
        input.publish(second);
        second[9] = 1;
        assert_eq!(input.take(), Some(second));
        second[9] = 0;
        second[10] = 0;
        assert_eq!(input.take(), Some(second));
        assert_eq!(input.take(), Some(second));
    }

    #[test]
    fn reader_and_tick_never_mix_control_packets() {
        let input = Arc::new(SharedInput::default());
        let producer = input.clone();
        let writer = std::thread::spawn(move || {
            for value in 1..=100_000u32 {
                let mut packet = [0; 12];
                let pedal = (value % 257) as i16;
                packet[3..5].copy_from_slice(&pedal.to_le_bytes());
                packet[5..7].copy_from_slice(&pedal.to_le_bytes());
                packet[7..9].copy_from_slice(&pedal.to_le_bytes());
                assert!(producer.publish(packet));
            }
        });
        for _ in 0..100_000 {
            let packet = input.take().unwrap();
            assert_eq!(packet[3..5], packet[5..7]);
            assert_eq!(packet[5..7], packet[7..9]);
            assert_eq!(packet[9], 0);
            assert_eq!(packet[10], 0);
        }
        writer.join().unwrap();
    }
    #[test]
    fn invalid_packet_preserves_controls_and_pending_edges() {
        let input = SharedInput::default();
        let mut valid = [0; 12];
        valid[5] = 200;
        valid[9] = 1;
        assert!(input.publish(valid));
        for (index, value) in [(0, 3), (1, 2), (2, 2), (6, 2), (8, 2), (9, 2), (10, 2), (11, 1)] {
            let mut invalid = valid;
            invalid[index] = value;
            assert!(!input.publish(invalid));
        }
        assert_eq!(input.take(), Some(valid));
    }

    #[test]
    fn closed_input_cannot_retain_throttle_or_accept_late_packets() {
        let input = SharedInput::default();
        let mut packet = [0; 12];
        packet[6] = 1;
        packet[9] = 1;
        assert!(input.publish(packet));
        input.close();
        input.close();
        assert_eq!(input.take(), None);
        assert!(!input.publish(packet));
        assert_eq!(input.take(), None);
        assert_eq!(input.state.lock().unwrap().packet, [0; 12]);
    }

    #[test]
    fn slow_writer_keeps_only_latest_snapshot_then_result() {
        let output = Outbox::default();
        for tick in 0..10_000u32 {
            output.publish(Arc::from(tick.to_le_bytes()));
        }
        output.finish(Arc::from([wire::S2C_RESULT]));
        output.publish(Arc::from([99u8])); // Finish cannot be overwritten.
        assert_eq!(&*output.take().unwrap(), &9999u32.to_le_bytes());
        assert_eq!(&*output.take().unwrap(), &[wire::S2C_RESULT]);
        assert!(output.take().is_none());
    }

    #[test]
    fn close_discards_pending_snapshot_and_wakes_writer() {
        let output = Arc::new(Outbox::default());
        let receiver = output.clone();
        let writer = std::thread::spawn(move || receiver.take());
        output.close();
        assert!(writer.join().unwrap().is_none());
        output.publish(Arc::from([1u8]));
        output.finish(Arc::from([wire::S2C_RESULT]));
        assert!(output.take().is_none());
        let pending = Outbox::default();
        pending.publish(Arc::from([2u8]));
        pending.close();
        assert!(pending.take().is_none());
    }

    #[test]
    fn blocked_writer_does_not_block_publication_or_accumulate_history() {
        use std::io::{self, Write};
        use std::sync::mpsc;
        use std::time::Duration;
        struct SlowWriter {
            started: mpsc::Sender<()>,
            release: mpsc::Receiver<()>,
            messages: Vec<Vec<u8>>,
        }
        impl Write for SlowWriter {
            fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
                if self.messages.is_empty() {
                    self.started.send(()).unwrap();
                    self.release.recv_timeout(Duration::from_secs(3)).unwrap();
                }
                self.messages.push(bytes.to_vec());
                Ok(bytes.len())
            }
            fn flush(&mut self) -> io::Result<()> { Ok(()) }
        }
        let output = Arc::new(Outbox::default());
        output.publish(Arc::from([1u8]));
        let receiver = output.clone();
        let (started_tx, started_rx) = mpsc::channel();
        let (release_tx, release_rx) = mpsc::channel();
        let writer = std::thread::spawn(move || {
            let mut sink = SlowWriter {
                started: started_tx, release: release_rx, messages: Vec::new(),
            };
            super::write_client(&mut sink, &receiver);
            sink.messages
        });
        started_rx.recv_timeout(Duration::from_secs(3)).unwrap();
        for tick in 0..10_000u32 {
            output.publish(Arc::from(tick.to_le_bytes()));
        }
        output.finish(Arc::from([wire::S2C_RESULT]));
        release_tx.send(()).unwrap();
        assert_eq!(writer.join().unwrap(), vec![
            vec![1], 9999u32.to_le_bytes().to_vec(), vec![wire::S2C_RESULT],
        ]);
    }

    #[test]
    fn reader_requires_one_complete_hello_before_input() {
        use std::io::Cursor;
        let mut packet = [0; 12];
        packet[6] = 1;
        packet[9] = 1;
        let mut valid = vec![wire::C2S_HELLO, 3, b'A', b'n', b'a', wire::C2S_LOADED, wire::C2S_INPUT];
        valid.extend_from_slice(&packet);
        for length in 0..valid.len() {
            let input = SharedInput::default();
            input.state.lock().unwrap().loading = true;
            super::read_client(&mut Cursor::new(&valid[..length]), &input);
            assert_eq!(input.take(), Some([0; 12]));
        }
        let input = SharedInput::default();
            input.state.lock().unwrap().loading = true;
        super::read_client(&mut Cursor::new(&valid), &input);
        assert_eq!(input.take(), Some(packet));
        let mut duplicate_after_input = valid.clone();
        duplicate_after_input.extend_from_slice(&[wire::C2S_HELLO, 0, wire::C2S_INPUT]);
        duplicate_after_input.extend_from_slice(&[0; 12]);
        let input = SharedInput::default();
            input.state.lock().unwrap().loading = true;
        super::read_client(&mut Cursor::new(duplicate_after_input), &input);
        assert_eq!(input.take(), Some(packet));

        let mut no_hello = vec![wire::C2S_INPUT];
        no_hello.extend_from_slice(&packet);
        for invalid in [no_hello, vec![wire::C2S_HELLO, 16], vec![255]] {
            let input = SharedInput::default();
            input.state.lock().unwrap().loading = true;
            super::read_client(&mut Cursor::new(invalid), &input);
            assert_eq!(input.take(), Some([0; 12]));
        }
        let mut duplicate = vec![wire::C2S_HELLO, 0, wire::C2S_HELLO, 0, wire::C2S_INPUT];
        duplicate.extend_from_slice(&packet);
        let input = SharedInput::default();
            input.state.lock().unwrap().loading = true;
        super::read_client(&mut Cursor::new(duplicate), &input);
        assert_eq!(input.take(), Some([0; 12]));
    }

    #[test]
    fn fragmented_reader_preserves_latest_levels_and_pending_edges() {
        use std::io::{self, Read, Cursor};
        struct ByteReader(Cursor<Vec<u8>>);
        impl Read for ByteReader {
            fn read(&mut self, bytes: &mut [u8]) -> io::Result<usize> {
                let size = bytes.len().min(1);
                self.0.read(&mut bytes[..size])
            }
        }
        let mut first = [0; 12];
        first[5] = 200;
        first[9] = 1;
        let mut second = [0; 12];
        second[7] = 100;
        second[10] = 1;
        let mut bytes = vec![wire::C2S_HELLO, 0, wire::C2S_LOADED, wire::C2S_INPUT];
        bytes.extend_from_slice(&first);
        bytes.push(wire::C2S_INPUT);
        bytes.extend_from_slice(&second);
        let input = SharedInput::default();
        input.state.lock().unwrap().loading = true;
        super::read_client(&mut ByteReader(Cursor::new(bytes)), &input);
        second[9] = 1;
        assert_eq!(input.take(), Some(second));
    }

    #[test]
    fn race_start_waits_for_hello_and_rejects_closed_seats() {
        use std::time::Duration;
        let input = SharedInput::default();
        assert!(!input.wait_hello(Duration::ZERO));
        assert!(input.greet());
        assert!(input.wait_hello(Duration::ZERO));
        assert!(!input.greet());
        input.close();
        assert!(!input.wait_hello(Duration::ZERO));
        assert!(!input.greet());

        for disconnect in [false, true] {
            let input = Arc::new(SharedInput::default());
            let receiver = input.clone();
            let waiter = std::thread::spawn(move || receiver.wait_hello(Duration::from_secs(3)));
            if disconnect { input.close(); } else { assert!(input.greet()); }
            assert_eq!(waiter.join().unwrap(), !disconnect);
        }
    }

    #[test]
    fn loaded_gate_requires_hello_and_wakes_on_load_or_close() {
        use std::time::Duration;
        assert!(!SharedInput::default().begin_load());
        assert!(!SharedInput::default().loaded());
        for load in [true, false] {
            let input = Arc::new(SharedInput::default());
            assert!(input.greet());
            assert!(!input.loaded());
            assert!(input.begin_load());
            assert!(!input.begin_load());
            assert!(!input.wait_loaded(Duration::ZERO));
            let receiver = input.clone();
            let waiter = std::thread::spawn(move || receiver.wait_loaded(Duration::from_secs(3)));
            if load {
                assert!(input.loaded());
                assert!(!input.loaded());
            } else {
                input.close();
                assert!(!input.loaded());
            }
            assert_eq!(waiter.join().unwrap(), load);
            input.close();
            assert!(!input.wait_loaded(Duration::ZERO));
        }
    }

    #[test]
    fn reader_rejects_input_before_loaded_and_duplicate_loaded() {
        use std::io::Cursor;
        let mut packet = [0; 12];
        packet[6] = 1;
        for prefix in [
            vec![wire::C2S_LOADED],
            vec![wire::C2S_HELLO, 0],
            vec![wire::C2S_HELLO, 0, wire::C2S_LOADED],
            vec![wire::C2S_HELLO, 0, wire::C2S_LOADED, wire::C2S_LOADED],
        ] {
            let arm = prefix.len() == 4;
            let mut bytes = prefix;
            bytes.push(wire::C2S_INPUT);
            bytes.extend_from_slice(&packet);
            let input = SharedInput::default();
            input.state.lock().unwrap().loading = arm;
            super::read_client(&mut Cursor::new(bytes), &input);
            assert_eq!(input.take(), Some([0; 12]));
        }
    }

    #[test]
    fn startup_arguments_require_an_explicit_valid_port() {
        let parse = |args: &[&str]| {
            let args: Vec<String> = args.iter().map(|arg| arg.to_string()).collect();
            super::parse_start(&args).map(|options| options.port)
        };
        assert_eq!(parse(&["disc.cue"]), Ok(7878));
        assert_eq!(parse(&["disc.cue", "--reimport"]), Ok(7878));
        assert_eq!(parse(&["disc.cue", "1234", "--reimport"]), Ok(1234));
        let forced = ["disc.cue".to_string(), "--reimport".to_string()];
        assert!(super::parse_start(&forced).unwrap().reimport);
        assert_eq!(parse(&["Track 01.bin", "1"]), Ok(1));
        assert_eq!(parse(&["disc.cue", "65535"]), Ok(65535));
        for port in ["", "0", "65536", "-1", "+1", "12x", " 7878", "1.5"] {
            assert!(parse(&["disc.cue", port]).is_err(), "accepted {port:?}");
        }
        for args in [vec![], vec![""], vec!["disc.cue", "7878", "extra"]] {
            assert!(parse(&args).is_err());
        }
    }

    #[test]
    fn incomplete_cached_field_requires_source_and_preserves_cache_on_import_failure() {
        let nonce = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap().as_nanos();
        let root = std::env::temp_dir().join(format!("rage-start-cache-{}-{nonce}", std::process::id()));
        std::fs::create_dir(&root).unwrap();
        let source = root.join("missing.bin");
        let cache = root.join("missing.bin.server-cache");
        super::cache::write(&cache, *b"SCES_006.96\0\0\0\0\0", 123, &vec![0; 135 * 8]).unwrap();
        let identity = std::fs::read(cache.join("identity")).unwrap();
        let error = super::load_race(source.to_str().unwrap(), false, &super::Plan::default()).err().unwrap();
        assert!(error.contains("failed to load CUE"));
        assert_eq!(std::fs::read(cache.join("identity")).unwrap(), identity);
        assert!(super::load_race(cache.to_str().unwrap(), false, &super::Plan::default()).err().unwrap().contains("invalid cache"));
        assert!(super::load_race(cache.to_str().unwrap(), true, &super::Plan::default()).err().unwrap().contains("requires a CUE"));
        std::fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn start_message_matches_c_client_fixture() {
        let boot = *b"SCES_006.96\0\0\0\0\0";
        let mut expected = vec![0x82, 9, 0, 0, 3, 0, 150, 0, 0, 0, 2];
        expected.extend_from_slice(&boot);
        expected.extend_from_slice(&[0, 0, 0x70, 0x56, 0x34, 0x12,
                                    0, 0, 0x71, 0x56, 0x34, 0x12]);
        expected.extend_from_slice(&[0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01]);
        expected.extend_from_slice(&[8, 7, 6, 5, 4, 3, 2, 1]);
        assert_eq!(super::start_message(&super::Plan::default(), boot, 0x0123456789ABCDEF, 0x0102030405060708), Some(expected));
    }

    #[test]
    fn presentation_pose_matches_c_decoder_layout() {
        let mut bytes = Vec::new();
        super::append_pose(&mut bytes, super::sim::CarPose {
            status: 1, pitch: -1, roll: 1, steering: -80,
            wheels: 4096, brake: 256, progress: 99, rpm: 8000, throttle: 256, clutch: 2, gear: 3, ground: -600, roll_speed: -7, ..Default::default()
        });
        let mut expected = vec![0; 65];
        expected[0] = 1;
        expected[17..21].copy_from_slice(&(-1i32).to_le_bytes());
        expected[21] = 1;
        expected[25..29].copy_from_slice(&(-80i32).to_le_bytes());
        expected[30] = 16;
        expected[34] = 1;
        expected[37] = 99;
        expected[41..45].copy_from_slice(&8000i32.to_le_bytes());
        expected[46] = 1;
        expected[49] = 2;
        expected[53] = 3;
        expected[57..61].copy_from_slice(&(-600i32).to_le_bytes());
        expected[61..65].copy_from_slice(&(-7i32).to_le_bytes());
        assert_eq!(bytes, expected);
        bytes.clear();
        super::append_pose(&mut bytes, super::sim::CarPose {
            status: 2, ..Default::default()
        });
        assert_eq!(bytes.len(), 65);
        assert_eq!(bytes[0], 2);
        assert!(bytes[1..].iter().all(|byte| *byte == 0));
    }

    #[test]
    fn result_matches_c_finish_and_retirement_fixture() {
        let mut bytes = vec![wire::S2C_RESULT];
        super::append_result(&mut bytes, true, 1, 1000);
        super::append_result(&mut bytes, false, 0, -1);
        assert_eq!(bytes, [0x84, 1, 1, 0xE8, 3, 0, 0, 0, 0, 255, 255, 255, 255]);
    }

}
