# Multiplayer

Status: an incomplete authoritative multiplayer prototype, not a verified
playable multiplayer release. The target architecture and remaining scope below
still apply in full.

Implemented foundations:

- Owned C human/AI race contexts and retained client assets/frames.
- Rust server using the actual C simulation at 50 Hz, with independently owned
  two-human room workers and owned completion records.
- One validated Plan for class/course/laps/reverse/countdown and seat choices.
  Client-selected car/transmission is frozen at ready and checked before loading.
- Version 26 C/Rust wire protocol, source identity, bounded transport, results
  validation and fixed-history remote interpolation. Two human connections share
  the retail field with active AI, including the smaller final-class grid.
- Main-menu `MULTIPLAYER` entry into the windowed prototype, using an INI server
  address, create/auto/join-by-code/browser selection and a preliminary car/AT/MT picker
  with connected Ready/revoke confirmation.
  It shares the existing window/input configuration and
  returns to the title; those visual transitions still need live verification.

Local verification: 71 Rust unit tests, two startup tests and ten legal-PAL
integrations pass. C protocol/connection/socket tests run headlessly in Release
and ASan/UBSan. TCP process integration explicitly skips because this environment
prohibits binding a port. The three-platform CI workflow is prepared but has not
run on GitHub. These checks do not prove a playable networked race or GPU output.

CPU asset integration remains separate: `client_race` covers retained ownership,
all class/course packs and missing meshes; its `materials` mode checks decoded
textures after releasing the import archive. Configure `RAGE_SIM_DISC_BIN` or
`RAGE_RUNTIME_DISC` with a legal image; without one the integration skips with
code 77. Material decoding, prepared-mesh lookup and upload-queue tests do not
replace live visual/performance verification.

The race stream publishes a 7317-byte correction/checkpoint + snapshot pair
from one authoritative tick. The normal windowed client atomically validates
and restores the full physics state and prunes confirmed local inputs. The
diagnostic client checks envelope/clock/ack binding and reports poses/results,
without interpreting or applying physics. Both receivers retain fragmented
publications and coalesce bounded backlogs; results follow the final state.
Legal-PAL forward/reverse integrations now deliver these full publications
through both Unix streams, including either human disconnecting. This is not
live TCP/GPU verification. Initial bounded local prediction is now enabled below.

Protocol 26 requires a 708-byte car configuration packet (type 0x88, format 1)
after Start and before Loaded. It carries the two human variant IDs and their
complete 352-byte little-endian specifications. The server encodes its prepared
specifications; the client verifies both variant IDs and human seats before
applying either, rebuilds engine/drive values, then starts the simulation.
The diagnostic client consumes and checks the same packet without running
physics. Missing or malformed configuration fails setup. Local cars.toml is not
read by this owned race path. The server optionally loads `--cars=path` once
through the existing compiled catalog parser and passes its immutable overlay
to each owned grid. Only present physics fields replace retail defaults;
the source archive/cache remains unchanged. Invalid explicitly selected files
fail startup before listening. Price/unlocks are not applied by this physics
path. Server admission to race preparation respects `manual_only` when present,
otherwise retaining disc availability. PAL coverage checks both forced-MT
rejection of AT and restoration of AT, including unchanged loaded policy after
a missing replacement file. After Welcome, before Pick/Ready, the server sends
type 0x89, format 1 and a four-byte little-endian AT availability mask for all
32 retail variants. The connected client picker and Ready screen use that mask;
no local disc-based transmission gate runs before connecting. Missing/invalid
availability fails setup. Packet and fragmented socket tests include bit 31.
The diagnostic client also consumes the packet. Live picker appearance remains
unverified.
PAL integrations deliberately corrupt client specs/engine values before applying
the packet and check restoration against the server. This is not live lobby proof.
Another PAL integration checks partial catalog overrides, unchanged retail source
identity/data, independent already-created races, and preservation of a loaded
catalog when a replacement file is invalid or missing.

Prediction stops on the tick when the local driver finishes or retires, including
a transition inside the prediction horizon. It does not advance the other cars
after that transition. The existing full-race PAL integration checks both human
finishes against the complete native checkpoint in forward and reverse runs;
it also asserts that both transition checks were actually exercised.

Local presentation now blends native human poses from two bounded predictions,
using the mapped host-clock fraction and the full 40 ms PAL motion interval
rather than alternating a held tick with a 20 ms transition. The camera and
speed display use this same presented pose. Physics/checkpoints remain untouched;
terminal transitions snap to the newer state and the lead limit disables further
interpolation. Narrow tests cover angle wrap, endpoints, discrete brake controls,
model mismatch, terminal snapping and unchanged output on invalid data. Game
builds and headless checks pass; smoothness and correction jumps still need live
network/GPU verification.

Still required: compiled-in
SQLite persistence, verified local prediction/correction behavior and performance,
verified shared races and results in two windows, live multi-room networking,
and Linux/Windows/region compatibility checks. No live GPU performance improvement
is claimed. The rest of this file contains the design and implementation history;
older test counts there are historical, not the current verification total.

Completion audit (current worktree; headless passes do not close live gates):

| Requirement | Current evidence | Remaining gate |
| --- | --- | --- |
| Independent authoritative C rooms and AI | Rust unit tests and concurrent PAL traces | Live simultaneous TCP rooms |
| Agreed effective human car data | Protocol 26, catalog/source isolation and C application tests | Live selection with server overrides |
| Local prediction and corrections | Native checkpoint replay and bounded presentation tests | Two-window latency/collision/performance checks |
| Main-menu lobby and same-window return | Client code and scripted menu tests; game builds | Actual windowed lobby/race/results/return |
| Room/entrant/result persistence | No SQLite dependency or store in the server | Vendor and compile SQLite; implement and test transactions |
| Linux/Windows/regions | Prepared CI; legal PAL headless integrations | Actual platform builds/runs and other-region sources |

The TCP process fixture was rerun and returned explicit skip 77 because bind is
denied (`Operation not permitted`). SSH to darwine was rerun and is also denied.
The SQLite amalgamation is absent from the searched local dependency caches;
the official download still fails DNS. Earlier Computer Use approval rejected
launching Rage Racer, so no two-window/GPU observation is asserted. No live job
is waiting to complete these checks. These are outstanding requirements, not
alternative definitions of completion.

AI snapshot preparation now reads RPM from the rival layout rather than the
overlapping player drivetrain. AI has no player gearbox/clutch, so those pose
fields are zero; the common pedal fields retain actual AI values. A narrow
owned-race test distinguishes both layouts. A legal-PAL integration constructs
two-human/AI fields across all six classes, four courses and both directions,
steps 100 ticks, validates AI poses through Rust encoding and C decoding, and
checks movement plus empty inactive seats. Version 16 also transmits the complete
field: start carries ten optional AI descriptors after the two human choices and
source hashes, and snapshots contain twelve poses. Server setup selects active
retail starts; clients construct the same human/AI field from decoded metadata.
AI retains its authored model/behavior/seed and inactive slots remain empty.
Client application and interpolation use the correct human/rival drivetrain
layout. The HUD counts configured entrants, including retired cars. Result packets
still report the two human participants, with places in the complete field.
The full forward/reverse wire integrations now compare every field seat through
C application, including skipped packets and real Unix-stream delivery with
either human disconnecting. This is shared simulation/transport evidence, not
two live GPU windows or TCP verification.

Room ownership now begins after the pair's hello, before Ready or asset loading.
The room worker owns its session through setup, race and results; the listener
can admit another pair while an earlier room is waiting/loading. Setup failures
close their owned connections and are reaped through the same room manager.
Capacity includes preparing rooms and their IDs are not recycled. Workers share
an immutable imported archive through `Arc` only while constructing owned races.
A socket-free test keeps one setup waiting while another worker starts, then
checks owner cleanup. The PAL parallel trace test now performs asset/grid
preparation concurrently as well as stepping, drops the source owners before
stepping and compares the complete AI field against sequential runs. Both pass.
Admission now keeps bounded owned pairs and polls their hello state without
waiting on a peer. Each accepted seat retains a five-second absolute hello
deadline; a greeted first seat waits at most 90 seconds for its partner. Timeout
or disconnection drops only that pair. The 16-room limit includes both admission
and preparing/running rooms. A failed newcomer attach preserves an earlier peer
and its original deadline. Small clock-injected tests replace the removed
blocking accept/hello loops and cover both boundaries, readiness preservation,
independent pairs and cleanup without sleeping or opening a socket. The server
automatic pairing described here predates the explicit routing below.
SSH to darwine was rechecked and remains prohibited (`Operation not permitted`).

The first participant now configures class/course/laps/reverse in the existing
window after welcome. The screen edits a local choice, commits on confirmation
and preserves the original on cancellation. Protocol 17 adds a five-byte creator
settings message (type 0x06 plus four u8 fields), validated by the same compiled
C rules on both sides. Only the creator's owned input accepts it; changing options
revokes that participant's Ready. All seat locks freeze settings and car choices
atomically before import/start, and the decoded Start carries the selected values.
Guest and post-freeze changes are rejected. Ready now waits until confirmation or
disconnection; the 60-second Loaded deadline remains. The windowed client likewise
waits for Start without a room-decision timer. Once its first byte arrives, an
absolute five-second receive deadline prevents stalled/trickled packets from
holding the client indefinitely. A socket-pair test covers idle waiting, partial
receive, unchanged deadline, expiry and disconnect with unchanged output. Game/
smoke builds and Release/ASan/UBSan socket checks pass; no prolonged live TCP wait
is claimed. The blocking diagnostic client's original two-minute Start wait is
unchanged. Noninteractive startup keeps
the server operator's command-line plan. Pure menu tests cover every field and
cancel; socket tests check emitted bytes; narrow Rust tests check truncation,
creator permissions and the complete settings-to-Start boundary. Release game/
smoke builds, C Release/ASan/UBSan and 56 Rust unit plus two startup and seven PAL
integration tests pass. Appearance and live TCP remain unverified. Room browsing,
live lobby verification remains outstanding.

Protocol 18 adds an explicit room request after Hello and before Welcome:
type `0x07` followed by a little-endian u64. The INI setting
`multiplayer.connect_room=create` (or `0`) creates a room; a positive decimal
code joins that exact open room. Missing or `auto` selects a vacant room or
creates one when capacity permits. Codes are bounded by signed 64-bit range;
the wire value UINT64_MAX denotes auto. Invalid, missing or full explicit rooms
are rejected without falling back to another room. Protocol 19 returns the
assigned code in an eleven-byte Welcome: type, version, seat and little-endian
u64 room code. Zero and out-of-range assigned codes are rejected atomically.
The windowed waiting screen shows this code and HOST/GUEST; live appearance
remains unverified. Returning to a full interactive lobby remains required.

Admission owns at most 32 pending connections, each with one five-second
Hello/room-request deadline. Open and preparing/running rooms share the
16-room limit. Codes are allocated on creation and never recycled during the
process lifetime. Explicit creators wait for a partner without a join timer;
automatic pairing retains its 90-second partner deadline. Routing transfers
the socket, reader, input and output ownership to the selected seat and binds
creator permissions before Welcome. Narrow tests cover exact routing, capacity,
truncated requests, deadline boundaries, guest permissions and worker cleanup.
C socket-pair tests verify polling and blocking room-request bytes, while pure
parser tests cover decimal bounds and unchanged output on rejection. Release
and ASan/UBSan checks pass; this does not establish live TCP or lobby UI behavior.

The separate `poc/multiplayer` branch only shares selections between processes;
its cars do not see each other and it is not the implementation of this plan.

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

The windowed multiplayer client now copies its archive and source identity
through the image already mounted by normal startup. It no longer re-selects
disc configuration or falls back to a hardcoded PAL path. `LoadRaceIso` is the
shared owned-copy loader for file-based server import and the host's mounted
sector reader; the latter retains CUE offsets and can use its existing CHD
backend. Synthetic Mode 1/2 tests compare bytes/boot/executable identity with
file loading, then clear the reader/source bytes and read the independent copy.
Release and ASan/UBSan disc tests pass; game and smoke builds pass. Mounted
CHD and live GUI multiplayer remain unverified.

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
`.github/workflows/multiplayer-check.yml` now registers headless Release checks
for Linux/macOS/Windows plus Linux C transport ASan/UBSan. CI installs libclang
for bindgen and fetches the locked Rust build dependencies once, then CTest
runs offline. No disc, SDL or authored-car generation is required. A fresh local
macOS build using those same headless/embedding-off options passes all five C
checks, 48 Rust unit tests and two startup tests; TCP integration explicitly
skips due to the local bind restriction. Workflow YAML parses, but GitHub jobs
and Linux/Windows execution are not yet verified. No release publishing step
is included.
The workflow also builds an optimized standalone server with the locked,
offline dependencies. Its Windows job inspects PE imports through LLVM and
rejects redistributable MSVC runtime DLLs, including debug/suffixed variants.
A fresh macOS Release server builds, runs its normal usage/error entry point
and imports only `libSystem` according to `otool -L`. The local Rust toolchain
warned that its optional debug stripping tool could not load `libLLVM.dylib`;
the server executable itself has no LLVM dependency and launches. Windows
import inspection and the GitHub release-build steps remain unexecuted here.
`mp_protocol` is registered in both full and headless builds, independently
of Rust, SDL and legal-disc integration. Build `mp_protocol_tests`, then run
`ctest --test-dir <build> -R '^mp_protocol$' --output-on-failure`. The same
target is instrumented when `RAGE_ENABLE_SANITIZERS=ON`; no manual compiler
command or full game build is needed. Full/headless Release and headless
ASan/UBSan execution pass. `-L multiplayer` includes this C test as well as
the optional Rust checks below.
Connect/socket/transport fixtures and client argument checks now use the same
headless registration, with common warnings/sanitizer setup rather than a
second registration in the full game's test directory. Build
`mp_connect_tests mp_socket_tests mp_transport_tests` alongside the protocol
target; the transport target builds its compiled client dependency. Five C
checks pass in full/headless Release and headless ASan/UBSan. TCP integration
explicitly skips with code 77 when binding is prohibited; this remains the
case in the current environment. Unix socket-pair fragmentation/cleanup tests
execute normally. Socket/transport fixtures are Unix-only; connection and
invalid-argument tests are also registered on Windows, but have not run there.
To include server checks in CTest, configure a non-sanitized build with
`-DRAGE_TEST_SERVER=ON` and Cargo/rustc installed, then build `rage-sim`,
`rage-data` and `rage-mp-protocol`. `ctest --test-dir <build> -L multiplayer
--output-on-failure` runs `server_unit`; configuring `RAGE_SIM_DISC_BIN` also
registers `server_retail`, which runs only the four ignored disc integrations.
Cargo uses that build's C libraries and a separate target directory inside it.
The two CTest entries share a Cargo resource lock and never fetch dependencies
(`--offline`); they use `--release --locked`, so the tests exercise optimized
Rust code and cannot rewrite dependency selection. The Rust dependency cache
must already be populated. All 48 unit tests, two startup tests and four PAL
integrations pass with the optimized Rust/C boundary on macOS. Cargo and
rustc paths can be supplied as `RAGE_CARGO_EXECUTABLE` and
`RAGE_RUSTC_EXECUTABLE`. Rust remains optional for normal game builds.
CTest supplies the actual archive output directory, including the selected
configuration for multi-config generators (`ctest -C Release`). Cargo expects
`rage-sim.lib`, `rage-data.lib`, `rage-mp-protocol.lib` for MSVC targets and
`lib*.a` otherwise. Direct Cargo invocations must point `RAGE_SIM_LIB_DIR` at
that same archive directory. macOS unit/startup/PAL checks pass after this
build-path change; Windows linking/runtime are not yet verified.
An independent Ninja Multi-Config headless build now verifies the configuration
path: all three C archives were built under `Release/`, and CTest with
`-C Release` linked those exact archives for 48 Rust unit tests, two startup
tests and four PAL integrations. They pass on macOS. This validates
multi-config directory selection, not the MSVC filename/linker branch.
The repository's `.cargo/config.toml` enables `crt-static` only for MSVC,
matching the C archives' Release `/MT` runtime and the standalone shipping
contract. Use Release C archives for the Windows server; CMake Debug archives
use `/MTd`, which is a different runtime. macOS Cargo/configuration and 48 unit
plus two startup tests pass with the conditional config; actual Windows CRT
linkage still needs the Windows CI run and binary dependency inspection.
Pure C protocol operations are now built once as `rage-mp-protocol`, available
in headless CMake builds and shared by the game, test client and C tests. The
Rust integration links this same library. Build the server dependencies with
`cmake --build build/sim-release --target rage-sim rage-data rage-mp-protocol`.
The real PAL two-driver forward/reverse finish test now decodes server start
metadata through C, verifies source identity and selected setup, and starts its
client from the decoded race setup and countdown. Every simulated server tick is serialized,
decoded and applied through the production C client API to a separately owned
client race. Tick/elapsed/countdown, position, speed and entrant status match;
final results also pass actual C decoding and final-state/time validation.
An additional forward/reverse race delivers only the first, every 31st and final
snapshot. Both cases compare every transmitted pose field after C application,
plus countdown and authoritative clocks, and validate the final result. The
two seats use distinct retail automatic variants and RNG seeds; decoded setup
checks both selections and seeds before constructing the independent client.
All 47 Rust tests and two startup tests pass with legal PAL integration enabled.
Game/tool builds and five related C tests also pass. This establishes real simulation-to-client wire/state integration
without sockets or GPU, not a playable TCP race. Pure-test symbol auditing
still shows no socket/clock dependencies.
Cargo now tracks all three external static-library files (`rage-sim`, `rage-data`
and `rage-mp-protocol`)
as build-script dependencies. Rebuilding either C library therefore relinks the
server and Rust tests instead of silently retaining an older simulation/importer.
Verification changed each library's modification time separately without changing
its bytes: Cargo reported that exact dependency as dirty and rebuilt the package;
39 unit tests and two startup tests passed after each change. CMake still builds
the libraries explicitly; Cargo does not compile their C sources.

