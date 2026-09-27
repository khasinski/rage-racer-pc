//! Thin safe wrapper around the generated `rage-sim`/`rage-data` FFI bindings
//! (see ../build.rs and ../../docs/multiplayer.md, "Simulation API"). This
//! owns exactly the objects the C API returns and frees them on drop; it
//! does not add any policy beyond what the C functions already document.

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use std::ffi::CString;

pub fn decode_input(packet: &[u8; 12]) -> Option<DriverInput> {
    if packet[11] != 0 { return None; }
    let input = DriverInput {
        steering: SteeringInput {
            mode: packet[0] as u32,
            left: packet[1] as i32,
            right: packet[2] as i32,
            angle: i16::from_le_bytes([packet[3], packet[4]]) as i32,
        },
        throttle: i16::from_le_bytes([packet[5], packet[6]]),
        brake: i16::from_le_bytes([packet[7], packet[8]]),
        shiftUp: packet[9] as i32,
        shiftDown: packet[10] as i32,
    };
    if unsafe { ValidDriverInput(&input) } != 0 { Some(input) } else { None }
}

pub struct RaceArchive {
    data: *mut RaceData,
    // Imported bytes are immutable. Hash once per load, including cache
    // validation, rather than scanning the whole archive for every room.
    fingerprint: u64,
    catalog: Option<Box<RageCarCatalog>>,
}

impl RaceArchive {
    pub fn load_cache(path: &std::path::Path) -> Option<Self> {
        let cached = super::cache::read(path).ok()?;
        let cpath = CString::new(cached.file.to_str()?).ok()?;
        let ptr = unsafe { LoadRaceArchive(cpath.as_ptr()) };
        if ptr.is_null() { return None; }
        let archive = Self { data: ptr, catalog: None,
            fingerprint: unsafe { ArchiveFingerprint((*ptr).data.cast(), (*ptr).size) } };
        if archive.fingerprint() != cached.fingerprint { return None; }
        unsafe {
            (*ptr).boot = cached.boot.map(|byte| byte as _);
            (*ptr).executable = cached.executable;
        }
        Some(archive)
    }

    pub fn save_cache(&self, path: &std::path::Path) -> std::io::Result<()> {
        if let Ok(cached) = super::cache::read(path) {
            if cached.boot == self.boot() && cached.executable == self.executable() && cached.fingerprint == self.fingerprint() {
                return Ok(());
            }
        }
        let archive = unsafe { &*self.data };
        let data = unsafe { std::slice::from_raw_parts(archive.data, archive.size) };
        super::cache::write(path, self.boot(), self.executable(), data)
    }

    /// Loads a CUE or raw Track 01 BIN. `LoadRaceDisc` (rage-data) does not
    /// touch SDL, audio or game state.
    pub fn load_disc(path: &str) -> Option<Self> {
        let cpath = CString::new(path).ok()?;
        let ptr = unsafe { LoadRaceDisc(cpath.as_ptr()) };
        if ptr.is_null() {
            None
        } else {
            Some(Self { data: ptr, catalog: None,
                fingerprint: unsafe { ArchiveFingerprint((*ptr).data.cast(), (*ptr).size) } })
        }
    }

    pub fn executable(&self) -> u64 { unsafe { (*self.data).executable } }

    pub fn load_catalog(&mut self, path: &str) -> Result<(), String> {
        let path = CString::new(path).map_err(|_| "invalid catalog path")?;
        let mut catalog = Box::new(RageCarCatalog::default());
        let mut error = [0; 256];
        if unsafe { LoadCarCatalog(path.as_ptr(), catalog.as_mut(), error.as_mut_ptr(), error.len()) } == 0 {
            return Err(unsafe { std::ffi::CStr::from_ptr(error.as_ptr()) }.to_string_lossy().into_owned());
        }
        self.catalog = Some(catalog);
        Ok(())
    }

    pub fn automatic(&self, variant: i32) -> Option<bool> {
        let mut available = 0;
        if unsafe { ReadRaceCarTransmission(self.data, variant, &mut available) } == 0 {
            return None;
        }
        if let Some(catalog) = &self.catalog {
            for entry in &catalog.entries[..catalog.count] {
                if unsafe { CarCatalogVariant(entry.modelIndex, entry.grade) } == variant &&
                    entry.fields & RageCarCatalogField_RAGE_CAR_FIELD_MANUAL_ONLY as u64 != 0 {
                    return Some(entry.manualOnly == 0);
                }
            }
        }
        Some(available != 0)
    }
    pub fn availability_message(&self) -> Option<Vec<u8>> {
        let mut mask = 0;
        for variant in 0..CAR_MODEL_VARIANT_COUNT as i32 {
            if self.automatic(variant)? { mask |= 1u32 << variant; }
        }
        let mut packet = vec![0; MP_AVAILABILITY_WIRE_SIZE as usize];
        if unsafe { MpEncodeAvailability(mask, packet.as_mut_ptr(), packet.len()) } == 0 { return None; }
        Some(packet)
    }

    pub fn fingerprint(&self) -> u64 {
        self.fingerprint
    }

    pub fn boot(&self) -> [u8; 16] {
        std::array::from_fn(|i| unsafe { (*self.data).boot[i] as u8 })
    }

    pub fn copy_track(&self, class_index: i32, course_index: i32) -> Option<TrackDataOwned> {
        let ptr = unsafe { CopyRaceTrack(self.data, class_index, course_index) };
        if ptr.is_null() {
            None
        } else {
            Some(TrackDataOwned(ptr))
        }
    }

    pub fn rivals(&self, class: i32, course: i32, reverse: bool)
                  -> Option<[Option<crate::Rival>; crate::AI_COUNT]> {
        let track = self.copy_track(class, course)?;
        let events = unsafe { (*track.as_ptr()).events.as_ref()? };
        Some(std::array::from_fn(|i| {
            let seat = crate::SEAT_COUNT + i;
            (events.rivalStarts[reverse as usize][seat].activeFlag != -1).then_some(
                crate::Rival { model: (seat - 1) as u8, slot: (seat - 1) as u8,
                    seed: 0xA17E_0000 + seat as u32 })
        }))
    }

    fn as_ptr(&self) -> *const RaceData {
        self.data
    }
}

impl Drop for RaceArchive {
    fn drop(&mut self) {
        unsafe { FreeRaceData(self.data) };
    }
}

// Safety: RaceData is an immutable view over the loader's own owned buffer
// once returned. Ownership can move between threads; import completes before
// publication to room workers, which construct independently owned races.
unsafe impl Send for RaceArchive {}
// The archive is immutable after import. Each copy_track/ReadRaceCar call
// reads those retained bytes and constructs independent owned output; room
// workers never mutate the source. Arc keeps it alive through setup.
unsafe impl Sync for RaceArchive {}

pub struct TrackDataOwned(*mut TrackData);

impl TrackDataOwned {
    fn as_ptr(&self) -> *const TrackData {
        self.0
    }
}

impl Drop for TrackDataOwned {
    fn drop(&mut self) {
        unsafe { FreeTrackData(self.0) };
    }
}

unsafe impl Send for TrackDataOwned {}

/// One car's pose, read out of a stepped `RaceSim` for a network snapshot.
#[derive(Clone, Copy, Debug, Default)]
pub struct CarPose {
    pub status: u8,
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub yaw: i32,
    pub pitch: i32,
    pub roll: i32,
    pub steering: i32,
    pub wheels: i32,
    pub brake: i32,
    pub progress: i32,
    pub rpm: i32,
    pub throttle: i32,
    pub clutch: i32,
    pub gear: i32,
    pub ground: i32,
    pub roll_speed: i32,
    pub speed: i32,
    pub lap: i32,
    pub place: i32,
}

/// Owns a `RaceSim` context. Construction borrows the archive/track for the
/// call only, matching `InitRaceGrid`'s documented contract (the race copies
/// what it needs); this wrapper still keeps them alive for the whole race's
/// lifetime because `route`/`events` inside `RaceSim` borrow the track view.
pub struct Race {
    sim: Box<RaceSim>,
    _track: TrackDataOwned,
}

pub enum SeatPlan {
    Empty,
    Human { grid: i32, variant: i32, manual: bool, seed: u32 },
    Rival { grid: i32, model: i32, slot: i32, seed: u32 },
}

