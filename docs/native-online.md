# Native desktop online

Open **ONLINE** in the main menu. All account, room, chat and race screens
run in the native game, with the same panel style as Esc settings. No browser
or launcher is required. Select the server origin in the profile panel, or set
`online.server` in the INI. The default is `https://rageracer.polanka.ovh`;
use `http://127.0.0.1:7243` for local development.
The game reuses the mounted disc image; it must match the server's disc ID.

## Online Arcade

The Online entry screen offers **Play online** (guest auto-join), **Play with
friends** (rooms and invitations), and **Profile & settings**. Name editing,
optional account login/registration, and the server address live under profile
settings; no credentials or server configuration are required to start racing.

Desktop sky rendering uses the race's authored RGB cloud panorama and
interpolated top-left screen coordinates, independently of GPU fragment Y
conventions. Online material-cache keys retain their authored model identity
in the low 16 bits so lamp surfaces, glows and projected lights work for the
entire field. The existing car-lights setting also applies to online races.

Joining shows a preparation card until the first server snapshot has produced
a presentation frame, followed by a visible takeover countdown. The spectator
chase camera remains in place when the server hands over the car; the change
of ownership does not switch the driver into the cockpit. Arrivals without a
scheduled seat see an explicit watching/waiting message.

New grid starts and mid-race takeovers have separate countdown labels. The
car's retail HUD is prepared during the countdown, so taking control does
not perform a second asset decode. When a native scene is awaiting its first
checkpoint, presentation retains the last native image instead of flashing
the suspended offline frontend.

Finish presentation keeps coasting and the classified time visible for up to
2.5 seconds, including when results precede the final checkpoint. It does
not add another delay if that finisher has already coasted while others race.
Driving input/feedback stop, effects fade, and results retain the race scene
behind the panel. Arcade results show the next automatic start countdown;
the next race resets finish/HUD/input state. Both server modes send the final
checkpoint over ordered WebSocket before results, even when ordinary race
traffic uses unreliable RTC, so a lost last datagram cannot lose the finish.

The speedometer is the retail car-specific dial, decoded from the selected
disc together with its day/night palettes and manual/automatic digit strips.
The race core supplies the evaluated needle, shift-lamp, gear and speed
packets. The session provider passes them to the desktop GPU's ordinary HUD
sprite/quad renderer; the atlas uploads once per prepared car, not every
frame. The usual HUD edge-anchor setting applies. Positions, race time and
notifications occupy the upper-left panel and do not duplicate speed/gear.
The existing `poc.speed_units=kmh|mph` override also applies online: the same
hash-verified PAL/NTSC-U face-and-palette deltas select the bitmap, together
with the corresponding retail speed conversion. A mismatched source keeps
its original dial and units.

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

The native RFC6455 handshake explicitly uses a fresh HTTP/1.1 connection.
The HTTP authentication request may negotiate HTTP/2, but that connection must
not be reused for a manual WebSocket Upgrade (including on macOS system curl).

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

`/healthz` reports the running build; `/api/status` also reports capability,
disc identity and aggregate connection/race counts. Existing browser clients
retain checkpoint format 2. Native clients advertise `client=native-v1` in
their WebSocket URL to receive epoch-tagged private-race frames.

For CapRover backend-only upgrades, `scripts/package-deploy.mjs --base-image`
creates a small layer over the current image, preserving its browser assets,
disc and installed dependencies. Verify dependency versions before selecting
this route. Normal full rebuilds use `deploy/Dockerfile`. Back up `/data/rage.db`
with SQLite's online backup API and retain the old image/version for rollback.

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
`online_race_lifecycle` runs the native executable against real C/WASM frames
with a deterministic classified finish, delayed final packet and duplicate
results. It checks the start HUD, coasting camera, result-scene lifetime,
blocked driving input and the next round. Set `RAGE_TEST_DISC` to enable it.

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

## Production deployment, 2026-11-07

The initial native rollout to `https://rageracer.polanka.ovh` used CapRover
image/version 17, backend build `f93f0726b`. That layer preserved version 16's browser assets,
operator-provided disc and dependencies. Image/version 16 remains available
for rollback. Persistent storage is the existing `/data` volume.

Before rollout, SQLite's online backup was saved as
`/data/backups/rage-before-native-2026-11-07T15-10-11-193Z.sqlite` with restricted
permissions. Post-deployment comparison confirmed all 33 original accounts
and every original race/result row were retained. No schema change was needed.
The browser index SHA-256 remained
`84849a1edfeb3c5bcdcef415378c7b5cfb4570624c8ad80f8351ccac3bb310cc`.

Preflight used an isolated copy of production SQLite and verified both the
existing browser's private race and native Arcade. Production verification
confirmed HTTPS, the native takeover/countdown and race traffic over UDP.
Container health and the Swarm rollout completed successfully. The temporary
preflight container and database copy were removed; the recovery backup remains.

### Finish delivery update

The current production backend is image/version **18**, build `6930f66d5`,
layered over version 17 for reliable final-checkpoint delivery in Arcade and
private races. Version 17 remains available for rollback. The browser index
and simulation WASM hashes are unchanged; UDP port 7243 remains published.

Before rollout, a fresh online SQLite backup was saved at
`/data/backups/rage-before-finish-2026-11-07T16-28-32.660Z.sqlite` (mode 0600).
All 37 existing accounts remain, all 29 race rows and 166 result rows are
identical to the backup. Startup re-salted the configured administrator
password as usual; its value was verified unchanged. Production native
verification added one guest and confirmed HTTPS, takeover and UDP traffic.
Container health and the Swarm rollout completed successfully.
