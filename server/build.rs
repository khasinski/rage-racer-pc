use std::env;
use std::path::{Path, PathBuf};

/// Links the pre-built `rage-sim`/`rage-data` static libraries (see
/// ../docs/multiplayer.md) and generates Rust bindings for the narrow
/// headless race API (game/race_grid.h and what it pulls in). This does not
/// build the C libraries: run the existing CMake headless build first
/// (`cmake -S . -B build/sim-release -DRAGE_BUILD_PORT=OFF
/// -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build
/// build/sim-release`), or point RAGE_SIM_LIB_DIR at another build
/// directory that already contains librage-sim.a/librage-data.a.
fn main() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let repo_root = manifest_dir.parent().expect("server/ has a parent directory");

    let lib_dir = env::var("RAGE_SIM_LIB_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| repo_root.join("build/sim-release"));

    for lib in ["rage-sim", "rage-data"] {
        let lib_path = lib_dir.join(format!("lib{lib}.a"));
        if !lib_path.is_file() {
            panic!(
                "{} not found. Build it first:\n\
                 cmake -S . -B build/sim-release -DRAGE_BUILD_PORT=OFF \
                 -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release\n\
                 cmake --build build/sim-release --target rage-sim rage-data\n\
                 (run from the repo root) or set RAGE_SIM_LIB_DIR to an \
                 existing build directory.",
                lib_path.display()
            );
        }
        println!("cargo:rustc-link-lib=static={lib}");
    }
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rerun-if-env-changed=RAGE_SIM_LIB_DIR");

    let include_dir = repo_root.join("include");
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_arg(format!("-I{}", include_dir.display()))
        .allowlist_function("InitRaceSim")
        .allowlist_function("AddRaceDriver")
        .allowlist_function("AddRaceRival")
        .allowlist_function("StartRaceSim")
        .allowlist_function("SetRaceInput")
        .allowlist_function("ValidDriverInput")
        .allowlist_function("StepRaceSim")
        .allowlist_function("RacePosition")
        .allowlist_function("RaceTime")
        .allowlist_function("RaceLapTime")
        .allowlist_function("RetireRaceDriver")
        .allowlist_function("InitRaceGrid")
        .allowlist_function("ReadRaceData")
        .allowlist_function("ReadRaceCar")
        .allowlist_function("CopyRaceTrack")
        .allowlist_function("LoadRaceDisc")
        .allowlist_function("LoadRaceArchive")
        .allowlist_function("ArchiveFingerprint")
        .allowlist_function("FreeRaceData")
        .allowlist_function("FreeTrackData")
        .allowlist_type("RaceSim")
        .allowlist_type("SimDriver")
        .allowlist_type("RaceEntrant")
        .allowlist_type("RaceSeatKind")
        .allowlist_type("DriverInput")
        .allowlist_type("RaceData")
        .allowlist_type("TrackData")
        .allowlist_var("DRIVER_SEAT_LIMIT")
        .allowlist_var("SIM_.*")
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