impl Race {
    pub fn config_message(&self) -> Option<Vec<u8>> {
        if self.sim.phase != SimRacePhase_SIM_SETUP { return None; }
        let mut config = MpCarConfig::default();
        for seat in 0..crate::SEAT_COUNT {
            let driver = &self.sim.drivers[seat];
            if driver.rival != 0 || driver.status != SimDriverStatus_SIM_DRIVING ||
                driver.variant < 0 || driver.variant >= CAR_MODEL_VARIANT_COUNT as i32 { return None; }
            config.variant[seat] = driver.variant as u8;
            config.specs[seat] = driver.spec;
        }
        let mut message = vec![0; MP_CONFIG_WIRE_SIZE as usize];
        if unsafe { MpEncodeConfig(&config, message.as_mut_ptr(), message.len()) } == 0 { return None; }
        Some(message)
    }
    fn grid(seats: &[SeatPlan]) -> Option<[RaceEntrant; DRIVER_SEAT_LIMIT as usize]> {
        if seats.len() > DRIVER_SEAT_LIMIT as usize { return None; }
        let mut entrants = [RaceEntrant::default(); DRIVER_SEAT_LIMIT as usize];
        for (grid, seat) in seats.iter().enumerate() {
            entrants[grid] = match *seat {
                SeatPlan::Empty => RaceEntrant {
                    kind: RaceSeatKind_RACE_SEAT_EMPTY,
                    ..Default::default()
                },
                SeatPlan::Human {
                    grid,
                    variant,
                    manual,
                    seed,
                } => RaceEntrant {
                    kind: RaceSeatKind_RACE_SEAT_HUMAN,
                    grid,
                    model: variant,
                    rivalSlot: 0,
                    manual: manual as i16,
                    seed,
                },
                SeatPlan::Rival { grid, model, slot, seed } => RaceEntrant {
                    kind: RaceSeatKind_RACE_SEAT_AI,
                    grid,
                    model,
                    rivalSlot: slot,
                    manual: 0,
                    seed,
                },
            };
        }
        Some(entrants)
    }

    pub fn new(
        archive: &RaceArchive,
        track: TrackDataOwned,
        seats: &[SeatPlan],
        laps: i32,
        reverse: bool,
    ) -> Option<Self> {
        let entrants = Self::grid(seats)?;
        let mut sim: Box<RaceSim> = Box::default();
        let ok = unsafe {
            InitRaceGrid(
                sim.as_mut(),
                archive.as_ptr(),
                track.as_ptr(),
                entrants.as_ptr(),
                archive.catalog.as_deref().map_or(std::ptr::null(), |catalog| catalog),
                laps,
                reverse as i32,
            )
        };
        if ok == 0 {
            return None;
        }
        Some(Race { sim, _track: track })
    }

    pub fn start(&mut self, countdown_ticks: u32) -> bool {
        unsafe { StartRaceSim(self.sim.as_mut(), countdown_ticks) != 0 }
    }

    pub fn set_input(&mut self, slot: i32, input: &DriverInput) -> bool {
        unsafe { SetRaceInput(self.sim.as_mut(), slot, input) != 0 }
    }

    pub fn retire(&mut self, slot: i32) -> bool {
        unsafe { RetireRaceDriver(self.sim.as_mut(), slot) != 0 }
    }

    /// Returns true if the tick actually advanced the race (see StepRaceSim).
    pub fn accepts_input(&self, seat: usize) -> bool {
        self.sim.drivers[seat].status == SimDriverStatus_SIM_DRIVING &&
            self.sim.drivers[seat].car.activeFlag != -1
    }

    pub fn input_tick(&self, seat: usize) -> u32 { self.sim.drivers[seat].inputTick }

    pub fn step(&mut self) -> bool {
        unsafe { StepRaceSim(self.sim.as_mut()) != 0 }
    }

    pub fn tick(&self) -> u32 {
        self.sim.tick
    }

    pub fn elapsed(&self) -> u32 { self.sim.elapsed }

    pub fn phase(&self) -> u32 {
        self.sim.phase
    }

    pub fn is_finished(&self) -> bool {
        self.sim.phase == SimRacePhase_SIM_FINISHED
    }

pub fn result(&self, seat: usize) -> (bool, u8, i32) {
    if self.sim.drivers[seat].status != SimDriverStatus_SIM_DRIVER_FINISHED {
        return (false, 0, -1);
    }
    (true, unsafe { RacePosition(self.sim.as_ref(), seat as i32) } as u8,
     unsafe { RaceTime(self.sim.as_ref(), seat as i32) })
}

    /// Build one publication from the same immutable post-step state. The
    /// outbox must replace this whole payload, never its two packets separately.
    pub fn state_message(&self, acknowledged: [u32; crate::SEAT_COUNT]) -> Option<Vec<u8>> {
        let snapshot = crate::protocol::snapshot_message(self.tick(), self.elapsed(),
            self.phase() as u8, std::array::from_fn(|seat| self.pose(seat)), acknowledged);
        let mut packet = vec![0; MP_CORRECTION_WIRE_SIZE as usize + snapshot.len()];
        if unsafe { MpEncodeCorrection(self.sim.as_ref(), acknowledged.as_ptr(),
            packet.as_mut_ptr(), MP_CORRECTION_WIRE_SIZE as usize) } == 0 { return None; }
        packet[MP_CORRECTION_WIRE_SIZE as usize..].copy_from_slice(&snapshot);
        Some(packet)
    }

    pub fn pose(&self, seat: usize) -> CarPose {
        let driver = &self.sim.drivers[seat];
        let status = match driver.status {
            SimDriverStatus_SIM_DRIVING => 1,
            SimDriverStatus_SIM_DRIVER_FINISHED => 2,
            _ => 0,
        };
        // Safety: bodyRotation and the named-field arm are the same bytes;
        // reading the Vec4 arm of this union is exactly as valid as reading
        // the struct arm the simulation itself writes.
        let yaw = unsafe { driver.car.__bindgen_anon_1.bodyRotation.y };
        // AI and human drivetrain storage is different. The common pose and
        // pedal fields share offsets, but AI has no player gearbox/clutch.
        let (rpm, clutch, gear) = if driver.rival != 0 {
            // Same layout view as C's AsConstRivalCar; SimDriver owns the
            // complete storage and GameCarRuntime has matching alignment.
            let rival = unsafe { &*((&driver.car as *const PlayerCarRuntime).cast::<GameCarRuntime>()) };
            (rival.engineRpm, 0, 0)
        } else {
            let drive = unsafe { driver.car.__bindgen_anon_3.drive };
            (drive.engineRpm, drive.clutch as i32, drive.gear as i32)
        };
        CarPose {
            status,
            x: driver.car.x,
            y: driver.car.y,
            z: driver.car.z,
            yaw,
            pitch: unsafe { driver.car.__bindgen_anon_1.bodyRotation.x },
            roll: unsafe { driver.car.__bindgen_anon_1.bodyRotation.z },
            steering: driver.car.steeringAngle,
            wheels: driver.car.wheelRotation,
            brake: unsafe { driver.car.__bindgen_anon_3.drive.brakeInput as i32 },
            progress: driver.car.trackProgress,
            rpm,
            throttle: unsafe { driver.car.__bindgen_anon_3.drive.acceleratorInput.value as i32 },
            clutch,
            gear,
            ground: driver.car.modelY,
            roll_speed: driver.car.bodyRollVelocity,
            speed: driver.car.speed,
            lap: driver.car.lap as i32,
            place: unsafe { RacePosition(self.sim.as_ref(), seat as i32) },
        }
    }
}

// Safety: see RaceArchive/TrackDataOwned; the race thread is the sole owner
// while it exists, so moving the whole Race into that thread is sound even
// though the raw pointers inside are not implicitly Send.
unsafe impl Send for Race {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ai_pose_reads_its_own_drivetrain_layout() {
        let mut race = Race { sim: Box::default(), _track: TrackDataOwned(std::ptr::null_mut()) };
        let driver = &mut race.sim.drivers[0];
        driver.status = SimDriverStatus_SIM_DRIVING;
        driver.car.__bindgen_anon_3.drive.engineRpm = 6700;
        driver.car.__bindgen_anon_3.drive.clutch = 123;
        driver.car.__bindgen_anon_3.drive.gear = 4;
        let human = race.pose(0);
        assert_eq!((human.rpm, human.clutch, human.gear), (6700, 123, 4));
        let driver = &mut race.sim.drivers[0];
        driver.rival = 1;
        let rival = unsafe { &mut *((&mut driver.car as *mut PlayerCarRuntime).cast::<GameCarRuntime>()) };
        rival.engineRpm = 4200;
        rival.acceleratorInput = 256;
        rival.brakeInput = 128;
        let ai = race.pose(0);
        assert_eq!((ai.rpm, ai.clutch, ai.gear), (4200, 0, 0));
        assert_eq!((ai.throttle, ai.brake), (256, 128));
    }

