//! Standalone Rage Racer multiplayer server (see ../docs/multiplayer.md).
//!
//! Prototype milestone: exactly two human seats per room, a creator-selected course,
//! stepped by the real retail simulation (`rage-sim`) from network input.
//! Rooms support create/join/list, peer lobby state and Ready. SQLite persistence
//! and live end-to-end verification remain pending.
//!
//! Usage: rage-racer-server <CUE, Track 01 BIN or cache directory> [port]
//! [--reimport] [--class=1..6] [--course=1..4] [--laps=1..6] [--reverse] [--cars=path]

mod sim;
mod cache;
mod protocol;
mod outbox;

use protocol::{wire, start_message, encode_result};
#[cfg(test)]
use protocol::snapshot_message;
use outbox::Outbox;

use sim::{Race, RaceArchive, SeatPlan};
use std::io::{ErrorKind, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

const SEAT_COUNT: usize = 2;
const FIELD_COUNT: usize = sim::DRIVER_SEAT_LIMIT as usize;
const AI_COUNT: usize = FIELD_COUNT - SEAT_COUNT;
const TICK_HZ: u64 = sim::SIM_TICK_RATE as u64;
const INPUT_TIMEOUT: Duration = Duration::from_secs(5);

#[derive(Clone, Copy)]
struct Seat { model: i32, manual: bool, seed: u32 }

#[derive(Clone, Copy)]
struct Rival { model: u8, slot: u8, seed: u32 }

#[derive(Clone, Copy)]
struct Plan {
    class: i32, course: i32, laps: i32, reverse: bool,
    countdown: u32,
    seats: [Seat; SEAT_COUNT],
    rivals: [Option<Rival>; AI_COUNT],
}

impl Default for Plan {
    fn default() -> Self {
        Self { class: 0, course: 0, laps: 3, reverse: false,
            countdown: 3 * sim::SIM_TICK_RATE as u32,
            rivals: [None; AI_COUNT], seats: std::array::from_fn(|seat| Seat {
                model: 0, manual: false, seed: 0x1234_5670 + seat as u32,
            }) }
    }
}

impl Plan {
    fn valid(&self) -> bool {
        let mut slots = 0u32;
        for rival in self.rivals.iter().flatten() {
            if rival.model as usize >= FIELD_COUNT - 1 || rival.slot as usize >= FIELD_COUNT - 1 ||
                slots & (1 << rival.slot) != 0 { return false; }
            slots |= 1 << rival.slot;
        }
        (0..6).contains(&self.class) && (0..4).contains(&self.course) &&
            (1..=sim::PLAYER_LAP_TIME_CAPACITY as i32).contains(&self.laps) &&
            self.seats.iter().all(|seat| (0..sim::CAR_MODEL_VARIANT_COUNT as i32).contains(&seat.model))
    }
}

/* Keep the race on its original time axis after a delayed step. Moving the
 * deadline to now on every overrun permanently loses simulation time. */
fn advance_deadline(next: &mut Instant, now: Instant) -> Duration {
    *next += Duration::from_nanos(1_000_000_000 / TICK_HZ);
    next.saturating_duration_since(now)
}

/// One coherent packet plus pending gear edges. The network reader publishes
/// all controls under the same lock; the tick consumes both edges together.
#[derive(Default, Clone, Copy, PartialEq, Eq)]
enum Stage {
    #[default]
    Waiting,
    Ready,
    Loading,
    Loaded,
    Racing,
    Closed,
}

#[derive(Default)]
struct Input {
    packet: [u8; 12],
    sequence: u32,
    applied_sequence: u32,
    name: Option<Vec<u8>>,
    stage: Stage,
    choice: Choice,
    race: Option<sim::MpRaceOptions>,
    room: Option<u64>,
    list_requested: bool,
    creator: bool,
    last_input: Option<Instant>,
}

#[derive(Default, Clone, Copy, PartialEq, Eq, Debug)]
struct Choice { variant: i32, manual: bool }

#[derive(Debug, PartialEq, Eq)]
enum Load { Waiting, Closed, Invalid }

impl Input {
    fn begin_load(&mut self) -> Option<Choice> {
        if self.stage != Stage::Ready { return None; }
        self.stage = Stage::Loading;
        Some(self.choice)
    }
}

#[derive(Default)]
struct SharedInput {
    state: Mutex<Input>,
    ready: std::sync::Condvar,
}

impl SharedInput {
    fn greet(&self, name: &[u8]) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.stage == Stage::Closed || current.name.is_some() || name.len() > 15 { return false; }
        current.name = Some(name.to_vec());
        self.ready.notify_all();
        true
    }

    fn wait_hello(&self, timeout: Duration) -> bool {
        let current = self.state.lock().unwrap();
        let (current, _) = self.ready.wait_timeout_while(current, timeout,
            |state| state.name.is_none() && state.stage != Stage::Closed).unwrap();
        current.name.is_some() && current.stage != Stage::Closed
    }

    fn pick(&self, variant: u8, manual: u8) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.name.is_none() || variant as u32 >= sim::CAR_MODEL_VARIANT_COUNT || manual > 1 { return false; }
        if matches!(current.stage, Stage::Loading | Stage::Loaded) { return true; }
        if !matches!(current.stage, Stage::Waiting | Stage::Ready) { return false; }
        current.choice = Choice { variant: variant as i32, manual: manual != 0 };
        current.stage = Stage::Waiting;
        self.ready.notify_all();
        true
    }

    fn set_ready(&self, ready: bool) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.name.is_none() { return false; }
        // A Ready change can cross the authoritative Start on the wire. Once
        // frozen, acknowledge that stale lobby input without changing the plan
        // or disconnecting a participant who is about to report Loaded.
        if matches!(current.stage, Stage::Loading | Stage::Loaded) { return true; }
        if !matches!(current.stage, Stage::Waiting | Stage::Ready) { return false; }
        current.stage = if ready { Stage::Ready } else { Stage::Waiting };
        self.ready.notify_all();
        true
    }

    fn request_list(&self) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.name.is_none() || current.room.is_some() || current.stage != Stage::Waiting { return false; }
        current.list_requested = true;
        true
    }

    fn take_list_request(&self) -> bool {
        std::mem::take(&mut self.state.lock().unwrap().list_requested)
    }

    fn configure_race(&self, bytes: [u8; 4]) -> bool {
        let options = sim::MpRaceOptions { classIndex: bytes[0], course: bytes[1],
            laps: bytes[2], reverse: bytes[3] };
        if unsafe { sim::MpValidRaceOptions(&options) } == 0 { return false; }
        let mut current = self.state.lock().unwrap();
        if !current.creator || current.name.is_none() ||
            !matches!(current.stage, Stage::Waiting | Stage::Ready) { return false; }
        current.race = Some(options);
        current.stage = Stage::Waiting;
        self.ready.notify_all();
        true
    }

    fn choose_room(&self, code: u64) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.name.is_none() || current.stage != Stage::Waiting || current.room.is_some() ||
            (code > i64::MAX as u64 && code != u64::MAX) { return false; }
        current.room = Some(code);
        true
    }

    fn wait_stage(&self, stage: Stage, timeout: Duration) -> bool {
        if !matches!(stage, Stage::Ready | Stage::Loaded) { return false; }
        let current = self.state.lock().unwrap();
        let (current, _) = self.ready.wait_timeout_while(current, timeout,
            |state| state.stage != stage && state.stage != Stage::Closed).unwrap();
        current.stage == stage
    }

    fn loaded(&self) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.stage != Stage::Loading { return false; }
        current.stage = Stage::Loaded;
        self.ready.notify_all();
        true
    }

    fn publish(&self, packet: [u8; 12]) -> bool { self.publish_command(packet, None) }

    fn publish_command(&self, mut packet: [u8; 12], sequence: Option<u32>) -> bool {
        if sim::decode_input(&packet).is_none() {
            return false;
        }
        let mut current = self.state.lock().unwrap();
        if !matches!(current.stage, Stage::Loaded | Stage::Racing) {
            return false;
        }
        match sequence {
            Some(value) if value > current.sequence => current.sequence = value,
            None if current.sequence == 0 => {},
            _ => return false,
        }
        packet[9] |= current.packet[9];
        packet[10] |= current.packet[10];
        current.packet = packet;
        current.last_input = Some(Instant::now());
        true
    }

    fn start(&self, now: Instant) -> bool {
        let mut current = self.state.lock().unwrap();
        if current.stage != Stage::Loaded { return false; }
        current.stage = Stage::Racing;
        current.last_input = Some(now);
        true
    }

    #[cfg(test)]
    fn take(&self) -> Option<[u8; 12]> { self.take_at(Instant::now()) }

    #[cfg(test)]
    fn take_at(&self, now: Instant) -> Option<[u8; 12]> {
        self.sample_at(now, true).map(|(packet, _)| packet)
    }

    fn acknowledge(&self, sequence: u32) -> bool {
        let mut state = self.state.lock().unwrap();
        if sequence < state.applied_sequence || sequence > state.sequence { return false; }
        state.applied_sequence = sequence;
        true
    }

    fn sample_at(&self, now: Instant, expect_input: bool) -> Option<([u8; 12], u32)> {
        let mut current = self.state.lock().unwrap();
        if expect_input && current.stage == Stage::Racing && current.last_input.is_some_and(|last|
            now.saturating_duration_since(last) >= INPUT_TIMEOUT) {
            current.stage = Stage::Closed;
            current.packet = [0; 12];
            self.ready.notify_all();
        }
        if current.stage != Stage::Racing {
            return None;
        }
        let packet = current.packet;
        let sequence = current.sequence;
        current.packet[9] = 0;
        current.packet[10] = 0;
        Some((packet, sequence))
    }

    fn close(&self) {
        let mut current = self.state.lock().unwrap();
        current.stage = Stage::Closed;
        current.packet = [0; 12];
        self.ready.notify_all();
    }
}

fn client_writer(mut stream: TcpStream, input: Arc<SharedInput>, output: Arc<Outbox>) -> bool {
    let success = write_client(&mut stream, &output);
    output.close();
    input.close();
    let _ = stream.shutdown(std::net::Shutdown::Both);
    success
}

