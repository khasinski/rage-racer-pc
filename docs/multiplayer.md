# Multiplayer

Status: multiplayer is still a plan. A caller-owned human/AI race context is
implemented and tested without opening a window. There is no server yet.
CPU car presentation also writes to a caller-owned `RenderWorld`, without
controller, game-scene or renderer callbacks. GPU resource registration and
playable client adoption remain unfinished.
The branch `poc/multiplayer` only
shares a course, class and car between two processes. The cars do not see
each other, and that branch is not the start of this plan.

## Decision

The dedicated server is a second binary, `rage-racer-server`, written in
Rust. The game the player launches stays the C port. Rust is the server
language because the race has to be stepped by the simulation that already
exists in C, and Rust links that C into one file without a second runtime.

One process hosts many rooms. Each room is its own race in memory, with its
own entrants and its own 50 Hz clock. SQLite, compiled into the binary, stores
rooms and results. It does not store car positions.

The server is the referee. Clients send driver inputs. The server applies
those inputs to human cars, steps the retail simulation, and sends back
positions. Finish order and collisions are the server's numbers. A client
shows its own car immediately and draws the other cars from the snapshots it
receives.

## What this is not

- Not a second copy of the game that happens to pick the same menu entries.
  Two local races that loaded the same course are not one race.
- Not a JavaScript or browser port. Moving the game to another language does
  not make the cars share a world.
- Not lockstep of two full clients. The retail step is integer and fixed-rate,
  which is useful inside the server, but the clients are not trusted to agree
  with each other.
- Not accounts, matchmaking, voice, spectators, or rollback. A driver types a
  name. A room has a code.

The client, its tools and its tests stay C. The server is the exception: a
separate Rust build in this repository. It is not required to launch a
single-player game.

## Binaries

| Binary | Role |
| --- | --- |
| `Rage Racer` | The game. Single-player as today. Multiplayer is a menu entry that connects out. |
| `rage-racer-server` | No window and no GPU. Listens on one TCP port. Owns the rooms and the races. |

The operator runs the server with a legal disc image once, the same way a
player does, so the process can import tracks and cars. After that import it
runs from the generated data. Players do not upload track data when they join.

Suggested layout when this is implemented:

```
server/                 Rust package, builds rage-racer-server
server/src/             lobby, rooms, protocol, SQLite
<sim static library>    C race step with a small API the server links in
```

The Rust build consumes a static library produced by the existing CMake
build. It does not reimplement the car.

## Server data cache

Use an extracted `RAGE.BIN` as the server's persistent source archive, the same
format the existing client export writes and `LoadRaceArchive` reads. Do not
introduce a second serialized physics format. Each room copies its selected
track physics and specifications from this archive; the archive is shared
immutable input, not shared race state.

On first start, a configured CUE or Track 01 BIN is read by `LoadRaceDisc` and
validated before committing the cache. Write to a temporary file beside the
destination, close it successfully, then replace the cache atomically. A failed
import must preserve an existing cache and remove only its own temporary file.
Later starts validate and load the cache without requiring the disc again.
A missing or invalid cache triggers import when a source image is configured;
otherwise report that the operator must select the source image. Never start
rooms with partial data or silently substitute another region.

Cache metadata must record the identified disc revision and the import/schema
version, so a cached archive cannot hide a region or format mismatch. Retain the
raw archive separately from room car overrides: changing room configuration
must not rewrite the imported retail source. The server sends the agreed car
configuration to participants before prediction begins.

The archive loader and validation are implemented and tested. Atomic cache
writing, metadata and first-start orchestration belong to the server startup
implementation; they are not implemented yet.

## Lobby

A room is a name, a code, a course, a class, a seat count, and a state:
open, racing, or finished. The player who creates it picks the course, the
class and how many seats. Everyone else picks a car and marks ready. The
server, not the creating player, decides when the countdown starts and tells
every member to load that course.

Empty seats stay computer cars. Human seats use the player-car physics, not
the rival AI. The retail grid is one human car plus eleven rival slots.
Turning several of those slots into human cars, all stepped by the player-car
update, is the main change in the C simulation. The first racing milestone
can cap a room at two humans and leave the rest of the grid to the AI.

SQLite tables, one file beside the server:

- `room` — code, name, course, class, seats, state, created time
- `entrant` — room, name, car, seat, ready, connected
- `result` — room, name, place, time, finished or retired

Restarting the server may restore open rooms from these rows. It never
restores a half-finished race. Frame state lives only in memory and dies
with the room. There is no password table.

## Menu

Multiplayer is a new screen in the C game, drawn with the port's existing
8x8 font, the same way the force-feedback row was added. It does not need
new art from the disc.

The screen remembers the last server address. From there the player sees the
room list, joins with a code, or creates a room. Creating a room is the
course, the class and the seat count. Inside a room each player picks a car
and sets ready. When the server starts the race, the client loads that
course and stops listening to local menu choices for the session.

## Race step

The server advances each racing room's clock at 50 Hz. Current PAL physics
runs every second tick (25 Hz); inputs retain their latest levels and pending
gear edges until that physics step consumes them. A snapshot carries each
car's position, heading, speed, and the race clock. Region normalization must
be verified before sharing a room between clients from different regions.

The client's own car is predicted from the local inputs so the wheel does not
wait on the network. Cars driven by other people are drawn between the two
newest snapshots. When the server's version of the local car disagrees, the
client corrects toward the server. The server's collision and the server's
finish order are the ones that count.

Wire messages, on the one TCP connection, are small and binary. The set:

- hello with the driver's name
- list rooms, create room, join by code, leave
- pick car, ready
- start, carrying the course and class the server chose
- input for one step
- snapshot of the field
- result when the room finishes

A client that falls behind skips to the newest snapshot. The server does not
keep a history for them.

## Simulation API

Current preparation:

- `rage-sim` contains explicit per-car input, engine, motion, track contact,
  collision response, lap crossing and world integration. Its core functions
  do not read controller, renderer, audio or active-race globals.
- `InitDriver` is shared with production car initialization; `DriverInput` and
  lap crossing are also used by the client. The client now uses `FinishDriver`
  for body kick, pose, jump/landing, tilt, crest events and contact response.
  Landing duration and skid angle are returned to client-only sound handlers;
  they do not repeat physics. Client drivetrain and motion now use the same
  `StepCarDrivetrain` / `StepCarMotion` as the server, with tyre voices presented
  between them. A real client/server comparison checks complete car state and
  RNG across all four motion states, countdown/start, stopped-speed and
  wheelspin thresholds. `AdvanceDriver` now also shares control feedback,
  world integration, track progress/contact and shift pose with the client.
  Client-only adapters read the device and present sound. Single-player still
  retains its original player/AI field collision ordering; `RaceSim` uses the
  documented simultaneous-movement policy below.
- Engine sound/display jitter uses a local frame-mixed RNG rather than
  advancing physics RNG. Its client test uses the real generator and checks
  unchanged car and physics seed across idle, rev-limit and sound-bank cases.
  Engine presentation accepts a const car, explicit specification and
  finished flag instead of reading the active race phase;
  its narrow test defines neither `g_CarSpec` nor `g_RacePhase`. The unused
  write-only `g_EngineRpmSnapshot` duplicate is removed; displayed RPM and
  jitter retain their initialization/presentation regression coverage. Player
  initialization now also clears the tachometer shift light; its regression
  starts from a lit previous-race state and verifies the full reset. A predicted local seat can
  therefore supply its own specification without installing global car data.
  Idle jitter uses canonical `SinAngle` instead of renderer/PsyZ trig. Its
  test runs headless using the real table, including positive intermediate
  phase and negative-phase clamping, without forced compatibility headers.
- `ApplyCarSpec` applies an explicit catalog entry to a caller-owned retail
  specification without reading the client catalog. The client uses it too.
  Parsing/validation also run without client globals. `race_sim` parses TOML,
  applies its automatic shift points and scale, then checks real seat setup and
  shifting while retaining unspecified retail fields. Model/grade selection is
  shared and rejects grades belonging to the next model. `LoadCarCatalog` also reads files into caller-owned catalogs without client
  globals; integer parsing rejects `strtol` overflow even on platforms with
  32-bit `long` (Windows). Narrow tests cover scalar/array overflow and exact
  signed 32-bit boundaries. Failed reads/parsing preserve the destination and embedded NUL
  bytes are rejected. The client uses this loader before installing its
  metadata. `FindCarEntry` selects an exact model/grade in sparse, unordered
  catalogs. The client uses the same lookup without maintaining a second
  variant-indexed copy. Manual-only availability remains room policy.