Current startup: `rage-racer-server <CUE or Track 01 BIN or cache directory> [port] [--reimport] [--class=1..6] [--course=1..4] [--laps=1..6] [--reverse]`. The default
port is 7243; an explicitly invalid or zero port is rejected before importing
data. Missing/invalid source and listener failures report an error and exit
unsuccessfully without a panic. The selected track and grid are prepared before
opening the listener. Accept/socket-clone failures cancel existing seats;
hello/loading failures share the same cancellation path, clearing controls
and queued output before shutting down connections. Writer join failures are
reported as an unsuccessful exit rather than another panic. A narrow test
checks cancellation of every seat and pending message without creating sockets.
Each accepted seat must complete hello within five seconds before the server
waits for the next connection. Previously the first seat's hello deadline did
not begin until both TCP connections existed. Reader thread-creation failures
also cancel the session instead of panicking. Writer startup uses fallible
socket cloning and thread creation, retains control sockets for cancellation
and joins writers already created if a later writer cannot start.
The hello wait now services the coordinator at bounded 10 ms intervals and
checks previously admitted seats for disconnection, retaining its absolute
five-second deadline. A narrow test covers coordinator-driven greeting,
expired deadlines, earlier-seat loss and invalid seat indices without sockets.
The listener now accepts without blocking the setup monitor. Once one seat
has greeted, joining the next has a 90-second absolute deadline and cancels
if any waiting seat closes. A failed welcome does not renew that deadline.
An empty server can still wait indefinitely for its first participant. Accepted
streams explicitly return to blocking mode for the existing worker adapters.
Narrow accept-loop tests inject transient/fatal accept errors and seat loss,
and use an already-expired deadline without a real TCP listener or a 90-second
wait. All 29 Rust unit tests and two startup process tests pass; live TCP and
Windows listener behavior remain unverified.
A narrow readiness test verifies
that an incomplete hello times out and cannot begin loading after cancellation.
The prototype's `Session` owns control sockets, inputs, outboxes and all reader/
writer handles. Every early return drops that owner: cancellation closes the
session before joining its workers, including partially started sessions.
Normal completion drains result writers first, then shuts down and joins the
readers. No reader is intentionally detached. A narrow ownership test starts
workers waiting on input/output, drops their session and checks that both have
completed while a separate session remains usable; it needs no TCP or assets.
The running-room loop is now separate from process startup and consumes its
own `Race` and `Session`. Archive import, listener and CLI configuration remain
outside that ownership boundary. A failed C simulation step terminates the
room instead of repeatedly publishing the same tick. Existing session,
scheduler, protocol and cache coverage passes (25 Rust unit tests and two
startup process tests); simultaneous running rooms and their dispatcher are
not implemented or verified by this extraction.
A narrow room-loop regression calls the real C simulation through Rust with
setup phase and exhausted tick/elapsed counters. Each rejected step returns
failure and closes every input/outbox belonging to that room, while another
session remains usable. These states reject before route access, so the test
needs neither imported assets nor sockets and introduces no mock simulation
or test-only production API. All 26 Rust unit tests and two startup process
tests pass. Running-race GPU/network behavior is outside this test's scope.
Narrow argument tests and subprocess tests
cover rejected arguments and unavailable source without running a race or
binding a port. A disc start writes an immutable directory named by appending
`.server-cache` to its source path. Later starts using the same command select
that cache without requiring the source image to exist. Operators can also
pass the cache directory directly. `--reimport` explicitly refreshes data from
the specified image (for example, after changing its revision); it never uses
cached data when the source is unavailable. Without this flag, a valid cache
remains the selected imported revision, even if the source image changes.
Invalid/missing cache or a field that cannot be prepared triggers import from
the configured image. A failed import preserves existing cache data. A direct
cache-directory start has no configured source and reports how to select one
if validation fails. The track/grid are validated before cache publication or
opening the listener.


## Server data cache

The Rust archive owner stores its content fingerprint once when loading either
disc data or a cache. Start packets and cache identity comparisons reuse that
value rather than rehashing the immutable RAGE.BIN for each room. Cache loading
still hashes the newly loaded bytes and compares with validated metadata; this
does not weaken file corruption/replacement checks. Existing cache round-trip,
corruption and source-recovery tests plus both real PAL race integrations pass
(42 Rust tests and two startup tests). No live startup-time improvement is
claimed without measurement.

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

The prototype now publishes a complete immutable cache directory by renaming
a sibling temporary directory after writing and syncing `RAGE.BIN` and an
`identity` sidecar. The sidecar records schema version 1, boot serial, executable
fingerprint, archive fingerprint and archive length. Loading checks metadata,
the entire archive checksum and the existing C archive validator; it restores
source identity for the normal handshake. No new serialized physics format is
introduced. Replacement writes a separate immutable `data-*` generation and
switches the `current` pointer only after successful publication. Original
archives and prior generations remain unchanged. Failed publication cleans
only its own temporary files and unselected generation. Matching existing data
is reused; re-running a disc import repairs invalid or different cached data.
Pointer names reject absolute paths, traversal and extra lines. Pointer
replacement has been tested on macOS; Windows replacement behavior remains
unverified. Automatic cache selection and source reimport are connected to
startup; a narrow incomplete-field test verifies source fallback and cache
preservation when import fails. Real PAL checks verify automatic loading with
the source absent, automatic repair of invalid metadata with the source
present, and forced reimport refusing to use cache when the source is absent.

Narrow filesystem tests cover byte-exact archive preservation, C-loader source
identity, existing-generation preservation, invalid schema, every truncated
metadata length and changed archive content. A real PAL import followed by
startup from its cache after removing the source symlink prepared the race
successfully; both runs then stopped at the sandbox's denied TCP bind. This
proves cache loading without the disc, not a running networked race.
The real PAL cache was also deliberately changed to an invalid schema, repaired
by importing the disc again, then loaded successfully through its new generation
after removing the source symlink. Replacement tests verify old-generation
preservation, pointer validation and cleanup when pointer publication fails.

## Lobby

Driving-input admission now belongs to `SharedInput::publish` under the same
lock as its stage, rather than only to a pre-read socket check. Waiting, ready,
loading and closed seats cannot update controls or their input timestamp;
loaded seats can queue controls for countdown/start. A narrow state test walks
the actual hello/pick/ready/load transitions and verifies preserved choice,
packet and timestamp on rejection. Control/concurrency fixtures now enter
loaded state through the production transitions; unrelated-session ownership
checks use lobby greeting rather than injecting driving input into waiting
seats. All 46 Rust unit tests, two startup tests and four PAL integrations pass.

Connection ownership is now indexed by seat rather than connection arrival:
`Session` has optional stream/reader slots and `attach(seat, stream)` admits a
connection only into an in-range vacant slot. Clone/thread failures publish no
partial slot. Loading requires every slot, and dropping a sparse session shuts
down present streams before joining its workers. The ownership regression uses
a worker in seat one with seat zero empty, without sockets. Existing Rust unit,
startup and PAL integration tests pass; TCP admission and interactive seat
selection remain unverified/unimplemented respectively.

Server admission and race loading now have separate ownership boundaries:
`load_session` accepts/greetings seats, then transfers its `Session` to
`start_session`. The latter has no listener and consumes the selected plan and
connections through ready/load/start. An interactive room coordinator can
reuse this loading path without accepting another pair or recreating their
input state. Existing 43 unit tests, two startup tests and four PAL integrations
pass; room creation/join messages and lobby UI remain unimplemented.
The loading boundary rejects invalid plans and sessions without both owned
connections/readers, or with writers already started, before waiting/freezing
choices or copying assets. A socket-free regression checks that greeted/ready
inputs alone cannot substitute for actual connections and remain ready until
their owner is dropped. All 44 unit tests, two startup tests and four PAL
integrations pass.

The windowed client also checks its selected variant's retail transmission
metadata before opening a connection. Missing/invalid metadata reports a data
error; AT on a manual-only variant reports the specific car and the
`multiplayer.connect_manual=1` setting, then releases the archive. Server-side
validation remains authoritative. The synthetic archive unit fixture now
contains independent first/last transmission flags and verifies all 32 mappings,
invalid variants, NULL inputs and truncated metadata without importing a disc
or running a race. Game build and race-data/car-asset tests pass in release and
ASan/UBSan. The live GUI diagnostic remains unverified.

Retail transmission availability now has an owned-data API separate from menu
globals: `ReadCarTransmission` reads the bounded model metadata flag and
`ReadRaceCarTransmission` resolves its variant through the archive. Invalid
flags, truncated data or invalid variants fail without changing the output.
The server checks that metadata before copying a track and rejects automatic
selection for a manual-only variant; it does not silently change the selected
transmission. Set `multiplayer.connect_manual=1` for those retail cars. Room
overrides remain unimplemented, so this policy uses original disc metadata.
The legal PAL integration checks manual/automatic admission for all 32 variants
and confirms both kinds exist. Narrow C tests cover unaligned buffers, all
truncations and every invalid flag. All 44 Rust tests including three legal-disc
tests and two startup tests, C car-asset tests and reader ASan/UBSan pass.
Only PAL was available in the inspected local image directories; NTSC behavior
and cross-region room compatibility remain unverified.

Operators can select the prototype's fixed race through startup options
`--class=1..6`, `--course=1..4`, `--laps=1..6` and `--reverse`. Class/course use
one-based CLI numbers and become zero-based selectors in the shared Plan.
Defaults remain class/course one, three laps, forward. Options after source
(and optional positional port) may appear in any order; duplicates, unknown
options, malformed numbers and out-of-range plans fail before source I/O.
The chosen Plan is used by preflight, every paired room and start metadata;
this is operator configuration, not per-room interactive lobby selection.
Narrow parser/wire tests and startup subprocess rejection tests pass. The
legal PAL finish integration now obtains its forward/reverse one-lap plans
from this same parser before running actual races. All 43 Rust tests including
both disc integrations and two startup tests pass.

Protocol version 11 adds pick (type 0x05, retail variant u8, manual boolean u8).
Ready/loading waits now monitor every seat rather than waiting on seats in
sequence. A previously ready/loaded seat disconnecting cancels setup even while
another seat is pending. Existing per-seat condition waits are bounded to 10 ms
between field checks, only during setup; the absolute 20/60-second deadlines
are retained. A narrow asset/socket-free test covers incomplete fields, both
completed fields and cancellation of a previously loaded seat. All 39 Rust unit
tests and two startup tests pass without warnings. Live TCP cancellation remains
unverified.
A greeted waiting/ready seat can change its choice; each valid pick revokes
ready. Invalid or truncated picks preserve choice/readiness. Entering loading
freezes and returns that seat's choice, and the session builds one selected
plan for both C race preparation and start metadata; server-owned seeds remain
unchanged. The windowed prototype sends `multiplayer.connect_car` (0..31,
default 0) and `multiplayer.connect_manual` (0/1, default 0) before automatic
ready. Empty/default choices remain compatible with the updated headless tool.
Narrow tests exercise the production plan-selection boundary with different
cars/transmissions, revocation, truncation, bounds and post-freeze rejection.
Client/tool builds, 33 Rust unit tests, two startup tests, C protocol/socket
tests and socket ASan/UBSan pass. TCP integration remains skipped; interactive
lobby UI, agreed overrides and manual-only policy remain outstanding.
Numeric connection settings are now parsed once before opening a session:
missing values keep defaults, but explicitly invalid port/car/transmission
values return an error instead of silently selecting defaults. The pure parser
uses the existing bounded integer parser and commits all three settings only
on success. Narrow tests cover boundaries, empty/partial values and overflow
with unchanged output on failure; client/tool builds, C protocol/socket/argument
tests and protocol ASan/UBSan pass. This is configuration validation, not live
network or UI verification.
Freezing a selected plan now holds every seat lock in consistent grid order,
validates all readiness before changing any stage, and then freezes the whole
field. A regression first failed with the previous sequential loop: a late
unready seat left an earlier seat loading. It now verifies unchanged seats on
invalid plan/unready rejection and successful retry with changed selections.
The state transition lives on Input, used by the coordinator and narrow tests;
the unused single-seat locking entry point is removed. All 34 Rust unit tests
and two startup tests pass without warnings. This does not implement lobby
retry policy or persistence; the prototype still cancels failed setup.