fn write_client<W: Write>(stream: &mut W, output: &Outbox) -> bool {
    while let Some(message) = output.take() {
        if !write_all_or_drop(stream, &message) { return false; }
    }
    output.result_taken()
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
    let mut header = [0u8; 1];
    loop {
        if read_exact_or_none(stream, &mut header).is_none() {
            return;
        }
        match header[0] {
            wire::C2S_HELLO if !input.wait_hello(Duration::ZERO) => {
                let mut len_buf = [0u8; 1];
                if read_exact_or_none(stream, &mut len_buf).is_none() {
                    return;
                }
                let length = len_buf[0] as usize;
                let mut name = [0u8; 15];
                if length > name.len() || read_exact_or_none(stream, &mut name[..length]).is_none() {
                    return;
                }
                if !input.greet(&name[..length]) { return; }
                let name = String::from_utf8_lossy(&name[..length]);
                println!("rage-racer-server: hello from {name:?}");
            }
            wire::C2S_LOADED => {
                if !input.loaded() { return; }
            }
            wire::C2S_READY => {
                let mut flag = [0];
                if read_exact_or_none(stream, &mut flag).is_none() || flag[0] > 1 ||
                    !input.set_ready(flag[0] != 0) { return; }
            }
            wire::C2S_LIST => { if !input.request_list() { return; } }
            wire::C2S_COMMAND => {
                let mut command = [0; 16];
                if read_exact_or_none(stream, &mut command).is_none() { return; }
                let sequence = u32::from_le_bytes(command[..4].try_into().unwrap());
                let packet = command[4..].try_into().unwrap();
                if !input.publish_command(packet, Some(sequence)) { return; }
            }
            wire::C2S_PICK => {
                let mut choice = [0; 2];
                if read_exact_or_none(stream, &mut choice).is_none() ||
                    !input.pick(choice[0], choice[1]) { return; }
            }
            wire::C2S_RACE => {
                let mut options = [0; 4];
                if read_exact_or_none(stream, &mut options).is_none() || !input.configure_race(options) { return; }
            }
            wire::C2S_ROOM => {
                let mut code = [0; 8];
                if read_exact_or_none(stream, &mut code).is_none() ||
                    !input.choose_room(u64::from_le_bytes(code)) { return; }
            }
            wire::C2S_INPUT => {
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

fn close_session(streams: &[Option<TcpStream>], inputs: &[Arc<SharedInput>], outputs: &[Arc<Outbox>]) {
    for input in inputs { input.close(); }
    for output in outputs { output.close(); }
    for stream in streams.iter().flatten() { let _ = stream.shutdown(std::net::Shutdown::Both); }
}

/* Own the connection lifetime, including partially started sessions. Drop
 * closes sockets before joining readers, so blocked reads cannot outlive it. */
struct Session {
    streams: [Option<TcpStream>; SEAT_COUNT],
    inputs: [Arc<SharedInput>; SEAT_COUNT],
    outputs: [Arc<Outbox>; SEAT_COUNT],
    readers: [Option<std::thread::JoinHandle<()>>; SEAT_COUNT],
    writers: [Option<std::thread::JoinHandle<bool>>; SEAT_COUNT],
    last_lobby: Option<[u8; 53]>,
}

impl Session {
    fn new() -> Self {
        Self {
            last_lobby: None,
            streams: std::array::from_fn(|_| None),
            inputs: std::array::from_fn(|seat| Arc::new(SharedInput {
                state: Mutex::new(Input { creator: seat == 0, ..Default::default() }),
                ..Default::default()
            })),
            outputs: std::array::from_fn(|_| Arc::new(Outbox::default())),
            readers: std::array::from_fn(|_| None),
            writers: std::array::from_fn(|_| None),
        }
    }

    fn attach(&mut self, seat: usize, stream: TcpStream) -> std::io::Result<()> {
        if seat >= SEAT_COUNT || self.streams[seat].is_some() || self.readers[seat].is_some() {
            return Err(std::io::Error::new(ErrorKind::InvalidInput, "invalid or occupied seat"));
        }
        let reader_stream = stream.try_clone()?;
        let input = self.inputs[seat].clone();
        let output = self.outputs[seat].clone();
        let reader = std::thread::Builder::new().name(format!("seat-{seat}-reader"))
            .spawn(move || client_reader(reader_stream, input, output))?;
        self.streams[seat] = Some(stream);
        self.readers[seat] = Some(reader);
        Ok(())
    }

    fn receive(&mut self, seat: usize, mut source: Session) -> std::io::Result<()> {
        if seat >= SEAT_COUNT || self.streams[seat].is_some() || self.readers[seat].is_some() ||
            self.inputs[seat].state.lock().unwrap().name.is_some() ||
            source.inputs[0].state.lock().unwrap().name.is_none() {
            return Err(std::io::Error::new(ErrorKind::InvalidInput, "invalid room seat transfer"));
        }
        std::mem::swap(&mut self.streams[seat], &mut source.streams[0]);
        std::mem::swap(&mut self.readers[seat], &mut source.readers[0]);
        std::mem::swap(&mut self.inputs[seat], &mut source.inputs[0]);
        std::mem::swap(&mut self.outputs[seat], &mut source.outputs[0]);
        self.inputs[seat].state.lock().unwrap().creator = seat == 0;
        Ok(())
    }

    fn finish_writers(&mut self) -> bool {
        let mut success = true;
        for writer in self.writers.iter_mut().filter_map(Option::take) {
            if !matches!(writer.join(), Ok(true)) {
                eprintln!("rage-racer-server: snapshot writer failed");
                success = false;
            }
        }
        success
    }

    fn can_load(&self) -> bool {
        self.streams.iter().all(Option::is_some) && self.readers.iter().all(Option::is_some) &&
            self.writers.iter().all(Option::is_none) &&
            self.inputs.iter().all(|input| {
                let state = input.state.lock().unwrap();
                state.name.is_some() && matches!(state.stage, Stage::Waiting | Stage::Ready)
            })
    }

    fn wait_for(&self, stage: Stage, deadline: Option<Instant>, mut poll: impl FnMut()) -> bool {
        if !matches!(stage, Stage::Ready | Stage::Loaded) {
            return false;
        }
        loop {
            poll();
            let mut pending = None;
            for input in &self.inputs {
                let current = input.state.lock().unwrap();
                if current.stage == Stage::Closed { return false; }
                if current.stage != stage { pending = Some(input); }
            }
            let Some(input) = pending else { return true; };
            let remaining = deadline.map(|end| end.saturating_duration_since(Instant::now()))
                .unwrap_or(Duration::from_millis(10));
            if remaining.is_zero() { return false; }
            // A different seat can disconnect while this one's condition
            // variable waits. Bound that wait and recheck the entire field.
            let wait = remaining.min(Duration::from_millis(10));
            input.wait_stage(stage, wait);
        }
    }

    fn publish_lobby(&mut self, plan: &Plan) -> bool {
        let id = self.inputs[0].state.lock().unwrap().room;
        let Some(id) = id.filter(|id| *id > 0 && *id <= i64::MAX as u64) else { return true; };
        let Some(bytes) = protocol::lobby_message(&lobby_view(id, &self.inputs, plan)) else { return false; };
        if self.last_lobby == Some(bytes) { return true; }
        for stream in self.streams.iter_mut().flatten() {
            if !write_all_or_drop(stream, &bytes) { return false; }
        }
        self.last_lobby = Some(bytes);
        true
    }

    fn wait_plan(&mut self, plan: Plan, mut poll: impl FnMut()) -> Option<Plan> {
        loop {
            poll();
            if !self.publish_lobby(&plan) { return None; }
            match self.load_plan(plan) {
                Ok(selected) => return Some(selected),
                Err(Load::Waiting) => {
                    // Wake promptly on any seat change/disconnection. The next
                    // attempt validates and freezes all seats under one lock set.
                    let current = self.inputs[0].state.lock().unwrap();
                    drop(self.inputs[0].ready.wait_timeout(current, Duration::from_millis(10)).unwrap());
                }
                Err(_) => return None,
            }
        }
    }

    fn load_plan(&self, mut plan: Plan) -> Result<Plan, Load> {
        if !plan.valid() { return Err(Load::Invalid); }
        // Lock in seat order. Reader workers only ever acquire their own seat;
        // no choice/readiness change can slip between validation and freezing.
        let mut inputs: [_; SEAT_COUNT] = std::array::from_fn(|seat| self.inputs[seat].state.lock().unwrap());
        if inputs.iter().any(|input| input.stage == Stage::Closed) { return Err(Load::Closed); }
        if inputs.iter().any(|input| !matches!(input.stage, Stage::Waiting | Stage::Ready)) {
            return Err(Load::Invalid);
        }
        if inputs[1].race.is_some() { return Err(Load::Invalid); } // Only the creator selects the race.
        if let Some(options) = inputs[0].race {
            plan.class = options.classIndex as i32;
            plan.course = options.course as i32;
            plan.laps = options.laps as i32;
            plan.reverse = options.reverse != 0;
        }
        if !plan.valid() { return Err(Load::Invalid); }
        if inputs.iter().any(|input| input.stage != Stage::Ready) { return Err(Load::Waiting); }
        for (seat, input) in inputs.iter_mut().enumerate() {
            let choice = input.begin_load().expect("all seats are locked and ready");
            plan.seats[seat].model = choice.variant;
            plan.seats[seat].manual = choice.manual;
        }
        Ok(plan)
    }
}

impl Drop for Session {
    fn drop(&mut self) {
        close_session(&self.streams, &self.inputs, &self.outputs);
        let readers = self.readers.iter_mut().filter_map(Option::take).map(|worker| worker.join());
        let writers = self.writers.iter_mut().filter_map(Option::take).map(|worker| worker.join().map(|_| ()));
        for result in readers.chain(writers) {
            if result.is_err() { eprintln!("rage-racer-server: session worker failed"); }
        }
    }
}

struct Start<'a> { source: &'a str, port: u16, reimport: bool, plan: Plan, cars: Option<&'a str>, http: Option<u16> }

fn parse_start(args: &[String]) -> Result<Start<'_>, &'static str> {
    let usage = "usage: rage-racer-server <CUE, Track 01 BIN or cache directory> [port] [--reimport] [--class=1..6] [--course=1..4] [--laps=1..6] [--reverse] [--cars=path] [--http=port]";
    if args.is_empty() || args[0].is_empty() || args[0].starts_with("--") { return Err(usage); }
    let explicit_port = args.get(1).filter(|arg| !arg.starts_with("--"));
    let port = match explicit_port {
        None => 7243,
        Some(text) => {
            if text.is_empty() || !text.bytes().all(|byte| byte.is_ascii_digit()) {
                return Err("port must be an integer from 1 to 65535");
            }
            text.parse::<u16>().ok().filter(|port| *port != 0)
                .ok_or("port must be an integer from 1 to 65535")?
        }
    };
    let mut plan = Plan::default();
    let mut reimport = false;
    let mut cars = None;
    let mut http = None;
    let mut seen = Vec::new();
    for arg in &args[1 + explicit_port.is_some() as usize..] {
        let (key, value) = arg.split_once('=').unwrap_or((arg, ""));
        if seen.contains(&key) { return Err("duplicate server option"); }
        seen.push(key);
        match (key, value) {
            ("--cars", value) if !value.is_empty() => cars = Some(value),
            ("--reimport", "") if arg == "--reimport" => reimport = true,
            ("--reverse", "") if arg == "--reverse" => plan.reverse = true,
            ("--http", value) => {
                if value.is_empty() || !value.bytes().all(|byte| byte.is_ascii_digit()) {
                    return Err("--http port must be an integer from 1 to 65535");
                }
                http = Some(value.parse::<u16>().ok().filter(|port| *port != 0)
                    .ok_or("--http port must be an integer from 1 to 65535")?);
            }
            ("--class" | "--course" | "--laps", value) => {
                if value.is_empty() || !value.bytes().all(|byte| byte.is_ascii_digit()) {
                    return Err("invalid race option");
                }
                let number = value.parse::<i32>().map_err(|_| "invalid race option")?;
                match key {
                    "--class" => plan.class = number - 1,
                    "--course" => plan.course = number - 1,
                    _ => plan.laps = number,
                }
            }
            _ => return Err(usage),
        }
    }
    if !plan.valid() { return Err("invalid race option"); }
    Ok(Start { source: &args[0], port, reimport, plan, cars, http })
}

fn prepare_race(archive: &RaceArchive, plan: &Plan) -> Option<Race> {
    if !plan.valid() { return None; }
    if !archive.boot().contains(&0) || archive.boot()[0] == 0 || archive.executable() == 0 { return None; }
    for seat in &plan.seats {
        if !archive.automatic(seat.model)? && !seat.manual { return None; }
    }
    let track = archive.copy_track(plan.class, plan.course)?;
    let mut seats: Vec<SeatPlan> = plan.seats.iter().enumerate()
        .map(|(grid, seat)| SeatPlan::Human {
            grid: grid as i32, variant: seat.model, manual: seat.manual, seed: seat.seed,
        }).collect();
    seats.extend(plan.rivals.iter().enumerate().map(|(i, rival)| match rival {
        Some(rival) => SeatPlan::Rival { grid: (SEAT_COUNT + i) as i32,
            model: rival.model as i32, slot: rival.slot as i32, seed: rival.seed },
        None => SeatPlan::Empty,
    }));
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
    let plan = start.plan;
    let (mut archive, _) = match load_race(start.source, start.reimport, &plan) {
        Ok(prepared) => prepared,
        Err(error) => {
            eprintln!("rage-racer-server: {error}");
            return std::process::ExitCode::FAILURE;
        }
    };
    if let Some(cars) = start.cars {
        if let Err(error) = archive.load_catalog(cars) {
            eprintln!("rage-racer-server: car configuration: {error}");
            return std::process::ExitCode::FAILURE;
        }
    }
    let availability = match archive.availability_message() {
        Some(packet) => packet,
        None => {
            eprintln!("rage-racer-server: invalid transmission metadata");
            return std::process::ExitCode::FAILURE;
        }
    };
    let archive = Arc::new(archive);
    let port = start.port;

    let listener = match TcpListener::bind(("0.0.0.0", port)) {
        Ok(listener) => listener,
        Err(error) => {
            eprintln!("rage-racer-server: cannot listen on port {port}: {error}");
            return std::process::ExitCode::FAILURE;
        }
    };
    if let Err(error) = listener.set_nonblocking(true) {
        eprintln!("rage-racer-server: cannot configure listener: {error}");
        return std::process::ExitCode::FAILURE;
    }
    println!("rage-racer-server: listening on port {port}, waiting for {SEAT_COUNT} players");

    let http_listener = match start.http {
        Some(http_port) => match TcpListener::bind(("0.0.0.0", http_port))
            .and_then(|listener| listener.set_nonblocking(true).map(|_| listener)) {
            Ok(listener) => {
                println!("rage-racer-server: status page at http://127.0.0.1:{http_port}/");
                Some(listener)
            }
            Err(error) => {
                eprintln!("rage-racer-server: cannot listen for --http on port {http_port}: {error}");
                return std::process::ExitCode::FAILURE;
            }
        },
        None => None,
    };

    let mut rooms = Rooms::default();
    let mut joining: Vec<Joining> = Vec::new();
    let mut incoming: Vec<Incoming> = Vec::new();
    loop {
        for finish in rooms.reap() { finish.report(); }
        let now = Instant::now();
        let mut index = 0;
        while index < joining.len() {
            if !joining[index].session.publish_lobby(&plan) {
                joining.remove(index);
                continue;
            }
            match joining[index].poll(now) {
                Ok(false) => index += 1,
                Err(error) => {
                    eprintln!("rage-racer-server: room admission cancelled: {error}");
                    joining.remove(index);
                }
                Ok(true) => {
                    let room = joining.remove(index);
                    let archive = archive.clone();
                    if let Err(error) = rooms.start(room.id, room.session, move |session| {
                        let (race, session, selected) = start_session(&archive, plan, session, || {})?;
                        run_room(race, session, selected).map_err(str::to_owned)
                    }) {
                        eprintln!("rage-racer-server: cannot start room: {error}");
                    }
                }
            }
        }
        let mut index = 0;
        while index < incoming.len() {
            if incoming[index].request(now).is_err() {
                incoming.remove(index);
                continue;
            }
            if incoming[index].session.inputs[0].take_list_request() {
                let bytes = protocol::room_list(&rooms.directory(&joining, &plan)).unwrap();
                if incoming[index].session.streams[0].as_mut().unwrap().write_all(&bytes).is_err() {
                    incoming.remove(index);
                    continue;
                }
                // Browsing is an explicit decision phase after the bounded Hello.
                incoming[index].deadline = now + Duration::from_secs(60);
            }
            match incoming[index].request(now) {
                Ok(None) => index += 1,
                Err(error) => {
                    eprintln!("rage-racer-server: connection rejected: {error}");
                    incoming.remove(index);
                }
                Ok(Some(code)) => {
                    let mut peer = incoming.remove(index);
                    let target = match select_room(&joining, code,
                        joining.len() + rooms.active.len() < ROOM_LIMIT) {
                        Ok(Some(target)) => target,
                        Ok(None) => {
                            let id = match rooms.new_id() {
                                Ok(id) => id,
                                Err(error) => { eprintln!("rage-racer-server: {error}"); continue; }
                            };
                            let mut room = Joining::new(now, id);
                            if code == 0 { room.join_deadline = None; }
                            joining.push(room);
                            println!("rage-racer-server: created room {id}");
                            joining.len() - 1
                        }
                        Err(error) => { eprintln!("rage-racer-server: {error}"); continue; }
                    };
                    let seat = joining[target].seat().unwrap();
                    {
                        let mut input = peer.session.inputs[0].state.lock().unwrap();
                        input.creator = seat == 0;
                        input.room = Some(joining[target].id);
                    }
                    let welcome = protocol::welcome_message(seat, joining[target].id).unwrap();
                    let stream = peer.session.streams[0].as_mut().unwrap();
                    let sent = stream.write_all(&welcome).and_then(|_| stream.write_all(&availability));
                    if sent.is_ok() && joining[target].session.receive(seat, peer.session).is_ok() {
                        joining[target].hello[seat] = Some(now + Duration::from_secs(5));
                    } else if joining[target].hello.iter().all(Option::is_none) {
                        joining.remove(target);
                    }
                }
            }
        }
        match listener.accept() {
            Ok((stream, _)) => {
                if incoming.len() >= ROOM_LIMIT * SEAT_COUNT {
                    let _ = stream.shutdown(std::net::Shutdown::Both);
                } else {
                    match Incoming::new(stream, Instant::now()) {
                        Ok(peer) => incoming.push(peer),
                        Err(error) => eprintln!("rage-racer-server: cannot admit connection: {error}"),
                    }
                }
            }
            Err(error) if matches!(error.kind(), ErrorKind::WouldBlock | ErrorKind::Interrupted) => {}
            Err(error) => eprintln!("rage-racer-server: cannot accept connection: {error}"),
        }
        if let Some(http_listener) = &http_listener {
            match http_listener.accept() {
                Ok((stream, _)) => serve_http_status(stream, &rooms.directory(&joining, &plan)),
                Err(error) if matches!(error.kind(), ErrorKind::WouldBlock | ErrorKind::Interrupted) => {}
                Err(error) => eprintln!("rage-racer-server: cannot accept status connection: {error}"),
            }
        }
        std::thread::sleep(Duration::from_millis(10));
    }
}

/* Admission owns sockets/readers while hello is incomplete. Poll is inert and
 * takes its clock from the coordinator; no listener wait or timer thread. */
struct Joining {
    id: u64,
    session: Session,
    hello: [Option<Instant>; SEAT_COUNT],
    join_deadline: Option<Instant>,
}

impl Joining {
    fn new(now: Instant, id: u64) -> Self {
        Self { id, session: Session::new(), hello: [None; SEAT_COUNT],
            join_deadline: Some(now + Duration::from_secs(90)) }
    }

    fn seat(&self) -> Option<usize> { self.hello.iter().position(Option::is_none) }

    fn poll(&self, now: Instant) -> Result<bool, &'static str> {
        let mut complete = true;
        for (seat, deadline) in self.hello.iter().enumerate() {
            let Some(deadline) = deadline else {
                if self.join_deadline.is_some_and(|deadline| now >= deadline) { return Err("another seat did not join"); }
                complete = false;
                continue;
            };
            let input = self.session.inputs[seat].state.lock().unwrap();
            if input.stage == Stage::Closed { return Err("a joining seat disconnected"); }
            if input.name.is_none() {
                if now >= *deadline { return Err("seat did not complete hello"); }
                complete = false;
            }
        }
        Ok(complete)
    }
}

struct Incoming { session: Session, deadline: Instant }

impl Incoming {
    fn new(stream: TcpStream, now: Instant) -> std::io::Result<Self> {
        stream.set_nonblocking(false)?;
        stream.set_nodelay(true)?;
        stream.set_write_timeout(Some(Duration::from_millis(100)))?;
        let mut session = Session::new();
        session.inputs[0].state.lock().unwrap().creator = false;
        session.attach(0, stream)?;
        Ok(Self { session, deadline: now + Duration::from_secs(5) })
    }

    fn request(&self, now: Instant) -> Result<Option<u64>, &'static str> {
        let input = self.session.inputs[0].state.lock().unwrap();
        if input.stage == Stage::Closed { return Err("connection closed"); }
        if input.room.is_some() { return Ok(input.room); }
        if now >= self.deadline { return Err("hello/room request timed out"); }
        Ok(None)
    }
}

fn select_room(rooms: &[Joining], code: u64, available: bool) -> Result<Option<usize>, &'static str> {
    if code != 0 {
        if let Some(index) = rooms.iter().position(|room|
            (code == u64::MAX || room.id == code) && room.seat().is_some()) {
            return Ok(Some(index));
        }
        if code != u64::MAX { return Err("selected room is missing or full"); }
    }
    if available { Ok(None) } else { Err("server room capacity reached") }
}

/* Lobby admission owns the listener; starting a selected field consumes only
 * its connections. Future room selection can use this same loading boundary. */
fn start_session(archive: &RaceArchive, plan: Plan, mut session: Session,
                 mut poll: impl FnMut()) -> Result<(Race, Session, Plan), String> {
    if !plan.valid() || !session.can_load() {
        return Err("invalid plan or incomplete session".into());
    }
    let mut plan = session.wait_plan(plan, &mut poll).ok_or("room cancelled before loading")?;
    plan.rivals = archive.rivals(plan.class, plan.course, plan.reverse)
        .ok_or("selected AI field could not be read")?;
    let mut race = prepare_race(archive, &plan).ok_or("selected field could not be prepared")?;
    let msg = start_message(&plan, archive.boot(), archive.fingerprint(), archive.executable())
        .ok_or("invalid race plan")?;
    let config = race.config_message().ok_or("invalid car configuration")?;
    for (seat, stream) in session.streams.iter_mut().enumerate() {
        let stream = stream.as_mut().ok_or("missing seat connection")?;
        if !write_all_or_drop(stream, &msg) || !write_all_or_drop(stream, &config) {
            return Err(format!("seat {seat} failed to receive start"));
        }
    }
    let load_deadline = Instant::now() + Duration::from_secs(60);
    if !session.wait_for(Stage::Loaded, Some(load_deadline), &mut poll) {
        return Err("clients did not finish loading".into());
    }
    let started = Instant::now();
    if session.inputs.iter().any(|input| !input.start(started)) || !race.start(plan.countdown) {
        return Err("a seat disconnected before race start".into());
    }
    for (seat, stream) in session.streams.iter().enumerate() {
        let stream = stream.as_ref().ok_or("missing seat connection")?;
        let input = session.inputs[seat].clone();
        let output = session.outputs[seat].clone();
        let writer = stream.try_clone().and_then(|stream| {
            std::thread::Builder::new().name(format!("seat-{seat}-writer"))
                .spawn(move || client_writer(stream, input, output))
        }).map_err(|error| format!("cannot start seat {seat} writer: {error}"))?;
        session.writers[seat] = Some(writer);
    }
    println!("rage-racer-server: race started, {} laps", plan.laps);
    Ok((race, session, plan))
}

struct Finish {
    room: Option<u64>,
    plan: Plan,
    names: [Vec<u8>; SEAT_COUNT],
    results: [(bool, u8, i32); SEAT_COUNT],
    writes_ok: bool,
}

impl Finish {
    fn capture(race: &Race, session: &Session, plan: Plan) -> Self {
        Self {
            room: None,
            plan,
            names: std::array::from_fn(|seat|
                session.inputs[seat].state.lock().unwrap().name.clone().unwrap_or_default()),
            results: std::array::from_fn(|seat| race.result(seat)),
            writes_ok: false,
        }
    }

    fn report(self) {
        println!("rage-racer-server: completed room={:?} class={} course={} laps={} reverse={}",
            self.room,
            self.plan.class + 1, self.plan.course + 1, self.plan.laps, self.plan.reverse);
        for (name, (finished, place, time)) in self.names.iter().zip(self.results) {
            println!("rage-racer-server: result name={:?} finished={finished} place={place} time={time} ms",
                String::from_utf8_lossy(name));
        }
        if !self.writes_ok { eprintln!("rage-racer-server: result writer failed"); }
    }
}

struct Room {
    id: u64,
    inputs: [Arc<SharedInput>; SEAT_COUNT],
    task: Option<std::thread::JoinHandle<Result<Finish, String>>>,
}

impl Room {
    fn join(&mut self) -> Option<Finish> {
        match self.task.take()?.join() {
            Ok(Ok(mut finish)) => {
                finish.room = Some(self.id);
                return Some(finish);
            }
            Ok(Err(error)) => eprintln!("rage-racer-server: room failed: {error}"),
            Err(_) => eprintln!("rage-racer-server: room worker panicked"),
        }
        None
    }
}

impl Drop for Room {
    fn drop(&mut self) {
        for input in &self.inputs { input.close(); }
        if let Some(finish) = self.join() { finish.report(); }
    }
}

#[derive(Default)]
struct Rooms { active: Vec<Room>, last_id: u64 }

const ROOM_LIMIT: usize = 16;

/* Human-readable status page, read by curl or a browser: what a player asking
 * "is this actually running" wants to see, without touching the game socket
 * protocol. Best-effort: a slow or malformed client gets dropped, never lets
 * the room-stepping thread stall. Room contents come from the same
 * Rooms::directory() the in-game room browser already uses. */
fn serve_http_status(mut stream: TcpStream, rooms: &[protocol::RoomInfo]) {
    let _ = stream.set_read_timeout(Some(Duration::from_millis(200)));
    let _ = stream.set_write_timeout(Some(Duration::from_millis(200)));
    let mut discard = [0u8; 512];
    let _ = stream.read(&mut discard);

    let mut rows = String::new();
    for room in rooms {
        let state = match room.state { 0 => "lobby", 1 => "loading", 2 => "racing", _ => "?" };
        rows.push_str(&format!(
            "<tr><td>{}</td><td>{}</td><td>{}/{}</td><td>{}/{}</td>\
             <td>class {} course {} laps {}{}</td></tr>\n",
            room.code, state, room.occupied.count_ones(), SEAT_COUNT,
            room.ready.count_ones(), SEAT_COUNT,
            room.options[0] + 1, room.options[1] + 1, room.options[2],
            if room.options[3] != 0 { " reverse" } else { "" }));
    }
    if rows.is_empty() { rows.push_str("<tr><td colspan=5>no rooms yet</td></tr>\n"); }
    let body = format!(
        "<!doctype html><html><head><meta charset=utf-8>\
         <meta http-equiv=refresh content=1>\
         <title>rage-racer-server</title>\
         <style>body{{font-family:monospace}} td,th{{padding:2px 10px;text-align:left}}</style>\
         </head><body><h1>rage-racer-server</h1>\
         <table border=1><tr><th>room</th><th>state</th><th>occupied</th><th>ready</th><th>options</th></tr>\n\
         {rows}</table></body></html>");
    let response = format!(
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\
         Content-Length: {}\r\nConnection: close\r\n\r\n{}", body.len(), body);
    let _ = stream.write_all(response.as_bytes());
}

fn lobby_view(id: u64, inputs: &[Arc<SharedInput>; SEAT_COUNT], defaults: &Plan) -> protocol::Lobby {
    // Same seat order as load_plan: readiness/settings describe one
    // atomic room view. No lock survives encoding or socket writes.
    let states: [_; SEAT_COUNT] = std::array::from_fn(|seat| inputs[seat].state.lock().unwrap());
    let options = states[0].race.map(|race|
    [race.classIndex, race.course, race.laps, race.reverse])
    .unwrap_or([defaults.class as u8, defaults.course as u8,
    defaults.laps as u8, defaults.reverse as u8]);
    let mut room = protocol::RoomInfo { code: id, options, occupied: 0, ready: 0, state: 0 };
    for (seat, input) in states.iter().enumerate() {
        if input.name.is_some() && input.stage != Stage::Closed { room.occupied |= 1 << seat; }
        if input.stage == Stage::Ready { room.ready |= 1 << seat; }
        if matches!(input.stage, Stage::Loading | Stage::Loaded) { room.state = room.state.max(1); }
        if input.stage == Stage::Racing { room.state = 2; }
    }
    let seats = std::array::from_fn(|seat| {
        let input = &states[seat];
        if room.occupied & (1 << seat) == 0 { return protocol::LobbySeat::default(); }
        let mut peer = protocol::LobbySeat { variant: input.choice.variant as u8,
            manual: input.choice.manual, ..Default::default() };
        if let Some(name) = &input.name {
            peer.length = name.len() as u8;
            peer.name[..name.len()].copy_from_slice(name);
        }
        peer
    });
    protocol::Lobby { room, seats }
}

impl Rooms {
    fn directory(&self, joining: &[Joining], defaults: &Plan) -> Vec<protocol::RoomInfo> {
        let mut result: Vec<_> = joining.iter().map(|room|
        lobby_view(room.id, &room.session.inputs, defaults).room).chain(self.active.iter().map(|room| {
            let mut view = lobby_view(room.id, &room.inputs, defaults).room;
            view.state = view.state.max(1);
            view
        })).collect();
        result.sort_unstable_by_key(|room| room.code);
        result
    }

    fn reap(&mut self) -> Vec<Finish> {
        let mut completed = Vec::new();
        let mut index = 0;
        while index < self.active.len() {
            if self.active[index].task.as_ref().is_some_and(|task| task.is_finished()) {
                let mut room = self.active.swap_remove(index);
                if let Some(finish) = room.join() { completed.push(finish); }
            } else {
                index += 1;
            }
        }
        completed
    }

    fn available(&self) -> bool {
        self.active.len() < ROOM_LIMIT
    }

    fn new_id(&mut self) -> std::io::Result<u64> {
        let id = self.last_id.checked_add(1).filter(|id| *id <= i64::MAX as u64)
        .ok_or_else(|| std::io::Error::new(ErrorKind::Other, "room identifiers exhausted"))?;
        self.last_id = id;
        Ok(id)
    }

    fn start(&mut self, id: u64, session: Session,
    work: impl FnOnce(Session) -> Result<Finish, String> + Send + 'static) -> std::io::Result<()> {
        if !self.available() {
            return Err(std::io::Error::new(ErrorKind::WouldBlock, "room capacity reached"));
        }
        if id == 0 || id > self.last_id || self.active.iter().any(|room| room.id == id) {
            return Err(std::io::Error::new(ErrorKind::InvalidInput, "invalid or occupied room identifier"));
        }
        let inputs = session.inputs.clone();
        let task = std::thread::Builder::new().name(format!("room-{id}"))
        .spawn(move || work(session))?;
        self.active.push(Room { id, inputs, task: Some(task) });
        Ok(())
    }
}

/* One authoritative step, independent of pacing and listener admission. */
fn step_room(race: &mut Race, session: &Session,
seat_alive: &mut [bool; SEAT_COUNT]) -> Result<bool, &'static str> {
    let mut accepted = [None; SEAT_COUNT];
    for (seat, input) in session.inputs.iter().enumerate() {
        let Some((packet, sequence)) = input.sample_at(Instant::now(), race.accepts_input(seat)) else {
            if seat_alive[seat] {
                seat_alive[seat] = false;
                race.retire(seat as i32);
                session.outputs[seat].close();
                if let Some(stream) = &session.streams[seat] {
                    let _ = stream.shutdown(std::net::Shutdown::Both);
                }
            }
            continue;
        };
        let controls = sim::decode_input(&packet).ok_or("invalid sampled controls")?;
        if race.set_input(seat as i32, &controls) { accepted[seat] = Some(sequence); }
    }
    if !race.step() {
        eprintln!("rage-racer-server: room simulation could not advance");
        return Err("simulation could not advance");
    }

    for (seat, sequence) in accepted.into_iter().enumerate() {
        if race.input_tick(seat) == race.tick() {
            if let Some(sequence) = sequence {
                if !session.inputs[seat].acknowledge(sequence) { return Err("invalid input acknowledgement"); }
            }
        }
    }
    if !seat_alive.iter().any(|alive| *alive) {
        println!("rage-racer-server: every seat disconnected, stopping");
        return Ok(false);
    }
    let acknowledged = std::array::from_fn(|seat|
        session.inputs[seat].state.lock().unwrap().applied_sequence);
    let snapshot: Arc<[u8]> = race.state_message(acknowledged)
        .ok_or("invalid correction publication")?.into();
    for (seat, output) in session.outputs.iter().enumerate() {
        if seat_alive[seat] { output.publish(snapshot.clone()); }
    }
    if race.is_finished() {
        println!("rage-racer-server: race finished at tick {}", race.tick());
        return Ok(false);
    }
    Ok(true)
}

/* Each room consumes its own simulation and connection owner. No archive,
* listener enters the running race. The frozen room plan is retained for results. */
fn run_room(mut race: Race, mut session: Session, plan: Plan) -> Result<Finish, &'static str> {
    let mut next_tick = Instant::now();
    let mut seat_alive = [true; SEAT_COUNT];
    while step_room(&mut race, &session, &mut seat_alive)? {
        let wait = advance_deadline(&mut next_tick, Instant::now());
        if !wait.is_zero() { std::thread::sleep(wait); }
    }
    finish_room(&race, &mut session, plan)
}