- Each `SimDriver` retains its last field `DriverStep`, contact flag and
  `stepTick`. Presentation can consume landing/skid/contact results once
  per physics tick without re-running physics or modifying the race.
  Intermediate clock ticks retain them; the next field step replaces them.
  A real landing test checks duration 19, retention and replacement with
  zero on the following physics step. This is local simulation feedback,
  not a network packet or a server sound player. Landing/contact sound
  handlers accept explicit audibility instead of reading `g_RacePhase`.
  `driver_cues` tests those handlers alone: landing/contact thresholds, muted
  playback, side/mirror selection, absent slip and unchanged car state. It
  does not initialize or step race physics.
- `RaceSim` owns human and AI cars, copied specifications/hulls, prepared human engines,
  inputs, per-seat RNG, wrong-way duration, countdown and lap/finish ticks.
  Route/event data is immutable and borrowed for the race's lifetime. The
  context has no pointers into itself, so copying it restores its state.
- Its C API initializes the context, adds human or authored AI seats, accepts
  input, starts and steps the race, and retires participants. Human input
  is rejected for AI seats. Results are stored per participant. A result
  is not the client's Grand Prix retirement policy: every human may finish.
- The context uses a 50 Hz clock and executes PAL retail physics every second
  tick (25 Hz). Running unscaled physics at 50 Hz would double game speed.
  Region normalization and client prediction integration are not verified yet.
  Lap results use exact ticks; the client's jittered display time is separate.
- During countdown the same drivetrain step updates pedals and engine RPM
  at 25 Hz with `started = false`; AI remains stationary. Human speed remains
  zero and position/collisions are not integrated. Inputs and gear edges are consumed on those steps too.
- Road contact, human/AI silhouette and AI/AI collision corners are separate
  copied data; the two collision paths use different retail hull scales. The client and headless use the same immutable retail hull tables;
  narrow tests check distinct geometry and atomic rejection of missing hulls.
- Contacts are detected after all drivers move, then unordered pairs are
  resolved once in stable seat order. Same-tick finishes use seat order too.
  This is deterministic; multi-car pileup tuning and finer finish tie-breaking
  are still to validate.

Evidence: `race_sim` tests two independent contexts with two human drivers,
real initialization, motion and collisions on different tracks; compares
interleaving with standalone execution; restores the whole context; and checks
clock cadence, pending gear edges, atomic rejection of invalid driver setup,
retirement and a finish-boundary fixture.
`driver_field`, `driver_init`, `driver_contact` and `race_lap` provide smaller
regression boundaries. `client_driver` compares real `UpdatePlayerCar` plus
its drivetrain adapter against `MoveDriver`/`FinishDriver` over 40 steps,
forward/reverse, digital/analog input, manual/automatic transmission and
countdown/active per-car rules (16 combinations), including pedals and shift
input. This compares per-car operations, not `RaceSim` countdown movement:
the room countdown deliberately updates only the drivetrain.
Complete car state and physics RNG agree each step. Only device input, sound
and the absent rival-contact adapter are stubbed; no host-state object, SDL
or PsyZ is linked. Field collision policy remains a separate boundary.
`driver_field` also exercises three simultaneously
overlapping human cars in sparse seats through the last grid slot, checks
that each car reports contact, and reproduces full car/RNG/step state over
20 restored steps while another room is interleaved. This verifies
deterministic pileup execution; it does not establish handling balance. `race_input` tests input acceptance without initializing
cars, tracks or stepping physics: pedal/steering/flag bounds, pending gear
edges, seat ownership and lifecycle rejection, with unchanged race state on
invalid commands. `race_lifecycle` tests start-once, retirement, preserved
results, completed-race immutability and clock overflow without loading a
track or initializing car physics. It also rejects missing/invalid routes
at driver setup before touching the seat; inactive cars cannot start a room
and become retired before countdown engine updates; route validation is shared with
race initialization. `race_setup` separately checks owned specification,
hull/road-corner and launch-threshold copies, unchanged source configuration,
seat occupancy rejection, malformed grid/manual/launch setup rejection and
borrowed immutable route storage without
stepping a race. Native input structs are not a wire format.
`triangle_area` compares 4096 cases with PsyZ and
checks that simulation clipping leaves all GTE registers unchanged; track
search and car collision grids now use that pure arithmetic. `hull_rotation`
compares all 4096 yaw angles with varied pitch/roll and signed hull coordinates
against PsyZ. Simulation hull rotation uses only its four necessary matrix
entries and no longer calls `RotMatrix` or `ApplyMatrix`. Existing motion,
drivetrain, initialization and finish
sweeps retain their expected results. These tests do not yet constitute a networked multiplayer race.

`ReadTrackRoute` validates a point/arc asset and builds an immutable view without
installing global track state. The client installer uses this same parser. Its
headless test checks independent buffers, every truncated size and invalid arc
references; the caller still owns the storage and its lifetime.

`ReadTrackEvents` now shares the existing structural event-table validation
with the production installer without changing client globals. Its headless
test covers truncation, invalid key indices/loops and independent buffers.
This validates the asset layout; imported storage ownership and semantic
consistency with the selected route/region are still to verify.

`TrackData` now owns copied point/arc and event blocks from a runtime pack,
loaded by `CopyTrackData` or `LoadTrackData` and released by `FreeTrackData`
after borrowing races finish. Models/textures are not retained. Pack-header
validation is shared with the production loader. A headless test verifies
file loading, source-buffer release, independent copies and malformed headers.
This layer reads extracted runtime packs. Disc import and car-data selection
are provided by `rage-data`, described below.

`ReadCarSpec` copies retail specifications from a structurally validated car
pack without installing globals or loading audio/textures. The production race
loader uses it before catalog overrides and engine preparation. Its headless
test checks truncation, independent copies and unchanged output on failure.
This is the parser boundary. Standalone car selection/import is implemented
by `ReadRaceCar` and `LoadRaceDisc` below; catalog application remains an
explicit operation on the copied specification.

`rage-data` provides archive selection separately from the race step.
`ReadRaceData` validates the complete little-endian RAGE.BIN index and every
nonempty byte range, leaving output unchanged on failure. Its view borrows
the source buffer. `ReadRaceCar` copies a variant specification (0..31);
`CopyRaceTrack` owns the selected class/course physics pack (0..5 / 0..3),
so both results survive releasing the archive. Selection uses the same asset
indices as the client, now declared in a renderer-independent header. A narrow
test covers every selector, truncation, invalid ranges, missing entries and
source lifetime. The same narrow test loads retail specifications and applies
two independent sparse room catalogs: absent fields retain retail values,
unselected variants stay untouched, and overrides never change the archive
or the other room. No race, menu or renderer is initialized. `LoadRaceDisc` opens a CUE or raw Track 01 BIN using the same
CUE resolver, raw-sector reader and ISO reader as the client. It does not
initialize audio, SDL or game state. `LoadRaceArchive` opens an extracted
RAGE.BIN. Disc loading also records the boot serial using the same isolated
SYSTEM.CNF/root-directory parser as the client, without scanning movie data.
Bare archives and unidentified discs have an empty serial; this must not be
treated as proof of a region. A boot serial alone does not identify every
disc revision. These loaders own their archive buffers; `FreeRaceData` releases
only their returned objects, while `ReadRaceData` views keep caller ownership.
Copied tracks and specifications survive releasing an owned archive. Synthetic
tests cover MODE1/MODE2 sectors, CUE track offsets, independent loads, truncated
sectors, corrupt archives and missing files.

`retail_data_tests <CUE or Track 01 BIN>` reads RAGE.BIN through the existing ISO
reader and validates every player specification and track physics pack. On the
local PAL disc, all 32 specifications and 24 tracks pass. It also runs two
human cars for ten simulated seconds after countdown on each track in both
directions, for every specification (1536 scenarios), checking movement, valid
segment indices and deterministic context restoration at every tick. These
use manual transmission: raw retail specifications include MT-only cars and
are tested before catalog overrides. A separate catalog run applies
overrides to copied retail specifications and checks automatic upshifts
for both humans across the same 1536 scenarios. Configure
`-DRAGE_SIM_DISC_BIN="/path/to/Track 01.bin"` to include this optional test in
CTest. The PAL test additionally completes one-lap races on all four course layouts,
and oval races of one through six laps, with two humans in
both directions. A test controller produces analog steering and pedal inputs
from a read-only car copy; it does not move cars or inject lap progress. The
test checks each lap time, both finish statuses, places/times and frozen state
for each driver after finishing while the other driver continues.
This verifies the four course layouts and every supported oval lap count;
full-race coverage of every class/car combination and region
equivalence remain to verify. No disc data is included in the repository.