Protocol version 10 introduces explicit client ready (type 0x04 followed by
boolean 0/1). Hello alone no longer starts loading. Ready may be revoked while
waiting; loading/loaded/closed seats reject further changes. After both hello
messages the prototype uses one shared 20-second ready deadline, then sends
start and begins the existing loaded gate. The windowed and headless prototype
clients currently confirm ready automatically after welcome. Interactive car
selection and lobby UI remain unimplemented. Narrow Rust tests cover the gate,
revocation, disconnect wake-up, malformed/truncated ready and late messages;
C socket tests verify blocking/polling ready bytes and invalid flags. Client/
tool builds, 31 Rust unit tests, two startup tests, C protocol/socket tests and
socket ASan/UBSan pass. TCP integration remains explicitly skipped here.

The prototype now retains each complete hello name in that seat's owned input
state instead of only logging and discarding it. Presence of the name replaces
both greeted flags as the handshake's source of truth. Raw protocol bytes
(at most 15) are preserved; lossy UTF-8 conversion remains display-only and
empty names retain existing compatibility. Disconnect retains identity for
future results while clearing controls. Narrow reader tests cover every
truncated hello, duplicate-name preservation, size bounds and independent
seats. All 27 Rust unit tests and two startup process tests pass. Lobby display,
name policy and SQLite persistence are not connected yet.
Input readiness now uses one stage (waiting/loading/loaded/closed) instead of
three independently mutable flags. Reader fixtures no longer force loading
before hello: they decode hello, call the production begin-load gate and decode
the continuation, including byte fragmentation and truncated input. Existing
ordering, duplicate-loaded, disconnect and timeout coverage passes without
adding a test-only state-setting API.

The Rust simulation adapter can now construct mixed human/AI/empty grids.
Its explicit rival entry separates authored grid position, logical model,
behavior slot and RNG seed; the existing C InitRaceGrid remains responsible
for retail validation and atomic construction. The pure grid mapper rejects
too many seats before indexing fixed storage, with a narrow mixed-grid test.
Human entries now explicitly carry authored grid position and a retail
`variant`, distinct from the rival's logical `model`. A network seat's index
therefore need not be its starting position. The mixed-grid fixture places
human seat zero at grid five with variant 31, and rival seat two at grid seven
with logical model nine/behavior slot three. Current network setup still
assigns grid positions zero/one; transmitting a selected grid is future work.
All 28 Rust unit tests and two startup tests pass, along with the C race-grid
sanitizer test. That earlier prototype selected two humans and sent only their
snapshots. Version 16 now includes the agreed AI start/snapshot data and client
presentation described at the top of this file.

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

Connected NOT READY seats can now select variants with left/right and toggle
AT/MT with up; Ready seats must first revoke Ready before editing. Both screens
reuse the same transmission metadata scan and pure `MpChangeCar` operation.
The menu retains one in-flight Pick and commits its local choice only after
the write completes. Pick, like Ready, may overlap the single Start receive.
A valid Pick crossing the server's freeze is ignored without changing the plan
or disconnecting the seat. The client retains a bounded bitset of choices it
actually sent and accepts an earlier requested choice in authoritative Start;
an unrequested car/transmission still fails rather than silently changing cars.
Scripted tests cover variant wrap, AT/MT, Ready editing lock, successful changes,
an earlier choice winning the freeze and an unrequested choice rejection.
Socket tests check Pick during a partial Start receive; Rust reader tests check
late Pick followed by Loaded and unchanged frozen choices. Release game/smoke,
C Release/ASan/UBSan, 68 Rust unit, two startup and seven legal-PAL integrations
pass. Live GUI/TCP behavior remains unverified.

Protocol 21 adds owned lobby updates (`0x86`), sent only when the current room
view differs from its last published packet. The waiting screen shows the peer's
name, variant number, AT/MT and Ready, or waits for a vacant seat. Local Ready is
shown in the title. The 53-byte packet has type/version, the existing 15-byte room
record and two 18-byte seat records (variant/manual/name length/15 raw name bytes).
Absent seats and unused name bytes are canonical zero. Names are retained as raw
bounded protocol bytes; display replaces nonprintable bytes without changing them.
Lobby and directory views use the same ordered-lock snapshot operation, with
owned peer names and settings. Connection routing binds the actual room code
before Welcome. Admission and Ready waiting publish changes through the session
owner; no second socket reader or shared simulation state is introduced.
The snapshot uses two fixed lock guards and 15-byte owned name buffers rather
than allocating a guard vector and cloning names on each poll. Lobby encoding
uses a fixed room record directly, sharing validation with directory encoding;
it no longer constructs an intermediate directory Vec. Names retain explicit
length and canonical zero padding, checked by both encoders/decoders. Existing
Rust-to-C golden and owned-view checks pass after the change. This removes those
allocations structurally; no measured live performance gain is claimed.

The Start receive path accepts these updates before Start. Polling returns a
distinct update status while preserving the Start output, then resets the packet
deadline for the next message. The diagnostic blocking receiver retains one
two-minute absolute decision deadline across lobby updates. Wrong room codes,
invalid choices/masks/names/padding and incomplete packets cannot replace the
previous view. Socket-pair tests cover fragmented lobby, Ready, Start and Loaded,
blocking receipt and wrong-room rejection. Pure C truncation/validation tests,
Rust-to-C encoding checks, owned view/cache tests and scripted peer-label tests
pass, alongside Release game/smoke and ASan/UBSan checks. The TCP process fixture
also includes lobby, but still skips here because binding is prohibited. This
is implemented peer presentation, not live two-window/TCP verification.

BROWSE ROOMS now connects, completes Hello and requests the directory before
choosing a room. The existing font/input screen shows one selected room's code,
class/course/direction/laps and OPEN/FULL/LOADING/RACING status. Up/down wraps
the list, right refreshes and confirm selects only a vacant open room. Refresh
preserves the selected code when it remains present; empty/disappearing lists
are safe. Cancel leaves the original code unchanged and closes through the
normal host cleanup. Car selection still precedes connection and the connected
waiting screen follows admission and displays the shared peer update above.
`MpClientPollList` owns one request and its header/body receive, with a fixed
five-second deadline and bounded count before reading the body. Partial packets,
invalid metadata, expiry and EOF preserve the visible list/count. Completing a
room request prevents further browsing. Socket-pair tests deliver every byte,
refresh to empty, verify no duplicate requests and room-request transition,
and reject invalid version/count/ready mask and expiry. Scripted menu tests cover
selection, unavailable races, empty lists, refresh, cancellation and failures.
Release and ASan/UBSan checks plus game/smoke builds pass. TCP and live browser
appearance remain unverified.

Interactive entry now selects AUTO JOIN, CREATE ROOM or JOIN BY CODE before
choosing a car and opening a connection. The code editor uses 19 decimal digits:
left/right selects a digit, up/down changes it and confirm validates the full
signed-64-bit positive range. Cancel first returns to the mode choice, then
returns to the title; the original configured code is committed only on success.
Zero and overflow remain on the editor with a visible error. The noninteractive
startup path continues to use INI directly. Narrow scripted menu tests cover
all modes, cancellation, invalid codes, maximum value, overflow, digit wrapping
and cursor wrapping without sockets, SDL or assets. Release and ASan/UBSan pass;
game/smoke builds pass. Live appearance and TCP remain unverified. This is the
create/join entry, not a room browser.

The connected waiting screen now starts NOT READY after Pick and toggles Ready
on a fresh confirmation. Cancel leaves the room. Its owned local state lives in
`MpWaitRoom` alongside the other menu screens; scripted tests call that production
loop with bounded transport fixtures rather than constructing a race. They cover
ready/revoke, blocked writes retaining their original value, cancellation,
disconnect/send failure and Start winning over a same-frame confirmation.
Noninteractive startup retains automatic Ready.

Ready waiting and plan freezing now use one retrying gate. Each attempt locks
all seats in order and either freezes the complete current field, keeps waiting,
or reports a closed/invalid room. A readiness change between observations no
longer cancels the room. Only a successfully frozen plan reaches asset loading.
Two asset/socket-free tests exercise a revoke/change-car/ready sequence and
closed/invalid cancellation, checking the latest choices and unchanged other
seats. Release unit/startup and all seven legal-PAL integrations pass. This
fixes the admission boundary independently of the shared peer display above.

Only Ready writes may overlap an in-progress Start receive. Socket-pair tests
verify bytes in both directions and preservation of the receive prefix/deadline;
other setup writes remain rejected. Pending Ready is completed before consuming
Start and transitioning to Loaded. A valid Ready arriving after the server froze
the plan is ignored in Loading/Loaded, rather than disconnecting the player;
the frozen choice and stage remain unchanged. Reader tests cover a late revoke
followed by Loaded, while malformed booleans remain rejected. Release game/smoke,
C Release/ASan/UBSan and the Rust unit/startup/PAL checks pass. Peer readiness,
connected car changes and live two-window/TCP behavior are still outstanding.

The windowed client's independent chase rig now uses the existing
`camera.chase_height`, `camera.chase_distance`, `camera.chase_pitch` and
`camera.chase_turn_lookahead` adjustments. It does not borrow the single-player
camera state. Existing configured/invalid-value checks plus a new missing-settings
default test pass, and game/smoke builds pass. The rig still differs from the
retail chase geometry; live visual behavior remains unverified.

`MainLoop` now has a host-activity boundary for `GAME_SCENE_MULTIPLAYER`:
it calls `PortRunMultiplayer` between legacy frames, then returns to
`GAME_SCENE_ENTER_TITLE` without booting again. The host entry no longer accepts
an unused port-config argument. A narrow loop test requests that scene and
checks one host call, return routing after failure and no extra boot; game and
smoke builds pass. The main menu now includes `MULTIPLAYER`, with a built-in
pixel label independent of disc textures. Selecting it preserves the current
Grand Prix/Custom selection and save pointers, then enters this host activity.
The prototype uses the INI server address. Main-menu entry now offers a preliminary
text picker for all 32 retail variants: left/right selects, up/down toggles AT/MT
when available, confirm joins and marks ready, and cancel returns without opening
a connection. Transmission metadata is read once into a local availability mask;
manual-only variants remain MT. `MpChangeCar` is pure, with headless tests for
wraparound, all variants, transmission constraints and atomic rejection, also
passing ASan/UBSan. The picker now shows variant number, catalog name, grade,
AT/MT and the retail gear count. Cosmetic names are copied once from the host
catalog; model/grade mapping uses `CarCatalogVariant`, independent of current
owned grades or menu selection. Specifications are loaded only when the selected
variant changes. Labels do not apply local physics overrides to the server race.
There is no model preview yet. Game/smoke builds and seven relevant catalog,
profile, race-data, protocol and frame checks pass; live appearance remains
unverified.
The initial implementation described here predates the create/join and connected
Ready, browser and peer-state screens above.
Connecting/waiting status remains in the same window.
After a matching authoritative result, interactive entry now closes its socket
and shows both seats' places/times or retired status until confirm/cancel.
Times reuse `FormatLapTime`; no client finish calculation is introduced. The
owned race GPU frame is cleared before menu text, since that dedicated present
path otherwise hides overlays. Noninteractive startup retains automatic exit.
Game/smoke builds plus protocol, host-frame and time-format unit checks pass.
Live result presentation remains unverified: the attempted Time Attack GPU
scenario failed during SDL display/GPU initialization in this environment.
Interactive failures now show their reason in the game before returning to the
title: settings, connection/handshake, archive/choice mismatch, load/presentation
failure, invalid snapshots/results and input/send timeout. Cancellation remains
a normal return. Session cleanup releases the queued frame, race, socket and
archive and restores the previous renderer before acknowledging an error.
Results and errors share one acknowledgement loop in the menu module; its
scripted test verifies the error text and fresh confirmation, with no time
formatting or network/GPU dependency. Game/smoke, menu/protocol/frame checks and
menu ASan/UBSan pass; live appearance remains unverified.
`DrawHostMenuFrame` reuses normal boot's frame buffers/font without dispatching
scenes or ticking race/save/audio state; the frame test verifies that boundary.
Selection/results now live in `src/port/mp_menu.c`, separate from network,
race resources and GPU ownership. `mp_menu` runs those production screens with
scripted frame/input and small metadata-reader fixtures, reusing the production
catalog mapping and picker state operation. It checks wrapped car selection,
labels/grades, AT/MT and manual-only behavior, one metadata scan, spec reads only
on car changes, cancellation/invalid metadata, both local-seat result labels,
retired rows, held-button rejection and explicit acknowledgement. The time
formatter has its own existing unit test; this fixture checks the values passed
to it. No socket, SDL, GPU or disc import is required. Full/headless Release and
ASan/UBSan pass, and the prepared CI jobs now build this target. This tests menu
behavior, not live rendering or the complete lobby.
Startup's temporary INI command passes noninteractive mode because it has no
booted menu assets; that path retains configured choices and automatic ready.
Game/smoke builds and protocol/frame tests pass. Live picker appearance/input
and renderer transitions remain unverified.
It temporarily enables the modern
renderer and restores the previous renderer during cleanup. Input is initialized
by the caller rather than resetting controller state on menu entry.
Returning consumes pressed/repeat edges from the session while retaining held
buttons, so START+SELECT cannot immediately activate the title screen. The
loop regression checks this boundary without loading a race or opening sockets.
Five menu/navigation/loop tests pass, and the label's reveal bounds pass
ASan/UBSan. Game and smoke builds pass; live appearance, return-to-title asset
restoration and window/renderer transitions remain unverified. The temporary
startup command below still exits after its run.
CPU rasterization of the actual emitted label tiles exposed a low-contrast
multiplayer glyph color. It now uses black like the existing rows; tests check
both selection palettes and ASan/UBSan still passes. This verifies the label
pixels, not its compositing over the live title background. All seven rows in
both states emit 8492 tiles in total, well within the 8 MiB frame primitive
buffer for a single menu state.

The temporary windowed connection command can now be cancelled using the
normal PAD_CANCEL mapping while connecting/handshaking or reporting loaded.
START+SELECT exits the race without treating normal brake/steering controls as
cancel. Cancellation uses the existing resource cleanup and returns success
to process startup. This command still exits the game process; returning to
the future lobby is not implemented. Six duplicate setup polling loops are now
one explicit connect/hello/welcome/pick/ready/start sequence, retaining each
transport operation's nonblocking state/deadline. Game build and existing
protocol/socket tests pass. Input cancellation and GUI behavior are not yet
verified in a live window; asset preparation remains synchronous.