    #[test]
    #[ignore = "requires RAGE_SIM_DISC_BIN legal retail source"]
    fn server_catalog_overlays_owned_races_and_preserves_retail_source() {
        let path = std::env::var("RAGE_SIM_DISC_BIN").unwrap();
        let mut archive = RaceArchive::load_disc(&path).unwrap();
        let identity = (archive.fingerprint(), archive.executable(), archive.boot());
        let plan = crate::Plan::default();
        let original = crate::prepare_race(&archive, &plan).unwrap();
        let original_config = original.config_message().unwrap();
        let file = std::env::temp_dir().join(format!("rage-server-catalog-{}.toml", std::process::id()));
        std::fs::write(&file, "[[cars]]\nid = \"car_00_g0\"\nmodel = 0\ngrade = 0\nautomatic_acceleration_scale = 1100\n").unwrap();
        archive.load_catalog(file.to_str().unwrap()).unwrap();
        let changed = crate::prepare_race(&archive, &plan).unwrap();
        assert_eq!(changed.sim.drivers[0].spec.automaticAccelerationScale, 1100);
        assert_eq!(changed.sim.drivers[0].spec.torqueCurve, original.sim.drivers[0].spec.torqueCurve);
        assert_ne!(original.sim.drivers[0].spec.automaticAccelerationScale, 1100);
        assert_eq!(identity, (archive.fingerprint(), archive.executable(), archive.boot()));
        let mut retail = GameCarSpec::default();
        assert_eq!(unsafe { ReadRaceCar(archive.as_ptr(), 0, &mut retail) }, 1);
        assert_eq!(retail.automaticAccelerationScale, original.sim.drivers[0].spec.automaticAccelerationScale);
        let expected = changed.config_message().unwrap();
        std::fs::write(&file, "[[cars]]\nmodel = garbage\n").unwrap();
        assert!(archive.load_catalog(file.to_str().unwrap()).is_err());
        assert_eq!(crate::prepare_race(&archive, &plan).unwrap().config_message().unwrap(), expected);
        std::fs::write(&file, "[[cars]]\nid = \"car_00_g0\"\nmodel = 0\ngrade = 0\nmanual_only = true\n").unwrap();
        archive.load_catalog(file.to_str().unwrap()).unwrap();
        assert_eq!(archive.automatic(0), Some(false));
        let message = archive.availability_message().unwrap();
        let mut allowed = 0;
        assert_eq!(unsafe { MpDecodeAvailability(message.as_ptr(), message.len(), &mut allowed) }, 1);
        assert_eq!(allowed & 1, 0);
        assert!(crate::prepare_race(&archive, &plan).is_none());
        let mut manual = plan;
        for seat in &mut manual.seats { seat.manual = true; }
        assert!(crate::prepare_race(&archive, &manual).is_some());
        std::fs::write(&file, "[[cars]]\nid = \"car_00_g0\"\nmodel = 0\ngrade = 0\nmanual_only = false\n").unwrap();
        archive.load_catalog(file.to_str().unwrap()).unwrap();
        assert_eq!(archive.automatic(0), Some(true));
        let message = archive.availability_message().unwrap();
        assert_eq!(unsafe { MpDecodeAvailability(message.as_ptr(), message.len(), &mut allowed) }, 1);
        assert_eq!(allowed & 1, 1);
        assert!(crate::prepare_race(&archive, &plan).is_some());
        std::fs::remove_file(&file).unwrap();
        assert!(archive.load_catalog(file.to_str().unwrap()).is_err());
        assert_eq!(archive.automatic(0), Some(true));
        let raw = unsafe { &*archive.as_ptr() };
        assert_eq!(unsafe { ArchiveFingerprint(raw.data.cast(), raw.size) }, identity.0);
        assert_eq!(original.config_message().unwrap(), original_config,
            "loading/replacing catalog mutated an already-owned race");
    }

    #[test]
    #[ignore = "requires RAGE_SIM_DISC_BIN legal retail source"]
    fn real_mixed_field_ai_poses_pass_c_wire_validation() {
        let path = std::env::var("RAGE_SIM_DISC_BIN").expect("select a legal disc image");
        let archive = RaceArchive::load_disc(&path).unwrap();
        let mut checked_ai = 0;
        for class in 0..6 {
            for course in 0..4 {
                for reverse in [false, true] {
                    let mut plan = crate::Plan { class, course, laps: 1, reverse,
                        ..Default::default() };
                    plan.rivals = archive.rivals(class, course, reverse).unwrap();
                    let ai: Vec<_> = plan.rivals.iter().enumerate()
                        .filter_map(|(i, rival)| rival.map(|_| crate::SEAT_COUNT + i)).collect();
                    checked_ai += ai.len();
                    let mut race = crate::prepare_race(&archive, &plan).unwrap();
                    let packet = crate::start_message(&plan, archive.boot(),
                        archive.fingerprint(), archive.executable()).unwrap();
                    let mut start = MpStart::default();
                    assert_ne!(unsafe { MpDecodeStart(packet[1..].as_ptr(), packet.len() - 1,
                        &mut start) }, 0);
                    let mut setup = RaceSetup::default();
                    assert_ne!(unsafe { MpBuildSetup(&start, &mut setup) }, 0);
                    for (i, rival) in plan.rivals.iter().enumerate() {
                        let seat = crate::SEAT_COUNT + i;
                        if let Some(rival) = rival {
                            assert_eq!(setup.entrants[seat].kind, RaceSeatKind_RACE_SEAT_AI);
                            assert_eq!(setup.entrants[seat].model, rival.model as i32);
                            assert_eq!(setup.entrants[seat].rivalSlot, rival.slot as i32);
                            assert_eq!(setup.entrants[seat].seed, rival.seed);
                        } else {
                            assert_eq!(setup.entrants[seat].kind, RaceSeatKind_RACE_SEAT_EMPTY);
                            assert_eq!(race.sim.drivers[seat].status, SimDriverStatus_SIM_EMPTY);
                        }
                    }
                    let starts: Vec<_> = ai.iter().map(|&seat| {
                        let pose = race.pose(seat); (pose.x, pose.z)
                    }).collect();
                    assert!(race.start(0));
                    for _ in 0..100 {
                        assert!(race.step());
                        for &seat in &ai {
                            let pose = race.pose(seat);
                            assert_eq!((pose.gear, pose.clutch), (0, 0));
                            let packet = crate::protocol::snapshot_message(race.tick(),
                                race.elapsed(), race.phase() as u8, [pose; crate::FIELD_COUNT], [0; crate::SEAT_COUNT]);
                            let mut decoded = MpSnapshot::default();
                            assert_ne!(unsafe { MpDecodeSnapshot(packet[1..].as_ptr(),
                                packet.len() - 1, &mut decoded) }, 0,
                                "class {class}, reverse {reverse}, AI seat {seat}, pose {pose:?}");
                            assert_eq!(decoded.seats[0].rpm, pose.rpm);
                        }
                    }
                    for (&seat, start) in ai.iter().zip(starts) {
                        let pose = race.pose(seat);
                        assert_ne!((pose.x, pose.z), start, "AI seat {seat} did not move");
                    }
                }
            }
        }
        assert!(checked_ai > 0);
    }

    #[test]
    fn mixed_grid_keeps_rival_behavior_and_authored_position_separate() {
        let seats = [
            SeatPlan::Human { grid: 5, variant: 31, manual: true, seed: 42 },
            SeatPlan::Empty,
            SeatPlan::Rival { grid: 7, model: 9, slot: 3, seed: 123 },
        ];
        let grid = Race::grid(&seats).unwrap();
        assert_eq!(grid[0].kind, RaceSeatKind_RACE_SEAT_HUMAN);
        assert_eq!(grid[0].grid, 5);
        assert_eq!(grid[0].model, 31);
        assert_eq!(grid[0].manual, 1);
        assert_eq!(grid[0].seed, 42);
        assert_eq!(grid[1].kind, RaceSeatKind_RACE_SEAT_EMPTY);
        assert_eq!(grid[2].kind, RaceSeatKind_RACE_SEAT_AI);
        assert_eq!(grid[2].grid, 7);
        assert_eq!(grid[2].model, 9);
        assert_eq!(grid[2].rivalSlot, 3);
        assert_eq!(grid[2].seed, 123);
        for seat in &grid[3..] { assert_eq!(seat.kind, RaceSeatKind_RACE_SEAT_EMPTY); }
        let oversized: Vec<_> = (0..=DRIVER_SEAT_LIMIT).map(|_| SeatPlan::Empty).collect();
        assert!(Race::grid(&oversized).is_none());
    }