`ConfigureRival` selects an authored AI configuration by logical model ID
and applies it using explicit track length/grid position. Both the client and
`AddRaceRival` use this selection; the headless core does not read active-race globals. Its narrow test
checks signed retail encoding, clamping, RPM halfword preservation and
independent configurations, model IDs differing from grid positions, every
table entry and invalid-ID fallback to entry zero. `AddRaceRival` installs this
authored AI into `RaceSim`; duplicate AI slots, inactive grid entries and missing setup are
rejected without modifying the context.
`SeedRivalSpeedKey` also takes an explicit authored table for one car. Its
headless test covers locally reversed keys, an early terminator, a full
48-entry table, independent cars and inactive/missing inputs. The client's
field loop uses this same helper. `StepRivalTargetSpeed` now interpolates
limits and steps sliding from an explicit table/direction. Its narrow test
covers slot-specific limits, off-pair marker movement, invalid marker recovery
and the retail lap reset that uses the old pair for one last frame. Existing
client AI and speed-unit regressions still pass. `StepRivalAcceleration`
now operates on one car with explicit race/attract rules; the client uses one
shared field loop. Its existing regression test runs headless with local cars
and real angle arithmetic, retaining boost/braking/overflow coverage and
adding state restoration, inactive cars and wraparound yaw. `StepRivalLine`
uses an explicit racing-line table; its headless test covers strict lateral
bounds, blocked/trailing cars, sentinel/end-of-table wrapping, invalid indices
and the one-frame lap-reset behavior. The client retains a shared adapter
for its global AI field. `MoveRival` now advances one explicit car without
renderer/GTE calls. Player and rival body offsets share the same pure fixed-
point transform. The existing 864-state movement checksum remains unchanged;
that regression now runs headless with local cars rather than the full host
state, rotation/render code and PsyZ. `FinishRival` now handles one car's
wheel phase, pose, jump/landing, body kick, crest event and collision drag
using explicit event data, track length and direction. A separate headless
test checks real physics, rounding, restoration and inactive cars; existing
client body-stage ordering coverage remains. `PlaceRival` applies knockback
and road pose from an explicit route/direction; the client's two-pass field
ordering is preserved. A headless test checks the real composed operations,
height, timer decay, restoration and rejection of missing route data.
`SteerRival` and `ClampRivalLine` also take explicit routes; steering uses the
same route-offset heading helper as the driver core. Their existing narrow
tests now run without SDL/PsyZ or active-track globals, preserving forward/
reverse lookahead, lateral bounds, airborne heading and word wrapping.
`InitRival` places and resets a rival from explicit route/start/walk/direction
and model data. The client retains its grid-selection adapter. A headless
test checks real placement, forward/reverse heading, independent cars,
inactive grid entries and unchanged state on invalid initialization.
`AdvanceRival` composes authored speed/line, steering, acceleration, movement,
progress and road placement before field collisions; `FinishRival` follows
those collisions. It accepts an optional immutable traffic field; it does not
supply GP rubber-banding.
The retail test exercises all 11 start slots against their same-index authored
configuration on every PAL pack in both directions (528 ten-second cases),
checking deterministic restoration, actual movement and unchanged inactive
slots. The default client grid is the identity mapping; a course-specific
permutation selects drawable models in the renderer, without changing the
logical model ID used for AI configuration.

`AvoidRivalTraffic` reads an explicit field of human and AI cars and changes
only the avoiding rival. The client retains its scene/field adapter. Existing
traffic characterization passes unchanged; a narrow headless test covers two
humans, shoulder blocking, inactive/missing entries and state restoration.
`RaceSim` snapshots the field before movement so all AI scan the same positions
and speeds regardless of movement order.

`FindRivalContact` reads an explicit AI pair, hulls and track length without
GTE state; `ApplyRivalCollision` applies the detected response. The client
retains its first-hit field policy. Existing collision characterization and
a narrow headless test cover response restoration, inactive/missing cars and
read-only detection. `StepDriverField` now resolves human/human, human/AI and
AI/AI pairs with their respective retail responses after every car moves.
Human/AI uses the existing human collision detector/response; AI/AI retains
its distinct hull scale and knockback rules.

`race_mixed` checks a sparse field with two humans and two AI, human input
rejection for AI, atomic setup rejection, AI remaining still in countdown,
collisions, independent/restored contexts, retirement and an AI finish boundary.
The PAL retail test also steps the maximum 12-human grid on every pack
in both directions (48 ten-second scenarios), including authored positions
whose AI active flag is disabled. Every driver must move, retain a valid
track segment, and reproduce the complete context from a copy at each tick.
This passes macOS Release and ASan/UBSan; it validates simulation capacity,
not lobby seat policy or the visual spacing of those starts. The same retail
race runner handles two-human, mixed and full-human fields, including
automatic/catalog runs; it checks movement and active status for every human
in short runs instead of maintaining a separate full-grid implementation.

The PAL retail test additionally runs two humans plus the remaining active AI
seats on all 24 packs in both directions (48 ten-second scenarios), skipping
inactive authored grid entries. It completes 18 mixed races: both directions
on all four layouts and one through six oval laps. It checks every participant's
lap ticks, unique finish places, chronological finish order and frozen state
after finishing, with normal motion and no injected lap progress. These use
the default logical model IDs and the same configuration selection as the
client. AI/AI uses the shared retail `g_CarCollisionCorners`, separately from
`g_OpponentHullCorners` used by human contact queries. A narrow collision test
checks a separation where the former hull hits and the latter does not. Human
seats may use a position whose authored AI flag is inactive; that flag controls
AI presence, not whether a human can be placed there. The final-class PAL
reverse grid exercises this case.

The retail test optionally accepts one pack index (88..134, even) after the
BIN path (or `all`) to isolate a failed case without opening the game.

Remaining preparation:

- Compare PAL and NTSC specifications, route/event data and clock behavior
  using actual regional discs; select an explicit shared-room physics policy.
- Connect the multiplayer client race path to `RaceSim`, so prediction uses
  the same field/contact policy as the server. Shared per-car operations are
  already used by single-player; its historical field ordering is retained.
- Validate the headless boundary on Linux/Windows. Local macOS Release and
  sanitizer results do not prove platform equivalence. The current environment
  rejects SSH to `darwine` with `Operation not permitted`.

The race player-model draw now accepts rival-model selection and steering
explicitly. It no longer chooses the custom model or reads `g_PlayerCar` to
steer another drawable. Camera/intro callers supply the subject's steering,
with legacy local camera calls retaining their local-player selection. Render
tests cover opposite steering for separate draws, restoration of the supplied
object, and the native-player path. Asset-bank ownership and drawing the full
multiplayer field still require integration.

