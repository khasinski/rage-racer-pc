# Goal: standalone multiplayer server + two racing clients

Continue the multiplayer work on branch `poc/multiplayer` in
`/Users/hasik/Projects/rage-port`, per `docs/multiplayer.md`. The goal is a
standalone server process and two game clients that can connect to it and
race against each other and see each other's cars.

Follow the "Order of work" in `docs/multiplayer.md`: finish the headless
race context and client adoption (step 1), then build the Rust server with
its protocol/lobby (step 2), then connect two clients through it so each
window shows the other player's car (step 3). Do not skip ahead to
collision authority, results, or multi-room support before step 3 is real
and playable.

Keep the existing discipline already visible in `docs/multiplayer.md`: pure,
testable C functions with explicit inputs instead of globals, narrow
regression tests alongside each extraction, and no invented capability
claimed without a passing test or a real run. The Rust server links the
existing C simulation as a static library; it does not reimplement the
race step.
