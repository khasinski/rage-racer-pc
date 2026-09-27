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

pub struct RaceArchive(*mut RaceData);

impl RaceArchive {
    pub fn load_cache(path: &std::path::Path) -> Option<Self> {
        let cached = super::cache::read(path).ok()?;
        let cpath = CString::new(cached.file.to_str()?).ok()?;
        let ptr = unsafe { LoadRaceArchive(cpath.as_ptr()) };
        if ptr.is_null() { return None; }
        let archive = Self(ptr);
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
        let archive = unsafe { &*self.0 };
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
            Some(RaceArchive(ptr))
        }
    }

    pub fn executable(&self) -> u64 { unsafe { (*self.0).executable } }

    pub fn fingerprint(&self) -> u64 {
        unsafe { ArchiveFingerprint((*self.0).data.cast(), (*self.0).size) }
    }

    pub fn boot(&self) -> [u8; 16] {
        std::array::from_fn(|i| unsafe { (*self.0).boot[i] as u8 })
    }

    pub fn copy_track(&self, class_index: i32, course_index: i32) -> Option<TrackDataOwned> {
        let ptr = unsafe { CopyRaceTrack(self.0, class_index, course_index) };
        if ptr.is_null() {
            None
        } else {
            Some(TrackDataOwned(ptr))
        }
    }

    fn as_ptr(&self) -> *const RaceData {
        self.0
    }
}

impl Drop for RaceArchive {
    fn drop(&mut self) {
        unsafe { FreeRaceData(self.0) };
    }
}

// Safety: RaceData is an immutable view over the loader's own owned buffer
// once returned; the server only ever reads through it from one thread at a
// time (the race-step thread), guarded by the caller holding the only handle.
unsafe impl Send for RaceArchive {}

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
    Human { model: i32, manual: bool, seed: u32 },
}

impl Race {
    pub fn new(
        archive: &RaceArchive,
        track: TrackDataOwned,
        seats: &[SeatPlan],
        laps: i32,
        reverse: bool,
    ) -> Option<Self> {
        if seats.len() > DRIVER_SEAT_LIMIT as usize { return None; }
        let mut entrants = [RaceEntrant::default(); DRIVER_SEAT_LIMIT as usize];
        for (grid, seat) in seats.iter().enumerate() {
            entrants[grid] = match *seat {
                SeatPlan::Empty => RaceEntrant {
                    kind: RaceSeatKind_RACE_SEAT_EMPTY,
                    ..Default::default()
                },
                SeatPlan::Human {
                    model,
                    manual,
                    seed,
                } => RaceEntrant {
                    kind: RaceSeatKind_RACE_SEAT_HUMAN,
                    grid: grid as i32,
                    model,
                    rivalSlot: 0,
                    manual: manual as i16,
                    seed,
                },
            };
        }

        let mut sim: Box<RaceSim> = Box::default();
        let ok = unsafe {
            InitRaceGrid(
                sim.as_mut(),
                archive.as_ptr(),
                track.as_ptr(),
                entrants.as_ptr(),
                std::ptr::null(),
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
    pub fn step(&mut self) -> bool {
        unsafe { StepRaceSim(self.sim.as_mut()) != 0 }
    }

    pub fn tick(&self) -> u32 {
        self.sim.tick
    }

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
            rpm: unsafe { driver.car.__bindgen_anon_3.drive.engineRpm },
            throttle: unsafe { driver.car.__bindgen_anon_3.drive.acceleratorInput.value as i32 },
            clutch: unsafe { driver.car.__bindgen_anon_3.drive.clutch as i32 },
            gear: unsafe { driver.car.__bindgen_anon_3.drive.gear as i32 },
            ground: driver.car.modelY,
            roll_speed: driver.car.bodyRollVelocity,
        }
    }
}

// Safety: see RaceArchive/TrackDataOwned; the race thread is the sole owner
// while it exists, so moving the whole Race into that thread is sound even
// though the raw pointers inside are not implicitly Send.
unsafe impl Send for Race {}