Multiplayer is a new screen in the C game, drawn with the port's existing
8x8 font, the same way the force-feedback row was added. It does not need
new art from the disc.

The screen remembers the last server address. From there the player sees the
room list, joins with a code, or creates a room. Creating a room is the
course, the class and the seat count. Inside a room each player picks a car
and sets ready. When the server starts the race, the client loads that
course and stops listening to local menu choices for the session.

## Race step

The stream race fixture now also disconnects either seat after tick 100, in
both forward and reverse races. Shutdown joins the actual input reader before
the production room step observes the loss. Only present peers consume
snapshots/results; the other seat retires, and the survivor finishes first.
C result matching checks the retired sentinel and the final snapshot. Completion
captures names from the actual stream session, including the disconnected seat.
The closed writer reports failure without blocking the survivor's result.
All 50 unit tests, two startup tests and six PAL integrations pass in Release.

`step_room` now owns one production input/retirement/simulation/snapshot step;
`run_room` handles the 50 Hz pacing; `finish_room` rejects unfinished races,
captures the owned completion, publishes results and joins writers. The full socket-pair
race integration calls this same step without pacing, rather than consuming
inputs and stepping simulation through a parallel test implementation. Readers
publish inputs, the production step consumes them and publishes snapshots, and
clients receive those queued snapshots and results; the fixture no longer
republishes either. Two asset-free tests cover loss of every input (one C step
retires the field without track data) and refusal to publish unfinished results.
All 52 unit tests, two startup tests and six PAL integrations pass in Release. This does not replace
real-time network/window verification.

On Unix, an additional legal-PAL integration sends driving inputs, every
snapshot and final result of complete forward/reverse races through two
actual socket pairs. Inputs use the production C encoder, `read_client`,
SharedInput admission/consumption and the simulation's decoder. Header/payload
are split across writes. A test-only Read adapter acknowledges at the next
header read, after publication, so accelerated simulation needs no sleeps
or state polling; received control bytes must match the encoded input.
Each seat uses the production Outbox and `write_client`; both peers verify the
received bytes, and the received packet is decoded/applied by the existing C
client checks. Final writers must report success and join. Three-second I/O
deadlines bound failures. This reuses the existing race fixture rather than
duplicating its simulation loop. All 50 unit tests, two startup tests and five
PAL integrations pass in Release. This does not prove real-time input pacing,
TCP, lobby UI or
two-window rendering. Windows does not register this Unix-stream integration.

The bounded outgoing queue and its two lifecycle tests now live in
`server/src/outbox.rs`, with no simulation/protocol/socket dependency. They
can be compiled directly with `rustc --edition=2021 --test
server/src/outbox.rs -o <test-binary>` and run without Cargo, bindgen or C
libraries. The server uses that same implementation; the blocked-write adapter
test remains alongside actual socket/session integration. Both standalone tests
and all 48 server unit tests, two startup tests and four PAL integrations pass.

Reader and result-writer ownership both use optional per-seat slots now;
the session no longer allocates dynamic worker lists. Completion takes and
joins each present writer exactly once, including sparse fields, and reports
both I/O failure and panic before teardown. Previously an ordinary failed
socket write ended the worker normally and was incorrectly counted as success.
The worker now returns its write outcome. Socket/asset-free tests check success,
failure, panic and repeated drain; a failing `Write` adapter exercises the actual
result-writing path. Closing a queue before its result is consumed also reports
failure; the writer succeeds only after consuming and writing the final result.
A narrow in-memory test checks discarded and successfully written results.
All 50 Rust unit tests, two startup tests and four PAL
integrations pass in Release. This records local write completion, not peer
receipt, and preserves authoritative race results even when sending fails.

Input lifecycle now distinguishes loaded assets from a running session.
`start` consumes the loaded stage once and starts the input-loss clock;
repeated start/loaded/ready/pick operations cannot reset that clock or reopen
the lobby. Buffered controls while another player loads are neither consumed
nor expired as if the race were already running; `take` requires Racing and
leaves pre-start shift edges intact. Reader fixtures now explicitly start the
loaded input before consuming it, and malformed/truncated reader checks inspect
the unchanged packet directly rather than obtaining fake waiting-state controls.
Timestamp-driven tests check these boundaries without sleeping; 48 Rust unit
tests, two startup tests and four PAL integrations pass.

The coordinator assigns each started room a monotonic process-local identifier,
retained in its completion record and worker name. Removing/reordering active
rooms cannot change that identity; rejected admissions consume no identifier,
and exhaustion fails rather than wrapping. Ownership/capacity tests cover
completion identity, reuse prevention and exhaustion. This is not yet a room
code on the wire or a persistent SQLite identifier across server restarts.

Running-room workers return an owned `Finish` containing raw driver names and
authoritative finish tuples and the frozen room Plan instead of a process exit
code. `load_session` returns the exact Plan frozen from both clients' choices;
that value travels with the room and completion, preserving course/class/laps/
direction and actual car/transmission/seed selections. The same tuples
build the result packet; writer failure is recorded separately and does not
discard a completed race. `Rooms::reap` returns completed records exactly once
to the coordinator after joining their workers; capacity checks are read-only.
The coordinator reports them explicitly, including while accept/hello/ready/loaded
gates are pending, so an idle listener does not indefinitely retain completed
rooms. Setup polling calls the coordinator without holding seat locks and does
not renew stage deadlines. Per-seat hello keeps its absolute five-second
deadline while servicing the coordinator. Narrow tests cover hello/ready/loading/cancel
and receiving an actual worker completion through the accept poll.
Shutdown still cancels/joins remaining owners. An asset-free ownership/reaping
test and the real forward/reverse
race integrations verify that names/results survive releasing both race and
session. SQLite persistence remains unimplemented; this is its completion-data
boundary, not durable storage or proof of peer receipt. CTest passes 43 unit
tests, two startup tests and four PAL integrations.

Session input/outbox ownership and running-room inputs now use arrays sized by
the protocol's seat count. They cannot represent a missing entrant merely by
shrinking a Vec. Plan freezing also locks a fixed array in seat order instead
of allocating a temporary guard Vec; redundant count checks are removed.
Sockets and worker handles remain vectors because setup can be only partially
connected/spawned, and the existing cleanup still owns those partial resources.
Existing independent-seat, atomic-freeze, partial-session, capacity and worker
shutdown tests pass, along with all three legal PAL integrations (44 Rust tests
and two startup tests). No protocol or room-capacity change is implied.

The windowed client now accepts decoded results only when they match its latest
authoritative finished snapshot: every finished result must correspond to a
finished car, and every non-finisher to a retired car. Results arriving before
any snapshot, during racing, or contradicting either entrant cancel the session
instead of reporting success. The comparison is a pure function separate from
wire decoding and rendering. Narrow tests cover mixed finish/retirement,
missing input and contradictory phase/status. Game builds, protocol/socket tests
and protocol ASan/UBSan pass; live windowed TCP completion remains unverified.

The windowed client now receives before sending input, and stops sending once
an authoritative finished snapshot is applied. The server closes a connection
after draining its result; writing first could fail and discard a result already
waiting in the receive buffer. This also preserves the final-snapshot/result
two-poll sequence. A local socket-pair regression sends both packets and closes
the peer before polling, then verifies final state followed by the retained
result. The game builds; protocol/socket tests and socket ASan/UBSan pass.
The fixture verifies transfer behavior, not a live windowed TCP race.

A Unix-only local stream-pair test exercises the production generic read/write
adapters with actual OS I/O and bounded socket timeouts. Fragmented hello,
pick and ready messages freeze the selected car/transmission through the real
session gate; loaded enables input, whose exact decoded controls are observed
at EOF before cancellation clears them. Independently, the writer drains an
exact snapshot followed by the result over the other stream direction. This
requires neither a bound port nor imported assets. It covers adapter transport,
not the TCP worker cancellation wrapper or delivery to a disconnected client.
All 42 Rust tests including both legal PAL tests, and two startup tests pass
on macOS. Windows and live TCP/session integration remain unverified.

Process startup/import/listener setup is separate from `load_session`, which
owns one pair's handshake/loading resources and returns a prepared race/session.
Failures drop that session and leave existing rooms running. `Rooms` retains
worker handles, reaps completed workers and closes their inputs before joining
on removal/shutdown. The prototype caps running rooms at 16 and checks capacity
before accepting a new pair or preparing its assets. While full, it closes new
connections before hello/start and polls completed rooms every 100 ms so capacity
can recover without another connection. Starting a worker also checks the limit
defensively. A narrow test fills the manager with owned waiting workers, verifies
rejected-session cleanup and recovery after one worker completes, leaving other
rooms usable. All 38 Rust unit tests and two startup tests pass. An explicit
protocol-level busy response belongs to the future lobby; live TCP rejection is
not verified by this manager test.
Rooms are automatically paired, with fixed course settings; there is no room
code/list/SQLite yet. Archive preparation stays on the listener thread; only
independent copied simulations cross into workers, without sharing an archive
through an unsafe Sync implementation. Narrow tests verify owner shutdown and
reaping a failed real C step. All 37 Rust unit tests and two startup tests pass.
An explicitly ignored integration test was also run with a legal PAL image:
two distinct courses/classes/directions/models/transmissions/seeds produce the
same 1000-tick snapshot/RNG traces in parallel as in separate sequential runs,
after freeing the archive. This is CPU simulation evidence, not live TCP/GPU
concurrency, complete races or a throughput guarantee.
An additional explicitly ignored legal-disc integration test now completes
one-lap, two-human races in both directions through the Rust adapter. It uses
the existing C steering guidance on a copied car to produce validated human
inputs, not direct position/lap changes. Both entrants finish with unique places,
positive times and a winner time no greater than the runner-up's. The result
packet's type, both statuses/places and little-endian times match actual C
results; a further step is rejected and the serialized result remains unchanged.
Run with `RAGE_SIM_DISC_BIN` set to a legal image and
`cargo test --manifest-path server/Cargo.toml --offline -- --include-ignored`.
All 41 Rust tests (including both legal PAL integration tests) and two startup
tests pass. This verifies CPU race completion and serialization, not TCP
delivery, client rendering or full multiplayer gameplay.
The C result decoder also rejects finish times that contradict distinct places
among finished entrants. C `RaceTime` uses the shared elapsed clock captured at
finish, so a lower place cannot have a greater time. Equal times remain valid
when multiple cars finish within the same tick. Narrow protocol tests cover
both seat orders, equal-time acceptance and unchanged output on rejection;
protocol/socket tests and protocol ASan/UBSan pass. Retired entries and future
gaps in places are unaffected; this does not validate delivery against snapshots.

The server advances each racing room's clock at 50 Hz. Current PAL physics
runs every second tick (25 Hz); inputs retain their latest levels and pending
gear edges until that physics step consumes them. A snapshot carries each
car's position, heading, speed, and the race clock. Region normalization must
be verified before sharing a room between clients from different regions.

Snapshot encoding now fills a fixed stack packet instead of allocating a Vec
and then copying it into Arc storage every tick. The immutable packet receives
one shared allocation for every connected seat; finish results are also built
once and shared. A narrow test checks the full header and both seat boundaries
against explicit bytes, plus pointer identity through independent outboxes.
Result encoding also uses a fixed stack packet instead of an intermediate
Vec. Its pure encoder accepts finish values separately from Race; the narrow
fixture feeds its bytes to the actual C decoder, covering both seat orders,
retirement, distinct finish places and the saturated i32 time.
The existing nonzero pose fixture remains unchanged. All 35 Rust unit tests
and two startup tests pass. This proves wire/ownership behavior, not a measured
server throughput or frame-rate improvement.

Server packet validation and delivery use one `DriverInput` decoder instead
of separately reconstructing controls through a ten-argument simulation
adapter. The decoder uses the shared C validator and requires the reserved
byte to be zero. Narrow tests cover signed steering, pedal boundaries and all
nonzero reserved-byte values; rejected packets preserve pending shift edges.

The prototype schedules ticks against an absolute monotonic deadline. A late
step retains its timing debt and catches up without sleeping; it does not
reset the time axis or skip simulation steps. Previously each overrun moved
the deadline to the current time, permanently losing elapsed race time.
A narrow scheduler test uses supplied timestamps, without sleeping, sockets
or imported assets, and verifies recovery from a delay plus 1000 drift-free
deadlines. Sustained overload still requires capacity/load management; this
does not claim that the server can maintain 50 Hz at arbitrary load.

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

### Current prototype wire

Version 24 transmits a complete 7317-byte publication: a 6375-byte type 0x87
correction envelope immediately followed by its 942-byte type 0x83 snapshot.
The server owns/coalesces/writes the pair as one payload. The game receiver waits
for both packets, cross-checks them and applies the checkpoint before returning
the paired presentation snapshot. A malformed second packet cannot commit the
checkpoint. Diagnostic receivers report the snapshot after envelope binding
checks and never execute checkpoint physics. Polling drains at most 32 complete
publications per call, retaining partial bytes and deferring results until the
final accepted state has been returned. Initial bounded prediction is now enabled.

Version 22 adds numbered driving commands: type `0x09`, a nonzero increasing
little-endian u32 sequence and the existing twelve-byte input body (17 bytes
including type). The normal and diagnostic clients use this packet. An input
gets its sequence when it becomes the retained outgoing packet; replacing the
newest queued controls does not consume sequence numbers or rewrite an in-flight
prefix. Pending gear edges remain merged. Sequence exhaustion fails instead of
wrapping to zero; a failed blocking write is terminal, preventing a retry after
a partially written packet.

Server admission validates the complete input and seat stage before advancing
its sequence. Duplicate, regressing, zero, malformed and truncated commands
cannot replace controls or refresh their timestamp. The old unnumbered input
packet remains available for raw protocol fixtures, but is rejected after a
numbered command has been accepted. Receipt and input sampling do not advance
the applied sequence; the actual simulation step described below does.
Pure byte fixtures, fragmented/blocked socket-pair tests and two narrow Rust
handoff/reader tests cover this boundary without importing a disc or stepping
physics. Release and ASan/UBSan pass; all 70 Rust units, two startup checks and
eight legal-PAL integrations pass. Live TCP/GPU verification remains outstanding.

Version 23 appends two little-endian u32 applied-input sequences after the
snapshot's twelve poses. Zero means no numbered command has been consumed.
Each owned human driver records `inputTick` when countdown drivetrain or racing
physics actually consumes controls. Intermediate 50 Hz ticks retain the previous
acknowledgement: PAL physics consumes inputs at 25 Hz. The server samples controls
and their sequence together, then advances the acknowledgement only after a
successful C step whose `inputTick` matches the resulting race tick. Commands
received concurrently afterward cannot be acknowledged by that step. A retired
or finished driver's rejected controls do not advance it; failed simulation
publishes neither a new acknowledgement nor a snapshot.