Modern player mesh cache selection and model relocation now find the slot
owning the actual model asset through `FindCarModelSlot`, rather than assuming
the separately selected slot owns that geometry. The shared lookup also serves
serialized-asset resolution. Model-bank tests cover distinct native wrappers
for both installed slots, even when they share one serialized source, plus
unknown and null assets. This removes an identity mismatch; the existing
two-slot loader is still not a multiplayer field asset owner.
The live native importer also selects a resident model bank by the requested
asset key, using `FindCarAssetSlot`, instead of the global selected slot. It
rejects invalid keys and models absent from installed slots rather than caching
another car's geometry. Slot tests cover both distinct asset indices, missing
indices and stale indices on empty slots. The client and tools build, but the
native-world GPU test cannot run in the current environment: SDL reports no
displays and cannot create its GPU device. This is not visual verification.
Modern player submission now receives the same explicit model asset captured
by the classic draw, using it for both cache identity and wheel/horizon offsets.
The draw test checks that this asset crosses the renderer boundary. Entity,
allocation remains local-player-specific; this API change alone
does not enable drawing a remote human car.
Lamp publication no longer looks up physics cars in `g_PlayerCar`/`g_Cars`.
Each car submission copies its braking and track-zone daylight into per-frame
presentation storage keyed by entity, cleared at frame start. End-of-frame
lamp updates use those submitted values while retaining the previous lamp
state. Both near and far traffic submissions capture the inputs, as does the
local player's lamps-only path. Entity allocation and independent paint still
need integration before remote humans can use this path.
Native car submission now accepts an optional explicit paint entry. The
assembly and per-part submission no longer read the global car table or local
selection to recolor each part. Traffic supplies no custom paint; legacy local
draw/lamps callers resolve their entry once. Draw tests check the forwarded
entry, the last paintable model, nonpaintable/negative selections and an absent
table. Paint-color validity is still checked before copying colors into each
render instance. Native human-car submission also receives an explicit entity
ID, bounds-checks it and uses it for mesh parts, transform history and lamp
inputs. Legacy local callers keep their reserved local ID. A renderer-only
test crosses two human cars using the same model while reversing submission
order, checking independent positions, paint and brake/headlight state. This
does not initialize the game or require a GPU. Mapping room seats to those
IDs and loading the field's assets still require client integration.
`CarShape` is a caller-owned copy of the model's four placement halfwords,
read by `ReadCarShape` or selected from the first car pack by
`ReadRaceCarShape`. It does not borrow a model slot or carry geometry/image
pointers. The parser reads the placement prefix only; geometry and image
validation remain separate importer responsibilities. Narrow tests cover
unaligned input, prefix truncation, unchanged output on failure, all 32
archive selectors and values surviving release of an owned archive. These
tests pass Release and ASan/UBSan without a renderer. `GameRenderWorldSubmitHumanCar`
now consumes this placement copy, an explicit variant, entity and paint entry;
it does not consult installed model slots or local car selection. Existing
local drawing uses a legacy adapter that resolves its installed variant and
copies placement metadata before calling the same human path. A PAL import run
reads placement for all 32 variants and passes the isolated pack-88 race sweep.
This removes submission's slot dependency, not the live importer's requirement
that uncached geometry has an installed source bank. Field asset preparation
and complete human-submission regression coverage remain to implement.
The shared near-car assembly now uses `BuildCarParts`: explicit car pose,
placement and steering produce body/rear/front-wheel transforms without
globals, model banks or view matrices. Existing scene math is shared rather
than duplicated. The narrow `car_parts` test exercises the production assembly
for two independent cars, wheel sides/steering, road banking, roll velocity,
ground clamping, unchanged input/output on rejection and extreme integer
angles/heights. Wide intermediates avoid signed overflow in assembly offsets.
It passes in the client Release build and the headless sanitizer build; the
headless executable links only the system C library and sanitizer runtime.
`SubmitRaceView` is the client-side field adapter: it submits explicit
`RaceSim` seats and an owned `RaceView` containing model sources and per-seat
`RaceCarLook` variant/paint configuration.
Human and rival submissions receive stable seat IDs; the rival path no longer
requires a pointer into `g_Cars` to establish identity. Finished cars remain
visible; empty, retired and inactive seats are skipped. Invalid visible model
selection rejects the whole field before any draw. Its narrow test uses
recording renderer endpoints, not mock race physics, and covers sparse mixed
seats, paint isolation, unchanged race/look inputs, atomic rejection and all
12 human seats. It passes Release and ASan/UBSan. This adapter is built into
the client but not connected to a gameplay mode yet; course/native assets,
camera/HUD and the race lifecycle still need that integration.
`LoadRaceView` replaces the placement-only loader. It copies configuration and
owns each required human model/image once per variant, so identical models
share geometry while paint remains per-seat. Failure destroys only the new
candidate; an existing view remains usable. `FreeRaceView` releases the models
after all import/draw borrowers finish. The view test uses the real archive
parser and verifies separate views, missing late-seat models, 12 humans sharing
one source and independent paint. It frees source storage and clears input
configuration before submitting the prepared field. Native GPU resource
preparation and course/AI asset ownership remain separate responsibilities.
`ReadModelBank` now validates and resolves geometry into a caller-owned
`NativeModelBank` view without installing global renderer state. Existing
`RegisterModelBank` uses this same parser; failed reads preserve the previous
bank, and smaller replacement banks clear unused model pointers. Model-bank
types and payload/primitive validation are shared with the legacy installers,
not duplicated. `model_bank_parse` runs with no renderer globals or GPU and
covers independent source buffers, truncation, bad offsets/counts/primitive
batches, atomic rejection and replacing a larger bank. It passes Release and
ASan/UBSan. The view borrows immutable source bytes; this is the parsing
boundary, not yet a field geometry/image owner.
`CopyCarModelData`/`CopyRaceCarModel` now allocate an independent immutable
model/image source. Placement and resolved bank pointers refer to its copied
buffer; the car image remains available without a VRAM upload or active slot.
Release it with `FreeCarModelData` after all borrowers finish. Header ranges,
model streams and the complete image size are validated before publication.
The narrow `car_model_data` test covers unaligned source bytes, independent
copies, archive selection, every truncated size, malformed offsets/counts and
source release. It passes Release and ASan/UBSan. An actual PAL import validates
all 32 owned models and the pack-88 race sweep still passes. Feeding the owned
field images/banks to native import remains to implement.
The material decoder now reads an explicit bounded `TextureImage` rectangle
instead of assuming every source is a complete VRAM plane. Existing live/sky
conversion supplies its full snapshot; an owned car can supply just its
64x256-word source rectangle. `native_texture` checks 4/8/16-bit sampling,
palette lookup, independent sources, cropped/full image equivalence and
out-of-range/truncated buffers without SDL or GPU. It passes Release and
ASan/UBSan.
`NativeAssetImporterPrepareCar` now builds a mesh from an explicit owned model
bank and copies the canonical source into its session cache. Material decoding
uses that source's image rectangle, bypassing live slots, VRAM reads and track
texture generations. Repeated preparation accepts identical source bytes;
conflicting owned sources are rejected rather than replaced
while published frames may still reference them. `NativeAssetImporterPrepareRaceView`
prepares all owned human sources and rolls back only entries added by the call
on failure. It must run during asset preparation before field submission.
The direct CPU preparation/rollback regression is described below. Legacy and owned sources now coexist as described below; whole-field visual
verification remains outstanding.

Mesh sizing, allocation, writing and adoption now run through
`ImportBuildMeshEntry`, independent of the live importer cache. Both importer
source paths use this builder. Output is published only after both passes and
mesh validation succeed. The existing `native_mesh_writer` regression now
runs headless too, checking first/second-pass failure leaves output unchanged,
actual encoded geometry, and output surviving source destruction. It passes
Release and ASan/UBSan. The model-bank stream converter is shared with this
headless test: all four primitive formats exercise real position/normal/color
and UV/palette decoding. Missing model streams, excessive bank counts and
rebuilding an occupied output are rejected without replacing its resources.
The `car_model_data` regression also selects a nonempty model from a synthetic
archive, frees archive storage before converting its owned bank, then frees
the owned model before checking the produced mesh. This exercises the real
copy/parser/converter boundary without game state or GPU. It passes Release
and ASan/UBSan. `ImportPrepareCars` now takes explicit cache storage/count/capacity and a
variant-indexed source array. The live importer uses it for both individual
and complete-field preparation. The same narrow regression checks canonical
source reuse, late empty-model failure, capacity exhaustion after an addition,
conflicting existing keys, rollback preserving earlier resources, and cached
models/meshes surviving release of every input source. `ImportReleaseEntry`
shares cleanup between rollback and importer shutdown. These checks pass
Release and ASan/UBSan without SDL, GPU or game globals.
Model-bank validation now bounds indexed vertex and normal reads by the
remaining source bytes; malformed streams cannot reference beyond that
buffer even when their batch lengths are valid. Tests check every corner at
the last valid and first invalid index, preserving output on rejection.
This is a memory-safety bound, not proof that logical sections never overlap.
All 32 owned PAL models and the pack-88 simulation sweep still pass with
ASan/UBSan after the stricter checks.

`retail_model_tests <CUE or Track 01 BIN>` independently converts every
retail human model into one owned cache, without running race physics or
creating a GPU. It frees the archive before preparation and all input models
before checking every generated vertex/index and retained source. All 32 PAL
models pass Release and ASan/UBSan. Configuring `RAGE_SIM_DISC_BIN` registers
this as the optional functional test `retail_model`, separately from the race
runner. This verifies import and ownership, not visual appearance or GPU
material publication.