fn finish_room(race: &Race, session: &mut Session, plan: Plan) -> Result<Finish, &'static str> {
    if !race.is_finished() {
        for output in &session.outputs { output.close(); }
        return Err("race stopped before completion");
    }
    let mut finish = Finish::capture(race, session, plan);
    let result: Arc<[u8]> = encode_result(finish.results).into();
    for output in &session.outputs { output.finish(result.clone()); }
    finish.writes_ok = session.finish_writers();
    Ok(finish)
}

#[cfg(test)]

mod tests {
    use super::{read_client, Instant};
    fn loaded_input() -> SharedInput {
        let input = SharedInput::default();
        assert!(input.greet(b"Driver"));
        assert!(input.set_ready(true));
        assert!(input.state.lock().unwrap().begin_load().is_some());
        assert!(input.loaded());
        input
    }

    #[test]
    fn numbered_commands_preserve_edges_and_reject_replays_atomically() {
        let waiting = SharedInput::default();
        assert!(!waiting.publish_command([0; 12], Some(1)));
        assert_eq!(waiting.state.lock().unwrap().sequence, 0);
        let input = loaded_input();
        let mut up = [0; 12]; up[9] = 1;
        let mut down = [0; 12]; down[10] = 1;
        assert!(input.publish_command(up, Some(1)));
        assert!(input.publish_command(down, Some(5)));
        let before = {
            let state = input.state.lock().unwrap();
            (state.packet, state.sequence, state.last_input)
        };
        for sequence in [0, 1, 4, 5] {
            assert!(!input.publish_command([0; 12], Some(sequence)));
        }
        let mut invalid = [0; 12]; invalid[9] = 2;
        assert!(!input.publish_command(invalid, Some(6)));
        assert!(!input.publish([0; 12]));
        {
            let state = input.state.lock().unwrap();
            assert_eq!((state.packet, state.sequence, state.last_input), before);
            assert_eq!(state.applied_sequence, 0);
        }
        assert!(input.start(Instant::now()));
        let (first, sampled) = input.sample_at(Instant::now(), true).unwrap();
        assert_eq!(&first[9..11], &[1, 1]);
        assert_eq!(input.state.lock().unwrap().applied_sequence, 0);
        assert!(!input.acknowledge(6));
        assert!(input.publish_command([0; 12], Some(6)));
        assert!(input.acknowledge(sampled));
        assert!(!input.acknowledge(4));
        assert_eq!(input.state.lock().unwrap().applied_sequence, 5);
        assert_eq!(&input.take().unwrap()[9..11], &[0, 0]);
        assert!(input.publish_command([0; 12], Some(u32::MAX)));
        assert!(!input.publish_command(up, Some(0)));
        assert!(!input.publish_command(up, Some(u32::MAX)));
    }