    #[test]
    fn completion_owns_names_and_results_after_race_and_session_drop() {
        let mut race = Race { sim: Box::default(), _track: TrackDataOwned(std::ptr::null_mut()) };
        race.sim.phase = SimRacePhase_SIM_FINISHED;
        race.sim.drivers[0].status = SimDriverStatus_SIM_DRIVER_FINISHED;
        race.sim.drivers[0].place = 1;
        race.sim.drivers[0].finishTick = 50;
        race.sim.drivers[1].status = SimDriverStatus_SIM_RETIRED;
        let session = crate::Session::new();
        assert!(session.inputs[0].greet(b"Alice"));
        assert!(session.inputs[1].greet(b"Bob\xFF"));
        let mut plan = crate::Plan { class: 4, course: 3, laps: 1, reverse: true, ..Default::default() };
        let finish = crate::Finish::capture(&race, &session, plan);
        plan.class = 0;
        assert_ne!(finish.plan.class, plan.class);
        race.sim.drivers[0].finishTick = 100;
        drop(race);
        drop(session);
        assert_eq!(finish.names, [b"Alice".to_vec(), b"Bob\xFF".to_vec()]);
        assert_eq!(finish.results, [(true, 1, 1000), (false, 0, -1)]);
        assert_eq!((finish.plan.class, finish.plan.course, finish.plan.laps), (4, 3, 1));
        assert!(finish.plan.reverse);

        let mut rooms = crate::Rooms::default();
        let inputs = std::array::from_fn(|_| std::sync::Arc::new(crate::SharedInput::default()));
        let observer = inputs[0].clone();
        let task = std::thread::spawn(move || Ok(finish));
        rooms.active.push(crate::Room { id: 71, inputs, task: Some(task) });
        let deadline = std::time::Instant::now() + std::time::Duration::from_secs(3);
        while !rooms.active[0].task.as_ref().unwrap().is_finished() {
            assert!(std::time::Instant::now() < deadline);
            std::thread::yield_now();
        }
        let mut completed = Vec::new();
        completed.extend(rooms.reap());
        assert_eq!(completed.len(), 1);
        assert!(rooms.active.is_empty() && rooms.reap().is_empty());
        assert!(observer.take().is_none());
        drop(rooms);
        let finish = completed.pop().unwrap();
        assert_eq!(finish.room, Some(71));
        assert_eq!(finish.names, [b"Alice".to_vec(), b"Bob\xFF".to_vec()]);
        assert_eq!(finish.results, [(true, 1, 1000), (false, 0, -1)]);
        assert_eq!(finish.plan.class, 4);
        assert!(!finish.writes_ok); /* Failed writes do not erase the result. */
    }

    #[test]
    fn losing_all_inputs_retires_field_and_stops_without_track_data() {
        let mut state = Box::<RaceSim>::default();
        state.phase = SimRacePhase_SIM_RACING;
        for driver in &mut state.drivers[..crate::SEAT_COUNT] {
            driver.status = SimDriverStatus_SIM_DRIVING;
        }
        let mut race = Race { sim: state, _track: TrackDataOwned(std::ptr::null_mut()) };
        let mut session = crate::Session::new();
        for input in &session.inputs { assert!(input.greet(b"Lost")); input.close(); }
        let mut alive = [true; crate::SEAT_COUNT];
        assert!(!crate::step_room(&mut race, &session, &mut alive).unwrap());
        assert_eq!(alive, [false; crate::SEAT_COUNT]);
        assert_eq!(race.tick(), 1);
        assert!(race.is_finished());
        for seat in 0..crate::SEAT_COUNT {
            assert_eq!(race.pose(seat).status, 0);
            assert!(session.outputs[seat].take().is_none());
        }
        let finish = crate::finish_room(&race, &mut session, crate::Plan::default()).unwrap();
        assert_eq!(finish.results, [(false, 0, -1); crate::SEAT_COUNT]);
        assert_eq!(finish.names, [b"Lost".to_vec(), b"Lost".to_vec()]);
    }

    #[test]
    fn unfinished_room_cannot_publish_a_result() {
        let mut state = Box::<RaceSim>::default();
        state.phase = SimRacePhase_SIM_RACING;
        let race = Race { sim: state, _track: TrackDataOwned(std::ptr::null_mut()) };
        let mut session = crate::Session::new();
        assert!(crate::finish_room(&race, &mut session, crate::Plan::default()).is_err());
        for output in &session.outputs {
            assert!(output.take().is_none());
            assert!(!output.result_taken());
        }
    }

    #[test]
    fn failed_room_step_closes_only_its_own_session() {
        for (phase, tick, elapsed) in [
            (SimRacePhase_SIM_SETUP, 0, 0),
            (SimRacePhase_SIM_RACING, u32::MAX, 0),
            (SimRacePhase_SIM_RACING, 0, u32::MAX),
        ] {
            // These states fail the real C step before dereferencing route data;
            // no import or fake simulation implementation is necessary.
            let mut state = Box::<RaceSim>::default();
            state.phase = phase;
            state.tick = tick;
            state.elapsed = elapsed;
            let race = Race { sim: state, _track: TrackDataOwned(std::ptr::null_mut()) };
            let session = crate::Session::new();
            let inputs = session.inputs.clone();
            let outputs = session.outputs.clone();
            let other = crate::Session::new();
            assert!(crate::run_room(race, session, crate::Plan::default()).is_err());
            for input in inputs {
                assert!(input.take().is_none());
                assert!(!input.publish([0; 12]));
            }
            for output in outputs { assert!(output.take().is_none()); }
            assert!(other.inputs[0].greet(b"Other"));
            assert!(other.inputs[0].pick(9, 1));
        }
    }

    #[test]
    fn full_room_manager_rejects_and_recovers_capacity() {
        let mut rooms = crate::Rooms::default();
        for _ in 0..crate::ROOM_LIMIT {
            let input = std::sync::Arc::new(crate::SharedInput::default());
            let worker_input = input.clone();
            let task = std::thread::spawn(move || {
                worker_input.wait_stage(crate::Stage::Ready, std::time::Duration::from_secs(3));
                Err("fixture worker stopped".to_owned())
            });
            let inputs = std::array::from_fn(|seat| if seat == 0 { input.clone() }
                else { std::sync::Arc::new(crate::SharedInput::default()) });
            rooms.active.push(crate::Room { id: 71, inputs, task: Some(task) });
        }
        assert!(!rooms.available());
        let race = Race { sim: Box::default(), _track: TrackDataOwned(std::ptr::null_mut()) };
        let session = crate::Session::new();
        let rejected_inputs = session.inputs.clone();
        assert_eq!(rooms.start(1, session, move |session|
            crate::run_room(race, session, crate::Plan::default()).map_err(str::to_owned)).unwrap_err().kind(), std::io::ErrorKind::WouldBlock);
        for input in rejected_inputs { assert!(input.take().is_none()); }
        assert_eq!(rooms.active.len(), crate::ROOM_LIMIT);
        assert_eq!(rooms.last_id, 0); // Rejected admission consumes no ID.
        // Completing one room restores admission without another connection.
        rooms.active[0].inputs[0].close();
        let deadline = std::time::Instant::now() + std::time::Duration::from_secs(3);
        while !rooms.available() {
            rooms.reap();
            assert!(std::time::Instant::now() < deadline);
            std::thread::yield_now();
        }
        assert_eq!(rooms.active.len(), crate::ROOM_LIMIT - 1);
        assert!(rooms.active[0].inputs[0].greet(b"Waiting"));
    }