Camera positioning is now separate from renderer publication and local-car
drawing: `UpdateCameraPose` and `UpdateLookBehindPose` update only the supplied
`Camera`, reading a const subject car. Existing single-player `UpdateCamera`
and look-behind adapters still install the view and draw as before. The camera
regression interleaves two subjects, compares isolated execution, checks input
and renderer state remain unchanged, and observes no draw/view-publication
calls. Car/chase pose updates now skip track-camera lookup entirely and retain
track-node state until an authored-camera mode actually needs it. Their
regression runs with the track camera table absent, compares interleaved
subjects and checks no nearest-node lookup or renderer publication occurs.
Null camera/subject input leaves pose state unchanged. The complete client
build and five camera/intro/replay regressions pass after this change.
Authored track camera tables remain shared input; this is not yet a complete
multiplayer camera/HUD adapter or an independent headless camera subsystem.

`RacePosition` reads a selected seat's HUD rank without modifying race/car
state or initializing physics. Finished seats retain their recorded place;
unfinished drivers follow them, ordered by wide signed progress and stable
seat order on ties. Empty/retired/inactive driving seats return zero. A narrow
headless test covers the full human/AI grid, sparse lifecycle states, ties,
finished deactivated cars and overflowing 32-bit progress sums.
`DrawRacePosition` now takes that number explicitly; single-player supplies
its existing position at the call site. Its rendering regression supplies
12 while the global player remains third and checks the two-digit result.
`RaceView` now retains finished cars even when their physics `activeFlag` is
-1, as real race completion sets it. Its regression uses that actual state;
previous fixtures incorrectly left finished cars active. Full multiplayer
HUD timing/speed/transmission and client lifecycle integration remain open.

Explicit field submission now begins with `GameRenderWorldBeginCarField`,
after validating every visible seat. It clears earlier main-view human/AI
car submissions and lamp inputs, and marks the building `RenderWorld` as
having an explicit car source. The single-player end-of-frame publisher skips
its global `g_Cars` replacement for that frame. `RenderWorldBeginFrame` resets
this ownership marker; it cannot leak into a later single-player frame.
The CPU renderer regression checks stable preservation of scenery, non-seat
models and mirror passes, repeated field replacement and reset on the next
frame. Invalid field configuration does not begin replacement. Release and
headless ASan/UBSan tests pass; whole-client visual publication remains to
verify after gameplay integration.
The native CPU libraries now receive sanitizer instrumentation in headless
builds too, including mesh-cache and render-world internals. Previously only
the simulation/data libraries and their callers were instrumented there.
All 55 headless tests and the 32-model PAL import pass after this correction.

Tachometer rendering now receives an explicit const `CarTachometerSpec`,
gear and speed alongside RPM/lighting. Its draw test defines neither
`g_CarSpec` nor `g_PlayerCar`, switches between two dial specifications,
and retains regional speed/lighting/overflow coverage. The needle's four
local points are derived directly from the specification rather than kept in
`g_TachoNeedleQuad`; that global and its initialization writes are removed.
`BuildTachometerFace` names the remaining packet initialization and also
receives an explicit specification. The single-player adapters resolve their
local inputs at the call site; engine-display RPM and lighting state remain
client-side. This removes hidden selected-car data from drawing, not the
remaining shared texture/presentation setup or a complete multiplayer HUD.

The lap column now receives a borrowed millisecond table, visible/highlighted
rows, lap count, best time and layout mode explicitly. It no longer reads
`g_PlayerCar`, `g_LapCount` or `RaceHasRivals`. Existing race call sites pass
their local values without copying a car or allocating presentation state.
Its renderer regression supplies a different table/count/mode than the globals
and verifies times, colors and layout, preserving input state.
`RaceTime`/`RaceLapTime` provide read-only HUD milliseconds from the simulation's
50 Hz ticks. Finished elapsed time comes from the seat's frozen finish tick;
completed lap durations and the current lap are separate. Missing/retired or
unstarted laps return -1; large durations saturate and reversed tick ranges
are rejected before subtraction. A narrow test needs no track, initialization
or physics step. These accessors are ready for client HUD integration; the
multiplayer client does not yet call them in a running mode.

Owned human models now have explicit `RENDER_ASSET_OWNED` source identity on
the submitted mesh instance, derived from the explicit-field publication.
Modern mesh/material providers use the prepared importer source even when
the session otherwise uses a disk cache. A missing owned source is an error;
it cannot silently resolve the same key from disk. Legacy and owned importer
entries, authored replacements, prepared materials and GPU texture keys are
kept separate and resident, without evicting pointers used by earlier frames.
Source identity propagates through draw spans and texture reconstruction;
interpolation does not match cars across source changes. Owned materials do
not apply the global local-player team-marking overlay to remote cars. Per-seat
custom artwork remains unimplemented. Tests cover legacy/owned key coexistence,
material/texture-cache separation, draw-span propagation and missing-owned
lookup refusing disk fallback. GPU visual verification and a positive whole
client owned-source render still require gameplay integration.

Image-asset validation is now in `rage-data`, separate from VRAM uploads.
The client uploader calls that same validator; the headless `image_asset`
test covers every truncated size, invalid entry lengths, missing pixel/CLUT
blocks, destination bounds and unchanged input without game-state mocks.
The existing client upload regression still checks real upload calls.
Validation accepts raw byte buffers and copies only headers with `memcpy`,
so archive slices need not be word-aligned. The headless regression checks
all four alignments and truncated palette/pixel chains. All 57 headless
checks pass with ASan/UBSan after this change; the three client image/car/race
asset-loading regressions also pass. All 32 actual PAL `CAR_2ND` image chains
validate with the same parser under sanitizers.

Owned car material sources now include the common wheel/team-name texture
rectangle from boot asset 5 and its separate default-logo palette. `CopyRaceCarModel`
requires that validated source and copies it into each owned model; missing or
malformed shared imagery rejects the load rather than reading live VRAM.
The importer retains those bytes when copying models into its cache, and
canonical-key checks include shared imagery as well as geometry bytes.
The decoder borrows a small chain of independent bounded rectangles; it does
not allocate or upload a full VRAM plane. Invalid secondary sources and cycles
are rejected before writing output. Owned sources still avoid the local-player
custom-marking overlay; per-seat custom artwork is not implemented yet.

`CopyImageAssetPixels` reads a caller-selected rectangle from a validated image
chain without GPU calls or globals. Narrow tests cover clipping, palette/pixel
order, every source alignment and unchanged output on late invalid input.
Model/cache tests verify missing shared sources, atomic rejection of malformed
image updates, reuse conflicts and retained wheel/palette bytes after freeing
all input storage. `retail_model_tests` now decodes every material of all 32
PAL models after freeing both the archive and input models, failing if any
material has no opaque pixels. This passes with ASan/UBSan. It establishes
source completeness for those materials, not correct UV appearance or GPU
rendering; a positive whole-client field render remains to verify.

Engine RPM smoothing, display jitter, shift-light blinking and powered-bank
selection now update a caller-owned `EngineSound` through `StepEngineSound`.
The function reads explicit const drive/specification and frame/seed values;
it has no audio calls or global state. The existing single-player adapter
publishes its result to legacy HUD values and audio endpoints. The new narrow
`engine_sound` test compares two interleaved states against isolated execution,
checks read-only inputs, clutch smoothing and atomic rejection of missing data,
without audio mocks. Existing adapter characterization still passes. This
prepares independent per-car presentation; gameplay adoption remains open.

`DrawSimHud` now adapts one explicit human seat and its `EngineSound` to the
existing HUD draw functions: authoritative position, per-lap milliseconds,
fastest completed lap, selected dial/gear/speed/RPM and wrong-way warning.
It reads no selected-player globals and does not step or mutate physics.
Finished seats remain drawable after their active flag is cleared; absent,
retired, inactive or AI seats and invalid lap ranges draw nothing. It uses
the shared warning threshold rather than narrowing the simulation's counter
to the client's legacy 16-bit timer. There is no Grand Prix time-limit or
retirement policy in this adapter. `sim_hud` uses real race accessors and
recording draw endpoints only, checking sparse seat 11, active versus finished
lap times, completed-lap best selection, input immutability and rejection.
All 59 headless sanitizer tests and six client HUD/rules regressions pass.
The adapter is built into the client but not yet called by a gameplay mode;
HUD texture/packet initialization and course asset lifecycle still need wiring.