    #[test]
    fn command_reader_commits_only_complete_numbered_packets() {
        let mut bytes = vec![wire::C2S_COMMAND];
        bytes.extend_from_slice(&0x01020304u32.to_le_bytes());
        let mut packet = [0; 12]; packet[9] = 1;
        bytes.extend_from_slice(&packet);
        for length in 0..bytes.len() {
            let input = loaded_input();
            read_client(&mut std::io::Cursor::new(&bytes[..length]), &input);
            let state = input.state.lock().unwrap();
            assert_eq!(state.sequence, 0);
            assert_eq!(state.packet, [0; 12]);
        }
        let input = loaded_input();
        read_client(&mut std::io::Cursor::new(&bytes), &input);
        assert_eq!(input.state.lock().unwrap().sequence, 0x01020304);
        assert_eq!(input.state.lock().unwrap().packet, packet);
        let mut replay = bytes.clone();
        replay.extend_from_slice(&bytes);
        replay.push(wire::C2S_COMMAND);
        replay.extend_from_slice(&0x01020305u32.to_le_bytes());
        replay.extend_from_slice(&[0; 12]);
        let input = loaded_input();
        read_client(&mut std::io::Cursor::new(replay), &input);
        assert_eq!(input.state.lock().unwrap().sequence, 0x01020304);
    }

    #[test]
    fn completed_driver_can_wait_for_results_without_fresh_controls() {
        let input = loaded_input();
        let now = Instant::now();
        assert!(input.start(now));
        assert!(input.sample_at(now + super::INPUT_TIMEOUT, false).is_some());
        assert!(input.state.lock().unwrap().stage == Stage::Racing);
        assert!(input.sample_at(now + super::INPUT_TIMEOUT, true).is_none());
        let input = racing_input();
        input.close();
        assert!(input.sample_at(now, false).is_none());
    }

    fn racing_input() -> SharedInput {
        let input = loaded_input();
        assert!(input.start(super::Instant::now()));
        input
    }

    #[test]
    fn sparse_result_writers_are_joined_once_and_report_failure() {
        for outcome in [Some(true), Some(false), None] {
            let mut session = super::Session::new();
            session.writers[1] = Some(std::thread::spawn(move || {
                outcome.expect("fixture writer failure")
            }));
            assert_eq!(session.finish_writers(), outcome == Some(true));
            assert!(session.writers.iter().all(Option::is_none));
            assert!(session.finish_writers());
            assert!(session.inputs[1].greet(b"Still owned"));
        }
    }

    #[test]
    fn socket_write_failure_reaches_session_completion() {
        struct Broken;
        impl std::io::Write for Broken {
            fn write(&mut self, _: &[u8]) -> std::io::Result<usize> {
                Err(std::io::ErrorKind::BrokenPipe.into())
            }
            fn flush(&mut self) -> std::io::Result<()> { Ok(()) }
        }
        let mut session = super::Session::new();
        let output = session.outputs[1].clone();
        output.finish(Arc::from([wire::S2C_RESULT]));
        session.writers[1] = Some(std::thread::spawn(move || {
            super::write_client(&mut Broken, &output)
        }));
        assert!(!session.finish_writers());
        assert!(session.writers.iter().all(Option::is_none));
        assert!(session.finish_writers());
    }

