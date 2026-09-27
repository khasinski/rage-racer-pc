use std::env;
use std::path::{Path, PathBuf};

/// Links the pre-built `rage-sim`/`rage-data` static libraries (see
/// ../docs/multiplayer.md) and generates Rust bindings for the narrow
/// headless race API (game/race_grid.h and what it pulls in). This does not
/// build the C libraries: run the existing CMake headless build first
/// (`cmake -S . -B build/sim-release -DRAGE_BUILD_PORT=OFF
/// -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build
/// build/sim-release`), or point RAGE_SIM_LIB_DIR at another build
/// directory containing the three archives (`lib*.a`, or `*.lib` for MSVC).
fn main() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let repo_root = manifest_dir.parent().expect("server/ has a parent directory");

    let lib_dir = env::var("RAGE_SIM_LIB_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| repo_root.join("build/sim-release"));

    let msvc = env::var("CARGO_CFG_TARGET_ENV").as_deref() == Ok("msvc");
    for lib in ["rage-sim", "rage-data", "rage-mp-protocol"] {
        let filename = if msvc {
            format!("{lib}.lib")
        } else {
            format!("lib{lib}.a")
        };
        let lib_path = lib_dir.join(filename);
        if !lib_path.is_file() {
            panic!(
                "{} not found. Build it first:\n\
                 cmake -S . -B build/sim-release -DRAGE_BUILD_PORT=OFF \
                 -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release\n\
                 cmake --build build/sim-release --target rage-sim rage-data rage-mp-protocol\n\
                 (run from the repo root) or set RAGE_SIM_LIB_DIR to an \
                 existing build directory.",
                lib_path.display()
            );
        }
        // Cargo cannot infer changes to externally built static libraries.
        // Relink the server/tests after CMake rebuilds the C simulation/data.
        println!("cargo:rerun-if-changed={}", lib_path.display());
        println!("cargo:rustc-link-lib=static={lib}");
    }
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rerun-if-env-changed=RAGE_SIM_LIB_DIR");

    let include_dir = repo_root.join("include");
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_arg(format!("-I{}", include_dir.display()))
        .clang_arg(format!("-I{}", repo_root.join("src").display()))
        .allowlist_function("InitRaceSim")
        .allowlist_function("AddRaceDriver")
        .allowlist_function("AddRaceRival")
        .allowlist_function("StartRaceSim")
        .allowlist_function("SetRaceInput")
        .allowlist_function("ValidDriverInput")
        .allowlist_function("MpDecodeSnapshot")
        .allowlist_function("MpEncodeInput")
        .allowlist_function("MpDecodeStart")
        .allowlist_function("MpDecodeRoomList")
        .allowlist_function("MpDecodeLobby")
        .allowlist_function("MpBuildSetup")
        .allowlist_function("MpValidRaceOptions")
        .allowlist_function("MpMatchesArchive")
        .allowlist_function("MpApplySnapshot")
        .allowlist_function("MpEncodeCorrection")
        .allowlist_function("MpDecodeCorrection")
        .allowlist_function("MpDecodePublication")
        .allowlist_function("MpApplyCorrection")
        .allowlist_function("MpRememberCommand")
        .allowlist_function("MpCommandAt")
        .allowlist_function("MpReplayInput")
        .allowlist_function("MpPredictRace")
        .allowlist_function("MpEncodeConfig")
        .allowlist_function("MpDecodeConfig")
        .allowlist_function("MpApplyConfig")
        .allowlist_function("MpEncodeAvailability")
        .allowlist_function("MpDecodeAvailability")
        .allowlist_var("MP_AVAILABILITY_.*")
        .allowlist_var("MP_CONFIG_.*")
        .allowlist_var("MP_CORRECTION_.*")
        .allowlist_function("MpDecodeResult")
        .allowlist_function("MpMatchesResult")
        .allowlist_function("UpdateCarSteering")
        .allowlist_function("StepRaceSim")
        .allowlist_var("RACE_FRAME_WIRE_.*")
        .allowlist_function("EncodeRaceFrame")
        .allowlist_function("DecodeRaceFrame")
        .allowlist_function("ValidRaceFrame")
        .allowlist_function("SaveRaceFrame")
        .allowlist_function("RestoreRaceFrame")
        .allowlist_function("RacePosition")
        .allowlist_function("RaceTime")
        .allowlist_function("RaceLapTime")
        .allowlist_function("RetireRaceDriver")
        .allowlist_function("InitRaceGrid")
        .allowlist_function("ReadRaceData")
        .allowlist_function("ReadRaceCar")
        .allowlist_function("LoadCarCatalog")
        .allowlist_function("CarCatalogVariant")
        .allowlist_type("RageCarCatalogField")
        .allowlist_function("ReadRaceCarTransmission")
        .allowlist_function("CopyRaceTrack")
        .allowlist_function("LoadRaceDisc")
        .allowlist_function("LoadRaceArchive")
        .allowlist_function("ArchiveFingerprint")
        .allowlist_function("FreeRaceData")
        .allowlist_function("FreeTrackData")
        .allowlist_type("RaceSim")
        .allowlist_type("SimDriver")
        .allowlist_type("GameCarRuntime")
        .allowlist_type("RaceEntrant")
        .allowlist_type("RaceSeatKind")
        .allowlist_type("DriverInput")
        .allowlist_type("RaceData")
        .allowlist_type("TrackData")
        .allowlist_var("DRIVER_SEAT_LIMIT")
        .allowlist_var("SIM_.*")
        .allowlist_var("STEERING_.*")
        .allowlist_var("CAR_MODEL_VARIANT_COUNT")
        .allowlist_var("PLAYER_LAP_TIME_CAPACITY")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .derive_default(true)
        .generate()
        .expect("bindgen failed to generate rage-sim/rage-data bindings");

    bindings
        .write_to_file(out_dir.join("bindings.rs"))
        .expect("failed to write bindings.rs");
    println!("cargo:rerun-if-changed=wrapper.h");
}