An acknowledgement covers the latest control levels and merged gear edges
through that sequence, not a separate physics step for every received packet.
The transport adapter and presentation history reject regressing acknowledgements
without publishing caller output. The mutable local checkpoint saves/restores
`inputTick` and rejects future values. Pure C fixtures and fragmented socket pairs
cover trailer decoding and regression; a Rust-to-C golden uses two nonzero values.
The PAL step integration covers odd/even countdown lengths, classes one/six,
forward/reverse, retired seats and failed stepping. Game/smoke builds, C Release
and ASan/UBSan, 70 Rust units, two startup tests and nine PAL integrations pass.
The client now owns a 256-command ring of complete transmitted controls. Pending
or partially written packets are not recorded, and coalesced input retains the
exact levels/gear edges that were written. Local-seat acknowledgements remove
only the acknowledged prefix; future or regressing values reject atomically.
The Welcome assignment binds the local seat before acknowledgements can prune
its history. When the ring is full, polling retains the newest queued controls
and pending gear edges, and resumes transmission after acknowledgement. It
never silently overwrites unacknowledged history or allocates per frame.

Pure command-ring tests cover full capacity, repeated wrap, bounds, malformed
controls, duplicate/future acknowledgements and u32 exhaustion. Socket-pair tests
exercise 10,000 queued updates while full, partial transmission, resumption and
rejection of an acknowledgement beyond the completed writes. Validation lives
in a small shared C source, so protocol tests do not link the race step merely
to check command bounds. Input sampling is rejected before any wire bytes or
queue mutation when its native controls are invalid.

Completed/retired drivers stop sending new controls while waiting for results.
Their connected seat no longer expires for lack of driving input; explicit
reader closure still closes it. A clock-injected Rust test covers waiting past
the input deadline and disconnection without sleeping or importing a disc.
C Release and ASan/UBSan, 71 Rust units, two startup checks and nine PAL
integrations pass. This is transmitted-command history, not a client prediction
clock or replay schedule. Full correction-state encoding and prediction are
still missing; live TCP/GPU and other platforms/regions remain unverified.

Current protocol is version 23. Version 16 added the complete field; version 17
added creator-selected race options, version 18 added explicit room routing,
and version 19 extends Welcome to eleven bytes with the assigned room code.
Version 21 adds the lobby update described above. Version 20 added pre-join room listing: `0x08` requests a list after Hello,
before the room request. Response `0x85`, version, count is followed by count
15-byte records: u64 code, class/course/laps/reverse, occupied mask, ready mask,
state (0 open, 1 preparing, 2 racing). Count is bounded at 16, codes are unique
positive signed-64-bit values, and Ready is a subset of occupied seats.
Settings/readiness are copied under the same ordered seat locks as plan freezing;
no room lock is held during encoding or writing. Concurrent browse requests are
coalesced into one pending response. Successfully browsing extends the owned
decision deadline to 60 seconds; an already expired connection cannot renew it.
Open and active rooms are listed in code order; active owners never advertise
themselves as open, and completed rooms disappear when reaped. C decoding
validates the full packet before committing any room/count. Narrow tests cover
coherent owned views, readiness/loading/disconnect, request admission/coalescing,
empty/full lists, duplicates, truncations and bounds; a Rust encoder test passes
its actual packet through the compiled C decoder. Release and ASan/UBSan pass.
The windowed browser and transport adapter are connected as described above;
TCP and live directory browsing remain unverified.
Start is 125 bytes including type: the previous
55-byte human/source header followed by ten seven-byte AI descriptors
(`active`, logical model, authored behavior slot, u32 seed). Inactive descriptors
must be zero; active models/slots are bounded and behavior slots must be unique.
Their field/grid positions are 2..11. Snapshot is 942 bytes including type:
tick/elapsed/phase plus twelve 77-byte poses and two u32 input acknowledgements. The 13-byte two-human result packet is unchanged;
input numbering is described above. Older versions are rejected.
Small C tests cover a last-seat AI setup/application, preservation of overlapping
AI storage, empty slots and atomic rejection of duplicate/invalid metadata.
Rust/C golden packets include the last field seat. Game/smoke builds, Release
and ASan/UBSan C multiplayer checks, 68 Rust unit tests, two startup tests and
seven legal-PAL integrations pass. TCP integration skips because binding is
prohibited here. The following version entries are implementation history.

Protocol version 13 adds authoritative elapsed race ticks (u32) after the
simulation tick in each snapshot header. Header body is tick/elapsed/phase
(nine bytes); the two-seat packet is now 148 bytes including type and two
69-byte poses. C applies elapsed directly to RaceSim rather than deriving race
time from local countdown assumptions. Validation rejects elapsed greater than
tick, nonzero elapsed in setup/countdown and regressing elapsed in history,
state application or a coalesced receive batch.
Final-result validation also rejects finish times beyond the final snapshot's
elapsed clock. Conversion uses widened arithmetic and permits the C result
clock's i32 saturation. Tests cover the exact boundary, one millisecond beyond
it, a negative finish time and the saturated clock.
Narrow tests cover actual
nonzero clock application and atomic invalid-clock rejection; the Rust header
golden includes distinct nonzero tick/elapsed bytes.
Client snapshot application also advances its remaining countdown by received
tick differences, including skipped snapshots, and clears it in racing/finished
phases. A countdown-phase packet at or beyond the known start tick is rejected
before any state changes. A narrow test covers skipped ticks, exact transition
and subsequent authoritative elapsed time; no local simulation step is used.
Game/tool builds, five
related C tests, protocol/socket ASan/UBSan and all 44 Rust tests plus two startup
tests pass, including all three PAL integrations. Older versions are rejected.

Protocol version 12 adds signed longitudinal speed in simulation units as the
last i32 of each pose. A seat is now 69 bytes (status plus seventeen i32 fields),
and the two-seat snapshot is 144 bytes including type/tick/phase. The Rust server
reads actual car speed; C decoding applies it to the authoritative car and remote
presentation interpolates it without modifying simulation. Previously speed
stayed at its local initial value, unsuitable for speed-based HUD/effects.
Matching nonzero C/Rust fixtures cover signed bytes and both seat boundaries;
C tests cover state application and interpolation. Game/tool builds, all five
related C tests, protocol ASan/UBSan and all 44 Rust tests plus two startup tests
(including three PAL integrations) pass. Older protocol versions are rejected;
live HUD/GPU/network behavior still needs verification.

Snapshot decoding, history insertion and authoritative-state application now
share one pure payload validator. A finished race cannot contain a driving
entrant; retired and finished entrants remain valid. Narrow tests inject that
contradiction in either seat and verify unchanged decode output, history and
simulation, then accept consistent final states through all three paths. The
existing phase-acceptance fixture now uses a finished car in the finished phase
instead of accepting the contradictory state. Game/tool builds, all five related
C tests and protocol ASan/UBSan pass. This preserves the wire layout and does not
add transport or simulation dependencies to the pure test binary.

The headless driver now sends its initial input once, then receives before
subsequent writes and stops sending after a finished snapshot. It applies the
same history ordering and final-result consistency gates as the windowed client.
Reaching its requested snapshot count explicitly reports sampling completion;
only a validated result reports race completion. The TCP process fixture now
includes final snapshot/result followed by peer close and a valid result that
contradicts retired entrants. Tool and fixture build; five related unit tests
pass. Re-running TCP integration still returns skip 77 (`Operation not permitted`
at listener setup), so the added process scenarios are not runtime-verified.

Pure packet/settings/setup validation and presentation-history operations now
live in `src/port/mp_protocol.c`; socket lifecycle, transfers and deadlines stay
in `mp_client.c`. Public APIs and wire bytes are unchanged. The protocol test
links only the pure module, bounded parser and archive fingerprint helper; its
undefined-symbol audit contains no socket, select or clock calls. Existing
connection checks move intact to a separate `mp_connect` test (including Windows
socket linking), while socket-pair fixtures link the pure module explicitly.
Game/tool builds, all five related C tests and protocol/socket ASan/UBSan pass.
This reduces unit-test dependencies without removing transport coverage or
introducing a second implementation of decoding.

Blocking race-message receive (used by the headless C client) now has one
absolute deadline per complete packet: 65 seconds before the first decoded
snapshot and five seconds thereafter. Partial reads and interrupted retries
preserve that deadline; timeout is a terminal connection failure and leaves
decoded output unchanged. Polling receive remains nonblocking and the windowed
client keeps its existing snapshot watchdog. Narrow socket-pair tests inject an
expired deadline after a partial header and exercise a two-millisecond select
timeout without waiting for production limits. Client/tool builds, protocol/
socket tests and socket ASan/UBSan pass. Actual TCP stalls remain unverified.

The prototype has a versioned welcome and start; it does not implement the
complete lobby message set above. All integers are little-endian. Welcome is
type/version/seat (3 bytes). Version 15 start was type plus a 54-byte body:
version, course, class, laps, reverse (one byte each), countdown (u32), seat
count (one byte), boot serial (16 bytes, NUL-terminated), then two entries
containing model/manual (one byte each) and seed (u32), followed by a u64
archive-content fingerprint and a u64 executable fingerprint. Unknown versions and
invalid counts/selectors/flags are rejected without changing decoded state.
Client and server use matching byte fixtures for this exact layout.
The server encodes start into one fixed-size array, without growing a Vec.
Its asset-free golden fixture also decodes that array through the production
C API and checks both content identities and the boot serial.
Rust packet encoders and their three narrow golden/decoder tests now live in
`server/src/protocol.rs`, separate from connection and room coordination.
The existing real-race and stream tests continue using these same encoders;
this extraction changes neither wire bytes nor the available lobby messages.

The windowed prototype builds its field through the pure `MpBuildSetup`
adapter, sharing start validation with the wire decoder. Its narrow test checks
models, manual modes, seeds and reverse/class/course/laps, empty remaining
seats and atomic rejection without importing assets or starting a race.
It loads the selected pack/models, initializes the shared
seeds and countdown, and rejects empty/differing boot serials or archive fingerprints. The fingerprint
is FNV-1a over every RAGE.BIN byte, computed by the same C implementation on
both sides, and catches accidental source differences. It is not authentication
or proof of agreed car overrides. An additional nonzero executable fingerprint
is checked to distinguish source revisions; agreed car specifications still
must precede prediction. The current server
uses retail specifications with no room overrides. Input retains its prototype layout. A snapshot seat now occupies 77 bytes:
status (u8: retired 0, driving 1, finished 2), followed by nineteen i32 values: X/Y/Z, yaw, pitch, roll, steering,
wheel rotation, brake, track progress, RPM, throttle, clutch, gear, ground height, roll velocity, speed, lap and place.
Version 15's two-seat snapshot was 164 bytes including type; version 16 contains
all twelve seats (934 bytes). Lap is authoritative
and discrete in interpolation; applying it rejects negative/oversized values,
regressing laps or laps beyond the configured race, atomically across both seats.
Golden C/Rust packets and the real PAL race comparisons cover the new field;
52 Rust unit tests, two startup tests and six integrations pass in Release.
Place is calculated by the server's `RacePosition`, not reconstructed from
the client's incomplete progress fields. It updates the client driver's place,
including zero on retirement; a narrow transition test and complete stream
races cover this. Retired poses must have zero place; all places are bounded
by the retail field limit.
The owned ClientFrame now carries two HUD text rows. The multiplayer presenter
fills them from received rank/lap/gear and race elapsed time, using the existing
disc-region speed conversion (NTSC-U MPH, otherwise KPH). The renderer submits
solid bitmap-font pixels over the native world, independent of PS1 font atlases;
font data comes from the vendored SDL public-domain bitmap font. It retains no
race/menu globals for HUD presentation. The shader still needs a sampler binding;
it reuses an owned snapshot texture, copying one only when absent, without font
texture sampling. Frame resources own the text throughout presentation.
Headless menu tests cover explicit rank, lap completion, gear/speed/time text,
invalid/retired seats and both local-seat labels. HUD geometry uses the existing
overlay batch module, combining horizontal glyph pixels into rectangles. A narrow
CPU test reconstructs an actual glyph from emitted geometry, checks the 36-character
limit and widescreen placement, and verifies atomic publication on buffer exhaustion.
The tested exclamation glyph uses 72 vertices including shadow instead of 192
for individual pixels; this is a geometry reduction, not a measured FPS improvement.
Full game/smoke builds, overlay geometry tests and menu
ASan/UBSan pass. Live composition, readability and FPS remain unverified; this
does not provide individual lap times, audio or the full retail tachometer yet.
Brake/throttle are 0..256; clutch fits i16 and gear is 0..6. There is no compatibility
fallback for older protocol versions. Rust unit tests, server-startup process tests and the C
protocol/argument tests pass; the loopback integration test remains skipped
because binding a local port is denied. Builds do not prove a playable race.

## Simulation API

Prediction prerequisite: `owned_race_checkpoint_replays_pending_inputs_and_the_full_field`
encodes an owned RaceFrame between physics updates with an unconsumed gear edge,
steps 200 ticks, decodes the bytes into a separately prepared race with independent
track storage, restores it and replays the same inputs. Classes one/six and both
directions include authored AI/empty seats. Every encoded mutable state byte
matches at every subsequent tick, including RNG and hidden drivetrain/contact
state. This uses the production C codec and physics through Rust FFI and passes
on the legal PAL image; it is not client prediction or live network correction.

`SaveRaceFrame`/`RestoreRaceFrame` capture only mutable clocks, car/input/step/RNG/
status/lap state. Specifications, engine tables, hulls, thresholds and route data
remain in the owning RaceSim. A frame is smaller than RaceSim and contains
borrowed track/event identity guards: it is a local checkpoint, never wire bytes.
The operations now live in `race_frame.c`, separate from the race step.
`ValidRaceFrame` checks a complete frame without modifying it or its owner;
Save and Restore share this validation and leave their destinations unchanged
on rejection.

The separate `race_frame` test constructs scalar owned state without car, track
or engine initialization and never steps physics. Fifty malformed cases cover
owner/configuration, route indices, clocks, immutable model/transmission, gearbox/
motion/vertical states, pedal levels/latches, field roles, lifecycle flags and
finish count/unique places. It explicitly distinguishes AI storage from the
overlapping player gearbox. It checks exact whole-race restoration, unchanged
immutable fields and late-seat atomic rejection. The linked test binary contains
checkpoint/input validation operations, not StepRaceSim or InitDriver. This is a
smaller test boundary rather than another mocked full-race fixture.

An aborted countdown can legitimately finish with its unused countdown retained,
zero elapsed/finish count and no driving seats; validation preserves that existing
lifecycle. Racing still requires zero countdown. Per-seat lap/finish clocks and
lap times cannot exceed elapsed race time. The real forward/reverse PAL wire
integrations now save and validate every authoritative tick through complete
races, including terminal and disconnected seats. The pending-input/full-field
replay test still restores the C checkpoint and produces identical poses/RNG.
Game/smoke builds, C Release/ASan/UBSan, 71 Rust units, two startup checks and all
nine PAL integrations pass. Transmitted commands and their applied-sequence
acknowledgements are retained as described above.