    #[test]
    fn room_manager_reaps_a_failed_real_simulation_worker() {
        let race = Race { sim: Box::default(), _track: TrackDataOwned(std::ptr::null_mut()) };
        let session = crate::Session::new();
        let inputs = session.inputs.clone();
        let mut rooms = crate::Rooms::default();
        let id = rooms.new_id().unwrap();
        rooms.start(id, session, move |session|
            crate::run_room(race, session, crate::Plan::default()).map_err(str::to_owned)).unwrap();
        assert_eq!(rooms.active[0].id, 1);
        let deadline = std::time::Instant::now() + std::time::Duration::from_secs(3);
        while !rooms.active[0].task.as_ref().unwrap().is_finished() {
            assert!(std::time::Instant::now() < deadline);
            std::thread::yield_now();
        }
        rooms.reap();
        assert!(rooms.active.is_empty());
        for input in inputs { assert!(input.take().is_none()); }
        let race = Race { sim: Box::default(), _track: TrackDataOwned(std::ptr::null_mut()) };
        let id = rooms.new_id().unwrap();
        rooms.start(id, crate::Session::new(), move |session|
            crate::run_room(race, session, crate::Plan::default()).map_err(str::to_owned)).unwrap();
        assert_eq!(rooms.active[0].id, 2); // Reaping must not recycle IDs.
        drop(rooms);
        let mut rooms = crate::Rooms::default();
        rooms.last_id = u64::MAX;
        assert_eq!(rooms.new_id().unwrap_err().kind(), std::io::ErrorKind::Other);
        assert!(rooms.active.is_empty());
    }

    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn retail_transmission_metadata_controls_race_admission() {
        let path = std::env::var("RAGE_SIM_DISC_BIN").expect("select a legal disc image");
        let archive = RaceArchive::load_disc(&path).unwrap();
        let mut automatic_count = 0;
        for variant in 0..CAR_MODEL_VARIANT_COUNT as i32 {
            let automatic = archive.automatic(variant).expect("valid retail metadata");
            automatic_count += automatic as usize;
            let mut plan = crate::Plan::default();
            plan.seats[0].model = variant;
            plan.seats[0].manual = true;
            assert!(crate::prepare_race(&archive, &plan).is_some(), "manual variant {variant}");
            plan.seats[0].manual = false;
            assert_eq!(crate::prepare_race(&archive, &plan).is_some(), automatic,
                "automatic variant {variant}");
        }
        assert!(automatic_count > 0 && automatic_count < CAR_MODEL_VARIANT_COUNT as usize);
        assert!(archive.automatic(-1).is_none());
        assert!(archive.automatic(CAR_MODEL_VARIANT_COUNT as i32).is_none());
    }

    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn command_acknowledgement_follows_actual_countdown_and_race_steps() {
        let path = std::env::var("RAGE_SIM_DISC_BIN").expect("select a legal disc image");
        let archive = RaceArchive::load_disc(&path).unwrap();
        for class in [0, 5] {
            for reverse in [false, true] {
                for countdown in [3, 4] {
                    let mut plan = crate::Plan { class, reverse, countdown, ..Default::default() };
                    plan.rivals = archive.rivals(class, plan.course, reverse).unwrap();
                    let mut race = crate::prepare_race(&archive, &plan).unwrap();
                    assert!(race.start(countdown));
                    let session = crate::Session::new();
                    for input in &session.inputs {
                        assert!(input.greet(b"Driver"));
                        assert!(input.set_ready(true));
                    }
                    session.load_plan(plan).unwrap();
                    for input in &session.inputs {
                        assert!(input.loaded());
                        assert!(input.start(std::time::Instant::now()));
                    }
                    let mut alive = [true; crate::SEAT_COUNT];
                    let mut expected = [0; crate::SEAT_COUNT];
                    for sequence in 1..=countdown + 10 {
                        for seat in 0..crate::SEAT_COUNT {
                            let mut packet = [0; 12];
                            packet[5..7].copy_from_slice(&256i16.to_le_bytes());
                            packet[9] = (sequence % 2 == 1) as u8;
                            assert!(session.inputs[seat].publish_command(packet, Some(sequence)));
                            assert_eq!(session.inputs[seat].state.lock().unwrap().applied_sequence,
                                expected[seat]);
                        }
                        assert!(crate::step_room(&mut race, &session, &mut alive).unwrap());
                        for seat in 0..crate::SEAT_COUNT {
                            if race.input_tick(seat) == race.tick() { expected[seat] = sequence; }
                        }
                        for seat in 0..crate::SEAT_COUNT {
                            let packet = session.outputs[seat].take().unwrap();
                            let mut decoded = MpSnapshot::default();
                            let mut correction = MpCorrection::default();
                            assert_ne!(unsafe { MpDecodePublication(race.sim.as_ref(), packet.as_ptr(), packet.len(),
                                &mut correction, &mut decoded) }, 0);
                            assert_eq!(decoded.acknowledged, expected);
                            assert_eq!(decoded.tick, sequence);
                        }
                    }
                    // A retired seat's newly received controls are never applied or acknowledged.
                    assert!(race.retire(1));
                    let sequence = countdown + 11;
                    assert!(session.inputs[1].publish_command([0; 12], Some(sequence)));
                    assert!(crate::step_room(&mut race, &session, &mut alive).unwrap());
                    assert_eq!(session.inputs[1].state.lock().unwrap().applied_sequence, expected[1]);
                    for output in &session.outputs { assert!(output.take().is_some()); }
                    expected[0] = session.inputs[0].state.lock().unwrap().applied_sequence;
                    // Failed simulation must not publish an acknowledgement or snapshot.
                    let sentinel: std::sync::Arc<[u8]> = std::sync::Arc::from([0xA5]);
                    for output in &session.outputs { output.publish(sentinel.clone()); }
                    race.sim.tick = u32::MAX;
                    assert!(session.inputs[0].publish_command([0; 12], Some(sequence + 1)));
                    assert!(crate::step_room(&mut race, &session, &mut alive).is_err());
                    assert_eq!(session.inputs[0].state.lock().unwrap().applied_sequence, expected[0]);
                    for output in &session.outputs {
                        assert!(std::sync::Arc::ptr_eq(&output.take().unwrap(), &sentinel));
                    }
                }
            }
        }
    }

    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn owned_race_checkpoint_replays_pending_inputs_and_the_full_field() {
        use std::io::Write;
        let mut report = std::env::var_os("RAGE_MP_PREDICTION_REPORT").map(|path| {
            let mut file = std::fs::File::create(path).expect("create requested prediction report");
            writeln!(file, "class,reverse,lead,iterations,context_bytes,mean_ns,max_ns").unwrap();
            file
        });
        let path = std::env::var("RAGE_SIM_DISC_BIN").expect("select a legal disc image");
        let archive = RaceArchive::load_disc(&path).unwrap();
        fn input(tick: u32, seat: usize) -> DriverInput {
            DriverInput { throttle: 256, brake: if tick % 73 == 0 { 128 } else { 0 },
                shiftUp: (seat == 1 && tick % 19 == 0) as i32,
                shiftDown: (seat == 1 && tick % 43 == 0) as i32,
                ..Default::default() }
        }
        fn trace(race: &mut Race, mut replay: Option<&mut MpCommands>) -> Vec<[u8; RACE_FRAME_WIRE_SIZE as usize]> {
            let mut frames = Vec::new();
            let mut consumed = replay.as_ref().map_or(0, |commands| commands.acknowledged);
            for _ in 0..200 {
                for seat in 0..crate::SEAT_COUNT {
                    let mut controls = input(race.tick(), seat);
                    if seat == 1 {
                        if let Some(commands) = replay.as_deref_mut() {
                            assert_eq!(unsafe { MpRememberCommand(commands, commands.sent + 1,
                                race.tick() + 1, &controls) }, 1);
                            controls = race.sim.drivers[seat].input;
                            assert_eq!(unsafe { MpReplayInput(commands, race.tick() + 1,
                                &mut consumed, &mut controls) }, 1);
                        }
                    }
                    assert!(race.set_input(seat as i32, &controls));
                }
                assert!(race.step());
                let mut encoded = [0; RACE_FRAME_WIRE_SIZE as usize];
                assert_eq!(unsafe { EncodeRaceFrame(race.sim.as_ref(), encoded.as_mut_ptr(), encoded.len()) }, 1);
                frames.push(encoded);
            }
            frames
        }
        fn encoded(race: &RaceSim) -> [u8; RACE_FRAME_WIRE_SIZE as usize] {
            let mut bytes = [0; RACE_FRAME_WIRE_SIZE as usize];
            assert_eq!(unsafe { EncodeRaceFrame(race, bytes.as_mut_ptr(), bytes.len()) }, 1);
            bytes
        }
        fn check_prediction(authority: &RaceSim, history: &MpCommands,
                            samples: &[MpCommand], local: usize, case: &str) {
            let initial = encoded(authority);
            let retained = *history;
            let mut direct = Box::new(*authority);
            let mut predicted = Box::new(RaceSim::default());
            let mut next = 0;
            for lead in 1..=10 {
                while next < samples.len() && samples[next].tick <= authority.tick + lead {
                    assert_eq!(unsafe { SetRaceInput(direct.as_mut(), local as i32, &samples[next].input) }, 1);
                    next += 1;
                }
                assert_eq!(unsafe { StepRaceSim(direct.as_mut()) }, 1);
                assert_eq!(unsafe { MpPredictRace(authority, history, samples.as_ptr(), samples.len() as u32,
                    local as i32, authority.tick + lead, predicted.as_mut()) }, 1);
                assert_eq!(encoded(predicted.as_ref()), encoded(direct.as_ref()), "{case}, local {local}, lead {lead}");
                assert_eq!(encoded(authority), initial, "{case}: authority changed");
                assert_eq!((history.sent, history.acknowledged, history.lastTick, history.head, history.count),
                    (retained.sent, retained.acknowledged, retained.lastTick, retained.head, retained.count));
                for (old, new) in retained.entries.iter().zip(history.entries.iter()) {
                    assert_eq!((old.sequence, old.tick), (new.sequence, new.tick));
                    let mut a = [0; 13]; let mut b = [0; 13];
                    unsafe { MpEncodeInput(&old.input, a.as_mut_ptr()); MpEncodeInput(&new.input, b.as_mut_ptr()); }
                    assert_eq!(a, b, "{case}: retained input changed");
                }
            }
        }
        for class in [0, 5] {
            for reverse in [false, true] {
                let mut plan = crate::Plan { class, reverse, ..Default::default() };
                plan.seats[1].model = 9; plan.seats[1].manual = true;
                plan.rivals = archive.rivals(class, plan.course, reverse).unwrap();
                let mut race = crate::prepare_race(&archive, &plan).unwrap();
                assert!(race.start(3));
                // Predict across countdown and the first racing ticks, for either local seat.
                for local in 0..crate::SEAT_COUNT {
                    let history = MpCommands::default();
                    let samples = [
                        MpCommand { sequence: 1, tick: 1,
                            input: DriverInput { throttle: 256, shiftUp: 1, ..Default::default() } },
                        MpCommand { sequence: 0, tick: 4,
                            input: DriverInput { brake: 128, shiftDown: 1, ..Default::default() } },
                    ];
                    check_prediction(race.sim.as_ref(), &history, &samples, local,
                        &format!("class {class}, reverse {reverse}, countdown"));
                }
                let mut commands = Box::new(MpCommands::default());
                for _ in 0..37 {
                    for seat in 0..crate::SEAT_COUNT {
                        let controls = input(race.tick(), seat);
                        assert!(race.set_input(seat as i32, &controls));
                        if seat == 1 {
                            assert_eq!(unsafe { MpRememberCommand(commands.as_mut(), race.tick() + 1, race.tick() + 1, &controls) }, 1);
                        }
                    }
                    assert!(race.step());
                }
                // Save between physics ticks with an unconsumed gear edge.
                let edge = DriverInput { shiftUp: 1, ..Default::default() };
                assert!(race.set_input(1, &edge));
                assert_eq!(unsafe { MpRememberCommand(commands.as_mut(), 38, 38, &edge) }, 1);
                let acknowledged = [race.sim.drivers[0].inputTick, race.sim.drivers[1].inputTick];
                assert_eq!(acknowledged, [37, 37]);
                let mut wire = [0; MP_CORRECTION_WIRE_SIZE as usize];
                assert_eq!(unsafe { MpEncodeCorrection(race.sim.as_ref(), acknowledged.as_ptr(), wire.as_mut_ptr(), wire.len()) }, 1);
                let publication = race.state_message(acknowledged).unwrap();
                assert_eq!(&publication[..wire.len()], &wire);
                let mut snapshot = MpSnapshot::default();
                assert_eq!(unsafe { MpDecodeSnapshot(publication[wire.len() + 1..].as_ptr(),
                    publication.len() - wire.len() - 1, &mut snapshot) }, 1);
                assert_eq!(snapshot.tick, race.tick());
                assert_eq!(snapshot.elapsed, race.elapsed());
                assert_eq!(snapshot.acknowledged, acknowledged);
                let output = crate::Outbox::default();
                output.publish(publication.clone().into());
                output.finish(crate::encode_result([(false, 0, -1); 2]).into());
                let mut transmitted = Vec::new();
                assert!(crate::write_client(&mut transmitted, &output));
                assert_eq!(&transmitted[..publication.len()], publication.as_slice());
                assert_eq!(&transmitted[publication.len()..], crate::encode_result([(false, 0, -1); 2]).as_slice());
                let first = trace(&mut race, None);
                let mut corrected = crate::prepare_race(&archive, &plan).unwrap();
                assert_ne!(corrected.sim.route.points, race.sim.route.points);
                let mut correction = Box::new(MpCorrection::default());
                assert_eq!(unsafe { MpDecodePublication(corrected.sim.as_ref(), transmitted.as_ptr(),
                    publication.len(), correction.as_mut(), &mut snapshot) }, 1);
                assert_eq!(correction.frame.track, corrected.sim.route.points.cast());
                assert_eq!(unsafe { MpApplyCorrection(corrected.sim.as_mut(), commands.as_mut(), 1, correction.as_ref()) }, 1);
                assert_eq!(commands.acknowledged, 37);
                assert_eq!(commands.count, 1);
                assert_eq!(unsafe { (*MpCommandAt(commands.as_ref(), 0)).sequence }, 38);
                assert_eq!(unsafe { (*MpCommandAt(commands.as_ref(), 0)).input.shiftUp }, 1);
                let mut predicted = Box::new(RaceSim::default());
                let pending = MpCommand { sequence: 0, tick: corrected.tick() + 1,
                    input: input(corrected.tick(), 1) };
                let mut before = [0; RACE_FRAME_WIRE_SIZE as usize];
                assert_eq!(unsafe { EncodeRaceFrame(corrected.sim.as_ref(), before.as_mut_ptr(), before.len()) }, 1);
                assert_eq!(unsafe { MpPredictRace(corrected.sim.as_ref(), commands.as_ref(), &pending, 1,
                    1, corrected.tick() + 1, predicted.as_mut()) }, 1);
                let mut after = [0; RACE_FRAME_WIRE_SIZE as usize];
                assert_eq!(unsafe { EncodeRaceFrame(corrected.sim.as_ref(), after.as_mut_ptr(), after.len()) }, 1);
                assert_eq!(after, before, "prediction changed authority");
                assert_eq!(unsafe { EncodeRaceFrame(predicted.as_ref(), after.as_mut_ptr(), after.len()) }, 1);
                assert_eq!(after, first[0], "class {class}, reverse {reverse}: predicted first step");
                for local in 0..crate::SEAT_COUNT {
                    let empty = MpCommands::default();
                    let history = if local == 0 { &empty } else { commands.as_ref() };
                    let sample = MpCommand { sequence: 0, tick: pending.tick,
                        input: input(corrected.tick(), local) };
                    check_prediction(corrected.sim.as_ref(), history, &[sample], local,
                        &format!("class {class}, reverse {reverse}, held input"));
                    let unsent = [
                        MpCommand { sequence: history.sent + 1, tick: corrected.tick() + 1,
                            input: DriverInput { throttle: 200, shiftUp: 1, ..Default::default() } },
                        MpCommand { sequence: 0, tick: corrected.tick() + 3,
                            input: DriverInput { brake: 128, shiftDown: 1, ..Default::default() } },
                    ];
                    check_prediction(corrected.sim.as_ref(), history, &unsent, local,
                        &format!("class {class}, reverse {reverse}, two pending samples"));
                }
                if let Some(file) = report.as_mut() {
                    for lead in [0, 1, 5, 10] {
                        for _ in 0..50 {
                            assert_eq!(unsafe { MpPredictRace(corrected.sim.as_ref(), commands.as_ref(), &pending, 1,
                                1, corrected.tick() + lead, predicted.as_mut()) }, 1);
                        }
                        let mut sum = 0u128; let mut maximum = 0u128;
                        for _ in 0..500 {
                            let started = std::time::Instant::now();
                            assert_eq!(unsafe { MpPredictRace(corrected.sim.as_ref(), commands.as_ref(), &pending, 1,
                                1, corrected.tick() + lead, predicted.as_mut()) }, 1);
                            std::hint::black_box(predicted.tick);
                            let elapsed = started.elapsed().as_nanos();
                            sum += elapsed; maximum = maximum.max(elapsed);
                        }
                        writeln!(file, "{},{},{},500,{},{},{}", class + 1, reverse, lead,
                            std::mem::size_of::<RaceSim>(), sum / 500, maximum).unwrap();
                    }
                }
                assert_eq!(trace(&mut corrected, Some(commands.as_mut())), first, "class {class}, reverse {reverse}");
            }
        }
    }

    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn real_two_driver_races_finish_with_authoritative_wire_results() {
        run_real_race_wire(1, false, None);
    }

    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn real_race_client_recovers_across_skipped_snapshots() {
        run_real_race_wire(31, false, None);
    }

