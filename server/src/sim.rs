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

pub struct RaceArchive(*mut RaceData);

impl RaceArchive {
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
    pub active: bool,
    pub x: i32,
    pub y: i32,
    pub z: i32,
    pub yaw: i32,
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

    pub fn set_input(
        &mut self,
        slot: i32,
        steering_mode: u32,
        steering_left: bool,
        steering_right: bool,
        steering_angle: i32,
        throttle: i16,
        brake: i16,
        shift_up: bool,
        shift_down: bool,
    ) -> bool {
        let input = DriverInput {
            steering: SteeringInput {
                mode: steering_mode,
                left: steering_left as i32,
                right: steering_right as i32,
                angle: steering_angle,
            },
            throttle,
            brake,
            shiftUp: shift_up as i32,
            shiftDown: shift_down as i32,
        };
        unsafe { SetRaceInput(self.sim.as_mut(), slot, &input) != 0 }
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

    pub fn pose(&self, seat: usize) -> CarPose {
        let driver = &self.sim.drivers[seat];
        let active = driver.status == SimDriverStatus_SIM_DRIVING
            || driver.status == SimDriverStatus_SIM_DRIVER_FINISHED;
        // Safety: bodyRotation and the named-field arm are the same bytes;
        // reading the Vec4 arm of this union is exactly as valid as reading
        // the struct arm the simulation itself writes.
        let yaw = unsafe { driver.car.__bindgen_anon_1.bodyRotation.y };
        CarPose {
            active,
            x: driver.car.x,
            y: driver.car.y,
            z: driver.car.z,
            yaw,
        }
    }
}

// Safety: see RaceArchive/TrackDataOwned; the race thread is the sole owner
// while it exists, so moving the whole Race into that thread is sound even
// though the raw pointers inside are not implicitly Send.
unsafe impl Send for Race {}