    #[test]
    fn discarded_result_is_not_successfully_written() {
        let output = Outbox::default();
        output.finish(Arc::from([wire::S2C_RESULT]));
        output.close();
        let mut bytes = Vec::new();
        assert!(!super::write_client(&mut bytes, &output));
        assert!(bytes.is_empty());

        let output = Outbox::default();
        output.finish(Arc::from([wire::S2C_RESULT]));
        assert!(super::write_client(&mut bytes, &output));
        assert_eq!(bytes, [wire::S2C_RESULT]);
        output.close();
        assert!(output.result_taken());
    }

    #[test]
    fn loaded_seat_starts_once_and_timeout_begins_at_start() {
        let input = loaded_input();
        let mut packet = [0; 12];
        packet[5] = 200;
        assert!(input.publish(packet));
        let origin = super::Instant::now();
        assert_eq!(input.take_at(origin + super::INPUT_TIMEOUT * 2), None);
        assert!(input.state.lock().unwrap().stage == Stage::Loaded);
        assert!(input.start(origin));
        assert_eq!(input.take_at(origin), Some(packet));
        assert!(!input.start(origin + super::INPUT_TIMEOUT));
        assert_eq!(input.state.lock().unwrap().last_input, Some(origin));
        assert!(!input.loaded());
        assert!(!input.set_ready(true));
        assert!(!input.pick(9, 1));
        assert_eq!(input.take_at(origin + super::INPUT_TIMEOUT), None);
    }

    #[test]
    fn driving_controls_require_loaded_state_and_preserve_lobby_choices() {
        let input = SharedInput::default();
        let mut packet = [0; 12];
        packet[5] = 200;
        packet[9] = 1;
        assert!(!input.publish(packet));
        assert!(input.greet(b"Driver"));
        assert!(input.pick(9, 1));
        assert!(!input.publish(packet));
        assert!(input.set_ready(true));
        assert!(!input.publish(packet));
        assert!(input.state.lock().unwrap().begin_load().is_some());
        assert!(!input.publish(packet));
        {
            let state = input.state.lock().unwrap();
            assert_eq!(state.packet, [0; 12]);
            assert!(state.last_input.is_none());
            assert_eq!(state.choice, super::Choice { variant: 9, manual: true });
        }
        assert!(input.loaded());
        assert!(input.publish(packet));
        assert!(input.take().is_none());
        assert!(input.start(super::Instant::now()));
        assert_eq!(input.take(), Some(packet));
        input.close();
        assert!(!input.publish(packet));
    }

    #[test]
    fn joining_hello_deadlines_are_absolute_and_polling_is_inert() {
        let now = super::Instant::now();
        let mut joining = super::Joining::new(now, 7);
        assert_eq!(joining.seat(), Some(0));
        assert_eq!(joining.poll(now), Ok(false));
        joining.hello[0] = Some(now + std::time::Duration::from_secs(5));
        assert_eq!(joining.seat(), Some(1));
        assert_eq!(joining.poll(now + std::time::Duration::from_secs(4)), Ok(false));
        assert!(joining.poll(now + std::time::Duration::from_secs(5)).is_err());
        assert_eq!(joining.hello[0], Some(now + std::time::Duration::from_secs(5)));
        assert!(joining.session.inputs[0].greet(b"First"));
        assert_eq!(joining.poll(now + std::time::Duration::from_secs(6)), Ok(false));
        assert!(joining.poll(now + std::time::Duration::from_secs(90)).is_err());
        joining.hello[1] = Some(now + std::time::Duration::from_secs(100));
        assert_eq!(joining.seat(), None);
        assert_eq!(joining.poll(now + std::time::Duration::from_secs(90)), Ok(false));
        assert!(joining.session.inputs[1].greet(b"Second"));
        assert_eq!(joining.poll(now + std::time::Duration::from_secs(90)), Ok(true));
        assert!(joining.session.inputs[0].set_ready(true));
        assert_eq!(joining.poll(now + std::time::Duration::from_secs(90)), Ok(true));
        joining.session.inputs[0].close();
        assert!(joining.poll(now).is_err());
    }

    #[test]
    fn greeted_ready_inputs_do_not_replace_owned_connections() {
        let session = super::Session::new();
        assert!(!session.can_load());
        for input in &session.inputs {
            assert!(input.greet(b"Driver"));
            assert!(input.set_ready(true));
        }
        assert!(!session.can_load());
        for input in &session.inputs {
            assert!(input.wait_stage(super::Stage::Ready, std::time::Duration::ZERO));
        }
        let inputs = session.inputs.clone();
        drop(session);
        for input in inputs {
            assert!(input.state.lock().unwrap().stage == super::Stage::Closed);
        }
    }

    #[test]
    fn setup_waits_keep_coordinator_running() {
        let session = super::Session::new();
        for input in &session.inputs { assert!(input.greet(b"Driver")); }
        let deadline = super::Instant::now() + std::time::Duration::from_secs(3);
        assert!(session.wait_for(super::Stage::Ready, Some(deadline), || {
            for input in &session.inputs { assert!(input.set_ready(true)); }
        }));
        assert!(session.load_plan(super::Plan::default()).is_ok());
        assert!(session.wait_for(super::Stage::Loaded, Some(deadline), || {
            for input in &session.inputs { assert!(input.loaded()); }
        }));
        session.inputs[1].close();
        assert!(!session.wait_for(super::Stage::Loaded, Some(deadline),
        || {}));
    }

    #[cfg(unix)]
    #[test]
    fn duplex_stream_delivers_selected_input_snapshot_and_result() {
        use std::io::{Read, Write};
        use std::os::unix::net::UnixStream;
        let (mut peer, server) = UnixStream::pair().unwrap();
        for stream in [&peer, &server] {
            stream.set_read_timeout(Some(std::time::Duration::from_secs(3))).unwrap();
            stream.set_write_timeout(Some(std::time::Duration::from_secs(3))).unwrap();
        }
        let session = super::Session::new();
        let input = session.inputs[0].clone();
        let receiver = input.clone();
        let mut reader_stream = server.try_clone().unwrap();
        let reader = std::thread::spawn(move || {
            super::read_client(&mut reader_stream, &receiver);
            let controls = receiver.take();
            receiver.close();
            controls
        });
        // Fragment every setup byte across writes; production Read adapters
        // must preserve complete-message semantics on a real duplex stream.
        for byte in [super::wire::C2S_HELLO, 1, b'D', super::wire::C2S_PICK, 9, 1,
        super::wire::C2S_READY, 1] {
            peer.write_all(&[byte]).unwrap();
        }
        assert!(input.wait_stage(Stage::Ready, std::time::Duration::from_secs(3)));
        assert!(session.inputs[1].greet(b"Other"));
        assert!(session.inputs[1].set_ready(true));
        let plan = session.load_plan(super::Plan::default()).unwrap();
        assert_eq!(plan.seats[0].model, 9);
        assert!(plan.seats[0].manual);
        peer.write_all(&[super::wire::C2S_LOADED]).unwrap();
        assert!(input.wait_stage(Stage::Loaded, std::time::Duration::from_secs(3)));
        assert!(input.start(super::Instant::now()));
        let mut packet = [0; 12];
        packet[5..7].copy_from_slice(&256i16.to_le_bytes());
        peer.write_all(&[super::wire::C2S_INPUT]).unwrap();
        peer.write_all(&packet).unwrap();
        // EOF ends the reader after its complete input, without a timed poll.
        peer.shutdown(std::net::Shutdown::Write).unwrap();
        assert_eq!(reader.join().unwrap(), Some(packet));
        assert_eq!(input.state.lock().unwrap().packet, [0; 12]);
        let output = session.outputs[0].clone();
        let mut writer_stream = server;
        let writer_output = output.clone();
        let writer = std::thread::spawn(move || super::write_client(&mut writer_stream, &writer_output));
        let snapshot: std::sync::Arc<[u8]> = std::sync::Arc::new(super::snapshot_message(
        42, 0, super::sim::SimRacePhase_SIM_FINISHED as u8,
        [super::sim::CarPose::default(); super::FIELD_COUNT], [0; crate::SEAT_COUNT]));
        let result = super::encode_result([(false, 0, -1); 2]);
        output.publish(snapshot.clone());
        output.finish(result.into());
        let mut received = vec![0; snapshot.len() + result.len()];
        peer.read_exact(&mut received).unwrap();
        assert_eq!(&received[..snapshot.len()], snapshot.as_ref());
        assert_eq!(&received[snapshot.len()..], result.as_slice());
        assert!(writer.join().unwrap());
    }
    use super::{Input, SharedInput, Outbox, Stage, wire};
    use std::sync::Arc;

    #[test]
    fn explicit_room_selection_never_falls_back_to_another_room() {
        let now = super::Instant::now();
        let mut rooms = vec![super::Joining::new(now, 11), super::Joining::new(now, 12)];
        for room in &mut rooms { room.hello[0] = Some(now); }
        assert_eq!(super::select_room(&rooms, 12, false), Ok(Some(1)));
        assert_eq!(super::select_room(&rooms, u64::MAX, false), Ok(Some(0)));
        assert_eq!(super::select_room(&rooms, 0, true), Ok(None));
        assert!(super::select_room(&rooms, 0, false).is_err());
        assert!(super::select_room(&rooms, 99, true).is_err());
        rooms[0].hello[1] = Some(now);
        assert!(super::select_room(&rooms, 11, true).is_err());
        assert_eq!(super::select_room(&rooms, u64::MAX, false), Ok(Some(1)));
        rooms[1].hello[1] = Some(now);
        assert!(super::select_room(&rooms, u64::MAX, false).is_err());
        assert_eq!(super::select_room(&rooms, u64::MAX, true), Ok(None));
    }

    #[test]
    fn directory_copies_owned_lobby_state_and_does_not_mutate_sessions() {
        let now = super::Instant::now();
        let room = super::Joining::new(now, 7);
        assert!(room.session.inputs[0].greet(b"Host"));
        assert!(room.session.inputs[0].configure_race([5, 3, 6, 1]));
        assert!(room.session.inputs[0].set_ready(true));
        let rooms = super::Rooms::default();
        let open = vec![room];
        let snapshot = rooms.directory(&open, &super::Plan::default());
        assert_eq!(snapshot[0], super::protocol::RoomInfo { code: 7, options: [5, 3, 6, 1],
            occupied: 1, ready: 1, state: 0 });
        assert!(open[0].session.inputs[1].greet(b"Guest"));
        assert!(open[0].session.inputs[1].set_ready(true));
        assert_eq!(snapshot[0].occupied, 1);
        assert_eq!(rooms.directory(&open, &super::Plan::default())[0].ready, 3);
        assert!(open[0].session.load_plan(super::Plan::default()).is_ok());
        let loading = rooms.directory(&open, &super::Plan::default());
        assert_eq!((loading[0].occupied, loading[0].ready, loading[0].state), (3, 0, 1));
        open[0].session.inputs[1].close();
        assert_eq!(rooms.directory(&open, &super::Plan::default())[0].occupied, 1);
    }

    #[test]
    fn readiness_wait_retries_changes_and_freezes_the_latest_choices() {
        let mut session = super::Session::new();
        for input in &session.inputs {
            assert!(input.greet(b"Driver") && input.set_ready(true));
        }
        let inputs = session.inputs.clone();
        let mut polls = 0;
        let selected = session.wait_plan(super::Plan::default(), || {
            polls += 1;
            if polls == 1 {
                assert!(inputs[1].set_ready(false));
                assert!(inputs[1].pick(31, 1));
            } else {
                assert!(inputs[1].set_ready(true));
            }
        }).unwrap();
        assert_eq!(polls, 2);
        assert_eq!((selected.seats[1].model, selected.seats[1].manual), (31, true));
        for input in &session.inputs { assert!(input.state.lock().unwrap().stage == Stage::Loading); }
    }

    #[test]
    fn readiness_wait_cancels_closed_or_invalid_rooms_without_partial_freeze() {
        let mut session = super::Session::new();
        for input in &session.inputs { assert!(input.greet(b"Driver") && input.set_ready(true)); }
        let invalid = super::Plan { laps: 0, ..Default::default() };
        assert_eq!(session.load_plan(invalid).err(), Some(super::Load::Invalid));
        let mut polls = 0;
        assert!(session.wait_plan(invalid, || polls += 1).is_none());
        assert_eq!(polls, 1);
        assert!(session.inputs[0].state.lock().unwrap().stage == Stage::Ready);
        let guest = session.inputs[1].clone();
        assert!(session.wait_plan(super::Plan::default(), || guest.close()).is_none());
        assert!(session.inputs[0].state.lock().unwrap().stage == Stage::Ready);
        assert_eq!(session.load_plan(super::Plan::default()).err(), Some(super::Load::Closed));
    }