    #[cfg(unix)]
    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn real_race_inputs_snapshots_and_results_cross_both_streams() {
        run_real_race_wire(1, true, None);
    }

    #[cfg(unix)]
    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn surviving_stream_client_finishes_after_either_peer_disconnects() {
        for seat in 0..crate::SEAT_COUNT { run_real_race_wire(1, true, Some(seat)); }
    }

    #[cfg(unix)]
    struct RaceStreams {
        session: crate::Session,
        peers: Vec<Option<std::os::unix::net::UnixStream>>,
        accepted: Vec<std::sync::mpsc::Receiver<()>>,
    }

    // Acknowledges at the next header read, after read_client has published
    // the preceding input. Test traffic is exclusively 13-byte input packets.
    // This synchronizes accelerated simulation without sleeps or polling state.
    #[cfg(unix)]
    struct InputReader {
        stream: std::os::unix::net::UnixStream,
        remaining: usize,
        accepted: std::sync::mpsc::Sender<()>,
    }

    #[cfg(unix)]
    impl std::io::Read for InputReader {
        fn read(&mut self, bytes: &mut [u8]) -> std::io::Result<usize> {
            if self.remaining == 0 {
                self.accepted.send(()).map_err(|_| std::io::ErrorKind::BrokenPipe)?;
                self.remaining = 13;
            }
            let count = std::io::Read::read(&mut self.stream, bytes)?;
            self.remaining -= count;
            Ok(count)
        }
    }

    #[cfg(unix)]
    impl Drop for RaceStreams {
        fn drop(&mut self) {
            // Unblock the real readers before Session joins its workers.
            for peer in self.peers.iter().flatten() { let _ = peer.shutdown(std::net::Shutdown::Both); }
        }
    }

