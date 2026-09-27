use std::process::Command;

fn rejected(args: &[&str], expected: &str) {
    let output = Command::new(env!("CARGO_BIN_EXE_rage-racer-server"))
        .args(args).output().expect("run server");
    assert!(!output.status.success());
    let error = String::from_utf8_lossy(&output.stderr);
    assert!(error.contains(expected), "{error}");
    assert!(!error.contains("panicked"), "{error}");
    assert!(!String::from_utf8_lossy(&output.stdout).contains("listening"));
}

#[test]
fn invalid_arguments_fail_before_import_or_listening() {
    rejected(&[], "usage:");
    rejected(&["missing.cue", "bad"], "port must");
    rejected(&["missing.cue", "0"], "port must");
    rejected(&["missing.cue", "65536"], "port must");
    rejected(&["missing.cue", "7878", "extra"], "usage:");
    rejected(&["missing.cue", "--class=7"], "invalid race option");
    rejected(&["missing.cue", "--course=5"], "invalid race option");
    rejected(&["missing.cue", "--laps=0"], "invalid race option");
    rejected(&["missing.cue", "--reverse", "--reverse"], "duplicate server option");
}

#[test]
fn missing_source_reports_failure_without_panic() {
    rejected(&["/nonexistent/rage-server-test/Track 01.bin"], "failed to load CUE");
}