    #[test]
    fn browsing_requests_are_coalesced_and_stop_after_room_selection() {
        let input = SharedInput::default();
        assert!(!input.request_list());
        assert!(input.greet(b"Browser"));
        for _ in 0..100 { assert!(input.request_list()); }
        assert!(input.take_list_request());
        assert!(!input.take_list_request());
        super::read_client(&mut std::io::Cursor::new([wire::C2S_LIST]), &input);
        assert!(input.take_list_request());
        assert!(input.choose_room(0));
        assert!(!input.request_list());
        input.close();
        assert!(!input.request_list());
    }

    #[test]
    fn lobby_views_own_peer_data_and_publish_only_changes() {
        let mut session = super::Session::new();
        assert!(session.inputs[0].greet(b"Host"));
        session.inputs[0].state.lock().unwrap().room = Some(42);
        assert!(session.inputs[1].greet(b"Guest"));
        assert!(session.inputs[1].pick(31, 1) && session.inputs[1].set_ready(true));
        let defaults = super::Plan::default();
        let view = super::lobby_view(42, &session.inputs, &defaults);
        assert_eq!(&view.seats[1].name[..view.seats[1].length as usize], b"Guest");
        assert_eq!((view.seats[1].variant, view.seats[1].manual, view.room.ready), (31, true, 2));
        assert!(session.publish_lobby(&defaults));
        let first = session.last_lobby.unwrap();
        assert!(session.publish_lobby(&defaults));
        assert_eq!(session.last_lobby, Some(first));
        assert!(session.inputs[1].pick(9, 0));
        assert!(session.publish_lobby(&defaults));
        assert_ne!(session.last_lobby, Some(first));
        assert_eq!(view.seats[1].variant, 31);
        session.inputs[1].close();
        let current = super::lobby_view(42, &session.inputs, &defaults);
        assert_eq!(current.room.occupied, 1);
        assert_eq!(current.seats[1], super::protocol::LobbySeat::default());
    }

    #[test]
    fn routing_transfers_connection_workers_and_binds_guest_permissions() {
        let mut destination = super::Session::new();
        assert!(destination.inputs[0].greet(b"Host"));
        let mut source = super::Session::new();
        let input = source.inputs[0].clone();
        let output = source.outputs[0].clone();
        assert!(input.greet(b"Guest") && input.choose_room(11) && input.pick(9, 1));
        let reader = input.clone();
        source.readers[0] = Some(std::thread::spawn(move || {
            assert!(!reader.wait_stage(Stage::Ready, std::time::Duration::from_secs(3)));
        }));
        destination.receive(1, source).unwrap();
        assert!(Arc::ptr_eq(&input, &destination.inputs[1]));
        assert!(Arc::ptr_eq(&output, &destination.outputs[1]));
        assert!(destination.readers[1].is_some());
        assert_eq!(input.state.lock().unwrap().choice, super::Choice { variant: 9, manual: true });
        assert!(!input.configure_race([0, 0, 1, 0]));
        let rejected = super::Session::new();
        let rejected_input = rejected.inputs[0].clone();
        assert!(rejected_input.greet(b"Another"));
        assert_eq!(destination.receive(1, rejected).unwrap_err().kind(), std::io::ErrorKind::InvalidInput);
        assert!(rejected_input.state.lock().unwrap().stage == Stage::Closed);
        assert!(input.state.lock().unwrap().stage == Stage::Waiting);
        output.publish(Arc::from([7u8].as_slice()));
        assert_eq!(output.take().unwrap().as_ref(), &[7]);
        drop(destination);
        assert!(input.state.lock().unwrap().stage == Stage::Closed);
        assert!(output.take().is_none());
    }

    #[test]
    fn incoming_room_request_has_one_absolute_deadline_and_cancels_on_close() {
        let now = super::Instant::now();
        let incoming = super::Incoming { session: super::Session::new(),
            deadline: now + std::time::Duration::from_secs(5) };
        assert_eq!(incoming.request(now), Ok(None));
        assert!(incoming.request(now + std::time::Duration::from_secs(5)).is_err());
        let input = &incoming.session.inputs[0];
        assert!(!input.choose_room(0));
        assert!(input.greet(b"Creator"));
        assert!(!input.choose_room(i64::MAX as u64 + 1));
        assert!(input.choose_room(u64::MAX));
        assert!(!input.choose_room(0));
        assert_eq!(incoming.request(now), Ok(Some(u64::MAX)));
        input.close();
        assert!(incoming.request(now).is_err());
        let mut room = super::Joining::new(now, 5);
        room.join_deadline = None;
        room.hello[0] = Some(now);
        assert!(room.session.inputs[0].greet(b"Creator"));
        assert_eq!(room.poll(now + std::time::Duration::from_secs(86400)), Ok(false));
        // Welcome/availability now precede the car picker. Both greeted
        // participants can keep selecting without a hello/Ready deadline.
        room.hello[1] = Some(now);
        assert!(room.session.inputs[1].greet(b"Guest"));
        assert_eq!(room.poll(now + std::time::Duration::from_secs(172800)), Ok(true));
        for input in &room.session.inputs {
            assert!(input.state.lock().unwrap().stage == Stage::Waiting);
        }
        assert!(matches!(room.session.load_plan(super::Plan::default()), Err(super::Load::Waiting)));
    }

    #[test]
    fn room_request_reader_commits_only_a_complete_code() {
        use std::io::Cursor;
        let bytes = [wire::C2S_ROOM, 8, 7, 6, 5, 4, 3, 2, 1];
        for size in 0..bytes.len() {
            let input = SharedInput::default();
            assert!(input.greet(b"Driver"));
            super::read_client(&mut Cursor::new(&bytes[..size]), &input);
            assert!(input.state.lock().unwrap().room.is_none());
        }
        let input = SharedInput::default();
        assert!(input.greet(b"Driver"));
        super::read_client(&mut Cursor::new(bytes), &input);
        assert_eq!(input.state.lock().unwrap().room, Some(0x0102030405060708));
    }

    #[test]
    fn creator_race_options_freeze_atomically_with_car_choices() {
        let session = super::Session::new();
        for input in &session.inputs {
            assert!(input.greet(b"Driver"));
            assert!(input.set_ready(true));
        }
        assert!(!session.inputs[1].configure_race([5, 3, 6, 1]));
        assert!(session.inputs[1].state.lock().unwrap().race.is_none());
        assert!(!session.inputs[0].configure_race([6, 3, 6, 1]));
        assert!(session.inputs[0].wait_stage(Stage::Ready, std::time::Duration::ZERO));
        assert!(session.inputs[0].configure_race([5, 3, 6, 1]));
        assert!(session.load_plan(super::Plan::default()).is_err());
        assert!(session.inputs[1].wait_stage(Stage::Ready, std::time::Duration::ZERO));
        assert!(session.inputs[0].set_ready(true));
        let selected = session.load_plan(super::Plan::default()).unwrap();
        assert_eq!((selected.class, selected.course, selected.laps, selected.reverse), (5, 3, 6, true));
        let packet = super::start_message(&selected, *b"SCES_006.96\0\0\0\0\0", 1, 2).unwrap();
        let mut decoded = super::sim::MpStart::default();
        assert_ne!(unsafe { super::sim::MpDecodeStart(packet[1..].as_ptr(), packet.len() - 1,
            &mut decoded) }, 0);
        assert_eq!((decoded.classIndex, decoded.course, decoded.laps, decoded.reverse), (5, 3, 6, 1));
        assert!(!session.inputs[0].configure_race([0, 0, 1, 0]));
        let options = session.inputs[0].state.lock().unwrap().race.unwrap();
        assert_eq!((options.classIndex, options.course, options.laps, options.reverse), (5, 3, 6, 1));
    }

    #[test]
    fn race_options_reader_commits_only_complete_valid_messages() {
        use std::io::Cursor;
        for size in 0..5 {
            let session = super::Session::new();
            let input = &session.inputs[0];
            assert!(input.greet(b"Creator"));
            let message = [wire::C2S_RACE, 5, 3, 6, 1];
            super::read_client(&mut Cursor::new(&message[..size]), input);
            assert!(input.state.lock().unwrap().race.is_none());
        }
        let session = super::Session::new();
        let input = &session.inputs[0];
        assert!(input.greet(b"Creator"));
        super::read_client(&mut Cursor::new([wire::C2S_RACE, 5, 3, 6, 1]), input);
        let options = input.state.lock().unwrap().race.unwrap();
        assert_eq!((options.classIndex, options.course, options.laps, options.reverse), (5, 3, 6, 1));
    }

    #[test]
    fn waiting_room_setup_does_not_block_another_room() {
        use std::time::Duration;
        let mut rooms = super::Rooms::default();
        let session = super::Session::new();
        let waiting = session.inputs[0].clone();
        let (started, first_started) = std::sync::mpsc::channel();
        let id = rooms.new_id().unwrap();
        rooms.start(id, session, move |session| {
            started.send(()).unwrap();
            assert!(!session.inputs[0].wait_stage(Stage::Ready, Duration::from_secs(3)));
            Err("first setup cancelled".to_owned())
        }).unwrap();
        first_started.recv_timeout(Duration::from_secs(3)).unwrap();
        let second = super::Session::new();
        let second_input = second.inputs[0].clone();
        let (done, second_started) = std::sync::mpsc::channel();
        let id = rooms.new_id().unwrap();
        rooms.start(id, second, move |second| {
            done.send(()).unwrap();
            drop(second);
            Err("second setup cancelled".to_owned())
        }).unwrap();
        second_started.recv_timeout(Duration::from_secs(3)).unwrap();
        assert!(!rooms.active[0].task.as_ref().unwrap().is_finished());
        assert_eq!((rooms.active[0].id, rooms.active[1].id), (1, 2));
        waiting.close();
        drop(rooms); // Both session owners are closed and joined.
        assert!(waiting.take().is_none() && second_input.take().is_none());
    }

    #[test]
    fn room_owner_closes_and_joins_its_workers_without_closing_other_rooms() {
        use std::sync::atomic::{AtomicUsize, Ordering};
        let mut rooms = super::Rooms::default();
        let completed = Arc::new(AtomicUsize::new(0));
        let mut observers = Vec::new();
        for _ in 0..2 {
            let input = Arc::new(SharedInput::default());
            assert!(input.greet(b"Waiting"));
            observers.push(input.clone());
            let worker = input.clone();
            let completed = completed.clone();
            let task = std::thread::spawn(move || {
                assert!(!worker.wait_stage(Stage::Ready, std::time::Duration::from_secs(3)));
                assert!(worker.take().is_none());
                completed.fetch_add(1, Ordering::SeqCst);
                Err("fixture worker stopped".to_owned())
            });
            let inputs = std::array::from_fn(|seat| if seat == 0 { input.clone() }
            else { std::sync::Arc::new(super::SharedInput::default()) });
            rooms.active.push(super::Room { id: 71, inputs, task: Some(task) });
        }
        let other = SharedInput::default();
        assert!(other.greet(b"Other"));
        drop(rooms);
        assert_eq!(completed.load(Ordering::SeqCst), 2);
        for input in observers { assert!(input.take().is_none()); }
        assert!(other.set_ready(true));
    }

    #[test]
    fn session_gate_observes_the_entire_field_and_keeps_deadline() {
        let waiting = super::Session::new();
        let mut polls = 0;
        assert!(!waiting.wait_for(super::Stage::Ready, None, || {
            polls += 1;
            waiting.inputs[1].close();
        }));
        assert_eq!(polls, 1); // Untimed lobby wait still cancels promptly.
        let session = super::Session::new();
        let expired = super::Instant::now();
        assert!(!session.wait_for(super::Stage::Ready, Some(expired), || {}));
        for input in &session.inputs {
            assert!(input.greet(b"Driver"));
            assert!(input.set_ready(true));
        }
        assert!(session.wait_for(super::Stage::Ready, Some(expired), || {}));
        assert!(session.load_plan(super::Plan::default()).is_ok());
        assert!(session.inputs[0].loaded());
        assert!(!session.wait_for(super::Stage::Loaded, Some(expired), || {}));
        session.inputs[0].close();
        // A closed, previously loaded seat cancels immediately while the
        // other is still loading; an expired deadline keeps this test instant.
        assert!(!session.wait_for(super::Stage::Loaded, Some(expired), || {}));
        assert!(!session.wait_for(super::Stage::Loaded,
        Some(super::Instant::now() + std::time::Duration::from_secs(3)), || {}));
        let session = super::Session::new();
        for input in &session.inputs {
            assert!(input.greet(b"Driver"));
            assert!(input.set_ready(true));
        }
        assert!(session.load_plan(super::Plan::default()).is_ok());
        assert!(session.inputs[0].loaded());
        assert!(session.inputs[1].loaded());
        assert!(session.wait_for(super::Stage::Loaded, Some(expired), || {}));
        // Seat zero was ready/loaded before closing. It must still cancel
        // the field even if another seat has already reached the target.
        session.inputs[0].close();
        assert!(!session.wait_for(super::Stage::Loaded, Some(expired), || {}));
        assert!(!session.wait_for(super::Stage::Closed, Some(expired), || {}));
    }