    #[cfg(unix)]
    impl RaceStreams {
        fn new() -> Self {
            let mut session = crate::Session::new();
            let mut peers = Vec::new();
            let mut accepted = Vec::new();
            for seat in 0..crate::SEAT_COUNT {
                let (peer, mut server) = std::os::unix::net::UnixStream::pair().unwrap();
                peer.set_read_timeout(Some(std::time::Duration::from_secs(3))).unwrap();
                peer.set_write_timeout(Some(std::time::Duration::from_secs(3))).unwrap();
                server.set_write_timeout(Some(std::time::Duration::from_secs(3))).unwrap();
                let input = session.inputs[seat].clone();
                assert!(input.greet(if seat == 0 { b"Alice" } else { b"Bob\xFF" }));
                assert!(input.set_ready(true));
                assert!(input.state.lock().unwrap().begin_load().is_some());
                assert!(input.loaded());
                assert!(input.start(std::time::Instant::now()));
                let (sender, receiver) = std::sync::mpsc::channel();
                let mut reader = InputReader { stream: server.try_clone().unwrap(),
                    remaining: 13, accepted: sender };
                session.readers[seat] = Some(std::thread::spawn(move || {
                    crate::read_client(&mut reader, &input);
                    input.close();
                }));
                accepted.push(receiver);
                let output = session.outputs[seat].clone();
                session.writers[seat] = Some(std::thread::spawn(move || {
                    crate::write_client(&mut server, &output)
                }));
                peers.push(Some(peer));
            }
            Self { session, peers, accepted }
        }

        fn input(&mut self, seat: usize, input: &DriverInput) {
            use std::io::Write;
            let mut packet = [0; 13];
            unsafe { MpEncodeInput(input, packet.as_mut_ptr()); }
            // Split header from payload to exercise read_exact framing.
            let peer = self.peers[seat].as_mut().unwrap();
            peer.write_all(&packet[..1]).unwrap();
            peer.write_all(&packet[1..]).unwrap();
            self.accepted[seat].recv_timeout(std::time::Duration::from_secs(3)).unwrap();
            let received = self.session.inputs[seat].state.lock().unwrap().packet;
            assert_eq!(received.as_slice(), &packet[1..]);
        }

        fn disconnect(&mut self, seat: usize) {
            self.peers[seat].take().unwrap().shutdown(std::net::Shutdown::Both).unwrap();
            self.session.readers[seat].take().unwrap().join().unwrap();
            assert!(self.session.inputs[seat].take().is_none());
        }

        fn receive(&mut self, packet: &[u8]) -> Vec<u8> {
            use std::io::Read;
            let mut first = None;
            for peer in self.peers.iter_mut().flatten() {
                let mut received = vec![0; packet.len()];
                peer.read_exact(&mut received).unwrap();
                assert_eq!(received, packet);
                if first.is_none() { first = Some(received); }
            }
            first.unwrap()
        }
    }

    fn run_real_race_wire(stride: u32, stream_delivery: bool, disconnect: Option<usize>) {
        let path = std::env::var("RAGE_SIM_DISC_BIN").expect("select a legal disc image");
        let archive = RaceArchive::load_disc(&path).expect("load legal source");
        for reverse in [false, true] {
            #[cfg(unix)]
            let mut streams = stream_delivery.then(RaceStreams::new);
            #[cfg(not(unix))]
            assert!(!stream_delivery);
            let mut args = vec![path.clone(), String::from("--laps=1")];
            if reverse { args.push(String::from("--reverse")); }
            let mut plan = crate::parse_start(&args).unwrap().plan;
            plan.countdown = 50;
            plan.rivals = archive.rivals(plan.class, plan.course, reverse).unwrap();
            plan.seats[1].model = (1..CAR_MODEL_VARIANT_COUNT as i32)
                .find(|&variant| archive.automatic(variant) == Some(true))
                .expect("a second retail automatic variant");
            plan.seats[1].seed = 42;
            let start_packet = crate::start_message(&plan, archive.boot(), archive.fingerprint(),
                archive.executable()).unwrap();
            let mut start = MpStart::default();
            assert_ne!(unsafe { MpDecodeStart(start_packet[1..].as_ptr(), start_packet.len() - 1,
                &mut start) }, 0);
            assert_ne!(unsafe { MpMatchesArchive(&start, archive.as_ptr()) }, 0);
            let mut setup = RaceSetup::default();
            assert_ne!(unsafe { MpBuildSetup(&start, &mut setup) }, 0);
            assert_eq!(setup.laps, plan.laps);
            assert_eq!(setup.reverse, plan.reverse as i32);
            for seat in 0..crate::SEAT_COUNT {
                assert_eq!(setup.entrants[seat].model, plan.seats[seat].model);
                assert_eq!(setup.entrants[seat].manual != 0, plan.seats[seat].manual);
                assert_eq!(setup.entrants[seat].seed, plan.seats[seat].seed);
            }
            let mut race = crate::prepare_race(&archive, &plan).unwrap();
            // Construct from the actual C-decoded setup, not the server Plan.
            let client_seats: Vec<_> = setup.entrants.iter().map(|seat| match seat.kind {
                RaceSeatKind_RACE_SEAT_HUMAN => SeatPlan::Human { grid: seat.grid,
                    variant: seat.model, manual: seat.manual != 0, seed: seat.seed },
                RaceSeatKind_RACE_SEAT_AI => SeatPlan::Rival { grid: seat.grid,
                    model: seat.model, slot: seat.rivalSlot, seed: seat.seed },
                _ => SeatPlan::Empty,
            }).collect();
            let mut client = Race::new(&archive,
                archive.copy_track(setup.classIndex, setup.courseIndex).unwrap(),
                &client_seats, setup.laps, setup.reverse != 0).unwrap();
            let configuration = race.config_message().unwrap();
            let mut selected = MpCarConfig::default();
            assert_eq!(unsafe { MpDecodeConfig(configuration.as_ptr(), configuration.len(), &mut selected) }, 1);
            for seat in 0..crate::SEAT_COUNT {
                client.sim.drivers[seat].spec.torqueCurve[0] = -123;
                client.sim.drivers[seat].engine.peakOutput = -456;
            }
            assert_eq!(unsafe { MpApplyConfig(client.sim.as_mut(), &selected) }, 1);
            assert_eq!(client.config_message().unwrap(), configuration);
            for seat in 0..crate::SEAT_COUNT {
                assert_eq!(client.sim.drivers[seat].engine.peakOutput, race.sim.drivers[seat].engine.peakOutput);
            }
            assert!(race.start(plan.countdown));
            assert!(client.start(start.countdown));
            assert!(race.config_message().is_none());
            assert_ne!(client.sim.drivers[0].variant, client.sim.drivers[1].variant);
            let mut received = MpSnapshot::default();
            let mut checkpoint = Box::new(RaceFrame::default());
            let mut delivered = 0;
            let mut predicted_finishes = 0;
            #[cfg(unix)]
            let mut alive = [true; crate::SEAT_COUNT];
            for _ in 0..100_000 {
                if race.is_finished() { break; }
                #[cfg(unix)]
                if race.tick() == 100 {
                    if let Some(seat) = disconnect { streams.as_mut().unwrap().disconnect(seat); }
                }
                for seat in 0..crate::SEAT_COUNT {
                    #[cfg(unix)]
                    if streams.as_ref().is_some_and(|streams| streams.peers[seat].is_none()) { continue; }
                    if race.pose(seat).status != 1 { continue; }
                    // The existing C guidance computes steering only; submit
                    // normal human input through the real adapter/validator.
                    let mut guide = race.sim.drivers[seat].car;
                    let automatic = SteeringInput { mode: SteeringMode_STEERING_AUTOMATIC,
                        ..Default::default() };
                    unsafe { UpdateCarSteering(&mut guide, &automatic); }
                    let input = DriverInput {
                        steering: SteeringInput { mode: SteeringMode_STEERING_ANALOG,
                            angle: unsafe { guide.__bindgen_anon_3.drive.steerPos },
                            ..Default::default() },
                        throttle: if guide.speed < 400 { 256 } else { 0 },
                        brake: if guide.speed > 450 { 128 } else { 0 },
                        ..Default::default()
                    };
                    #[cfg(unix)]
                    if let Some(streams) = &mut streams {
                        streams.input(seat, &input);
                        continue;
                    }
                    assert!(race.set_input(seat as i32, &input));
                }
                // Direct-input runs also exercise a finish inside the prediction
                // horizon. Stream input is applied later by step_room.
                #[cfg(unix)]
                let before = streams.is_none().then(|| Box::new(*race.sim));
                #[cfg(not(unix))]
                let before = Some(Box::new(*race.sim));
                #[cfg(unix)]
                if let Some(streams) = &streams {
                    let running = crate::step_room(&mut race, &streams.session, &mut alive).unwrap();
                    assert_eq!(running, !race.is_finished());
                } else { assert!(race.step()); }
                #[cfg(not(unix))]
                assert!(race.step());
                if let Some(before) = before {
                    for seat in 0..crate::SEAT_COUNT {
                        if before.drivers[seat].status != SimDriverStatus_SIM_DRIVING ||
                            race.sim.drivers[seat].status != SimDriverStatus_SIM_DRIVER_FINISHED { continue; }
                        let mut predicted = Box::new(RaceSim::default());
                        assert_eq!(unsafe { MpPredictRace(before.as_ref(), &MpCommands::default(),
                            std::ptr::null(), 0, seat as i32, before.tick + 10, predicted.as_mut()) }, 1);
                        assert_eq!(predicted.tick, race.tick(), "prediction advanced past local finish");
                        let mut expected = [0; RACE_FRAME_WIRE_SIZE as usize];
                        let mut actual = expected;
                        assert_eq!(unsafe { EncodeRaceFrame(race.sim.as_ref(), expected.as_mut_ptr(), expected.len()) }, 1);
                        assert_eq!(unsafe { EncodeRaceFrame(predicted.as_ref(), actual.as_mut_ptr(), actual.len()) }, 1);
                        assert_eq!(actual, expected);
                        predicted_finishes += 1;
                    }
                }
                assert_eq!(unsafe { SaveRaceFrame(race.sim.as_ref(), checkpoint.as_mut()) }, 1,
                    "checkpoint at tick {}, phase {}", race.tick(), race.phase());
                if race.tick() != 1 && race.tick() % stride != 0 && !race.is_finished() { continue; }
                delivered += 1;
                let packet = crate::snapshot_message(race.tick(), race.elapsed(), race.phase() as u8,
                    std::array::from_fn(|seat| race.pose(seat)), [0; crate::SEAT_COUNT]);
                #[cfg(unix)]
                let packet = if let Some(streams) = &mut streams {
                    streams.receive(&race.state_message([0; crate::SEAT_COUNT]).unwrap())
                } else { packet.to_vec() };
                if packet[0] == 0x87 {
                    let mut correction = MpCorrection::default();
                    assert_ne!(unsafe { MpDecodePublication(client.sim.as_ref(), packet.as_ptr(), packet.len(),
                        &mut correction, &mut received) }, 0);
                    let mut commands = MpCommands::default();
                    assert_ne!(unsafe { MpApplyCorrection(client.sim.as_mut(), &mut commands, 0, &correction) }, 0);
                } else {
                    assert_ne!(unsafe { MpDecodeSnapshot(packet[1..].as_ptr(), packet.len() - 1, &mut received) }, 0);
                    assert_ne!(unsafe { MpApplySnapshot(client.sim.as_mut(), &received) }, 0);
                }
                assert_eq!(client.sim.tick, race.sim.tick);
                assert_eq!(client.sim.elapsed, race.sim.elapsed);
                assert_eq!(client.sim.countdown, race.sim.countdown);
                for seat in 0..crate::FIELD_COUNT {
                    let mut presented = client.pose(seat);
                    // Client HUD reads the received rank; its partial state
                    // cannot recompute the server's two-component progress.
                    if packet[0] != 0x87 { presented.place = client.sim.drivers[seat].place; }
                    assert_eq!(crate::protocol::pose_message(presented),
                        crate::protocol::pose_message(race.pose(seat)),
                        "client pose differs: reverse={reverse}, stride={stride}, seat={seat}");
                }
            }
            assert!(race.is_finished(), "race did not finish: reverse={reverse}");
            #[cfg(unix)]
            if streams.is_none() { assert_eq!(predicted_finishes, crate::SEAT_COUNT); }
            #[cfg(not(unix))]
            assert_eq!(predicted_finishes, crate::SEAT_COUNT);
            if stride == 1 { assert_eq!(delivered, race.tick()); }
            else { assert!(delivered > 1 && delivered < race.tick() / 2); }
            let results = [race.result(0), race.result(1)];
            for (seat, &(finished, place, time)) in results.iter().enumerate() {
                if disconnect == Some(seat) {
                    assert_eq!((finished, place, time), (false, 0, -1));
                } else {
                    assert!(finished && (1..=crate::FIELD_COUNT as u8).contains(&place) && time > 0);
                }
            }
            assert_ne!(results[0].1, results[1].1);
            if disconnect.is_none() {
                let first = (results[1].1 < results[0].1) as usize;
                assert!(results[first].2 <= results[1 - first].2);
            }
            let session = crate::Session::new();
            assert!(session.inputs[0].greet(b"Alice"));
            assert!(session.inputs[1].greet(b"Bob\xFF"));
            #[cfg(unix)]
            let finish = if let Some(streams) = &mut streams {
                let finish = crate::finish_room(&race, &mut streams.session, plan).unwrap();
                assert_eq!(finish.writes_ok, disconnect.is_none());
                finish
            } else { crate::Finish::capture(&race, &session, plan) };
            #[cfg(not(unix))]
            let finish = crate::Finish::capture(&race, &session, plan);
            assert_eq!(finish.plan.reverse, reverse);
            for seat in 0..crate::SEAT_COUNT {
                assert_eq!(finish.plan.seats[seat].model, race.sim.drivers[seat].variant);
            }
            assert_eq!(finish.results, results);
            let message = crate::protocol::encode_result(finish.results);
            #[cfg(unix)]
            let message = if let Some(streams) = &mut streams {
                streams.receive(&message)
            } else { message.to_vec() };
            assert_eq!(message.len(), 13);
            assert_eq!(message[0], crate::wire::S2C_RESULT);
            let mut result = MpResult::default();
            assert_ne!(unsafe { MpDecodeResult(message[1..].as_ptr(), message.len() - 1,
                &mut result) }, 0);
            assert_ne!(unsafe { MpMatchesResult(&received, &result) }, 0);
            for (seat, &(finished, place, time)) in results.iter().enumerate() {
                let offset = 1 + seat * 6;
                assert_eq!(&message[offset..offset + 2], &[finished as u8, place]);
                assert_eq!(&message[offset + 2..offset + 6], &time.to_le_bytes());
                assert_eq!(race.pose(seat).status, if disconnect == Some(seat) { 0 } else { 2 });
            }
            assert!(!race.step());
            assert_eq!(std::array::from_fn::<_, 2, _>(|seat| race.result(seat)), finish.results);
            drop(race);
            drop(session);
            assert_eq!(finish.names, [b"Alice".to_vec(), b"Bob\xFF".to_vec()]);
            assert_eq!(crate::protocol::encode_result(finish.results).as_slice(), message.as_slice());
        }
    }

