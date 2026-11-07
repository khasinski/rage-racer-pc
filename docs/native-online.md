# Native desktop online

Open **ONLINE** in the main menu. All account, room, chat and race screens
run in the native game, with the same panel style as Esc settings. No browser
or launcher is required. Select the server origin in the profile panel, or set
`online.server` in the INI. The development default is `http://127.0.0.1:7243`.
The game reuses the mounted disc image; it must match the server's disc ID.

## Online Arcade

**Online Arcade** is a separate casual mode alongside private rooms and Duel.
It joins one shared community automatically and starts with a single person.
The server reserves up to eight controllable cars, drives vacant seats with
the C autopilot and fills the rest of the twelve-car field with retail AI.
The reserved cars use player physics throughout: a takeover changes input
ownership, preserving the complete car state, specification and race progress.

After loading, a newcomer follows the selected car before a three-second
takeover countdown. The server switches its input owner at one authoritative
tick. Leaving or disconnecting restores the autopilot immediately. A dropped
account reserves its place for fifteen seconds so it can reconnect.

Public races use two laps, rotating classes/courses, with a four-minute upper
bound. After the first finisher the remaining field gets thirty seconds.
Results lead to a fifteen-second intermission and the next automatic start.
Members can vote for a legal next course, choose a next-race car and chat.
Late arrivals in the last twenty seconds stay for the next common start.
Chat history survives round changes; an empty session stops simulating after
the reconnect grace period. Private rooms retain their host/ready workflow.

Joined-in-progress results are marked and carry no full-race time. Best-lap
calculation excludes the lap that was already underway at takeover. Arcade
results are casual session results and are not inserted into the private-race
records/history database. CPU cars are always identified as CPU.

## Networking

Race input and checkpoints normally use an unordered WebRTC DataChannel with
zero retransmissions (SCTP/DTLS/UDP). HTTP and WebSocket carry authentication,
chat, room controls and WebRTC signalling. A heartbeat confirms both directions
before race traffic switches to UDP. Loss or silence restores the WebSocket
fallback and retries the UDP channel with backoff. The panel displays the active
route. Inbound RTC frames use bounded raw-DEFLATE decompression.

Arcade checkpoints include a round epoch. Late UDP packets from an earlier
round cannot overwrite a new race. Slot ownership and input queues stay on the
server; a client cannot drive a CPU-controlled or another person's seat.

The WebRTC implementation and TLS libraries are compiled into the executable.
`rage-online-core` isolates online simulation symbols from the offline game's
legacy globals. CMake copies its DLL/shared library beside the executable
(inside the macOS app) and supplies loader-relative search paths. Release and
editor staging include this module.

## Server operator

The server lives in `online/`. It uses the existing Node/TypeScript server
toolchain and the same C simulation compiled with Emscripten. Players only need
the native game and their own game image. Node 24+, npm and Emscripten are
needed by the operator/developer, not by desktop players.

```sh
npm --prefix online ci
npm --prefix online run build:sim
node online/server/main.ts --disc '/path/to/Track 01.bin' --port 7243
```

SQLite data defaults to `online/server/data/`. Deploy the updated server for
Arcade support; an older browser-only backend does not implement this mode.
Set `RAGE_RTC_PORT` and publish that UDP port; behind NAT, also set
`RAGE_RTC_PUBLIC_IP` to the reachable server address. `RAGE_RTC=0` deliberately
disables UDP for fallback diagnosis. HTTPS/WSS uses normal certificate
verification. Credentials and tokens are not written to the game INI/log.

## Verification

```sh
npm --prefix online run typecheck
RAGE_TEST_DISC='/path/to/Track 01.bin' npm --prefix online test
cmake --build build/release --target rage-racer-smoke online-client-check
PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy build/release/src/online/online-client-check \
  '/path/to/Track 01.bin' http://127.0.0.1:7243 arcade-cycle 300
```

The server tests cover lone-player start, exact-tick handover, late join,
disconnect/reconnect, input ownership, persistent chat, votes, automatic next
round and idle shutdown. With a disc, a real C/WASM test verifies that every
checkpoint byte except the input block is identical across takeover.
Native tests cover renderer ABI/provider lifetime and WebSocket framing,
fragmentation, masking, ping/pong, account replacement and invalid frames.

For silent hidden GPU captures use `rage-racer-smoke` with `hooks.online=arcade`
or `arcade-lobby`, `hooks.online_seconds`, `online.capture` (PPM scene target)
and `capture.online_panel` (PPM menu). Keep `PSYZ_HEADLESS=1` and
`SDL_AUDIODRIVER=dummy` set. Hardware wheel feedback and Windows/Linux execution
still require testing on those devices/platforms.

## Source boundary

The initial owned C runtime was adapted from `rage-port-online/web/engine`
and `web/wasm` on `experiment/webport` (based on `e6dbadb41`); the server was
adapted from that checkout's current `web/server` and `web/shared`. The sibling
checkout was only read. Online-specific changes live here. The offline runtime
continues to use the repository's main C sources; changes affecting both
physics implementations must be reviewed for parity until they are unified.
Third-party WebRTC dependency versions, sources and notices are listed in
`online-licenses.txt`.