Tachometer drawing now also receives the seat's AT/MT flag. Gear and speed
colors are derived at draw time, eliminating `g_HudGlyphClut` and its reset
writes. `DrawSpeedDigits` receives absolute position and color; it no longer
reads `g_CarSpec`, so a second seat's speed cannot use the local player's dial
offsets. The tachometer regression switches distinct dial offsets and AT/MT
colors, and the seat HUD checks transmission forwarding. The speed-digit
regression keeps its assertions enabled in Release. Full client compilation,
five affected client regressions and all 59 headless sanitizer tests pass.

Tachometer face initialization now adjusts a local sprite-description copy
rather than changing `g_TachoNeedleSprite` coordinates. The packet regression
builds two distinct dials and then restores the first while checking the
shared template never changes; it no longer defines `g_CarSpec`. The full
client build and six tachometer/HUD/initialization/content tests pass. Frame
packet storage is still owned by the renderer, not by each simulation seat.

`InitRaceGrid` now constructs an explicit human/AI field from a borrowed
`TrackData`, archive and seat manifest. Field seat, authored start position and
AI behavior slot are distinct choices. Human specifications start from the
selected retail variant, then apply a validated optional room catalog; private
engine/hull state and RNG belong to the result. Empty seats stay empty;
inactive requested AI, duplicate start positions/AI behavior slots and missing
car data are rejected. Failure on any entrant preserves the entire previous
race. Track/event storage still must outlive the race; the archive and catalog
need not. This is initialization for both future client/server callers, not
network serialization or a running client gameplay mode.

`race_grid` tests that constructor alone, including late failure, sparse humans
and AI, independent slot identities, invalid transmission, empty fields,
catalog defaults/overrides and source release. `retail_data_tests <BIN> grid`
is a fast isolated functional mode: all 24 PAL packs, both directions, full
12-human and mixed fields with authored inactive AI omitted, using real
initialization and 50 simulation ticks. It passes ASan/UBSan; the existing
pack-88 complete-race sweep also still passes. Configuring `RAGE_SIM_DISC_BIN`
registers this narrower `retail_grid` test alongside the longer retail sweep.

`ReadCarControls` now samples calibrated device inputs without consulting the
single-player race phase or auto-steer policy. Pedals and analog steering are
bounded for `SetRaceInput`; shifting retains sampled button edges. The legacy
`ReadDriverInput` and body-roll/pedal adapters still apply their existing
single-player policy through the same sampling code. Input-device state remains
a shared singleton, as intended; selected race/car state is not installed.
The existing `player_input` characterization now also runs headless without
SDL/PsyZ. Added checks compare sampling across legacy scene phases and submit
normal and extreme NeGcon controls to the real selected-seat input API.
All 61 headless sanitizer tests pass; the client input characterization passes.
This establishes the device-to-seat boundary, not input timing/network transport.
The bounded path now clamps raw NeGcon pressure before scaling/narrowing;
otherwise a positive out-of-range sample of 20000 wrapped negative and became
zero throttle. A regression fails on that previous behavior and checks negative,
half, full and excessive pressure samples. Legacy sampling retains its existing
signed-wrap characterization. Input equality checks compare fields rather than
struct padding, avoiding a platform-dependent test assumption. All 61 headless
sanitizer tests and the client input regression pass after the correction.

Grid initialization now retains each human's retail variant in `SimDriver`.
Standalone-spec initialization marks it unknown (-1); AI retains its separate
logical model identity. `LoadRaceView` and `SubmitRaceView` reject a look whose
variant differs from the simulation's known selection before replacing any
field draws. Tests cover mismatch during loading and submission with no draw
publication, plus preservation of selected metadata in grid construction.
The PAL grid sweep checks this identity on all 24 packs in both directions.
All 61 headless sanitizer checks, four client setup/view/HUD tests and the full
client build pass. This prevents configuration drift; it does not yet connect
the new race path to the main gameplay lifecycle.

`RaceView` now owns each human seat's engine presentation history.
`TickRaceView` advances it once per supplied simulation clock tick, independently
of how often the field/HUD is rendered. Repeated and older ticks leave the view
unchanged; invalid field/model identity is rejected before any presentation
state changes. Call it after each client simulation tick, not only when rendering
or receiving a sparse snapshot. This new path currently chooses a 50 Hz
presentation cadence; physics stays at 25 Hz, and the legacy single-player
presentation cadence is unchanged. Prediction rewind requires preparing a new
view; there is no hidden catch-up from missing physics history. The view test
checks two engines, repeat/rewind, rejected configuration and unchanged race/RNG.
All 61 headless sanitizer tests and three client view/HUD/engine tests pass.
The view's clock update is ready for the new lifecycle, not yet called by it.

`ClientRace` now owns the scene storage, a physics `TrackData` view, initialized `RaceSim` and
prepared human `RaceView` as one resource graph. `LoadClientRace` selects the
track, builds the explicit grid and loads its matching human models; failure
releases only the candidate. `FreeClientRace` releases view sources before
scene storage borrowed by the track view and simulation. Start/input/step use the existing
simulation API directly, without another set of lifecycle wrappers.
The caller's archive, setup and catalog need not outlive a successful load.
Course/AI render assets and GPU preparation are still separate responsibilities.

The scene-block parser validates the entire offset table before publishing
borrowed views; malformed input leaves the previous output untouched. Its
narrow headless test covers truncation, alignment, each invalid offset and
independent source buffers without a game loader. The legacy track installer
uses these views directly, removing its duplicate pointer/size arrays.
All 62 headless ASan/UBSan tests and four client parser/import regressions pass.
`SceneAsset` now owns copied runtime-pack storage and block views, and
`ClientRace` retains an independent copy selected from the archive. Narrow
ASan/UBSan tests cover independent copies surviving source release; the PAL
client constructor also checks separate scene storage after archive release.
This validates layout and lifetime, not typed render payloads. Texture packs,
GPU preparation and renderer adoption remain to implement.

`ReadTrackImages` now validates the five texture-pack blocks without uploads
or globals. Both legacy texture installers use this parser; failed validation
leaves its output unchanged. Headless image tests exercise late malformed
blocks, invalid offsets, truncation and unaligned source storage using real
image validators. `TrackImages` now owns a validated copy of this pack, rebasing its five
views into its own storage. `ClientRace` loads and frees it together with
scene/physics/model resources. Missing texture data rejects construction
without publishing state. Narrow sanitizer tests overwrite and release the
source, free one of two copies and validate the survivor. The PAL client
constructor checks independent texture bytes and validates every block after
archive release. Renderer integration and GPU preparation remain pending.

`ClientRace` no longer allocates or copies a second physics pack. Its embedded
`TrackData` borrows route/events from its owned scene blocks. `ReadTrackData`
validates both blocks before publishing the view; an invalid late event block
preserves the previous output. Standalone `CopyTrackData` still provides the
owned physics-only allocation used by server contexts. Narrow tests cover
borrowed identity and failed publication, and the PAL client constructor
checks that simulation pointers refer to the owned scene after archive release.

The scene block names now have one shared definition used by physics and
legacy runtime installation. `ClientRace` validates both primary/secondary
model banks and retains their views into owned scene storage, rather than
reading `g_ModelBanks`. The PAL constructor test loads every class/course
pair (24 packs), releases temporary races without changing the original and
revalidates the two surviving banks after releasing the archive. These views
are ready for explicit importer input; rendering still uses its legacy banks
until that adapter is connected.

`ClientRace` now builds and owns native meshes for both validated model banks
without installing them in global slots. `ImportBuildBankMesh` shares the
existing two-pass builder with car-cache preparation, removing duplicate
visitor adapters. The legacy importer also drops its never-used optional
car-bank parameter. PAL constructor coverage checks model counts and every
mesh index for all 24 packs, plus mesh independence after archive/other-race
release. GPU cache registration, course/terrain meshes and rendering adoption
remain to implement; these CPU imports are not a visually verified race.