`MpEncodeCorrection`/`MpDecodeCorrection` provide a 6375-byte
correction envelope: type 0x87, envelope format 1, two little-endian u32
applied-command sequences, and one complete checkpoint. Decoding binds the
checkpoint to the receiving race's resources and commits the state and both
acknowledgements together only after full validation. Nonzero acknowledgements
require consumed human input in that checkpoint. This does not authenticate
sequence values: the receiving session must also validate them against its
transmitted-command history before restoring state or pruning commands.
Narrow synthetic tests cover exact bytes, independent resource ownership,
roundtrip restoration, every truncated size and atomic late-field rejection.
Release and ASan/UBSan protocol/socket checks pass; game/smoke builds pass.
`MpApplyCorrection` additionally validates the local acknowledgement against
the transmitted-command ring before changing either the race or that ring.
Future/regressing acknowledgements, invalid late-seat state, mismatched track
ownership and malformed history reject without changing either destination.
Confirmed commands are pruned; pending commands remain for replay. The legal-PAL
integration now uses this correction envelope and application path for classes
1/6 in both directions: command 38's pending gear edge survives acknowledgement
37, and all encoded state matches for the next 200 ticks in an independently
prepared race. It does not yet replay the command ring on a prediction clock.
The socket adapter now uses `MpClientPollRaceState` reception in the windowed
race loop. It retains a fragmented publication in bounded per-connection storage, validates the whole
checkpoint, both acknowledgement directions and the local sent-command limit,
then applies state and prunes history atomically. Its authoritative correction
clock rejects duplicate/regressing ticks independently of the client's mutable
simulation clock. It coalesces at most 32 publications per call (4 means an
applied checkpoint plus its paired snapshot), preserving final-state/result
ordering. The shared diagnostic receiver does not apply physics, and explicitly
checks only envelope/clock/ack binding before reporting the paired snapshot.
Socket-pair tests send all 7317 bytes separately and
check unchanged output/race/commands until completion, plus terminal rejection
of repeated ticks, future local acknowledgements, remote acknowledgement
regression and invalid drivetrain. Release and ASan/UBSan checks pass.
`Race::state_message` builds a single owned publication containing correction
first and snapshot second from the same post-step race and acknowledgements.
Its fallible checkpoint encoding completes before returning a publication.
The PAL correction integration checks both packet clocks/acknowledgements and
passes the complete publication through the production outbox and writer before
results. `step_room` now publishes this payload, enabled by protocol 24.
`MpDecodePublication` decodes both packets into temporary owned
outputs and rejects mismatched clocks, acknowledgements, entrant status and
transmitted car/drivetrain fields before publishing either output. Finished
places must match the checkpoint; running HUD ranks remain snapshot values.
Synthetic tests reject all truncated publication lengths and independently
valid but inconsistent late-seat snapshot fields without changing either output.
The PAL correction/replay integration now uses this complete-publication decoder
on bytes produced by the real writer before applying its checkpoint.
The outbox must replace the whole pair when coalescing slow clients;
publishing its packets separately could pair different ticks or lose a checkpoint.
Full PAL forward/reverse stream tests now decode/apply the production paired
publication to the independently owned client, including either seat disconnecting.
This is adapter/simulation coverage, not live TCP/GPU correction verification.
Prediction/replay clocks remain outstanding.

`MpClock` now maps explicitly supplied monotonic nanoseconds to fixed 50 Hz
prediction targets. Ordinary authoritative observations retain the origin;
an overtaking server reanchors it. Targets cannot trail the latest authority
and freeze at ten ticks (200 ms) ahead during missing updates. The pure tests
cover exact tick boundaries, 24/30/50/60/144/240 Hz sampling, delayed observations,
clock regression, malformed state and saturating u32/u64 limits without sleeping.
The windowed client observes accepted publication clocks and stamps each input
with this mapped tick. Completed transmitted commands retain their sample tick;
an in-flight packet keeps its original tick while the newest queued sample
replaces older queued levels and merges gear edges. Ticks cannot regress even
after acknowledgements empty the ring. The existing saturated socket fixture
now checks original/latest ticks across 10,000 queued replacements and a partial
packet prefix. This supplies replay timing, not yet predicted physics or replay
execution. Tick stamps remain local; the authoritative server consumes controls
according to its own receipt/step schedule, not a client-provided time.

`MpReplayInput` supplies the fixed-step input selection: its local cursor starts
at the acknowledged sequence after correction, selects due command levels in
order, merges unconsumed gear edges and leaves future commands pending. It does
not mutate transmitted history or consult a device/clock. The caller supplies
the simulation's post-step input on the next call, so consumed edges cannot
repeat; an edge retained by a skipped physics step remains pending. Complete
history/cursor validation precedes either output change, including malformed
future entries. Narrow tests cover ring wrap, overdue/same-tick/future samples,
held levels, pending/consumed edges, acknowledgement resets and atomic rejection.
The PAL correction test now advances the restored race for 200 ticks using this
selector for the local human, comparing every encoded mutable field against
direct input application in classes 1/6 and both directions. Release, sanitizers
and server integrations pass. This proves controlled replay input selection;
the windowed client still needs a separate predicted simulation and integration
of replay with correction, sampled-but-unsent controls and presentation.

`MpClientPendingCommands` now exposes copies of the at-most-two unsent samples
without changing socket/history state: immutable in-flight command (including
its original tick) followed by newest queued controls (sequence zero until
assignment). The bounded accessor does not move unsent data into acknowledged
history and reports nothing after terminal connection failure. The saturated
socket test checks one/two/zero pending samples, retained gear edges across
10,000 replacements, the original tick after a partial transmitted prefix and
unchanged owner/output where applicable. This supplies the remaining local
samples for prediction; the runtime still needs to consume them in its separate
predicted simulation.

`MpPredictRace` now rebuilds a separate caller-owned scratch simulation from
the latest authority, replays unacknowledged commands and applies in-flight/
queued samples once at their due ticks. It runs the actual complete C field
(including AI/collisions), at most ten ticks ahead, without changing authority
or command history. Track/events remain immutable borrows from the live race
owner; scratch never frees them and is destroyed first. It is a separate static
archive object, so pure protocol tests still do not link StepRaceSim/InitDriver.
The windowed loop uses one allocated scratch context for predicted local car
transforms, chase camera and displayed speed; remote cars retain delayed history,
and status, timing and results remain authoritative. It advances one tick past
the sampled clock within the lead limit to preview controls between updates.
Every rendered frame currently rebuilds bounded prediction from authority:
this is a simple first implementation, not a measured performance improvement.
Lamp/presentation smoothing and live latency/performance still need validation.
The PAL correction integration verifies byte-identical predicted next-step state
with a pending gear edge and an unsent level sample in classes 1/6, both directions,
and confirms authority remains unchanged. It now also compares every prediction
lead from 1 through 10 ticks against a separate direct C simulation holding the
same input: every encoded field, including AI/RNG/contact/drivetrain, matches.
Repeated rebuilding preserves authority and all retained command ticks/inputs,
and does not reapply consumed gear edges within a predicted trace.
The lead-1..10 comparison now runs with either human seat as the local driver:
seat zero uses the retail automatic car, seat one the selected manual car. The
other human and AI remain in the complete physics comparison, so local-seat
selection cannot accidentally become a hardcoded seat-one prediction path.
The same matrix now includes both unsent samples together: in-flight throttle/
upshift at the first future tick and queued brake/downshift at the third. All
leads 1..10 match direct C input application, including held levels before the
second sample and no repeated consumed gear edge after it.
The matrix also starts from countdown tick zero and crosses the countdown-to-race
boundary: either local seat receives throttle/upshift at tick 1 and brake/downshift
at tick 4. Every target 1..10 is byte-identical to direct C stepping, including
phase/countdown/elapsed clocks, complete field state and unchanged authority.
This verifies simulation transition/replay policy, not countdown GPU presentation.
Prediction now refuses an empty local seat and keeps the authoritative context
unchanged once the local driver has finished or retired, even while other
entrants are racing. There is no local motion to rebuild while waiting for
results. Narrow synthetic `mp_prediction` cases cover both terminal statuses
in a still-running race, alongside the already-finished whole-race case;
Release and ASan/UBSan pass without assets or physics stepping.
Countdown, held-input and two-unsent-sample cases now share one test-only direct
simulation/comparison helper instead of three repeated lead loops. Each scenario
still supplies its own initial state and samples. The common check additionally
preserves all command-ring metadata as well as ticks/inputs for every lead;
this strengthens the two-pending case rather than removing coverage. The nine
PAL integrations pass after the test refactor.
The existing 200-tick replay check
still passes. `mp_prediction` separately covers scratch ownership, bounds,
malformed pending samples/history and terminal-state preservation without disc
import, stepping physics or SDL; Release and ASan/UBSan pass. CI now builds this
registered target on all supported platforms, but the workflow has not run here.

Optional CPU prediction measurement reuses the legal-PAL checkpoint integration:
`RAGE_MP_PREDICTION_REPORT=/tmp/rage-prediction-cost.csv ctest --test-dir
build/sim-release -R '^server_retail$' --output-on-failure` writes CSV only when
that explicit report path is supplied. Each class-1/6 forward/reverse sample
warms 50 calls then measures 500 calls at leads 0/1/5/10, using Release C/Rust,
the real grid, a pending gear command and unsent controls. The native scratch
context is 17488 bytes on this macOS build. Observed means were 0.52–11.86
microseconds per rebuild; the largest individual measured sample was 59.63
microseconds. This small early-race CPU sample does not establish whole-course,
GPU/frame-rate, network-latency or other-platform performance, and maxima are
observations rather than a worst-case bound. The ordinary integration run has
no measurement loop or report file. The measured cost does not justify adding
a prediction cache before live presentation/network tests provide evidence.

SQLite persistence is still pending: the current local dependency cache and
repository contain neither an SQLite amalgamation nor a cached bundled-SQLite
Rust dependency. This does not change the requirement to compile SQLite into
the standalone server; a platform-installed shared library is not a replacement.
The latest download attempt for the official SQLite amalgamation failed DNS
resolution. UI inventory is accessible, but selecting/launching the built Rage
Racer app through the UI adapter was denied with "Computer Use was not approved
to use Rage Racer". Neither event supplies the missing dependency or live visual
evidence; SQLite and two-window GPU/TCP verification remain outstanding.

`EncodeRaceFrame`/`DecodeRaceFrame` implement checkpoint format 1: 6365 bytes,
with one format byte, seven u32/i32 frame fields, and twelve records containing
116 bytes of input/step/status/clocks plus 412 bytes of car scalar state. Lists
specify field order, width and count; compiled offsets locate native fields
only and are never transmitted. Numeric values use explicit little-endian
encoding, and unions select human or rival meanings rather than overlapping
both. The common car prefix is one schema reused by both tables; shared lap
storage uses typed fields even for AI. Explicit reserved fields/byte arrays are
retained for complete restoration; implicit struct padding is not encoded.
Specifications, engine tables, hulls, route/event pointers and local identity
guards remain in the receiver's prepared owner. Decode binds those guards to
that owner and validates the complete frame before publishing it. Encoding a
malformed owner likewise leaves the destination byte buffer untouched.

The narrow C test uses nonzero patterns across car storage plus signed values
and a u16 golden, verifies byte-exact reconstruction under another route address,
checks output canaries, every truncated length, extra bytes, every unknown format
version and late-seat malformed car/role fields. Release/ASan/UBSan pass along
with the PAL replay proof above. Protocol 24 binds these checkpoints and input
acknowledgements to their paired snapshots on the transport. The next step is
the client's prediction/replay clock. No cross-platform or live GPU/
network correction claim is made.

This is not implemented prediction. Current pose snapshots cannot restore a
physics checkpoint: drivetrain acceleration, component velocity, collision/
route history and RNG are not all transmitted. Prediction must also associate
authoritative state with acknowledged input commands, then replay only the
unacknowledged inputs through the existing C step. Adding speculative stepping
to the current pose-application path alone would retain stale hidden state.
The next implementation needs checkpoint delivery associated with applied
input sequences and replay timing for retained commands, separate from
interpolated remote presentation and authoritative results.


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

The PAL client-resource test now also expands retained frame meshes into actual
CPU draw vertices and spans through `RenderBuildNativeDraws`. It frames the
car with an explicit camera, requires nonempty triangle output, then compares
the output after original instance storage and race owners are released. This
passes ASan/UBSan; it does not prove GPU rendering or visual correctness.

`DecodeFrameSky` now decodes from the retained race's texture bank, captured
palette and explicit copied camera layout. Missing/invalid layouts fail instead
of consulting legacy state. PAL resource coverage checks repeat output after
original camera/palette mutation and race-owner release, and rejected requests
preserving output. The test supplies a diagnostic layout; extraction of the
authored per-course layout into the owned camera remains pending. The client
build and focused sanitizer tests pass; the GPU sky loader is not switched yet.

Frame capture now publishes the owned environment's sky bands, fog distances,
cloud row and panorama identity while preserving caller camera pose/clipping.
`RetailSkyLayout` derives the immutable retail row pattern without accessing
the legacy map; a narrow test compares all four legal selections to that map
and checks invalid selections preserving output. PAL retained-resource coverage
checks camera selection from the actual script. Live GPU adoption remains pending.

The GPU backend now exposes an explicit retained resource source, and
`PrepareClientFrameGpu` connects owned frame mesh/material/sky decoding to it.
A source switch clears geometry/template and texture caches before releasing
the previous owner; texture-bank or captured-palette changes clear textures.
Ray geometry also receives a backend resource generation. The benchmark keeps
the active source instead of accidentally reverting to the global provider.
`ClientFrame` has main-thread retention, with overflow and partial-release
coverage in the PAL sanitizer test. Client/tools builds pass. No playable
client calls this entry yet, and GPU output/cache-switch behavior still needs
execution with a display; these builds do not prove visual correctness.

The frame GPU adapter now depends only on the resource-source contract, not
SDL or a device. Image allocation is paired with the source's own `freeImage`
callback rather than assuming SDL allocation for every provider. The headless
PAL test drives the real adapter callbacks through a retained sink, checking
copied-instance mesh resolution, real material/sky images, failed requests
preserving outputs, repeat retention and owner replacement. It does not test
GPU cache invalidation or rendering. The client/tools build and PAL sanitizer
run pass; playable integration remains pending.