    #[test]
    #[ignore = "requires a legal disc image in RAGE_SIM_DISC_BIN"]
    fn parallel_real_races_match_independent_sequential_traces() {
        let path = std::env::var("RAGE_SIM_DISC_BIN").expect("select a legal disc image");
        let archive = std::sync::Arc::new(RaceArchive::load_disc(&path).expect("load legal source"));
        let mut plan = crate::Plan::default();
        plan.seats[1].model = 9;
        plan.seats[1].manual = true;
        let mut other = crate::Plan { class: 2, course: 1, reverse: true, ..plan };
        other.seats[0] = crate::Seat { model: 6, manual: false, seed: 42 };
        other.seats[1] = crate::Seat { model: 31, manual: true, seed: 123 };
        plan.rivals = archive.rivals(plan.class, plan.course, plan.reverse).unwrap();
        other.rivals = archive.rivals(other.class, other.course, other.reverse).unwrap();
        let races: Vec<_> = [plan, other].iter()
            .map(|plan| crate::prepare_race(&archive, plan).unwrap()).collect();
        fn trace(mut race: Race) -> Vec<([u8; crate::protocol::SNAPSHOT_SIZE], [u32; 2])> {
            assert!(race.start(1));
            let mut frames = Vec::with_capacity(1000);
            for tick in 0..1000 {
                for seat in 0..2 {
                    if race.pose(seat).status != 1 { continue; }
                    let mut bytes = [0; 12];
                    bytes[5..7].copy_from_slice(&256i16.to_le_bytes());
                    bytes[9] = (seat == 1 && tick % 80 == 0) as u8;
                    assert!(race.set_input(seat as i32, &decode_input(&bytes).unwrap()));
                }
                assert!(race.step());
                frames.push((crate::snapshot_message(race.tick(), race.elapsed(), race.phase() as u8,
                    std::array::from_fn(|seat| race.pose(seat)), [0; crate::SEAT_COUNT]),
                    std::array::from_fn(|seat| race.sim.drivers[seat].random)));
                if race.is_finished() { break; }
            }
            frames
        }
        let mut races = races.into_iter();
        let first = trace(races.next().unwrap());
        let second = trace(races.next().unwrap());
        let left_archive = archive.clone();
        let right_archive = archive.clone();
        drop(archive); // Workers now own the import source through preparation.
        let gate = std::sync::Barrier::new(2);
        let (parallel_left, parallel_right) = std::thread::scope(|scope| {
            let left = scope.spawn(|| {
                gate.wait();
                let race = crate::prepare_race(&left_archive, &plan).unwrap();
                drop(left_archive); // Race keeps no archive borrow while stepping.
                trace(race)
            });
            let right = scope.spawn(|| {
                gate.wait();
                let race = crate::prepare_race(&right_archive, &other).unwrap();
                drop(right_archive);
                trace(race)
            });
            (left.join().unwrap(), right.join().unwrap())
        });
        assert_eq!(first, parallel_left);
        assert_eq!(second, parallel_right);
        assert_ne!(first, second);
        assert_eq!(first.len(), 1000);
        assert_eq!(second.len(), 1000);
    }
}