Course model parsing now belongs to the headless model-bank module.
`ReadCourseBank` validates all models before publishing a caller-owned view,
and legacy registration uses that same parser. `ClientRace` retains a course
bank into its owned scene storage. Validation also bounds the authored vertex
count by remaining geometry bytes, closing a possible out-of-bounds import.
Narrow parser tests cover oversized counts, invalid face indices, truncated
streams, null inputs and unchanged output on failure. All 24 PAL course packs
pass this stricter preparation; constructor coverage revalidates the course
bank after archive release. `ClientRace` also builds and owns the native course mesh. Course stream
conversion moves from the global importer to the shared CPU mesh builder;
the legacy adapter delegates to that same implementation. Stream readers
and texture-window decoding now share one header. Narrow tests cover all
four course primitives, coordinates/colors, palettes/pages, UV windows,
emissive flags, occupied output rejection and independent output storage.
PAL coverage checks course mesh model counts and every index for all 24
packs, including survival after archive/other-race release. Terrain meshes,
material preparation and gameplay renderer integration remain pending.

`ClientRace` now owns both reconstructed texture banks in `TrackPixels`.
Primary/secondary image chains, the standalone car entry and active images
are applied in retail order to bank 1; bank 0 copies that result and applies
the deferred chain. Unauthored coordinates start at zero, never stale pixels
from another scene. `CopyImageEntryPixels` shares clipping/CLUT copying with
image-chain extraction and validates before changing output. Narrow tests
cover overlapping upload order, retained bank 1, common pixels, malformed
late data and copies surviving source mutation/other-copy release. The PAL
constructor prepares all 24 packs and checks independent bank storage. This
is texture-source preparation; RGBA material coverage, GPU registration and
visual equivalence are not yet verified.

Environment-script validation is now in the headless data module rather than
the global environment update file. `ClientRace` validates and retains the
script and five palette records inside its owned scene, so later material
selection can inspect authored modes without installing global state. The
separate `environment_data` test covers truncated cue tables, alignment,
invalid sky rows/lengths/times/modes, missing terminators and immutable input.
PAL constructor coverage validates all 24 scripts and checks their lifetime
after archive release. Global environment animation is unchanged; explicit
per-race animation and usage-aware material selection remain to implement.

Cue lookup, loop-time normalization and fade-frame selection now accept
explicit source/time values in the headless environment module. Legacy
`SeekEnvironmentScript` uses these same functions, eliminating its separate
last/previous-cue scans. Narrow tests cover cue boundaries, negative/wrapped
time, clamped fade duration, loop crossings and invalid frame arguments.
This shares seek semantics without device/render mocks; ticking colors, fog
and dynamic palette uploads still uses the legacy environment state.

Color-slot and 16-color palette interpolation now use caller-owned outputs
in the headless environment module. The legacy tick invokes these same
functions and only uploads the resulting palette in its client adapter.
Narrow tests cover independent color states, course-2 near-ground versus
other-course far-ground slots, preserved fog metadata, signed interpolation
rounding and invalid calls leaving output unchanged. This extracts the shared
calculation; the environment clock/mode/transition owner remains legacy state
until the per-race lifecycle is connected.

`Environment` now owns an individual clock, cue cursor, transitions, color
slots, dynamic CLUT and fog distance. Its borrowed cues/palettes are retained
by `ClientRace`, which initializes its own environment and freezes the final
class as the legacy mode does. `TickEnvironment`/`SeekEnvironment` perform no
GPU calls and read no game globals. Legacy animation delegates to this same
core through a temporary adapter packing/unpacking its existing fields;
those compatibility globals remain until scene consumers adopt the object.
`environment_state` checks exact fade rounding, seeking/wrap/fog behavior,
failed initialization preserving output and 1,000 interleaved ticks matching
two isolated runs. PAL constructor coverage prepares all 24 environments
and advances two independently after releasing their archive. The client
rendering path still needs to consume these owned colors and dynamic CLUTs.

The first PAL material audit found that course `.1ST` data alone omits shared
car/atlas pixels from boot asset 5. `RaceAsset` exposes bounded borrowed raw
assets, and client preparation now applies that explicit boot image chain
before course uploads. This restored 12 previously transparent variants in
pack 88. The audit decodes all 1,854 bank/palette combinations for that pack;
511 combinations remain transparent, but every material has at least one
visible variant. This is not evidence that every actually selected scene
variant is correct; usage-aware checks and other packs remain to verify.
`DecodeTrackMaterial` uses explicit banks with existing palette/page rules.
Narrow tests cover base atlas preservation, course/terrain page bits, bank
selection and rejected inputs preserving output. The optional `retail_materials`
functional test runs the costly audit separately from the fast constructor
coverage. No GPU or legacy VRAM fallback is involved.

Terrain validation now belongs to the headless data module. `ReadTerrainBank`
publishes explicit grid/visibility/vertex/cell views only after validating the
whole pack; legacy installation uses the same parser. `ClientRace` retains
these views into its owned scene. Terrain streams now validate vertex indices
against the available vertex storage, rather than only checking record lengths.
The separate `terrain_bank` test covers all four invalid corner indices,
truncation, alignment, bad grid references, null inputs, unchanged output and
independent source buffers. All 24 PAL packs pass stricter terrain preparation
and remain valid after archive release. `ClientRace` now also owns the converted native terrain mesh. Terrain stream
conversion moves into the CPU mesh builder; the legacy importer delegates
to the same implementation with its active cell table. The palette regression
now runs headlessly and checks all six modes, full mesh conversion, late
missing-cell failure, occupied output rejection and source independence.
PAL constructor coverage checks terrain model counts and every index for
all 24 packs and retains usable meshes after archive/other-race release.
All 64 headless sanitizer tests pass. Material preparation, GPU registration
and gameplay renderer integration remain pending.

`client_race_tests <BIN>` runs this production constructor on PAL data with
two independent forward/reverse races. It checks late model mismatch preserves
an earlier race, releases the archive/configuration, steps both for 100 ticks,
then frees one and continues the other. Renderer endpoints record unexpected
calls; construction/stepping produce none. The test passes ASan/UBSan and is
registered as optional functional coverage when `RAGE_SIM_DISC_BIN` is supplied.
The full client build, three grid/view/HUD regressions and all 61 headless unit
tests pass. This establishes resource ownership for client adoption; it is not
yet a selectable or visually verified gameplay mode.

These are separate from the Rust lobby, transport and cache startup work.
The cache policy above is decided; its startup implementation remains part of
the server work. There is no Rust server or networking implementation yet.
Only the PAL disc is currently available in the inspected local disc,
Downloads and matching-decomp directories; cross-region equivalence has not
been measured. Until that verification exists, a room must not assume that
PAL and NTSC client prediction use equivalent physics data or clocks.

The simulation now links without SDL or PsyZ. The client-only global RNG
implementation is excluded from `rage-sim`; its narrow test compiles that
implementation directly. An undefined-symbol audit of the static simulation
archive shows no `g_*` references. Its callers use explicit per-seat seeds. `SinAngle`/`CosAngle` use the
canonical retail table, with exhaustive comparison to the original functions
in the client build. The headless test binary links only the system C library
on macOS. Run the same narrow simulation tests without building the client:

```sh
cmake -S . -B build/headless -DRAGE_BUILD_PORT=OFF -DBUILD_TESTING=ON
cmake --build build/headless -j 6
ctest --test-dir build/headless --output-on-failure
```

The headless simulation also passes the same tests and imported PAL race
sweeps in an optimized Release build. It uses the same signed-wrap and alias
compatibility flags as the recovered client code:

```sh
cmake -S . -B build/sim-release -DRAGE_BUILD_PORT=OFF -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/sim-release -j 6
ctest --test-dir build/sim-release --output-on-failure
```

ASan/UBSan also work without building the port, instrumenting the simulation,
data import and their test callers:

```sh
cmake -S . -B build/sim-sanitize -DRAGE_BUILD_PORT=OFF -DBUILD_TESTING=ON -DRAGE_ENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/sim-sanitize -j 6
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sim-sanitize --output-on-failure
```

All 61 headless narrow tests pass together with ASan/UBSan. LeakSanitizer is
unsupported on this macOS toolchain; this result covers address and undefined
behavior checks, not leak detection.


The C side grows a narrow headless API that the Rust server can call. It does
not open a window. One race context is enough to say:

- create a race for a course and class
- place a human or a computer car in a seat
- set that human's inputs
- step once
- read every car's pose, speed, lap state and whether the race is over
- destroy the context

Many rooms means many contexts. The server never calls into the renderer,
the disc layer, or the menu.

Track material decoding accepts an optional borrowed 16-color environment
palette. It overlays that palette for one decode without modifying either
owned texture bank or GPU state. Narrow coverage verifies animated colors,
unchanged source banks and absence of palette leakage into another race.
The client build and all 66 headless ASan/UBSan tests pass. This is CPU-side
preparation; dynamic GPU material updates remain to be connected.