`SubmitClientTerrain` publishes the owned terrain grid into an explicit world,
with owned mesh identity, bank/environment variants and the existing authored
region masks. Masked cells remain ray-only, following production behavior.
It validates indices and capacity before replacing main-pass owned terrain,
preserving cars, other geometry and mirror entries. The narrow `client_world`
test checks coordinates, masking, replacement, independent worlds and atomic
rejection. The PAL constructor test publishes all 24 packs and resolves every
cell through the race owner. Sanitizer coverage and the client build pass.
Course scenery/dynamic objects and playable adoption remain pending.

Course-object validation is now shared by the legacy runtime installer and
`ClientRace`, through a bounded aligned `ReadCourseObjects` view into owned
scene storage. Invalid model IDs, flags and truncated tables are rejected.
`SubmitClientScenery` publishes static objects with owned model identity,
camera-relative wrapped coordinates, authored fog flags and region masks.
Replacement preserves cars, terrain and other course entities; invalid input
or insufficient capacity leaves the world unchanged. Narrow parser/world
tests and legacy asset-loading regression pass. All 24 PAL packs publish
terrain and exactly their active static-object count under ASan/UBSan; some
packs legitimately have no active decorations. The client build passes.
Dynamic scenery and playable client integration are still pending.

Shuttle playback now has explicit `InitShuttle` / `StepShuttle` operations
accepting caller-owned state, endpoints, angles and timing. Legacy init/update
adapters use the same implementation, preserving wrapped integer coordinate
interpolation and dwell/endpoint order. The new narrow headless test checks
known intermediate poses, 1000 interleaved steps against isolated playback,
failed-input atomicity and extreme coordinates. All 74 headless sanitizer
tests, PAL resource coverage and the three existing shuttle regressions pass.
Owned client adoption of authored shuttle configuration and dynamic publication
remain pending; flyby/path animation still use legacy state.

Retained-frame coverage now composes terrain, static course objects and the
full configured human/AI field on all 24 PAL packs, captures them together,
then releases original instance storage and the race owner before resolving
every retained instance. The two independent forward/reverse scenes also feed
the real CPU triangle builder; spans must include both cars and terrain.
Repeated vertices and the selected terrain material's RGBA remain identical
after owner release. This passes ASan/UBSan. It tests CPU composition/resource
lifetime, not complete dynamic scenery, GPU output or playable client behavior.

Authored shuttle tables now live in the shared data library, preserving their
legacy symbols/bytes without duplicate initializers. `RetailShuttle` copies
configuration into each `ClientRace`; current/previous poses and the animation
clock are per race. `TickClientScenery` advances only the next simulation tick,
is inert on duplicates, rejects skipped/rewound ticks atomically, handles tick
wrap and freezes motion in the final class. Narrow tests and all 24 PAL packs
exercise this path; legacy content/ABI and shuttle regressions pass. Dynamic
mesh publication and playable use are still pending.

`SubmitClientShuttles` now publishes each owned shuttle's current/previous
pose with stable semantic identity and the production yaw/roll convention.
It replaces only shuttle entities, preserves other scene objects, stages both
instances before capacity checks and never advances animation. The legacy
model-1 fallback policy for a missing course-specific model is retained.
Camera coordinate validation is shared with static-object publication. Narrow
pose/replacement/error coverage and all 24 PAL retained-frame cases pass under
ASan/UBSan, and the client build passes. Flyby/path/other scripted scenery and
playable client adoption remain pending.

Spinner angle/rate playback now has a small caller-owned `Spinners` state
and a shared `TickSpinners` operation. It applies the old rate before the
512-frame refresh, using a local seed so cosmetic randomness cannot advance
physics RNG. The legacy draw adapter calls the same implementation; owned
client scenery updates it once per simulation tick, keeping previous angles
and the final-class freeze. Narrow tests cover wrap, refresh order, pause,
interleaved independence and rejected scenery updates preserving spinner state.
All 24 PAL pack cases check the enabled course/class combinations, and the
legacy spinning regression plus client build pass. Spinner placement/publication
is not yet owned; the legacy draw path still carries its original draw cadence.

Spinner placements now live with shuttle configuration in shared
`scenery_data.c`, preserving legacy bytes and symbols. Each client copies
placements and publishes current/previous spinner orientation through
`SubmitClientSpinners`. Single/group/disabled selection replaces only spinner
entities, preserving terrain, cars and shuttles. Dynamic instance construction,
pose conversion and atomic entity replacement are shared with shuttles.
Client selection/freeze uses the existing class-content definition. All 75
headless sanitizer tests, all 24 PAL retained-scene cases, legacy data/ABI and
spinning regressions pass; the client build passes. Scripted flyby/path/status
scenery and playable integration remain unfinished.

Owned scenery now includes the standard landmark on every course and the
class-selected coast landmark, with the oval's Z offset and the production
normal/night model, fog and palette selection. `SubmitClientLandmarks` shares
course pose/instance construction and atomic entity replacement with moving
props. The duplicate spinner/static placement layouts are one underlying
`SceneryPlacement` type, retaining the legacy alias and ABI. Built-in landmark
data lives in the shared library and is copied into each race. Narrow mode,
palette, offset and capacity tests, all 24 PAL retained-scene cases and legacy
static/data/ABI regressions pass; the client build passes. Flyby/path/status
scenery and playable adoption remain unfinished.

The current client input codec now explicitly zeros its reserved thirteenth
wire byte; the server reads twelve body bytes but consumes eleven fields.
A prefilled output-buffer test guards against sending uninitialized data.
The snapshot fixture now contains the correct little-endian bytes for 46877.
The unused `PutLE32` helper was removed to restore the strict-warning build.
Protocol and legacy static/data/ABI tests plus the full client build pass.
These codec checks do not establish end-to-end multiplayer correctness.

Snapshot decoding now requires the exact body size, a known simulation phase
and boolean active flags. Invalid packets preserve the destination, including
errors in the second seat after the first has been decoded. Narrow protocol
tests cover every truncated length, an oversized body, every unknown phase
and every non-boolean flag in both seats, plus all four accepted phases.
This validates the decoder boundary, not transport or playable integration.

The headless TCP driver now uses `mp_client.c` instead of duplicating its
socket loops and wire codec. Invalid arguments and transport/protocol failures
return failure, and every input byte comes from the tested encoder. A compiled
loopback fixture checks the real driver process, fragmented snapshot reception,
initialized input bytes and invalid-snapshot exit status. It builds but could
not run in the current sandbox because binding a local port is denied; that
specific permission failure is reported as a skipped integration test. Codec
and invalid-argument checks pass. This is not an end-to-end server test.

The server input handoff now publishes one complete control packet under one
mutex instead of reading independently updated atomics. Pending up/down gear
edges are ORed across received packets and cleared together when the tick takes
its sample; a newer zero edge can no longer erase an unconsumed request. Unused
connection/timestamp atomics and the wall-clock helper were removed. Two Rust
unit tests cover latest-level/edge retention and 100,000 concurrent publications
and reads without mixed fields. Offline server tests pass; this does not verify
socket transport, timing under load or playable client behavior.

Driver-command bounds now have one pure C validator, `ValidDriverInput`, shared
by `SetRaceInput` and the Rust network handoff through FFI. Raw flag bytes are
validated before boolean conversion. Rejected packets do not replace latest
controls or pending gear edges. The narrow C input test checks the validator
without loading race assets, and the Rust handoff regression checks preserved
state after invalid mode, flags and pedal ranges. The C input test and all
three Rust unit tests pass offline. Transport and playable adoption remain
unverified.

Vehicle shadow-map centering no longer searches for the hardcoded player entity
11 in one model bank. The scene producer marks a focus body; `RenderWorldFocus`
can select any existing main-view entity, preserving all flags on failure.
`RenderShadowCenter` follows that marker independently of model bank, falling
back to the camera when no finite focus is available. The legacy producer marks
its player body, including custom rival-car models. Owned client composition can
select its actual local seat after publishing the field. Narrow scene tests
cover seats 0/11, rival-bank identity, mirror exclusion, changed focus, invalid
selection atomicity and nonfinite positions; they pass under ASan/UBSan. This
tests center selection, not GPU shadow appearance or playable client adoption.

The server reader closes its input state on every exit and shuts down the
connection. The next tick retires that seat instead of repeating stale throttle;
write failures close the same state. Closing clears controls and pending edges,
is idempotent and rejects later publications. Socket writes have a 100 ms timeout.

Snapshot sending now runs on a dedicated writer per connection. Its outbox
holds only the newest unsent snapshot, shared immutably across connections;
the race thread publishes without calling socket I/O. Older pending snapshots
are replaced, while the final snapshot precedes the result marker. Closing
wakes the writer and drops pending data. Start/welcome writes remain in setup,
before the race clock. Writers are joined on room termination. Ten Rust unit
tests pass, including 10,000 publications while the actual writer loop is
blocked on a fake sink, latest-only delivery, final-message ordering and close
behavior. The server build passes. These tests avoid sockets and race assets;
actual TCP cleanup/throughput and playable clients remain unverified in the
sandbox that denies local port binding.

The reader requires exactly one complete hello before accepting input. Names
use a fixed 15-byte buffer matching the C client limit; oversized names, repeated
hello, input before hello and unknown commands terminate processing. Its real
read loop accepts a `Read` stream, so narrow tests cover every truncated hello/
input prefix, one-byte fragmentation, duplicate handshake after accepted input
and retained gear edges without sockets, disc import or physics initialization.
The prototype now waits for both complete handshakes before constructing and
starting the race. A condition variable wakes setup on hello or disconnect;
a five-second post-accept timeout cancels setup, closes both connections and
returns failure instead of starting with an uninitialized participant. The
narrow gate test covers pending, greeted, duplicate and disconnected states
plus cross-thread wakeup. All ten Rust tests and the server build pass. This
handshake gate is not lobby readiness or an acknowledgement of loaded assets;
those remain necessary for the playable client.

Prototype pose application now uses the narrow `MpApplySnapshot` boundary.
It accepts only newer ticks and nonregressing known phases, validates both
configured human seats before changing either, retires inactive seats and
rejects resurrection of a retired seat. It updates only pose/visibility and
the presentation tick/phase; it does not restore full authoritative physics,
finish results or prediction state. A memory-only test compares the complete
race before/after accepted poses and malformed/stale snapshots, including a
late-seat error with a different first-car position, unchanged RNG and other
seats. Client runtime failures now return failure instead of reporting success
on every loop exit. C protocol tests pass under ASan/UBSan; this does not prove
snapshot rendering.

Owned scenery now advances across missed snapshot ticks by replaying only its
small shuttle/spinner state, keeping previous poses from the final animation
tick. Candidate state is staged before committing, so invalid configuration,
rewinds and excessive gaps preserve all animation history. Work is bounded to
ten seconds (500 ticks) per update; the prototype reports failure for a larger
gap, since session resynchronization is not implemented. It never reruns race
physics or consumes physics RNG. The narrow test compares a 500-tick jump to
500 individual updates across a spinner-rate refresh and checks rewind
atomicity under ASan/UBSan. This is cosmetic catch-up, not full client recovery.

The POSIX client transfer loops retry EINTR. Socket setup enables SO_NOSIGPIPE
on macOS, and send uses MSG_NOSIGNAL where available, so a broken connection
reports failure without changing the process-wide signal handler. A narrow
compiled test includes the real private adapter implementation and uses a local
socket pair: it checks initialized input bytes, welcome version/seat validation,
invalid discarded snapshots, incomplete reads followed by EOF, signal-interrupted
receive, and a broken-peer send with SIGPIPE restored to its default action.
Socket pairs are permitted in the current sandbox even though TCP bind is not.
This validates the local POSIX adapter boundary, not a TCP server session or
Windows support.

The windowed prototype allocates its instance buffer once per session and
reuses `RenderWorldBeginFrame`, preserving camera history and using the actual
snapshot tick as frame identity. It releases the import archive after the
owned race is prepared. Retained frame copies still own their instance storage;
this removes the temporary scene-buffer allocation per snapshot, not all frame
allocations. Client builds and existing world/snapshot/resource-boundary tests
pass; no GPU performance or visual claim is established.

The standalone server and the actual `Rage Racer` client (not the sockets-only
test client) were run together end to end: `rage-racer-server` against the
real PAL disc, and two real client processes launched with
`--set multiplayer.connect_host=... --set multiplayer.connect_port=...
--set multiplayer.connect_name=...`, each completing the hello/welcome/start
handshake, building its `ClientRace` from the server's chosen course/class/laps,
and entering the live per-tick loop (send input, receive snapshot, submit a
frame, `VSync(0)`) for several seconds without error before a clean shutdown.
Server and client logs confirm both seats connected, the disc/boot check
passed, and the race started; process CPU time grew steadily at a plausible
50 Hz-paced rate with no crash. This is step 3 running as an actual playable
build for the first time. It is not a visual confirmation: the session's
screen capture could not reach the game's window (a different Space/output),
so on-screen car positions, the chase camera framing and course rendering are
still unverified by eye.

The prototype scene no longer publishes terrain twice with different texture
banks. `SubmitClientTerrain` replaces existing owned terrain, so the second
call discarded the first choice and did redundant work. One selected page now
feeds terrain, scenery, field, moving props and frame capture consistently.
The narrow world test verifies page changes replace the same semantic cells,
preserve cars and restore the original instance bytes when switching back.
It passes under ASan/UBSan; client builds pass. Dynamic track-page selection
and the actual rendered appearance remain to validate.

The prototype supplies vehicle daylight from its owned environment sky/horizon
instead of hardcoded full daylight, and checks presentation-update failure.
`TickRaceView` uses elapsed snapshot ticks for lamp fades; its first update
retains the one-tick initialization policy. The narrow presentation regression
checks a five-tick gap and unchanged driver state under ASan/UBSan. This does
not reconstruct past lighting zones, braking, wheel animation or drivetrain:
the prototype snapshot still lacks those authoritative presentation fields.

Protocol version 2 now carries authoritative body pitch/roll, steering, wheel
rotation, brake input and track progress in addition to position/yaw. Server
packing and C decoding share a nonzero signed-value fixture. Pose application
copies these fields without stepping physics; malformed brake values reject the
whole snapshot even in a late seat. This lets presentation consume actual wheel
angles, brake lamps and track zones instead of initial-race values. It still
omits drivetrain/RPM, full contact/ground state, finish results and prediction
state; a complete authoritative correction is not implemented. Twelve Rust
unit tests and the C protocol sanitizer test pass. Visual behavior and actual
TCP sessions remain unverified.

Engine presentation now replays elapsed cosmetic ticks using the latest drive
sample, bounded by the same ten-second catch-up limit as scenery. Frame-mixed
noise uses each intermediate tick without advancing physics RNG. The narrow
view test compares a gap against individual engine-presentation steps and
checks atomic rejection of an excessive gap under ASan/UBSan; client build
passes. Missing historical drivetrain samples are not reconstructed, and the
wire snapshot still lacks RPM/gear/clutch data.