    #[test]
    fn field_freeze_rejects_a_late_unready_seat_without_changing_other_seats() {
        let session = super::Session::new();
        for input in &session.inputs {
            assert!(input.greet(b"Driver"));
            assert!(input.pick(9, 1));
            assert!(input.set_ready(true));
        }
        let invalid = super::Plan { laps: 0, ..Default::default() };
        assert!(session.load_plan(invalid).is_err());
        for input in &session.inputs {
            assert!(input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        }
        assert!(session.inputs[1].set_ready(false));
        assert!(session.load_plan(super::Plan::default()).is_err());
        assert!(session.inputs[0].wait_stage(Stage::Ready, std::time::Duration::ZERO));
        assert!(session.inputs[0].pick(31, 1));
        assert!(session.inputs[0].set_ready(true));
        assert!(session.inputs[1].set_ready(true));
        let plan = session.load_plan(super::Plan::default()).unwrap();
        assert_eq!(plan.seats[0].model, 31);
        assert_eq!(plan.seats[1].model, 9);
        for input in &session.inputs {
            assert!(input.set_ready(false));
            assert!(input.state.lock().unwrap().stage == Stage::Loading);
        }
    }

    #[test]
    fn car_selection_revokes_ready_and_freezes_into_start_setup() {
        let session = super::Session::new();
        let input = &session.inputs[0];
        assert!(!input.pick(31, 1));
        assert!(input.greet(b"Driver"));
        assert!(input.pick(31, 1));
        assert!(input.set_ready(true));
        for (variant, manual) in [(32, 0), (255, 0), (0, 2), (0, 255)] {
            assert!(!input.pick(variant, manual));
            assert!(input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        }
        assert!(input.pick(9, 0));
        assert!(!input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        assert!(input.state.lock().unwrap().begin_load().is_none());
        assert!(input.set_ready(true));
        let other = &session.inputs[1];
        assert!(other.greet(b"Other"));
        assert!(other.pick(31, 1));
        assert!(other.set_ready(true));
        let plan = session.load_plan(super::Plan::default()).unwrap();
        assert!(input.pick(0, 1));
        assert!(other.pick(0, 0));
        assert_eq!(input.state.lock().unwrap().choice, super::Choice { variant: 9, manual: false });
        assert_eq!(other.state.lock().unwrap().choice, super::Choice { variant: 31, manual: true });
        let bytes = super::start_message(&plan, *b"SCES_006.96\0\0\0\0\0", 1, 2).unwrap();
        assert_eq!(&bytes[27..29], &[9, 0]);
        assert_eq!(&bytes[33..35], &[31, 1]);
        assert_eq!(plan.seats[0].seed, 0x1234_5670);
        assert_eq!(plan.seats[1].seed, 0x1234_5671);
    }

    #[test]
    fn pick_reader_commits_only_a_complete_valid_choice() {
        use std::io::Cursor;
        for bytes in [vec![wire::C2S_PICK], vec![wire::C2S_PICK, 31],
        vec![wire::C2S_PICK, 32, 0], vec![wire::C2S_PICK, 31, 2]] {
            let input = SharedInput::default();
            assert!(input.greet(b"Driver"));
            assert!(input.pick(3, 0));
            assert!(input.set_ready(true));
            super::read_client(&mut Cursor::new(bytes), &input);
            assert!(input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
            assert_eq!(input.state.lock().unwrap().choice.variant, 3);
        }
        let input = SharedInput::default();
        super::read_client(&mut Cursor::new([wire::C2S_PICK, 31, 1]), &input);
        assert_eq!(input.state.lock().unwrap().choice, super::Choice::default());
        assert!(input.greet(b"Driver"));
        super::read_client(&mut Cursor::new([wire::C2S_PICK, 31, 1, wire::C2S_READY, 1]), &input);
        assert_eq!(input.state.lock().unwrap().begin_load(), Some(super::Choice { variant: 31, manual: true }));
        super::read_client(&mut Cursor::new([wire::C2S_PICK, 0, 0, wire::C2S_LOADED]), &input);
        assert!(input.wait_stage(Stage::Loaded, std::time::Duration::ZERO));
        assert_eq!(input.state.lock().unwrap().choice.variant, 31);
    }

    #[test]
    fn ready_can_be_revoked_before_loading_and_close_wakes_waiters() {
        let input = SharedInput::default();
        assert!(!input.set_ready(true));
        assert!(input.greet(b"Driver"));
        assert!(input.state.lock().unwrap().begin_load().is_none());
        assert!(!input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        assert!(input.set_ready(true));
        assert!(input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        assert!(input.set_ready(false));
        assert!(input.state.lock().unwrap().begin_load().is_none());
        assert!(input.set_ready(true));
        assert!(input.state.lock().unwrap().begin_load().is_some());
        assert!(input.set_ready(false));
        assert!(input.state.lock().unwrap().stage == Stage::Loading);
        assert!(input.loaded());
        assert!(input.set_ready(true));
        assert!(input.state.lock().unwrap().stage == Stage::Loaded);
        for ready in [false, true] {
            let input = Arc::new(SharedInput::default());
            assert!(input.greet(b"Waiting"));
            let receiver = input.clone();
            let waiter = std::thread::spawn(move || receiver.wait_stage(Stage::Ready, std::time::Duration::from_secs(3)));
            if ready { assert!(input.set_ready(true)); } else { input.close(); }
            assert_eq!(waiter.join().unwrap(), ready);
        }
    }

    #[test]
    fn ready_reader_requires_a_complete_boolean_after_hello() {
        use std::io::Cursor;
        for bytes in [vec![wire::C2S_READY], vec![wire::C2S_READY, 2], vec![wire::C2S_READY, 255]] {
            let input = SharedInput::default();
            assert!(input.greet(b"Driver"));
            super::read_client(&mut Cursor::new(bytes), &input);
            assert!(!input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        }
        let input = SharedInput::default();
        super::read_client(&mut Cursor::new([wire::C2S_READY, 1]), &input);
        assert!(!input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        assert!(input.greet(b"Driver"));
        super::read_client(&mut Cursor::new([wire::C2S_READY, 1]), &input);
        assert!(input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        super::read_client(&mut Cursor::new([wire::C2S_READY, 0]), &input);
        assert!(!input.wait_stage(Stage::Ready, std::time::Duration::ZERO));
        super::read_client(&mut Cursor::new([wire::C2S_READY, 1]), &input);
        assert!(input.state.lock().unwrap().begin_load().is_some());
        super::read_client(&mut Cursor::new([wire::C2S_READY, 0, wire::C2S_LOADED]), &input);
        assert!(input.wait_stage(Stage::Loaded, std::time::Duration::ZERO));
    }

    #[test]
    fn incomplete_joining_room_does_not_block_or_cancel_another() {
        let now = super::Instant::now();
        let mut first = super::Joining::new(now, 7);
        first.hello = [Some(now); super::SEAT_COUNT];
        let mut second = super::Joining::new(now, 7);
        second.hello = [Some(now + std::time::Duration::from_secs(5)); super::SEAT_COUNT];
        for input in &second.session.inputs { assert!(input.greet(b"Driver")); }
        assert!(first.poll(now).is_err());
        assert_eq!(second.poll(now), Ok(true));
        let observer = first.session.inputs[0].clone();
        drop(first);
        assert!(observer.state.lock().unwrap().stage == Stage::Closed);
        assert_eq!(second.poll(now), Ok(true));
    }

    #[test]
    fn hello_name_is_owned_by_the_seat_and_committed_only_when_complete() {
        use std::io::Cursor;
        let name = b"Ana\xFF";
        let mut hello = vec![wire::C2S_HELLO, name.len() as u8];
        hello.extend_from_slice(name);
        for length in 0..hello.len() {
            let input = SharedInput::default();
            super::read_client(&mut Cursor::new(&hello[..length]), &input);
            assert!(input.state.lock().unwrap().name.is_none());
            assert!(!input.wait_hello(std::time::Duration::ZERO));
        }
        let input = SharedInput::default();
        super::read_client(&mut Cursor::new(&hello), &input);
        assert_eq!(input.state.lock().unwrap().name.as_deref(), Some(&name[..]));
        assert!(!input.greet(b"Replacement"));
        assert_eq!(input.state.lock().unwrap().name.as_deref(), Some(&name[..]));
        input.close();
        assert_eq!(input.state.lock().unwrap().name.as_deref(), Some(&name[..]));
        assert!(!input.wait_hello(std::time::Duration::ZERO));
        let other = SharedInput::default();
        assert!(!other.greet(&[b'A'; 16]));
        assert!(other.state.lock().unwrap().name.is_none());
        assert!(other.greet(&[b'A'; 15]));
        assert_eq!(other.state.lock().unwrap().name.as_deref(), Some(&[b'A'; 15][..]));
        assert_eq!(input.state.lock().unwrap().name.as_deref(), Some(&name[..]));
        let anonymous = SharedInput::default();
        assert!(anonymous.greet(b""));
        assert!(anonymous.wait_hello(std::time::Duration::ZERO));
    }

    #[test]
    fn stale_race_input_retires_without_retaining_controls() {
        let input = SharedInput::default();
        assert!(input.greet(b"Player"));
        assert!(input.set_ready(true)); assert!(input.state.lock().unwrap().begin_load().is_some());
        assert!(input.loaded());
        let mut packet = [0; 12];
        packet[5] = 200;
        packet[9] = 1;
        assert!(input.publish(packet));
        let origin = std::time::Instant::now();
        assert!(input.start(origin));
        assert_eq!(input.take_at(origin + super::INPUT_TIMEOUT - std::time::Duration::from_nanos(1)), Some(packet));
        assert_eq!(input.take_at(origin + super::INPUT_TIMEOUT), None);
        assert_eq!(input.state.lock().unwrap().packet, [0; 12]);
        assert!(!input.publish(packet));
        assert!(!input.start(origin));
        assert_eq!(input.take_at(origin), None);
    }

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
    fn sparse_session_drop_joins_waiting_workers_without_closing_another_session() {
        use std::sync::atomic::{AtomicUsize, Ordering};
        let mut first = super::Session::new();
        let second = super::Session::new();
        let observer = first.inputs[1].clone();
        let completed = Arc::new(AtomicUsize::new(0));
        let input = first.inputs[1].clone();
        let done = completed.clone();
        first.readers[1] = Some(std::thread::spawn(move || {
            assert!(!input.wait_stage(Stage::Loaded, std::time::Duration::from_secs(3)));
            done.fetch_add(1, Ordering::SeqCst);
        }));
        let output = first.outputs[0].clone();
        let done = completed.clone();
        first.writers[1] = Some(std::thread::spawn(move || {
            assert!(output.take().is_none());
            done.fetch_add(1, Ordering::SeqCst);
            true
        }));
        drop(first);
        assert_eq!(completed.load(Ordering::SeqCst), 2);
        assert!(observer.take().is_none());
        assert!(second.inputs[0].greet(b"Other"));
        assert!(second.inputs[0].pick(9, 1));
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
        assert!(input.state.lock().unwrap().begin_load().is_none());
        input.close(); /* The session cancels after its per-seat deadline. */
        assert!(!input.greet(b"Player"));
        assert!(input.state.lock().unwrap().begin_load().is_none());
        assert!(!input.wait_stage(Stage::Loaded, std::time::Duration::ZERO));
    }

    #[test]
    fn cancelling_session_clears_every_seat_and_pending_message() {
        let inputs: Vec<_> = (0..super::SEAT_COUNT)
        .map(|_| Arc::new(loaded_input())).collect();
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
            assert!(!input.wait_stage(Stage::Loaded, std::time::Duration::ZERO));
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
        let input = racing_input();
        assert_eq!(input.take(), Some([0; 12]));
        let mut first = [0; 12];
        first[5] = 100;
        first[9] = 1;
        assert!(input.publish(first));
        let mut second = [0; 12];
        second[5] = 200;
        second[10] = 1;
        assert!(input.publish(second));
        second[9] = 1;
        assert_eq!(input.take(), Some(second));
        second[9] = 0;
        second[10] = 0;
        assert_eq!(input.take(), Some(second));
        assert_eq!(input.take(), Some(second));
    }

    #[test]
    fn reader_and_tick_never_mix_control_packets() {
        let input = Arc::new(racing_input());
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
        let input = racing_input();
        let mut valid = [0; 12];
        valid[5] = 200;
        valid[9] = 1;
        assert!(input.publish(valid));
        let last_input = input.state.lock().unwrap().last_input;
        for (index, value) in [(0, 3), (1, 2), (2, 2), (6, 2), (8, 2), (9, 2), (10, 2), (11, 1)] {
            let mut invalid = valid;
            invalid[index] = value;
            assert!(!input.publish(invalid));
            assert_eq!(input.state.lock().unwrap().last_input, last_input);
        }
        assert_eq!(input.take(), Some(valid));
    }

    #[test]
    fn closed_input_cannot_retain_throttle_or_accept_late_packets() {
        let input = loaded_input();
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
            assert!(super::write_client(&mut sink, &receiver));
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
            let hello_end = length.min(5);
            super::read_client(&mut Cursor::new(&valid[..hello_end]), &input);
            if length >= 5 {
                assert!(input.set_ready(true)); assert!(input.state.lock().unwrap().begin_load().is_some());
                super::read_client(&mut Cursor::new(&valid[5..length]), &input);
            }
            assert!(input.take().is_none());
            assert_eq!(input.state.lock().unwrap().packet, [0; 12]);
        }
        let input = SharedInput::default();
        super::read_client(&mut Cursor::new(&valid[..5]), &input);
        assert!(input.set_ready(true)); assert!(input.state.lock().unwrap().begin_load().is_some());
        super::read_client(&mut Cursor::new(&valid[5..]), &input);
        assert!(input.start(super::Instant::now()));
        assert_eq!(input.take(), Some(packet));
        let mut duplicate_after_input = valid.clone();
        duplicate_after_input.extend_from_slice(&[wire::C2S_HELLO, 0, wire::C2S_INPUT]);
        duplicate_after_input.extend_from_slice(&[0; 12]);
        let input = SharedInput::default();
        super::read_client(&mut Cursor::new(&duplicate_after_input[..5]), &input);
        assert!(input.set_ready(true)); assert!(input.state.lock().unwrap().begin_load().is_some());
        super::read_client(&mut Cursor::new(&duplicate_after_input[5..]), &input);
        assert!(input.start(super::Instant::now()));
        assert_eq!(input.take(), Some(packet));

        let mut no_hello = vec![wire::C2S_INPUT];
        no_hello.extend_from_slice(&packet);
        for invalid in [no_hello, vec![wire::C2S_HELLO, 16], vec![255]] {
            let input = SharedInput::default();
            super::read_client(&mut Cursor::new(invalid), &input);
            assert!(input.take().is_none());
            assert_eq!(input.state.lock().unwrap().packet, [0; 12]);
        }
        let mut duplicate = vec![wire::C2S_HELLO, 0, wire::C2S_HELLO, 0, wire::C2S_INPUT];
        duplicate.extend_from_slice(&packet);
        let input = SharedInput::default();
        super::read_client(&mut Cursor::new(duplicate), &input);
        assert!(input.take().is_none());
        assert_eq!(input.state.lock().unwrap().packet, [0; 12]);
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
        let mut bytes = vec![wire::C2S_LOADED, wire::C2S_INPUT];
        bytes.extend_from_slice(&first);
        bytes.push(wire::C2S_INPUT);
        bytes.extend_from_slice(&second);
        let input = SharedInput::default();
        super::read_client(&mut ByteReader(Cursor::new(vec![wire::C2S_HELLO, 0])), &input);
        assert!(input.set_ready(true)); assert!(input.state.lock().unwrap().begin_load().is_some());
        super::read_client(&mut ByteReader(Cursor::new(bytes)), &input);
        assert!(input.start(super::Instant::now()));
        second[9] = 1;
        assert_eq!(input.take(), Some(second));
    }

    #[test]
    fn race_start_waits_for_hello_and_rejects_closed_seats() {
        use std::time::Duration;
        let input = SharedInput::default();
        assert!(!input.wait_hello(Duration::ZERO));
        assert!(input.greet(b"Player"));
        assert!(input.wait_hello(Duration::ZERO));
        assert!(!input.greet(b"Player"));
        input.close();
        assert!(!input.wait_hello(Duration::ZERO));
        assert!(!input.greet(b"Player"));

        for disconnect in [false, true] {
            let input = Arc::new(SharedInput::default());
            let receiver = input.clone();
            let waiter = std::thread::spawn(move || receiver.wait_hello(Duration::from_secs(3)));
            if disconnect { input.close(); } else { assert!(input.greet(b"Player")); }
            assert_eq!(waiter.join().unwrap(), !disconnect);
        }
    }

    #[test]
    fn loaded_gate_requires_hello_and_wakes_on_load_or_close() {
        use std::time::Duration;
        assert!(Input::default().begin_load().is_none());
        assert!(!SharedInput::default().loaded());
        for load in [true, false] {
            let input = Arc::new(SharedInput::default());
            assert!(input.greet(b"Player"));
            assert!(!input.loaded());
            assert!(input.set_ready(true)); assert!(input.state.lock().unwrap().begin_load().is_some());
            assert!(input.state.lock().unwrap().begin_load().is_none());
            assert!(!input.wait_stage(Stage::Loaded, Duration::ZERO));
            let receiver = input.clone();
            let waiter = std::thread::spawn(move || receiver.wait_stage(Stage::Loaded, Duration::from_secs(3)));
            if load {
                assert!(input.loaded());
                assert!(!input.loaded());
            } else {
                input.close();
                assert!(!input.loaded());
            }
            assert_eq!(waiter.join().unwrap(), load);
            input.close();
            assert!(!input.wait_stage(Stage::Loaded, Duration::ZERO));
        }
    }

    #[test]
    fn reader_rejects_input_before_loaded_and_duplicate_loaded() {
        use std::io::Cursor;
        let mut packet = [0; 12];
        packet[6] = 1;
        for (case, prefix) in [
        vec![wire::C2S_LOADED],
        vec![],
        vec![wire::C2S_LOADED],
        vec![wire::C2S_LOADED, wire::C2S_LOADED],
        ].into_iter().enumerate() {
            let mut bytes = prefix;
            bytes.push(wire::C2S_INPUT);
            bytes.extend_from_slice(&packet);
            let input = SharedInput::default();
            if case != 0 {
                super::read_client(&mut Cursor::new([wire::C2S_HELLO, 0]), &input);
            }
            if case == 3 { assert!(input.set_ready(true)); assert!(input.state.lock().unwrap().begin_load().is_some()); }
            super::read_client(&mut Cursor::new(bytes), &input);
            assert!(input.take().is_none());
            assert_eq!(input.state.lock().unwrap().packet, [0; 12]);
        }
    }

    #[test]
    fn startup_race_options_select_one_valid_plan() {
        let catalog_args = vec!["disc.cue".into(), "--cars=custom cars.toml".into()];
        assert_eq!(super::parse_start(&catalog_args).unwrap().cars, Some("custom cars.toml"));
        for args in [vec!["disc.cue", "--cars="], vec!["disc.cue", "--cars=a", "--cars=b"]] {
            assert!(super::parse_start(&args.into_iter().map(String::from).collect::<Vec<_>>()).is_err());
        }
        let args: Vec<String> = ["disc.cue", "--reverse", "--class=6", "--course=4",
        "--laps=1", "--reimport"].into_iter().map(String::from).collect();
        let start = super::parse_start(&args).unwrap();
        assert_eq!(start.port, 7243);
        assert!(start.reimport);
        assert_eq!((start.plan.class, start.plan.course, start.plan.laps), (5, 3, 1));
        assert!(start.plan.reverse);
        let wire = super::start_message(&start.plan, *b"SCES_006.96\0\0\0\0\0", 123, 456).unwrap();
        assert_eq!(&wire[2..6], &[3, 5, 1, 1]);
        for option in ["--class=0", "--class=7", "--course=0", "--course=5",
        "--laps=0", "--laps=7", "--class=-1", "--course=1x", "--laps=",
        "--laps=99999999999999999999", "--reverse=1", "--reimport=", "--unknown"] {
            let args = vec![String::from("disc.cue"), option.into()];
            assert!(super::parse_start(&args).is_err(), "accepted {option}");
        }
        for option in ["--class=1", "--course=1", "--laps=3", "--reverse", "--reimport"] {
            let args = vec![String::from("disc.cue"), option.into(), option.into()];
            assert!(super::parse_start(&args).is_err(), "accepted duplicate {option}");
        }
    }

    #[test]
    fn startup_arguments_require_an_explicit_valid_port() {
        let parse = |args: &[&str]| {
            let args: Vec<String> = args.iter().map(|arg| arg.to_string()).collect();
            super::parse_start(&args).map(|options| options.port)
        };
        assert_eq!(parse(&["disc.cue"]), Ok(7243));
        assert_eq!(parse(&["disc.cue", "--reimport"]), Ok(7243));
        assert_eq!(parse(&["disc.cue", "1234", "--reimport"]), Ok(1234));
        let forced = ["disc.cue".to_string(), "--reimport".to_string()];
        assert!(super::parse_start(&forced).unwrap().reimport);
        assert_eq!(parse(&["Track 01.bin", "1"]), Ok(1));
        assert_eq!(parse(&["disc.cue", "65535"]), Ok(65535));
        for port in ["", "0", "65536", "-1", "+1", "12x", " 7243", "1.5"] {
            assert!(parse(&["disc.cue", port]).is_err(), "accepted {port:?}");
        }
        for args in [vec![], vec![""], vec!["disc.cue", "7243", "extra"]] {
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
    fn snapshot_keeps_header_and_both_seat_boundaries() {
        let first = super::sim::CarPose { status: 1, x: i32::MIN, roll_speed: -7, speed: i32::MAX, place: 2, ..Default::default() };
        let second = super::sim::CarPose { status: 2, x: i32::MAX, roll_speed: 19, speed: i32::MIN, place: 1, ..Default::default() };
        let last = super::sim::CarPose { status: 1, x: -250, rpm: 4200, lap: 1,
            place: 12, ..Default::default() };
        let bytes = super::snapshot_message(0x12345678, 0x01020304, 2,
        std::array::from_fn(|seat| match seat {
            0 => first, 1 => second, seat if seat == super::FIELD_COUNT - 1 => last,
            _ => Default::default(),
        }), [0x01020304, u32::MAX]);
        let mut expected = [0; 10 + super::FIELD_COUNT * 77 + super::SEAT_COUNT * 4];
        expected[..10].copy_from_slice(&[0x83, 0x78, 0x56, 0x34, 0x12, 4, 3, 2, 1, 2]);
        expected[10] = 1;
        expected[11..15].copy_from_slice(&i32::MIN.to_le_bytes());
        expected[71..75].copy_from_slice(&(-7i32).to_le_bytes());
        expected[75..79].copy_from_slice(&i32::MAX.to_le_bytes());
        expected[83..87].copy_from_slice(&2i32.to_le_bytes());
        expected[87] = 2;
        expected[88..92].copy_from_slice(&i32::MAX.to_le_bytes());
        expected[148..152].copy_from_slice(&19i32.to_le_bytes());
        expected[152..156].copy_from_slice(&i32::MIN.to_le_bytes());
        expected[160..164].copy_from_slice(&1i32.to_le_bytes());
        let end = 10 + (super::FIELD_COUNT - 1) * 77;
        expected[end] = 1;
        expected[end + 1..end + 5].copy_from_slice(&(-250i32).to_le_bytes());
        expected[end + 41..end + 45].copy_from_slice(&4200i32.to_le_bytes());
        expected[end + 69] = 1;
        expected[end + 73] = 12;
        let ack = 10 + super::FIELD_COUNT * 77;
        expected[ack..ack + 4].copy_from_slice(&0x01020304u32.to_le_bytes());
        expected[ack + 4..].copy_from_slice(&u32::MAX.to_le_bytes());
        assert_eq!(bytes, expected);
        let mut decoded = super::sim::MpSnapshot::default();
        assert_ne!(unsafe { super::sim::MpDecodeSnapshot(bytes[1..].as_ptr(),
            bytes.len() - 1, &mut decoded) }, 0);
        assert_eq!(decoded.seats[super::FIELD_COUNT - 1].rpm, 4200);
        assert_eq!(decoded.acknowledged, [0x01020304, u32::MAX]);
        let packet: Arc<[u8]> = Arc::new(bytes);
        let outputs = [Outbox::default(), Outbox::default()];
        for output in &outputs { output.publish(packet.clone()); }
        for output in &outputs {
            let sent = output.take().unwrap();
            assert!(Arc::ptr_eq(&sent, &packet));
            assert_eq!(&*sent, &expected);
        }
    }



}