Imported meshes now retain their explicit asset source. Cache lookup and
material-slot lookup share the same source-aware resolver, rather than inferring
ownership from a car-only pointer. Narrow tests distinguish default and owned
entries with identical keys, reject invalid sources and resolve owned course
meshes without car data. This establishes identity, not GPU registration or
per-race cache lifetime; those are still pending.

`ClientRace` copies the track placement header and resolves participating AI
body/wheel placement, mesh base and palette into its own `RivalLook` records.
The course/model mapping is now immutable shared data rather than mutable
client initialization state. Unsupported bank selections reject construction.
Environment initialization also seeks the authored track offset instead of
always starting at zero. Narrow `track_look` tests cover unaligned input, every
truncation, all course/slot mappings and failed reads preserving output.
The PAL constructor sweep covers all 24 packs with mixed human/AI fields
(four contenders in the final class), and checks the authored environment
start. All 67 headless sanitizer tests, the client build and four affected
client regressions pass. Rival submission still uses the legacy adapter;
the owned rival publication path is described below.

`SubmitRaceView` now submits AI through the owned rival adapter with explicit
placement, track key and track-car palette variant. The adapter does not select
models from the current global track or require a legacy race scene ID. Shared
car-part assembly receives explicit asset source and material variants, retaining
legacy wheel palette selection. The narrow view test checks seat/model/shape/
palette forwarding and rejects missing placement or invalid variants before
starting the field. All 67 headless sanitizer tests, the client build and four
affected client tests pass. This proves CPU publication, not GPU cache registration
or visible gameplay. Track-zone sampling now takes explicit events, length and reverse direction.
`RaceView` supplies each human/AI car's resulting zone to ambient lighting and
lamp daylight input, without sampling the installed legacy track. The original
zone adapter shares this calculation. A narrow headless test interleaves two
tracks, checks unchanged sources, fade boundaries and special codes. View
coverage checks per-seat reverse sampling; all 68 headless sanitizer tests pass.
`RaceView` now owns each seat's lamp history and updates it once per 50 Hz
clock tick from explicit daylight and that seat's shelter/brake input. Rendering
receives a copy and skips legacy frame-based lamp accumulation for owned assets.
Narrow tests cover independent views, human/AI lamps, duplicate ticks, immediate
braking, invalid daylight preserving state and lamp forwarding. `RaceView` also owns current/previous car poses, advancing them only on accepted
ticks. Owned car assembly derives previous part transforms from the supplied
pose and never reads or writes the legacy transform history. Narrow tests
cover first-frame initialization, independent views, repeated drawing retaining
previous poses and reappearing seats starting without an interpolation jump.
Camera environment conversion now accepts the race's explicit `Environment`: sky
bands, fog color/range and cloud row are copied into a supplied `RenderCamera`,
leaving pose, clipping and panorama identity/layout unchanged. The legacy camera
uses the same conversion from its installed state. A narrow GPU-free test checks
independent environments, unit conversion and preserved camera/source fields.
The client build and all 69 headless sanitizer tests pass. The owned race still
needs camera/frame publication, panorama resources and GPU resource lifetime
integration; this helper alone does not make it a playable mode.

Sky decoding now shares one GPU-free implementation between the live importer
and owned sources. It accepts an explicit bounded texture image chain and
copied panorama layout, including a borrowed animated-palette overlay. The live
adapter retains its blank-capture retry policy, but the conversion no longer
reads global texture state. Narrow coverage checks complete panorama decoding,
grayscale modulation, invalid input preserving output and palette/source
independence. All 70 headless sanitizer tests, the client build and three affected
asset tests pass. Selecting/owning the authored tile map and attaching the decoded
panorama to a particular race's GPU cache are still pending.

The client constructor now prepares owned human mesh/material entries alongside
the four track banks, using the existing car-cache builder. No renderer callback
is needed to finish CPU preparation. `RetainClientRace` gives main-thread users
an explicit lifetime reference: releasing the original owner leaves scene,
textures, converted car meshes and simulation valid until the last reference
is released. PAL constructor coverage checks independent car caches after
archive release, retained-resource use and another race continuing after final
release. Retaining NULL or an overflowing reference count is rejected. The
client build, all 70 headless sanitizer tests and the PAL constructor sweep
pass. Captured frames and GPU cache registration are not wired to this lifetime
yet; CPU lifetime support alone does not establish their safety.

Car-part instance construction now has a GPU-free implementation accepting
explicit current/previous poses and body metadata. It resolves four semantic
parts, transforms, wheel palette variants and body-only lamp state without
render history or game globals. The owned rival adapter already uses it; the
legacy adapter shares its transform conversion. The narrow car-parts test checks
actual instance output, previous/current positions, wheel/component identity,
repeatability and failed input preserving output. All 70 headless sanitizer
tests, the client build and three affected client regressions pass. `RaceView`
still calls the global publication adapter; direct caller-owned world submission
and human adoption of the shared instance builder were pending at that point.

`SubmitRaceView` now builds human and AI instances directly into a supplied
`RenderWorld`, using the real shared car-parts implementation. It stages the
complete field before publication and checks available space after replacing
old car instances, preserving geometry/mirror entries. Invalid configuration
or insufficient capacity leaves the destination unchanged. The view test no
longer mocks the game renderer: it checks actual multipart transforms, paint,
palette, lighting and independent worlds. The now-unused owned-rival and global
field adapters are removed. PAL coverage publishes both races after archive
release. The client build, all 70 headless sanitizer tests and affected client
regressions pass. GPU source resolution and retained-frame integration remain
unimplemented; caller-owned CPU worlds are not yet a playable mode.

`FindClientMesh` resolves a prepared human/track entry within an explicit
`ClientRace`, checking source, key, set and mesh range; no global lookup or
implicit import occurs. `DecodeClientMaterial` consumes those owned sources,
explicit bank selection and an optional captured environment palette, including
human paint. Narrow coverage checks equal keys resolving different race data,
page/palette selection and invalid requests preserving pixels. PAL coverage
resolves published car parts and decodes owned human materials after archive
release. The previous PAL pack-88 material audit now uses this path and retains
its result: 511/1854 transparent variants, no material lacking any visible
variant. This does not verify the actual selected variant for every object.
All 71 headless sanitizer tests, the client build and three affected client
tests pass. GPU providers still need an explicit per-race context and retained
frame/material generation handling; this CPU resolver is not registered there.

`ClientFrame` now copies a completed CPU world, captures its environment palette
and texture-bank selection, retains `ClientRace` and prepares main-pass mesh
lookups through the explicit owner. Missing owned meshes reject the candidate
without altering existing frames. The resolver callback is compatible with the
renderer mesh-build interface. The prepared-mesh vector uses its own standard
C allocation and runs headless; lookup bounds-checks pointer addresses instead
of subtracting unrelated pointers. Narrow cache tests cover invalid worlds and
foreign pointers. PAL coverage mutates/releases original instance storage and
race ownership, then resolves and decodes retained frame resources; failed
capture balances only its candidate reference, and the second race survives.
All 72 headless sanitizer tests, the client build and three client regressions
pass. The live GPU backend still uses its existing global asset provider and
material cache; it has not been switched to `ClientFrame` yet.

## Order of work

1. **Headless race context.** The C API above. Human/AI stepping, imported PAL
   data and complete races are already tested. Finish client adoption and
   verify region/cache policy before networking relies on this boundary.
2. **Protocol and lobby.** Rust server, SQLite, rooms, join and ready. A
   small C test client, or a temporary command in the game, can list a room
   and sit in it. No cars move yet.
3. **Two humans in one room.** The server steps both from their inputs and
   the game draws the other car on the received poses. This is the first
   build in which the two windows see each other.
4. **Authority.** Server collisions, finish order, and the client correcting
   its own car when the snapshot disagrees. Results land in SQLite.
5. **More than one room at once, and AI in the empty seats.** The lobby
   already allowed this on paper. This step is the sim actually running them
   side by side.

Each step is playable to the extent of its own sentence. Step 3 is the one
that answers "the games should see each other."

## Out of scope until the five steps work

Prediction beyond the local car, rollback, lag compensation past "correct
toward the snapshot", more than the retail grid, a server browser on the
internet, accounts, and anti-cheat past "the server's result is the result".