Version 3 snapshots also carry the engine-presentation inputs read by
`StepEngineSound`: RPM, accelerator level, clutch and gear. Manual/automatic
mode remains in the agreed start setup. C decoding and pose application share
one validation predicate, rejecting invalid pedals, narrowed clutch values and
gear indices atomically. C/Rust nonzero byte fixtures cover every added field;
late-seat invalid-field tests preserve output. This supplies current drivetrain
presentation data, not full physics correction or historical samples.

Version 4 result messages contain two six-byte entries after type 0x84:
finished (u8), place (u8), milliseconds (i32). The server reads finish status,
place and time from the C simulation. Retired seats use false/0/-1; finished
seats have a distinct place and nonnegative time. The outbox retains the actual
result payload after the final snapshot, rather than synthesizing a type byte.
Clients decode and report those authoritative values. The C decoder rejects
truncation, invalid flags/places/times and duplicate finish places atomically.
A matching Rust byte fixture and real local-socket receive test pass, as do
thirteen Rust tests and the C sanitizer test. This does not implement results
UI, SQLite persistence or full prediction/correction.

Version 5 adds client-loaded (type 0x03, no body). The windowed client sends
it after the selected race and its presentation storage are initialized;
the headless test driver acknowledges after decoding setup. The server waits
for both confirmations before starting the simulation clock/countdown. Loading
has one shared 60-second deadline; failure closes the whole pending
session. The server enables confirmation immediately before sending setup, so an early
confirmation cannot satisfy the gate. Input before loaded and duplicate
confirmations terminate the reader.
Readiness uses the existing per-connection mutex/condition variable, with tests
for loading, disconnect wake-up and invalid message ordering. The C socket-pair
test verifies the actual confirmation byte. This is not lobby ready, nor proof
that GPU resources rendered successfully; loaded currently trusts the client.

The C socket adapter now includes Winsock instead of Windows failure stubs.
Each connection owns one balanced WSAStartup/WSACleanup pair, uses a SOCKET
without narrowing to int, and closes it with closesocket on every exit path.
Wire codecs and transfer loops remain shared with POSIX. The headless client
and protocol tests are enabled on Windows and link ws2_32. Local POSIX build
and socket tests are checked; Windows compilation/runtime remain unverified
because the darwine SSH connection is denied in this environment.

Race-message reception now has a nonblocking polling API. Its per-client
buffer retains a partial header/body across frames and consumes the complete
available message in one poll, avoiding a frame delay between header and body.
Decoding only commits a
complete valid message. An invalid message or EOF makes receive failure
terminal. The blocking headless API uses the same implementation. The windowed
prototype yields to VSync while awaiting bytes rather than blocking in receive.
A socket-pair test sends every snapshot byte separately and checks untouched
output until completion, full and back-to-back messages in single polls, empty
polling and terminal malformed-message failure;
it passes under ASan/UBSan. This does not yet make connection/handshake or input
writes asynchronous, and live window responsiveness remains unverified.

The windowed race also uses nonblocking input writes. The adapter retains at
most an in-flight wire packet and one newest control sample. Latest pedal and
steering levels replace obsolete queued levels; pending gear edges are merged.
An already transmitted packet prefix is never rewritten. Each call has bounded
work and handles would-block without disconnecting. POSIX uses O_NONBLOCK;
Windows uses FIONBIO on the same socket. A saturated socket-pair test publishes
10,000 inputs without growing history, then verifies exactly the retained and
latest packets, preserved gear edges, partial-prefix continuation, receive on
the nonblocking socket and terminal write failure. Windows behavior is still
unverified; the headless tool retains its blocking adapter API.

Version 6 appends the archive fingerprint to start metadata. C/Rust wire
fixtures include a nonzero u64 with explicit little-endian bytes. Narrow C
tests cover known fingerprint vectors, binary/length changes, same serial with
different content, invalid serial termination and missing inputs. This check
runs before client asset preparation; it does not replace cache metadata or
agreement on room-specific specifications. Hashing happens once at startup,
not per snapshot. Persistent server cache remains unfinished.

The windowed prototype now queues its retained ClientFrame into the existing
window's present callback. Previously preparing its GPU source did not select
that frame for presentation: the normal callback still consulted single-player
capture/world state. The owned path prepares and renders the queued world and
native sky directly, without legacy VRAM/overlay capture or replacing it with
GameRenderWorld. Replacing/clearing the frame and shutdown release the retained
owner. Modern rendering is required for this prototype; HUD and classic network
presentation are not implemented. The client build passes, but live GPU output
and two-window racing still require visual verification.

Version 7 snapshots also carry modelY (ground height) and bodyRollVelocity.
BuildCarParts reads them to clamp the body's height and construct wheel tilt;
previous snapshots left both at their initial-grid values even after movement.
The server now samples them from each authoritative car, and pose application
copies them without physics or RNG updates. Signed C/Rust byte fixtures and a
whole-state application comparison cover both fields; decoder truncation and
socket fragmentation tests use the larger 65-byte seat. Client build and narrow
protocol tests pass. This still is not a full contact-state correction or proof
of visually correct wheels/shadows on a live course.

Owned client environment now advances with the snapshot clock: every second
50 Hz tick updates the PAL environment, including skipped-snapshot catch-up.
Environment, shuttles and spinners are staged together, so invalid enabled
cues reject without partially changing presentation state. Disabled environment
remains unchanged. A narrow test compares a 500-tick jump with 250 real
environment updates and checks unchanged physics, repeated ticks and atomic
invalid-state rejection under ASan/UBSan. This cadence is PAL-only pending
region normalization. Frame lighting is derived from the same owned sky colors
as its camera instead of a default directional light. The client build and
four CPU tests pass; a broader native render test failed with an invalid GPU
device, so visual behavior remains unverified.

Version 8 also sends the disc executable's fingerprint. LoadRaceDisc reads the
file named by the identified boot serial; bare archives or unavailable code
retain zero identity. Server startup refuses an unidentified executable, and
client matching requires both archive and executable identities in addition
to serial. Import tests change executable bytes while retaining serial and
identical RAGE.BIN, covering raw BIN and offset CUE for Mode 1/2 under ASan/UBSan.
C/Rust fixtures cover the second u64 and C matching rejects changed/zero code
identity. A real PAL image passes source identification before listener setup;
listening is denied in this environment. These noncryptographic checks detect
accidental differences, not malicious clients. Cache metadata and agreed
room overrides still remain to be implemented.

Input initialization is shared at GameInitPad: BIOS buffer attachment, input
reset/calibration defaults and default button mappings now belong to that
controller boundary rather than the general single-player boot. Multiplayer
calls it before reporting loaded, then each loop refreshes VSync/host analog
sampling/UpdatePadState before ReadCarControls. Previously it bypassed the
MainLoop that performs those operations, so native controls could stay at
uninitialized/stale state. Narrow pad tests start from dirty state and check
fresh mappings/calibration/edge state and buffer attachment; player-input and
main-loop frame tests also pass. Live keyboard/controller behavior remains
unverified while no display is available.

Race input now expires after five seconds without a valid input packet, even
if TCP has not reported a disconnect. The deadline is reset when the race
starts, so loading does not consume it. Expiry clears held controls and gear
edges, retires the seat and closes its output/socket to unblock its reader.
The C race client already sends controls continuously, including unchanged
controls. Malformed packets never refresh this deadline. Narrow tests supply
timestamps directly to check the exact expiry boundary and late-packet
rejection without waiting five seconds or running a race. Network loss timing
on real platforms remains unverified.
The windowed client's receive watchdog also stops a race when no complete
snapshot arrives for five seconds, without relying on TCP EOF. Before the
first snapshot it allows 65 seconds for the server's 60-second loaded gate.
Partial packets do not renew the deadline. A pure nanosecond deadline test
checks both exact boundaries, clock regression and large timestamps; socket
and protocol tests pass. Live server-loss behavior remains unverified.

Client setup reception also has absolute monotonic deadlines: five seconds for
welcome and two minutes for start (allowing another seat to join). Interrupted
reads and packet fragments do not restart these budgets. Failure preserves
the public decoded output; callers must close the failed session. Socket-pair
tests cover a silent peer, a partial message that stalls and a complete welcome
without loading assets or waiting for the production deadlines. Client build,
protocol/socket tests and socket ASan/UBSan pass. Connect and hello writes still
block, and setup does not yet pump the window event loop; bounded reception is
not an asynchronous connection flow. Windows runtime remains unverified.
The windowed client now uses polling welcome/start reception and calls VSync
between pending polls, allowing the host event loop to run while waiting for
the second seat. Partial setup storage belongs to the connection; invalid
versions, EOF and elapsed deadlines make polling failure terminal without
publishing decoded output. Socket-pair tests cover every setup byte separately,
unchanged deadlines/output, expiry without sleeping, malformed welcome and
partial-message EOF. Client build and socket ASan/UBSan pass. Connect/hello
remain blocking and live window responsiveness still requires GUI verification.

The windowed client now also begins TCP connection in nonblocking mode and
polls writability/error state between host presentations, checking SO_ERROR
before declaring success. One five-second monotonic deadline covers the
connection attempt. Completed connections restore the setup socket mode;
the headless blocking connect API remains available. Clock conversion is
shared by setup reads/polls and connection polling. Narrow socket-pair tests
check successful readiness, socket-mode restoration, pending readiness without
deadline renewal, terminal expiry and invalid addresses. They do not establish
a TCP handshake: listener binding remains unavailable here. Client build,
protocol/socket tests and socket ASan/UBSan pass; Winsock and live GUI behavior
remain unverified. Hello and loaded writes still use the blocking adapter.

Hello and client-loaded now also use bounded nonblocking writes in the windowed
client, with host presentation between pending polls and a five-second absolute
deadline. The connection owns one small setup packet; a partial prefix and the
first supplied name remain unchanged while waiting for socket capacity. Setup
receive accepts the resulting nonblocking socket. Socket-pair tests saturate
the sender, verify unchanged deadline/name, resume after a transmitted prefix,
receive welcome, send exactly one loaded byte and reject expired writes.
Client build, protocol/socket tests and socket ASan/UBSan pass. This removes
blocking network operations from the windowed setup flow, but disc import and
asset preparation remain synchronous. Live GUI/TCP and Windows behavior are
still unverified.

The socket adapter now rejects race input/reception while connect or a setup
transfer is pending, and rejects starting setup over a partial race message.
Blocking setup APIs also refuse sockets already switched to polling mode.
This protects the shared receive buffer: a partial welcome has received bytes
but no race-message length, so crossing into the race parser could underflow
its next receive-size calculation. A narrow socket-pair test reproduces this
boundary, checks unchanged decoded output and no interleaved input bytes, and
verifies setup cannot replace an incomplete snapshot. Client build,
protocol/socket tests and socket ASan/UBSan pass. These transfer guards are not
a complete lobby/session state machine.

Before connecting, the windowed prototype now requests one host presentation
(to initialize the lazily created window) and verifies that modern device and
pipeline resources are ready. Missing GPU resources return failure without a
classic fallback or opening a network session. A real PAL client run with SDL's
dummy video driver exits with the explicit initialization error and no connect
attempt. Normal GUI verification is blocked by macOS application registration
in this environment; no successful GPU race is claimed. Client build and five
input/protocol tests pass.

Nonblocking race receive now coalesces up to 32 complete queued snapshots per
call rather than applying one old TCP message per rendered frame. It keeps the
newest valid pose, retains incomplete bytes and bounds work under continuous
traffic. A result following snapshots is held for the next call so the final
pose is published first. Any malformed snapshot, regressing tick or phase in
that batch rejects without changing caller output. Socket-pair tests cover a
40-snapshot backlog (32 plus eight), final snapshot/result order, contradictory
batches and output preservation under ASan/UBSan. Client build and protocol tests
pass; live network latency and prediction remain unverified/unimplemented.

## Order of work

Authoritative application and interpolated car presentation share `MpApplyPose`,
which copies only the eighteen wire fields and preserves model/ownership flags.
Remote brake/drivetrain values now come from the same delayed history sample as
their rendered position, instead of leaking the newest authoritative controls
into an older pose. A car-only test covers this mismatch and atomic invalid/null
rejection; existing snapshot tests still check race status/clocks independently.
Protocol/connect/socket tests, game/smoke builds, standalone protocol ASan/UBSan,
45 Rust unit tests, two startup tests and four PAL integrations pass. Visual
lamp behavior and live networking remain unverified.

Remote presentation now retains a fixed history of eight accepted snapshots
with one host/server clock origin instead of restarting a two-pose blend on
every arrival. The windowed prototype samples it with 100 ms delay; retained
tick brackets determine interpolation and missing history clamps to the
oldest/newest sample without extrapolation. The pure history API accepts
timestamps directly. Tests cover irregular arrivals, unchanged pose when a
new packet arrives at the same render time, bounded eviction, stale/invalid
samples, regressing clocks and large timestamps. Client build and C protocol/
socket tests pass. This is an initial fixed-delay buffer: adaptive latency,
clock-drift correction, local prediction and live GPU/network verification
remain outstanding. Latest authoritative status/model ownership is preserved.

`MpBlendPose` provides pure presentation interpolation of position, ground,
steering, roll velocity and cyclic body/wheel angles. It uses the shortest
4096-unit angular path, bounded fixed-point fractions and widened arithmetic;
controls remain discrete and terminal transitions select the newer pose.
Narrow tests cover angular wrap in both directions, full signed position
range, endpoints and atomic invalid-data rejection, including preservation of
the discrete wheel-blur model flag. The windowed client retains the last two
received snapshots and renders between them using the monotonic host clock.
Only the remote seat is delayed by their tick interval; the local seat still
uses the latest authoritative pose without prediction. Missing packets freeze
at the newest pose rather than extrapolating. Render submission accepts separate
presentation poses without copying or changing RaceSim; a narrow race-view
test verifies moved geometry and unchanged authoritative state. This initial
two-snapshot interpolation is not a jitter buffer, and live GPU/network
behavior still needs verification.

Multiplayer presentation owns its last submitted car poses and a separate
render-frame counter. Previous transforms now refer to the preceding submitted
image rather than the preceding server snapshot; multiple images between
snapshots therefore have distinct frame identities and motion history.
First-frame history uses the current pose. Model banks and visibility remain
authoritative even when separate presentation poses are supplied. Narrow
race-view tests check distinct current/previous transforms, ignored model-bank
changes in presentation data, and unchanged simulation state. Live temporal
GPU effects still require verification.

Version 9 snapshots distinguish retired (0), driving (1) and finished (2)
cars instead of treating every visible car as driving. Applying a snapshot
updates the client's presentation status without calculating local finish
times or places; those remain authoritative result-message data. Finished and
retired seats cannot return to driving, including within a coalesced batch.
Narrow protocol/socket tests verify terminal transitions and atomic rejection
without loading assets or running a race. Matching Rust serialization tests
cover the finished status byte. Both C test suites and all 18 Rust tests pass;
this does not establish live rendering or prediction behavior.


















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
