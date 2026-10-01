# Rage Racer online

Rage Racer in the browser, with online races run by a server. The race
physics, disc import and scene building are the desktop C code (`engine/`,
a separate copy of the repository's sources) compiled to WebAssembly. The
browser draws them with three.js.

- **Players** log in, choose their own Rage Racer disc (CUE or Track 01 BIN,
  read locally, never uploaded), then create or join rooms. A room fixes the
  course, direction, class, laps, number of players (up to 8) and whether the
  retail rivals fill the rest of the field. Players start together on their
  own grid: two lanes behind the retail player start, before the line. Rivals
  keep their authored starts, which the original spreads along the course
  ahead. The class decides which cars can be
  chosen, with the retail custom-race rule: every model the class has
  unlocked, at the grade that class buys. The Extreme Oval starts at class 3.
  A guest can save their account with a name and password at any time (the
  same account: races, paint and logo stay). The race forms start on the car
  the player last chose and mark the cars they painted; the results offer
  another race; next to each ping a mark shows a player who races over the
  WebSocket fallback; a banner says when the connection is lost or another
  window has the account (with a button to play here); and on a touch screen
  the race has on-screen controls.
- **The server** (`server/`) owns every race. It builds the field, runs the
  simulation at 50 Hz, applies the players' inputs and streams the complete
  race state (the engine's `RaceFrame` checkpoint) 25 times a second. It also
  sends lap, finish and retirement events, closes the race 90 s after the
  winner (at least), and stores the results. Accounts, sessions, rooms,
  races, results, lap records and each player's history live in SQLite
  (`node:sqlite`). `rooms.ts` is the lobby (connections and what members ask
  of rooms), `room.ts` one room, `room-rules.ts` what rooms accept,
  `race-clock.ts` the 50 Hz clock and `race.ts` one running race; `db.ts` the
  storage, `looks.ts` the checks of what the garage sends and `sim.ts` the
  simulation module.
- **The garage** shows your car turning on a showroom camera (the same
  renderer, one car on the start grid, `rw_build_showroom`) while you pick its
  two body colours from the game's 18-colour paint catalogue. The paint is
  saved on the server per car model (`/api/garage`), sent to everyone with the
  race's seats and drawn by each client. It is presentation only: the
  simulation, the wire format and the checks that compare the native and
  WebAssembly physics never see it. Two more things the retail DESIGN mode
  let you do are here: a **team logo** (a 64x64 picture with a 16-colour
  palette, drawn in an editor with a pen, fill, colour picker, flips, turns,
  moves and undo, or imported from any picture and reduced to fifteen colours)
  goes on the bonnet, and your **name** (six characters of the game's font)
  on the windscreen strip. The engine writes both into a copy of the car's
  shared texture page when it decodes the materials (`engine/src/port/car_custom.c`),
  so every client draws every player's logo and name. The logo is saved once
  per player (`/api/logo`). Changes are saved as they are made (a moment
  after the last one, and at once when leaving the garage or switching car).
  Tyres come next.
- **Practice offline** runs the same simulation locally against the rivals.
- **Sound** is the retail race audio: the desktop's own sound code (engine
  layers pitched from the car's curves, tyre, impact and landing cues, the
  announcer, track ambience) runs on the simulation and is mixed in C by a
  software SPU (`wasm/web_audio.c`, `wasm/web_spu.c`) from the disc's sample
  banks; an AudioWorklet plays it. Online you also hear the other drivers'
  engines. Choosing the CUE with its BINs (or the whole disc folder) adds the
  race music from the CD audio tracks; with the Track 01 BIN alone there is
  none. Audio starts with the first click or key press; M mutes.

## Running it

Requirements: Node 26 (see `mise.toml`), Emscripten 6 and CMake.

```sh
npm install
npm run wasm        # builds public/wasm/rage-web.* and server/wasm/rage-server.*
npm run build       # typechecks and builds the client into dist/
npm run seed        # accounts admin (administrator) and rage, with generated passwords
npm run server -- --disc "/path/to/Rage Racer (Europe) (Track 01).bin"
```

Open <http://localhost:7243/>. The server serves the client, the REST API
(`/api/register`, `/api/login`, `/api/me`, `/api/history`, `/api/records`, `/api/garage`, `/api/logo`) and
the WebSocket (`/ws`). Options: `--port` (default 7243), `--host`, `--db`
(default `server/data/rage.db`), `--static`. The server needs its own copy of
the disc, and players must use the same release (the lobby compares the boot
serial and a fingerprint of the game archive).

Timings (milliseconds, mainly for the checks): `RAGE_LOAD_TIMEOUT_MS`
(30 s for every player to load a race), `RAGE_FINISH_GRACE_MS` (at least
90 s to finish after the winner), `RAGE_OFFLINE_GRACE_MS` (60 s to come back
after a dropped connection outside a race), `RAGE_KEEPALIVE_MS` (2 s pings)
and `RAGE_DEAD_MS` (60 s of silence closes a connection).

For client development, `npm run dev` proxies `/api` and `/ws` to the server
at `RAGE_SERVER` (default `http://localhost:7243`).

## Deploying (CapRover)

```sh
npm run wasm && npm run build
node scripts/package-deploy.mjs "/path/to/Track 01.bin" deploy.tar
caprover deploy -n <machine> -a rageracer -t deploy.tar
```

The image (`deploy/Dockerfile`) runs the server on port 7243 with the disc
inside the image and the SQLite database in `/data`. The CapRover app needs:
- **Port and WebSockets:** container HTTP port 7243, with WebSocket support on.
- **Storage:** a persistent volume at `/data`.
- **HTTPS:** enabled, with HTTPS forced.
- **Administrator:** `RAGE_ADMIN_PASSWORD` creates or updates the `admin`
  account at start-up.
- **WebRTC (optional, recommended):** the race stream prefers a WebRTC data
  channel (unordered, so a lost packet does not hold up the ones behind it),
  on UDP port `RAGE_RTC_PORT` (7243 in the image). Publish that UDP port on
  the host, preferably in host mode (not through the swarm routing mesh),
  and set `RAGE_RTC_PUBLIC_IP` to the server's public address. Without them
  every client stays on the WebSocket; a channel that goes silent falls back
  to it within two seconds, and the client offers a new channel (after 1 s, then
  up to every 30 s) until one works again. Either end uses a channel only once
  it has heard the other over it. `RAGE_RTC=0` turns the channel off.

## Protocol

`shared/protocol.ts` lists every message. Control messages are JSON. The race
uses binary frames: the client sends 8 input words each 50 Hz tick, stamped
with the race tick it predicted them for, and the server sends the server
tick, per player the last input applied and how early the latest one
arrived, then the `RaceFrame` bytes. The server applies each input at its
stamped tick. A player's client restores each frame into its own copy of the
same race (`rw_apply_frame`, built from the same rules in
`wasm/web_rules.c`, compiled into both modules), replays its unacknowledged
inputs and keeps its clock a couple of ticks ahead of the server by that
margin. Corrections to a drawn car ease out over a few steps instead of
jumping. Spectators play frames back a few ticks behind the newest one.
Where it can, the race runs over a WebRTC data channel instead of the
WebSocket (`server/rtc.ts`, `?transport=ws` forces the WebSocket).

`node scripts/net-check.mjs "<disc>"` measures the corrections and jerk
under simulated latency, jitter and loss (the client takes the same
`?net=rtt:150,jitter:20,loss:1` query).

## Checks

```sh
npm run typecheck
npm run test:server -- "<disc>"   # accounts, rooms, rules, a full race to results, leaving/reconnecting/timeouts
npm run test:e2e -- "<disc>"      # two headless browsers race each other, then practice
npm run test:grid -- "<disc>"     # 8-player grid on every course variant
npm run test:finish -- "<disc>"   # finish fade, spectator hand-over, finish deadline
npm run test:garage -- "<disc>"   # the garage preview, saving paint, seeing it in a race
npm run test:duel -- "<disc>"     # a duel by link: the shared car, both drivers
npm run test:units                 # logo editing, the garage's requests, the prediction clock
npm run test:fallback -- "<disc>" # the data channel: fallback, recovery, a stalled page
npm run test:stall -- "<disc>"    # joining late, coming back, stalling: the page stays in the race
npm run test:replaced -- "<disc>" # two windows on one account
npm run test:connecting -- "<disc>" # messages sent while the socket opens
npm run test:ux -- "<disc>"       # saving a guest, the last car, race again, ping marks, touch controls
node scripts/smoke.mjs "<disc>"   # the simulation module on its own
node scripts/audio-check.mjs "<disc.cue>"  # engine sound follows rpm; race tunes found
# Native vs WebAssembly physics, tick by tick:
cmake -S . -B ../build/web-parity -DCMAKE_BUILD_TYPE=Release && cmake --build ../build/web-parity
../build/web-parity/rage-web-parity "<disc>" 2000 > native.txt
node scripts/parity.mjs "<disc>" 2000 > wasm.txt && cmp native.txt wasm.txt
```

## Not done yet

- **Sound gaps:** no SPU reverb (the retail tunnels' echo), no announcer
  lines about the rivals, no fly-by/shuttle scenery sounds, and no results
  music after the finish.
- **Redeploying stops running races.** Races live in the server's memory, so
  a redeploy ends them. Planned: on SIGTERM the server freezes every race,
  writes rooms, members and each race's exact `RaceFrame` to `/data`, tells
  the clients the race is paused for an update and exits. The new server
  restores the rooms and the paused races from that snapshot. Clients
  reconnect on their own (places are already kept), reload the race and
  report back, and the race resumes after a short countdown once every
  player is back or a timeout passes. Until then, check the room list for
  running races before deploying.
