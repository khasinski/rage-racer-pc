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
- **The server** (`server/`) owns every race. It builds the field, runs the
  simulation at 50 Hz, applies the players' inputs and streams the complete
  race state (the engine's `RaceFrame` checkpoint) 25 times a second. It also
  sends lap, finish and retirement events, closes the race 90 s after the
  winner (at least), and stores the results. Accounts, sessions, rooms,
  races, results, lap records and each player's history live in SQLite
  (`node:sqlite`).
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
(`/api/register`, `/api/login`, `/api/me`, `/api/history`, `/api/records`) and
the WebSocket (`/ws`). Options: `--port` (default 7243), `--host`, `--db`
(default `server/data/rage.db`), `--static`. The server needs its own copy of
the disc, and players must use the same release (the lobby compares the boot
serial and a fingerprint of the game archive).

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

## Protocol

`shared/protocol.ts` lists every message. Control messages are JSON. The race
uses binary frames: the client sends 8 input words each 50 Hz tick, and the
server sends the server tick followed by the `RaceFrame` bytes. The client
restores each frame into its own copy of the same race (`rw_apply_frame`),
which it built from the same rules (`wasm/web_rules.c`, compiled into both
modules). It plays frames back four ticks behind the newest one to hide
network jitter.

## Checks

```sh
npm run typecheck
npm run test:server -- "<disc>"   # accounts, rooms, rules, a full race to results
npm run test:e2e -- "<disc>"      # two headless browsers race each other, then practice
node scripts/grid-test.mjs "<disc>" # 8-player grid on every course variant
node scripts/finish-check.mjs "<disc>" # finish fade, spectator hand-over, finish deadline
node scripts/smoke.mjs "<disc>"   # the simulation module on its own
node scripts/audio-check.mjs "<disc.cue>"  # engine sound follows rpm; race tunes found
# Native vs WebAssembly physics, tick by tick:
cmake -S . -B ../build/web-parity -DCMAKE_BUILD_TYPE=Release && cmake --build ../build/web-parity
../build/web-parity/rage-web-parity "<disc>" 2000 > native.txt
node scripts/parity.mjs "<disc>" 2000 > wasm.txt && cmp native.txt wasm.txt
```

## Not done yet

- **Latency on your own car:** there is no client-side prediction, so your
  own car responds after a round trip plus the 80 ms playback buffer.
- **Reconnecting:** you cannot rejoin a race you disconnected from.
- **Sound gaps:** no SPU reverb (the retail tunnels' echo), no announcer
  lines about the rivals, no fly-by/shuttle scenery sounds, and no results
  music after the finish.
