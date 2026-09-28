/* WebAssembly bridge for the browser port (web/). It exposes a small, flat C
 * ABI over the same headless code the native modern renderer uses: disc
 * loading (rage-data), the race simulation (rage-sim), the client race and
 * scene submission (client_race.c, race_view.c, client_world.c) and the CPU
 * draw builder (render_mesh_build.c). The browser only uploads the resulting
 * world-space triangles and decoded textures; it never re-derives physics or
 * scene layout. */
#include <emscripten/emscripten.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "axis_curve.h"
#include "client_race.h"
#include "environment_view.h"
#include "game/car.h"
#include "game/race_data.h"
#include "game/race_grid.h"
#include "game/race_sim.h"
#include "game/asset_index.h"
#include "game/track.h"
#include "game/track_data.h"
#include "game/state.h"
#include "race_view.h"
#include "rage/chase_camera.h"
#include "render/car_lamps.h"
#include "render/car_paint.h"
#include "render/render_mesh_build.h"
#include "render/render_projection.h"
#include "render/render_shadow.h"
#include "render/render_world_frame.h"
#include "render/texture_mipmap.h"
#include "scene_matrix.h"
#include "web_hud.h"
#include "web_rules.h"
#include "web_audio.h"
#include "web_sky.h"

enum {
    WEB_INSTANCE_CAPACITY = 8192,
    WEB_VERTEX_CAPACITY = 600000,
    WEB_SPAN_CAPACITY = 32768,
    WEB_SPAN_FIELDS = 14,
    WEB_SPAN_ALPHA = 13, /* span field: 255 opaque, less while its car fades out */
    WEB_TEXTURE_BYTES = 256 * 256 * 4,
    WEB_PACKED_FLOATS = 22,
};

/* chase_camera.c reads optional tuning from the runtime config; the browser
 * has none, so every setting takes its built-in default. */
const char *RuntimeConfigGet(const char *key);
int RuntimeConfigEnabled(const char *key);
const char *RuntimeConfigGet(const char *key) { (void)key; return NULL; }
int RuntimeConfigEnabled(const char *key) { (void)key; return 0; }

/* ---- race state ---------------------------------------------------------- */
static RaceData *s_archive;
static ClientRace *s_race;
/* The seat this player drives (-1 for a spectator, who has no car of its
 * own), and whether a server steps the race. */
static int s_localSeat, s_net;

/* Client-side prediction. A player's client steps the race itself with its
 * own controls, so its car answers at once; each server frame rewinds the
 * race to the authoritative state and replays the controls the server had
 * not consumed yet. The simulation is deterministic, so only other players'
 * newer controls (and network timing) cause corrections. */
enum { INPUT_HISTORY = 512 };
typedef struct SentInput {
    DriverInput input;
    u32 tick; /* the local race tick it was used for */
} SentInput;
static struct Prediction {
    SentInput sent[INPUT_HISTORY];
    u32 inputSeq;          /* last sequence number handed out */
    int predicting;        /* stepping locally since the first frame */
    DriverInput tickInput; /* this tick's controls (rw_take_input) */
    int tickInputReady;
} s_predict;

/* Correction smoothing. When a server frame moves a predicted car, the
 * difference is kept as an offset on its drawn pose and eased out over a
 * few physics steps, so a correction slides instead of teleporting. Jumps
 * too large to be prediction error (a respawn) are shown at once. */
enum { SMOOTH_SNAP_DISTANCE = 3000 };
#define SMOOTH_DECAY 0.55f /* kept per 25 Hz step: ~90% gone after 4 steps */
static struct Smooth { float x, y, z, yaw; } s_smooth[DRIVER_SEAT_LIMIT];

/* The local controls. Gear, camera and mirror requests are edges: they stay
 * pending until a tick (or game frame) uses them. */
static struct Controls {
    DriverInput input;
    int pendingShiftUp, pendingShiftDown;
    u16 padHeld;       /* last pad sample's button bits */
    int pendingCamera; /* a camera-button press not yet used */
    int pendingMirror; /* +1 on, -1 off */
} s_controls;

/* Past the line a car drives on, easing off, and fades out over this many
 * physics steps (2.5 s); presentation only, as the simulation keeps a
 * finished car where it crossed the line. */
enum { FINISH_FADE_STEPS = 62 };
typedef struct FinishRun {
    int steps;          /* physics steps since the finish, 0 while racing */
    float motion[3];    /* its last step's motion while driving */
    s32 raw[3];         /* its last simulated position (no smoothing) */
    float ox, oy, oz;   /* distance travelled past the line */
} FinishRun;

/* Retail chase camera state (see RetailChaseView). */
typedef struct WebChase {
    s32 previousYaw, rampNeg, rampPos, yawLag, damping, stepLimit, step;
    int active;
} WebChase;

/* race_scene.c: the race starts in the car view; the camera button swaps it
 * with the chase view (chase preset 0, the only one retail selects), and
 * holding down in the chase view looks behind. */
typedef enum WebView { WEB_VIEW_CAR, WEB_VIEW_CHASE, WEB_VIEW_LOOK_BEHIND } WebView;

/* Presentation history at the simulation's physics steps (every second
 * 50 Hz tick): the browser draws between the last two, so motion is smooth
 * at any display rate instead of stepping at 25 Hz. */
static struct Presentation {
    /* The seat the camera and HUD follow: the local car, or another one
     * while spectating; a new one cuts on the next snapshot. */
    int viewSeat, viewCut;
    PlayerCarRuntime posePrevious[DRIVER_SEAT_LIMIT], poseCurrent[DRIVER_SEAT_LIMIT];
    FinishRun run[DRIVER_SEAT_LIMIT];
    RenderCamera cameraPrevious, cameraCurrent;
    WebChase chase;
    WebView selectedView, viewCurrent;
    u32 lastStepTick;
    int haveStep, lastTickStepped;
} s_view;

/* Rear-view mirror (render/mirror_pass.c, render/rear_view_mirror.c). Its
 * panel slides one PAL line per game frame between hidden and visible; the
 * mirror draws in the car view while racing. R1/L1 pressed while the camera
 * button is held switch it on/off (race_scene.c); finishing switches it off.
 * Retail also waits for an unlock and Grand Prix mode; the browser does not. */
enum { MIRROR_PANEL_HIDDEN_Y = -44, MIRROR_PANEL_VISIBLE_Y = 18 };
#define MIRROR_FOV_DEGREES 20.0f
/* 148x36 PAL pixels (mirror_pass.c MIRROR_WIDTH x MIRROR_HEIGHT). */
#define MIRROR_ASPECT (148.0f / 36.0f)
static struct Mirror {
    int enabled, draw;
    s32 panelPrevious, panelCurrent;
    RenderCamera previous, current;
    /* The last built frame's mirror: its draws follow the main view's. */
    uint32_t vertexCount, spanCount;
    float uniform[28], sky[WEB_SKY_FLOATS];
    float state[4]; /* drawn, panel top (PAL lines, may be negative), first vertex, vertex count */
} s_mirror;

/* ---- the last built frame -------------------------------------------------- */
static RenderMeshInstance *s_instances;
static RenderWorld s_world;
static RageNativeDrawVertex *s_vertices;
static RageNativeDrawSpan *s_spans;
static uint32_t s_spanFields[WEB_SPAN_CAPACITY * WEB_SPAN_FIELDS];
static float *s_packed;
static uint8_t *s_mipChain;
static struct Frame {
    uint64_t number;
    uint32_t vertexCount, spanCount;
    /* Track texture page the frame was built with (render/track_textures.c:
     * retail swaps the upper VRAM rows while the player is inside the
     * track's texture section range). Terrain and course carry it in their
     * material variant; track model banks decode against it directly. */
    int page;
    /* position, viewRow0, viewRow1, viewRow2, projection, fogColor, fogRange. */
    float camera[28];
    /* direction, ambient, diffuse, skyTop, skyHorizon, skyBottom. */
    float light[24];
    float sky[WEB_SKY_FLOATS];
    /* Vehicle shadow camera: position, rows 0..2, (scaleX, scaleY,
     * depthScale, depthOffset); zero when no map could be built. */
    float shadow[20];
    int shadowValid;
} s_frame;
static int32_t s_hud[16];

const RaceData *WebLoadedArchive(void) { return s_archive; }

/* The paint each human seat races in (two catalogue colours), or -1 for the
 * car's factory colours. Set before a race is prepared; presentation only, so
 * neither the server's simulation nor the wire format knows about it. */
static int s_paint[DRIVER_SEAT_LIMIT][2];
EMSCRIPTEN_KEEPALIVE void rw_set_paint(int seat, int first, int second) {
    if (seat < 0 || seat >= DRIVER_SEAT_LIMIT) return;
    const int valid = first >= 0 && first < RAGE_CAR_PAINT_COLOR_COUNT &&
                      second >= 0 && second < RAGE_CAR_PAINT_COLOR_COUNT;
    s_paint[seat][0] = valid ? first : -1;
    s_paint[seat][1] = valid ? second : -1;
}
static void ClearPaint(void) { memset(s_paint, -1, sizeof(s_paint)); }

/* The catalogue for a colour picker: how many colours, and one's RGB. */
EMSCRIPTEN_KEEPALIVE int rw_paint_count(void) { return RAGE_CAR_PAINT_COLOR_COUNT; }
EMSCRIPTEN_KEEPALIVE const uint8_t *rw_paint_swatch(int color) {
    static uint8_t rgb[3];
    return CarPaintSwatch((uint8_t)color, rgb) ? rgb : NULL;
}

static void ReleaseRace(void) {
    WebAudioStopRace();
    FreeClientRace(s_race);
    s_race = NULL;
}

EMSCRIPTEN_KEEPALIVE int rw_load_disc(const char *path) {
    ReleaseRace();
    ClearPaint();
    FreeRaceData(s_archive);
    s_archive = LoadRaceDisc(path);
    return s_archive != NULL;
}

/* Loads the field in setup and prepares the local presentation. The server
 * and every player build it from the same rules (web_rules.c WebBuildField). */
static int PrepareRace(int classIndex, int course, int reverse, int laps, int rivals,
                       const WebSeat *humans, int humanCount, int localSeat) {
    RaceSetup setup;
    int paint[DRIVER_SEAT_LIMIT][2];
    /* The paint set for this race is used up whether or not it starts. */
    memcpy(paint, s_paint, sizeof(paint));
    ClearPaint();
    if (!s_archive || laps < 1 || laps > WEB_MAX_LAPS || localSeat < -1 ||
        localSeat >= humanCount) return 0;
    ReleaseRace();
    memset(&setup, 0, sizeof(setup));
    setup.classIndex = classIndex;
    setup.courseIndex = course;
    setup.laps = laps;
    setup.reverse = reverse ? 1 : 0;
    if (!WebBuildField(s_archive, classIndex, course, reverse, humans, humanCount, rivals,
                       setup.entrants)) return 0;
    for (int seat = 0; seat < humanCount; ++seat) {
        setup.looks[seat].variant = humans[seat].variant;
        if (paint[seat][0] >= 0) {
            setup.looks[seat].hasPaint = 1;
            setup.looks[seat].paint.paintColor1 = (u8)paint[seat][0];
            setup.looks[seat].paint.paintColor2 = (u8)paint[seat][1];
        }
    }
    s_race = LoadClientRace(s_archive, &setup, NULL);
    if (!s_race) return 0;
    /* A spectator has no tachometer (rw_tachometer stays invisible). */
    if ((localSeat >= 0 && !WebHudPrepare(s_archive, humans[localSeat].variant)) ||
        !StartRaceSim(&s_race->sim, WEB_COUNTDOWN_TICKS)) {
        ReleaseRace();
        return 0;
    }
    if (!s_instances) s_instances = calloc(WEB_INSTANCE_CAPACITY, sizeof(*s_instances));
    if (!s_vertices) s_vertices = calloc(WEB_VERTEX_CAPACITY, sizeof(*s_vertices));
    if (!s_spans) s_spans = calloc(WEB_SPAN_CAPACITY, sizeof(*s_spans));
    if (!s_packed) s_packed = calloc((size_t)WEB_VERTEX_CAPACITY * WEB_PACKED_FLOATS, sizeof(*s_packed));
    if (!s_instances || !s_vertices || !s_spans || !s_packed) {
        ReleaseRace();
        return 0;
    }
    RenderWorldInit(&s_world, s_instances, WEB_INSTANCE_CAPACITY);
    s_localSeat = localSeat;
    memset(&s_predict, 0, sizeof(s_predict));
    memset(s_smooth, 0, sizeof(s_smooth));
    memset(&s_controls, 0, sizeof(s_controls));
    s_controls.input.steering.mode = STEERING_DIGITAL;
    memset(&s_view, 0, sizeof(s_view)); /* the car view, no snapshot yet */
    s_view.viewSeat = localSeat >= 0 ? localSeat : 0;
    /* mirror_pass.c ResetMirrorState. */
    memset(&s_mirror, 0, sizeof(s_mirror));
    s_mirror.enabled = 1;
    s_mirror.panelPrevious = s_mirror.panelCurrent = MIRROR_PANEL_HIDDEN_Y;
    memset(&s_frame, 0, sizeof(s_frame));
    /* Sound is presentation only: a race without its banks still runs. */
    WebAudioStartRace(s_archive, &s_race->sim, localSeat, classIndex);
    return 1;
}

/* Offline race: the local player alone, with or without the retail AI. */
EMSCRIPTEN_KEEPALIVE int rw_start_race(int classIndex, int course, int car, int manual,
                                       int reverse, int laps, int rivals) {
    const WebSeat human = {car, manual ? 1 : 0};
    s_net = 0;
    return PrepareRace(classIndex, course, reverse, laps, rivals, &human, 1, 0);
}

/* Networked race as the server announced it: humanSeats holds (variant,
 * manual) per human in seat order. The server's frames then drive it. */
EMSCRIPTEN_KEEPALIVE int rw_start_net_race(int classIndex, int course, int reverse, int laps,
                                           int rivals, int humanCount, const int32_t *humanSeats,
                                           int localSeat) {
    WebSeat humans[DRIVER_SEAT_LIMIT];
    if (!WebReadSeats(humanSeats, humanCount, humans)) return 0;
    s_net = 1;
    return PrepareRace(classIndex, course, reverse, laps, rivals, humans, humanCount, localSeat);
}

/* The local controls for this tick, to send (web_rules.h wire words) under
 * sequence number rw_input_seq(); gear edges are handed over once, as the
 * server's simulation accumulates them. The same controls drive the local
 * prediction on this tick. */
EMSCRIPTEN_KEEPALIVE int32_t *rw_take_input(void) {
    static int32_t words[WEB_INPUT_WORDS];
    DriverInput input = s_controls.input;
    input.shiftUp = s_controls.pendingShiftUp;
    input.shiftDown = s_controls.pendingShiftDown;
    s_controls.pendingShiftUp = s_controls.pendingShiftDown = 0;
    WebEncodeInput(&input, words);
    if (s_race && s_net && s_localSeat >= 0) {
        ++s_predict.inputSeq;
        s_predict.sent[s_predict.inputSeq % INPUT_HISTORY] = (SentInput){input, s_race->sim.tick + 1};
        s_predict.tickInput = input;
        s_predict.tickInputReady = 1;
    }
    return words;
}
/* The local race tick the controls rw_take_input handed out are used for. */
EMSCRIPTEN_KEEPALIVE uint32_t rw_input_tick(void) { return s_predict.sent[s_predict.inputSeq % INPUT_HISTORY].tick; }
EMSCRIPTEN_KEEPALIVE uint32_t rw_input_seq(void) { return s_predict.inputSeq; }

/* Applies one authoritative RaceFrame from the server. A spectator only
 * restores it. A player rewinds to it and replays the unconsumed controls up
 * to the local present: ackSeq is the last input the server had received and
 * arrivalTick the server tick that first used it. Returns how many ticks
 * later than predicted that input was used (the client clock's error; 0 for
 * a spectator), or INT32_MIN when the frame does not fit the race. */
/* Network diagnostics for players: how far each server frame moved the
 * predicted cars (world units), own car and the others, and how many ticks
 * were replayed. Read and reset by the network checks (scripts/net-check.mjs). */
static struct NetStats {
    double frames, ownSum, ownMax, otherSum, otherMax, otherCount, replaySum;
} s_netStats;
static float s_netStatsOut[6];
EMSCRIPTEN_KEEPALIVE const float *rw_net_stats(void) {
    const double frames = s_netStats.frames > 0 ? s_netStats.frames : 1;
    const double others = s_netStats.otherCount > 0 ? s_netStats.otherCount : 1;
    s_netStatsOut[0] = (float)s_netStats.frames;
    s_netStatsOut[1] = (float)(s_netStats.ownSum / frames);
    s_netStatsOut[2] = (float)s_netStats.ownMax;
    s_netStatsOut[3] = (float)(s_netStats.otherSum / others);
    s_netStatsOut[4] = (float)s_netStats.otherMax;
    s_netStatsOut[5] = (float)(s_netStats.replaySum / frames);
    return s_netStatsOut;
}
EMSCRIPTEN_KEEPALIVE void rw_net_stats_reset(void) { memset(&s_netStats, 0, sizeof(s_netStats)); }

static void RecordCorrections(const s32 before[DRIVER_SEAT_LIMIT][3], const int driving[DRIVER_SEAT_LIMIT],
                              u32 replayed) {
    s_netStats.frames += 1;
    s_netStats.replaySum += replayed;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const PlayerCarRuntime *car = &s_race->sim.drivers[seat].car;
        if (!driving[seat] || s_race->sim.drivers[seat].status != SIM_DRIVING) continue;
        const double dx = car->x - before[seat][0], dy = car->y - before[seat][1], dz = car->z - before[seat][2];
        const double moved = sqrt(dx * dx + dy * dy + dz * dz);
        if (seat == s_localSeat) {
            s_netStats.ownSum += moved;
            if (moved > s_netStats.ownMax) s_netStats.ownMax = moved;
        } else {
            s_netStats.otherSum += moved;
            s_netStats.otherCount += 1;
            if (moved > s_netStats.otherMax) s_netStats.otherMax = moved;
        }
    }
}

static void SmoothCorrections(const s32 before[DRIVER_SEAT_LIMIT][3], const s32 beforeYaw[DRIVER_SEAT_LIMIT],
                              const int driving[DRIVER_SEAT_LIMIT]) {
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const PlayerCarRuntime *car = &s_race->sim.drivers[seat].car;
        struct Smooth *smooth = &s_smooth[seat];
        const float dx = (float)(before[seat][0] - car->x), dy = (float)(before[seat][1] - car->y);
        const float dz = (float)(before[seat][2] - car->z);
        if (!driving[seat] || s_race->sim.drivers[seat].status != SIM_DRIVING ||
            dx * dx + dy * dy + dz * dz > (float)SMOOTH_SNAP_DISTANCE * SMOOTH_SNAP_DISTANCE) {
            memset(smooth, 0, sizeof(*smooth));
            continue;
        }
        smooth->x += dx;
        smooth->y += dy;
        smooth->z += dz;
        smooth->yaw += (float)((((beforeYaw[seat] - car->bodyYaw) & ANGLE_MASK) ^ 0x800) - 0x800);
    }
}

EMSCRIPTEN_KEEPALIVE int32_t rw_apply_frame(const uint8_t *wire, int size, uint32_t ackSeq,
                                            uint32_t arrivalTick) {
    static RaceFrame frame;
    struct Prediction *p = &s_predict;
    if (!s_race || !s_net || !wire || size != RACE_FRAME_WIRE_SIZE ||
        !DecodeRaceFrame(&s_race->sim, wire, (size_t)size, &frame)) return INT32_MIN;
    const u32 present = s_race->sim.tick;
    s32 before[DRIVER_SEAT_LIMIT][3], beforeYaw[DRIVER_SEAT_LIMIT];
    int driving[DRIVER_SEAT_LIMIT];
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const PlayerCarRuntime *car = &s_race->sim.drivers[seat].car;
        before[seat][0] = car->x;
        before[seat][1] = car->y;
        before[seat][2] = car->z;
        beforeYaw[seat] = car->bodyYaw;
        driving[seat] = s_race->sim.drivers[seat].status == SIM_DRIVING;
    }
    if (!RestoreRaceFrame(&s_race->sim, &frame)) return INT32_MIN;
    if (s_localSeat < 0) return 0;
    const int known = ackSeq && ackSeq <= p->inputSeq && p->inputSeq - ackSeq < INPUT_HISTORY;
    const int32_t error = known ? (int32_t)(arrivalTick - p->sent[ackSeq % INPUT_HISTORY].tick) : 0;
    if (!p->predicting) {
        p->predicting = 1; /* the local race starts at the server's present */
        return error;
    }
    const u32 oldest = p->inputSeq >= INPUT_HISTORY ? p->inputSeq - INPUT_HISTORY + 1 : 1;
    const u32 restored = s_race->sim.tick;
    while (s_race->sim.tick < present) {
        const u32 next = s_race->sim.tick + 1;
        for (u32 seq = ackSeq + 1 > oldest ? ackSeq + 1 : oldest; seq <= p->inputSeq; ++seq) {
            if (p->sent[seq % INPUT_HISTORY].tick == next)
                SetRaceInput(&s_race->sim, s_localSeat, &p->sent[seq % INPUT_HISTORY].input);
        }
        if (!StepRaceSim(&s_race->sim)) break;
    }
    if (present >= restored) RecordCorrections(before, driving, present - restored);
    SmoothCorrections(before, beforeYaw, driving);
    return error;
}

/* ---- spectating --------------------------------------------------------------
 * The camera and HUD can follow any car in the field; the next snapshot cuts
 * to it. Returns 1 when the seat holds a car. */
EMSCRIPTEN_KEEPALIVE int rw_set_view_seat(int seat) {
    if (!s_race || seat < 0 || seat >= DRIVER_SEAT_LIMIT ||
        s_race->sim.drivers[seat].status == SIM_EMPTY) return 0;
    if (seat != s_view.viewSeat) {
        s_view.viewSeat = seat;
        s_view.viewCut = 1;
        memset(&s_view.chase, 0, sizeof(s_view.chase));
    }
    return 1;
}
EMSCRIPTEN_KEEPALIVE int rw_view_seat(void) { return s_view.viewSeat; }
/* 1 once a seat's car has left the picture: retired, or finished and faded. */
EMSCRIPTEN_KEEPALIVE int rw_seat_gone(int seat) {
    if (!s_race || seat < 0 || seat >= DRIVER_SEAT_LIMIT) return 1;
    const SimDriverStatus status = s_race->sim.drivers[seat].status;
    return status == SIM_EMPTY || status == SIM_RETIRED ||
           (status == SIM_DRIVER_FINISHED && s_view.run[seat].steps > FINISH_FADE_STEPS);
}

/* ---- Controls: the desktop pad path --------------------------------------
 * The browser reports PS1 button bits (the keyboard mapped as input_config.c's
 * defaults, a gamepad as analog_pad.c's GamepadButtons) plus the gamepad's
 * stick and triggers. Without a gamepad the car reads the digital pad through
 * g_PadButtonPresets[0]; with one it reads analog_pad.c's emulated NeGcon
 * through g_NegconButtonPresets[0] and the default OPTIONS calibration, as
 * player_input.c does on the desktop. */
enum {
    DIGITAL_SHIFT_UP = PAD_R2 | PAD_R1,     /* g_PadButtonPresets[0][4] */
    DIGITAL_SHIFT_DOWN = PAD_L2 | PAD_L1,   /* g_PadButtonPresets[0][5] */
    NEGCON_SHIFT_UP = PAD_DOWN,             /* g_NegconButtonPresets[0][4] */
    NEGCON_SHIFT_DOWN = PAD_UP,             /* g_NegconButtonPresets[0][5] */
    CAMERA_BUTTON = PAD_TRIANGLE,           /* slot 6 of both presets */
    NEGCON_ANALOG_MAX = 0x6A,
    NEGCON_DEFAULT_STEER_RANGE = 25,        /* g_NegconSteerRange[g_NegconMaxTwist = 0] */
    NEGCON_DEFAULT_DEAD_ZONE = 6,           /* g_NegconSteerDeadZone[g_NegconSteerPlay = 1] */
    NEGCON_STEERING_SCALE = 13 * 512,
    PEDAL_FULLY_PRESSED = 0x100,
};

/* analog_pad.c's AxisShaped with the default input.steering/throttle/brake
 * curves (only steering has a linearity, 0.5). */
static float AxisShapedDefault(int axis, float linearity) {
    float magnitude = (float)(axis < 0 ? -axis : axis) / 32767.0f;
    float shaped = AxisCurve(magnitude > 1.0f ? 1.0f : magnitude, 0.0f, 1.0f, linearity, 1.0f);
    return axis < 0 ? -shaped : shaped;
}

/* init_pad.c's CalibrateNegconSteering with a centred neutral. */
static s32 CalibrateNegconSteer(int twist) {
    s32 delta = twist - 0x80, steering;
    if (delta > 0) {
        steering = delta - NEGCON_DEFAULT_DEAD_ZONE;
        if (steering < 0) steering = 0;
        if (steering > NEGCON_DEFAULT_STEER_RANGE) steering = NEGCON_DEFAULT_STEER_RANGE;
    } else {
        steering = delta + NEGCON_DEFAULT_DEAD_ZONE;
        if (steering > 0) steering = 0;
        if (steering < -NEGCON_DEFAULT_STEER_RANGE) steering = -NEGCON_DEFAULT_STEER_RANGE;
    }
    return steering;
}

static s16 NegconPedal(int pressure) {
    return (s16)(pressure * PEDAL_FULLY_PRESSED / NEGCON_ANALOG_MAX);
}

/* One pad sample: `held` PS1 button bits, the gamepad's left stick x
 * (-32768..32767) and triggers (0..32767), and whether a gamepad is active.
 * Gear and camera requests are edges and stay pending until a tick uses them. */
EMSCRIPTEN_KEEPALIVE void rw_set_pad(int held, int stickX, int rightTrigger, int leftTrigger,
                                     int gamepad) {
    const u16 buttons = (u16)held;
    const u16 pressed = buttons & (u16)~s_controls.padHeld;
    s_controls.padHeld = buttons;
    if (pressed & CAMERA_BUTTON) s_controls.pendingCamera = 1;
    if (buttons & CAMERA_BUTTON) {
        if (pressed & PAD_R1) s_controls.pendingMirror = 1;
        else if (pressed & PAD_L1) s_controls.pendingMirror = -1;
    }
    if (!gamepad) {
        s_controls.input.steering.mode = STEERING_DIGITAL;
        s_controls.input.steering.left = (buttons & PAD_LEFT) != 0;
        s_controls.input.steering.right = (buttons & PAD_RIGHT) != 0;
        s_controls.input.steering.angle = 0;
        s_controls.input.throttle = buttons & PAD_CROSS ? PEDAL_FULLY_PRESSED : 0;
        s_controls.input.brake = buttons & PAD_SQUARE ? PEDAL_FULLY_PRESSED : 0;
        if (pressed & DIGITAL_SHIFT_UP) s_controls.pendingShiftUp = 1;
        if (pressed & DIGITAL_SHIFT_DOWN) s_controls.pendingShiftDown = 1;
        return;
    }
    {
        const int twist = NegconTwist(AxisShapedDefault(stickX, 0.5f),
                                      (buttons & PAD_LEFT) != 0, (buttons & PAD_RIGHT) != 0,
                                      NEGCON_DEFAULT_STEER_RANGE);
        int analogI = (int)(AxisShapedDefault(rightTrigger, 0.0f) * (float)NEGCON_ANALOG_MAX);
        int analogII = (int)(AxisShapedDefault(leftTrigger, 0.0f) * (float)NEGCON_ANALOG_MAX);
        if (buttons & PAD_CROSS) analogI = NEGCON_ANALOG_MAX;
        if (buttons & PAD_SQUARE) analogII = NEGCON_ANALOG_MAX;
        s_controls.input.steering.mode = STEERING_ANALOG;
        s_controls.input.steering.left = s_controls.input.steering.right = 0;
        s_controls.input.steering.angle = CalibrateNegconSteer(twist) * NEGCON_STEERING_SCALE /
                                 NEGCON_DEFAULT_STEER_RANGE;
        s_controls.input.throttle = NegconPedal(analogI);
        s_controls.input.brake = NegconPedal(analogII);
    }
    if (pressed & NEGCON_SHIFT_UP) s_controls.pendingShiftUp = 1;
    if (pressed & NEGCON_SHIFT_DOWN) s_controls.pendingShiftDown = 1;
}


static float Daylight(const ClientRace *race) {
    RenderCamera environment;
    memset(&environment, 0, sizeof(environment));
    ApplyEnvironment(&environment, &race->env);
    return CarLightDaylight(environment.skyTopColor, environment.skyHorizonColor);
}

static RenderCamera BuildRaceCamera(const PlayerCarRuntime *car, WebView view,
                                    RenderCamera *mirror);

/* One game frame of the mirror panel (car_render_rules.c AdvanceMirrorPanelY
 * as rear_view_mirror.c drives it) and its on/off switches. */
static void AdvanceMirror(const RaceSim *sim, WebView view) {
    struct Mirror *m = &s_mirror;
    const int racing = sim->phase == SIM_RACING && s_localSeat >= 0 && s_view.viewSeat == s_localSeat &&
                       sim->drivers[s_localSeat].status == SIM_DRIVING;
    if (s_controls.pendingMirror && racing && view == WEB_VIEW_CAR) m->enabled = s_controls.pendingMirror > 0;
    s_controls.pendingMirror = 0;
    if (sim->phase >= SIM_RACING && !racing)
        m->enabled = 0; /* lap_and_finish.c: the finish turns it off. */
    m->panelPrevious = s_view.haveStep ? m->panelCurrent : MIRROR_PANEL_HIDDEN_Y;
    /* The panel starts moving with the race, as retail's unlock comes after
     * the start; it is hidden behind the countdown otherwise. */
    if (sim->phase >= SIM_RACING) {
        if (m->enabled) {
            if (m->panelCurrent < MIRROR_PANEL_VISIBLE_Y) ++m->panelCurrent;
        } else if (m->panelCurrent > MIRROR_PANEL_HIDDEN_Y) {
            --m->panelCurrent;
        }
    }
    /* mirror_pass.c MirrorPassIsAvailable, without the unlock and Grand Prix
     * conditions. */
    m->draw = m->enabled && view == WEB_VIEW_CAR && racing;
}

/* Snapshots every car's pose; a finished car drives on past the line. */
static void AdvanceFinishRuns(const RaceSim *sim) {
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &sim->drivers[seat];
        PlayerCarRuntime pose = driver->car;
        FinishRun *run = &s_view.run[seat];
        if (driver->status != SIM_DRIVER_FINISHED && run->steps) {
            run->steps = 0;
            run->ox = run->oy = run->oz = 0.0f;
        }
        if (driver->status == SIM_DRIVER_FINISHED) {
            if (run->steps <= FINISH_FADE_STEPS) {
                /* Ease off to about a third of the speed as it fades. */
                const float pace = 1.0f - 0.65f * (float)run->steps / FINISH_FADE_STEPS;
                ++run->steps;
                run->ox += run->motion[0] * pace;
                run->oy += run->motion[1] * pace;
                run->oz += run->motion[2] * pace;
            }
            pose.x += (s32)lroundf(run->ox);
            pose.y += (s32)lroundf(run->oy);
            pose.z += (s32)lroundf(run->oz);
        } else if (driver->status == SIM_DRIVING && s_view.haveStep) {
            run->motion[0] = (float)(pose.x - run->raw[0]);
            run->motion[1] = (float)(pose.y - run->raw[1]);
            run->motion[2] = (float)(pose.z - run->raw[2]);
        }
        run->raw[0] = driver->car.x;
        run->raw[1] = driver->car.y;
        run->raw[2] = driver->car.z;
        /* A recent correction still sliding out (see SmoothCorrections). */
        struct Smooth *smooth = &s_smooth[seat];
        pose.x += (s32)lroundf(smooth->x);
        pose.y += (s32)lroundf(smooth->y);
        pose.z += (s32)lroundf(smooth->z);
        pose.bodyYaw = (pose.bodyYaw + (s32)lroundf(smooth->yaw)) & ANGLE_MASK;
        smooth->x *= SMOOTH_DECAY;
        smooth->y *= SMOOTH_DECAY;
        smooth->z *= SMOOTH_DECAY;
        smooth->yaw *= SMOOTH_DECAY;
        s_view.posePrevious[seat] = s_view.haveStep ? s_view.poseCurrent[seat] : pose;
        s_view.poseCurrent[seat] = pose;
    }
}

/* The camera button and look-behind, as race_scene.c reads them once per
 * game frame; spectating someone else follows their car from behind. */
static WebView SelectView(int racing) {
    if (s_controls.pendingCamera && racing)
        s_view.selectedView = s_view.selectedView == WEB_VIEW_CAR ? WEB_VIEW_CHASE : WEB_VIEW_CAR;
    s_controls.pendingCamera = 0;
    if (s_view.viewSeat != s_localSeat) return WEB_VIEW_CHASE;
    return racing && s_view.selectedView == WEB_VIEW_CHASE && (s_controls.padHeld & PAD_DOWN)
               ? WEB_VIEW_LOOK_BEHIND : s_view.selectedView;
}

/* Snapshots poses and the race camera whenever the field physics stepped.
 * Retail reads the camera button and updates the camera once per game
 * frame, and the chase camera's yaw settling is tuned per frame, so all of
 * it advances only here. */
static void RecordPresentation(void) {
    const RaceSim *sim = &s_race->sim;
    struct Presentation *v = &s_view;
    /* The field moves every second clock tick while racing; the key follows
     * the race clock, so finished and spectated cars keep being presented. */
    const u32 stepTick = sim->phase >= SIM_RACING ? sim->elapsed / SIM_PHYSICS_INTERVAL + 1 : 0;
    RenderCamera camera, mirror;
    v->lastTickStepped = !v->haveStep || stepTick != v->lastStepTick;
    if (!v->lastTickStepped) return;
    AdvanceFinishRuns(sim);
    const WebView view = SelectView(sim->phase == SIM_RACING /* CanToggleRaceCamera */);
    camera = BuildRaceCamera(&v->poseCurrent[v->viewSeat], view, &mirror);
    AdvanceMirror(sim, view);
    /* A new view or car cuts; only frames within one view are interpolated. */
    if (v->viewCut) v->haveStep = 0;
    v->viewCut = 0;
    const int continuous = v->haveStep && view == v->viewCurrent;
    v->cameraPrevious = continuous ? v->cameraCurrent : camera;
    v->cameraCurrent = camera;
    s_mirror.previous = continuous ? s_mirror.current : mirror;
    s_mirror.current = mirror;
    v->viewCurrent = view;
    v->lastStepTick = stepTick;
    v->haveStep = 1;
}

/* One 50 Hz simulation tick. Returns the race phase, or -1 on failure. */
EMSCRIPTEN_KEEPALIVE int rw_tick(void) {
    DriverInput input;
    if (!s_race) return -1;
    input = s_controls.input;
    input.shiftUp = s_controls.pendingShiftUp;
    input.shiftDown = s_controls.pendingShiftDown;
    if (!s_net) {
        if (SetRaceInput(&s_race->sim, s_localSeat, &input))
            s_controls.pendingShiftUp = s_controls.pendingShiftDown = 0;
        StepRaceSim(&s_race->sim);
    } else if (s_predict.predicting) {
        /* A player predicts with this tick's controls (rw_take_input); a
         * spectator only shows the server's frames (rw_apply_frame). */
        if (s_predict.tickInputReady) SetRaceInput(&s_race->sim, s_localSeat, &s_predict.tickInput);
        s_predict.tickInputReady = 0;
        StepRaceSim(&s_race->sim);
    }
    if (!TickClientScenery(s_race)) return -1;
    TickRaceView(s_race->view, &s_race->sim, Daylight(s_race));
    RecordPresentation();
    WebAudioTick(&s_race->sim);
    return (int)s_race->sim.phase;
}

/* 1 when the last tick produced a new presentation snapshot. */
EMSCRIPTEN_KEEPALIVE int rw_last_tick_stepped(void) { return s_view.lastTickStepped; }

/* ---- Retail chase camera (track/camera_chase.c, mode 1) ------------------
 * The yaw settling is the retail integer code verbatim. The eye/look-at
 * geometry uses the same offsets, matrix order and angle formulas, evaluated
 * in floats instead of GTE fixed point: the camera is presentation only and
 * never feeds back into the simulation. */
static s32 Word(int64_t value) { return (s32)(uint32_t)(uint64_t)value; }

static s32 SquareRootInt(s32 value) {
    uint32_t x = value > 0 ? (uint32_t)value : 0u, root = 0, bit = 1u << 30;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= root + bit) { x -= root + bit; root = (root >> 1) + bit; }
        else root >>= 1;
        bit >>= 2;
    }
    return (s32)root;
}

static void SettleChaseYaw(WebChase *chase, s32 stepLimit, s32 acceleratedStep, int negative) {
    if (stepLimit < acceleratedStep) {
        chase->yawLag = negative ? -stepLimit : stepLimit;
        if (negative) chase->rampNeg = SquareRootInt(Word((int64_t)stepLimit * chase->damping));
        else chase->rampPos = SquareRootInt(Word((int64_t)stepLimit * chase->damping));
    } else {
        chase->yawLag = negative ? -acceleratedStep : acceleratedStep;
    }
}

static void AdvanceChaseYawRamp(WebChase *chase, s32 stepLimit, int negative) {
    s32 ramp, acceleratedStep;
    if (stepLimit > 0x40) stepLimit = 0x40;
    chase->stepLimit = stepLimit;
    ramp = Word((int64_t)(negative ? chase->rampNeg : chase->rampPos) + 8);
    acceleratedStep = Word((int64_t)ramp * ramp) / chase->damping;
    if (negative) { chase->rampPos = 0; chase->rampNeg = Word((int64_t)chase->rampNeg + 8); }
    else { chase->rampNeg = 0; chase->rampPos = Word((int64_t)chase->rampPos + 8); }
    chase->step = acceleratedStep;
    SettleChaseYaw(chase, stepLimit, acceleratedStep, negative);
}

static s32 ChaseYawDamping(s32 carSpeed) {
    s32 difference = Word((int64_t)0x4E2 - carSpeed), damping;
    if (carSpeed >= 0x321) {
        if (difference < 6) difference = 6;
        return ((((difference * 8) / 50) + 8) / 10) + 1;
    }
    damping = Word((int64_t)difference * 6);
    damping = Word((int64_t)damping * difference) / 2500;
    damping = Word((int64_t)damping - Word((int64_t)difference * 0x46) / 50);
    damping = Word((int64_t)damping + 0xE0) / 10;
    return damping > 0 ? damping : 1;
}

static void UpdateChaseYawStep(WebChase *chase, s32 targetYaw, s32 previousYaw) {
    s32 error = Word((int64_t)targetYaw - previousYaw);
    if (error >= 5) {
        if (error >= 0x800) AdvanceChaseYawRamp(chase, (((0x1000 - error) / 17) * 2) & ANGLE_MASK, 1);
        else AdvanceChaseYawRamp(chase, ((error / 17) * 2) & ANGLE_MASK, 0);
    } else if (error < -4) {
        if (error < -0x7FF) AdvanceChaseYawRamp(chase, (((0x1000 + error) / 17) * 2) & ANGLE_MASK, 0);
        else AdvanceChaseYawRamp(chase, ((Word(-(int64_t)error) / 17) * 2) & ANGLE_MASK, 1);
    } else {
        chase->yawLag = chase->rampNeg = chase->rampPos = 0;
    }
}

static SceneMat3 CarRotation(const PlayerCarRuntime *car) {
    return SceneMat3Multiply(SceneRotationZ(car->bodyRoll),
                             SceneMat3Multiply(SceneRotationX(car->bodyPitch),
                                               SceneRotationY(car->bodyYaw)));
}

/* Eye position and PS1 view angles, as CameraViewFromChaseCamera leaves them
 * for chase preset 0 (eye 0x3A up, 0x118 back). */
static void RetailChaseView(const PlayerCarRuntime *car, Vec3 *eye,
                            s32 *pitch, s32 *yaw, s32 *roll) {
    WebChase *chase = &s_view.chase;
    s32 target = car->bodyYaw & ANGLE_MASK, settled, lag;
    SceneMat3 cameraRotation, object, inverseObject, work;
    Vec3 focus, eyeWorld;
    s32 ex, ey, ez, distance, angleX;

    if (chase->active) {
        chase->previousYaw &= ANGLE_MASK;
        chase->rampNeg &= ANGLE_MASK;
        chase->rampPos &= ANGLE_MASK;
    } else {
        chase->previousYaw = target;
        chase->rampNeg = chase->rampPos = 0;
        chase->active = 1;
    }
    chase->damping = ChaseYawDamping(car->speed);
    UpdateChaseYawStep(chase, target, chase->previousYaw);
    settled = Word((int64_t)chase->previousYaw + chase->yawLag) & ANGLE_MASK;
    lag = Word((int64_t)target - settled);
    if (target < settled) { if (lag < -0x7FF) lag = Word((int64_t)lag + 0x1000); }
    else if (lag >= 0x800) lag = Word((int64_t)lag - 0x1000);
    chase->yawLag = lag;
    chase->previousYaw = settled;

    cameraRotation = SceneMat3Multiply(SceneRotationX(-0x80), SceneRotationY(-lag));
    object = CarRotation(car);
    inverseObject = SceneMat3Transpose(object);
    work = SceneMat3Transpose(SceneMat3Multiply(cameraRotation, object));

    focus = SceneRotatePoint(inverseObject, 0.0f, -0x3C, 0x32);
    eyeWorld = SceneRotatePoint(work, 0.0f, (float)ChaseCameraHeight(0x3A),
                           (float)ChaseCameraDistance(0x118));
    eye->x = (float)car->x + focus.x - eyeWorld.x;
    eye->y = (float)car->y + focus.y - eyeWorld.y;
    eye->z = (float)car->z + focus.z - eyeWorld.z;

    ex = (s32)lroundf(eyeWorld.x);
    ey = (s32)lroundf(eyeWorld.y);
    ez = (s32)lroundf(eyeWorld.z);
    distance = SquareRootInt(Word((int64_t)ex * ex + (int64_t)ez * ez));
    angleX = 0x400 - (Atan2(Word((int64_t)ey + 0x28), distance) & ANGLE_MASK);
    *yaw = 0x400 - (Atan2(ex, ez) & ANGLE_MASK) + ChaseCameraYawOffset(car->steeringAngle);
    *roll = Word((int64_t)car->bodyRoll - car->bodyRollVelocity);
    *pitch = angleX - 0x90 + ChaseCameraPitchOffset();
}

/* Mode 0, CameraViewFromCarBlock: the car's own pose, lifted along its up
 * axis and pitched by its tilt counter. */
static void RetailCarView(const PlayerCarRuntime *car, Vec3 *eye, s32 *pitch, s32 *yaw, s32 *roll) {
    const Vec3 lift = SceneRotatePoint(SceneMat3Transpose(CarRotation(car)), 0.0f, -0x1C0 / 16.0f, 0.0f);
    eye->x = (float)car->x + lift.x;
    eye->y = (float)car->y + lift.y;
    eye->z = (float)car->z + lift.z;
    *pitch = Word((int64_t)car->bodyPitch + car->tiltCounter);
    *yaw = car->bodyYaw;
    *roll = car->bodyRoll;
}

/* CameraViewFromLookBehind: the orbit camera turned round behind the car. */
static void RetailLookBehindView(const PlayerCarRuntime *car, Vec3 *eye, s32 *pitch, s32 *yaw,
                                 s32 *roll) {
    enum { LOOK_BEHIND_YAW = 0x800, LOOK_BEHIND_DISTANCE = 0xE0, LOOK_BEHIND_HEIGHT = 0x50 };
    const SceneMat3 object = CarRotation(car);
    const SceneMat3 cameraToWorld =
        SceneMat3Transpose(SceneMat3Multiply(SceneRotationY(-LOOK_BEHIND_YAW), object));
    const Vec3 focus = SceneRotatePoint(SceneMat3Transpose(object), 0.0f, 0.0f, 0x32);
    const Vec3 eyeWorld = SceneRotatePoint(cameraToWorld, 0.0f, LOOK_BEHIND_HEIGHT, LOOK_BEHIND_DISTANCE);
    eye->x = (float)car->x + focus.x - eyeWorld.x;
    eye->y = (float)car->y + focus.y - 0x28 - eyeWorld.y;
    eye->z = (float)car->z + focus.z - eyeWorld.z;
    *pitch = 0x400 - (Atan2((s32)lroundf(eyeWorld.y), LOOK_BEHIND_DISTANCE) & ANGLE_MASK);
    *yaw = 0x400 - (Atan2((s32)lroundf(eyeWorld.x), (s32)lroundf(eyeWorld.z)) & ANGLE_MASK);
    *roll = car->bodyRoll;
}

/* render_world_game.c's GameRenderWorldBuildCamera: near 1, the verified
 * race depth limit. A rear-facing (mirror) camera pre-rotates the view basis
 * by 180 degrees in its own local space, like an attached camera rig. */
static RenderCamera CameraFromView(Vec3 eye, s32 pitch, s32 yaw, s32 roll,
                                   float verticalFovDegrees, int rearFacing) {
    RenderCamera camera;
    SceneMat3 view, converted;

    memset(&camera, 0, sizeof(camera));
    camera.transform.position = (Vec3){eye.x, -eye.y, -eye.z};
    view = SceneMat3Multiply(SceneMat3Multiply(SceneRotationZ(roll), SceneRotationX(pitch)),
                             SceneRotationY(yaw));
    if (rearFacing) view = SceneMat3Multiply(SceneRotationY(0x800), view);
    RenderConvertPsxMatrix(view.m, converted.m);
    camera.transform.orientation = SceneQuaternion(SceneMat3Transpose(converted));
    camera.transform.hasOrientation = 1;
    camera.transform.rotation = (Vec3){-AngleToDegrees(pitch), -AngleToDegrees(yaw),
                                       -AngleToDegrees(roll)};
    camera.transform.scale = (Vec3){1.0f, 1.0f, 1.0f};
    camera.verticalFovDegrees = verticalFovDegrees;
    camera.nearPlane = 1.0f;
    camera.farPlane = 16384.0f;
    WebSkySetCamera(&camera, s_race, (s32)lroundf(eye.y), pitch, yaw, roll, rearFacing);
    return camera;
}

/* The race view with the PAL 320x240 projection (geom screen 320: 41.112
 * degrees), plus the rear-view mirror camera render_world_game.c's
 * GameRenderWorldPublishCurrentCamera derives from the same view: 20 degrees
 * vertically on the wide mirror target. The chase camera only settles while
 * it is the view, as retail's previousMode check restarts it otherwise. */
static RenderCamera BuildRaceCamera(const PlayerCarRuntime *car, WebView selected,
                                    RenderCamera *mirror) {
    Vec3 eye;
    s32 pitch, yaw, roll;

    if (selected == WEB_VIEW_CHASE) {
        RetailChaseView(car, &eye, &pitch, &yaw, &roll);
    } else {
        s_view.chase.active = 0;
        if (selected == WEB_VIEW_LOOK_BEHIND) RetailLookBehindView(car, &eye, &pitch, &yaw, &roll);
        else RetailCarView(car, &eye, &pitch, &yaw, &roll);
    }
    *mirror = CameraFromView(eye, pitch, yaw, roll, MIRROR_FOV_DEGREES, 1);
    return CameraFromView(eye, pitch, yaw, roll, 41.112f, 0);
}

/* Same rotation as modern_native_gpu.c's ModernNativeRotate. */
static void RotateByCamera(float out[3], const float in[3], const RenderCamera *camera) {
    float x = in[0], y = in[1], z = in[2];
    if (camera->transform.hasOrientation) {
        const Quaternion *q = &camera->transform.orientation;
        float length = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
        if (length > 0.0f) {
            float qx = -q->x / length, qy = -q->y / length;
            float qz = -q->z / length, qw = q->w / length;
            float xx = qx * qx, yy = qy * qy, zz = qz * qz;
            float xy = qx * qy, xz = qx * qz, yz = qy * qz;
            float wx = qw * qx, wy = qw * qy, wz = qw * qz;
            out[0] = (1.0f - 2.0f * (yy + zz)) * x + 2.0f * (xy - wz) * y + 2.0f * (xz + wy) * z;
            out[1] = 2.0f * (xy + wz) * x + (1.0f - 2.0f * (xx + zz)) * y + 2.0f * (yz - wx) * z;
            out[2] = 2.0f * (xz - wy) * x + 2.0f * (yz + wx) * y + (1.0f - 2.0f * (xx + yy)) * z;
            return;
        }
    }
    {
        float rx = -camera->transform.rotation.x * 0.017453292519943295f;
        float ry = -camera->transform.rotation.y * 0.017453292519943295f;
        float rz = -camera->transform.rotation.z * 0.017453292519943295f;
        float c = cosf(rz), s = sinf(rz), next;
        next = x * c - y * s; y = x * s + y * c; x = next;
        c = cosf(ry); s = sinf(ry);
        next = x * c + z * s; z = -x * s + z * c; x = next;
        c = cosf(rx); s = sinf(rx);
        next = y * c - z * s; z = y * s + z * c; y = next;
    }
    out[0] = x; out[1] = y; out[2] = z;
}

/* Same uniform block as modern_native_gpu.c's ModernNativeBuildCamera. */
static int BuildCameraUniform(const RenderCamera *camera, float aspect, float out[28]) {
    static const float axes[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float columns[3][3];
    memset(out, 0, 28 * sizeof(*out));
    if (!RenderPerspectiveScales(camera, aspect, &out[16], &out[17]) ||
        !RenderPerspectiveDepthTerms(camera, &out[18], &out[19])) return 0;
    out[0] = camera->transform.position.x;
    out[1] = camera->transform.position.y;
    out[2] = camera->transform.position.z;
    for (int axis = 0; axis < 3; ++axis) {
        RotateByCamera(columns[axis], axes[axis], camera);
        out[4 + axis] = columns[axis][0];
        out[8 + axis] = columns[axis][1];
        out[12 + axis] = columns[axis][2];
    }
    out[20] = camera->fogColor.x;
    out[21] = camera->fogColor.y;
    out[22] = camera->fogColor.z;
    if (isfinite(camera->fogNear) && isfinite(camera->fogFar) &&
        camera->fogNear > 0.0f && camera->fogFar > camera->fogNear) {
        out[24] = camera->fogNear;
        out[25] = camera->fogFar;
        out[26] = 1.0f / camera->fogNear;
        out[27] = out[26] - 1.0f / camera->fogFar;
    }
    return 1;
}

static const RageRuntimeMesh *ResolveMesh(void *context, const RenderMeshInstance *instance) {
    const RageImportedMeshEntry *entry = FindClientMesh(context, instance);
    return entry ? &entry->cached.mesh : NULL;
}

static void StoreVec3(float *out, Vec3 value) {
    out[0] = value.x; out[1] = value.y; out[2] = value.z; out[3] = 0.0f;
}

/* The rear-view mirror as the native modern renderer draws it
 * (modern_native_gpu.c Prepare, modern_renderer.c ModernCompositeNativeMirror):
 * the same world again from the rear camera, with the mirror's own aspect and
 * doubled fog range, built after the main view in the same vertex and span
 * buffers. The browser flips it into the 148x36 panel. */
static void BuildMirror(float t) {
    struct Mirror *m = &s_mirror;
    RenderWorld mirrorWorld;
    RenderCamera mirror;
    m->vertexCount = m->spanCount = 0;
    memset(m->state, 0, sizeof(m->state));
    if (!m->draw) return;
    RenderInterpolateCamera(&m->previous, &m->current, t, &mirror);
    ApplyEnvironment(&mirror, &s_race->env);
    /* render_world_game.c: the tiny mirror keeps useful silhouettes by
     * reaching twice as far into the fog as the main view. */
    mirror.fogNear *= 2.0f;
    mirror.fogFar *= 2.0f;
    mirrorWorld = s_world;
    mirrorWorld.camera = mirror;
    m->vertexCount = RenderBuildNativePassDraws(
        &mirrorWorld, RAGE_RENDER_PASS_MAIN, MIRROR_ASPECT, ResolveMesh, s_race,
        s_vertices + s_frame.vertexCount, WEB_VERTEX_CAPACITY - s_frame.vertexCount,
        s_spans + s_frame.spanCount, WEB_SPAN_CAPACITY - s_frame.spanCount, &m->spanCount);
    for (uint32_t i = 0; i < m->spanCount; ++i) s_spans[s_frame.spanCount + i].firstVertex += s_frame.vertexCount;
    if (!BuildCameraUniform(&mirror, MIRROR_ASPECT, m->uniform)) {
        m->vertexCount = m->spanCount = 0;
        return;
    }
    WebSkyUniform(&mirror, MIRROR_ASPECT, m->sky);
    m->state[0] = 1.0f;
    m->state[1] = (float)m->panelPrevious + (float)(m->panelCurrent - m->panelPrevious) * t;
    m->state[2] = (float)s_frame.vertexCount;
    m->state[3] = (float)m->vertexCount;
}

/* 255 for a span of anything but a fading car; less as its car fades out,
 * interpolated between steps like its pose. */
static uint8_t SpanAlpha(const RageNativeDrawSpan *span, float t) {
    /* sourceEntity is the drawn instance (entity is 0 outside model banks). */
    const uint32_t seat = span->sourceEntity;
    if (seat >= DRIVER_SEAT_LIMIT || !s_view.run[seat].steps) return 255;
    const float faded = ((float)s_view.run[seat].steps - 1.0f + t) / FINISH_FADE_STEPS;
    const float alpha = 1.0f - (faded < 0.0f ? 0.0f : faded > 1.0f ? 1.0f : faded);
    return (uint8_t)lroundf(alpha * 254.0f);
}

static uint32_t SpanTotal(void) { return s_frame.spanCount + s_mirror.spanCount; }

/* The whole scene, as the native client submits it. */
static int SubmitScene(int page) {
    return SubmitClientTerrain(s_race, page, &s_world) &&
           SubmitClientScenery(s_race, page, &s_world) &&
           SubmitRaceViewPoses(&s_race->sim, s_race->view, s_view.poseCurrent, s_view.posePrevious,
                               s_race->rivals, s_race->primaryMesh.cached.assetKey, 0, &s_world) &&
           SubmitClientShuttles(s_race, page, &s_world) &&
           SubmitClientSpinners(s_race, page, &s_world) &&
           SubmitClientLandmarks(s_race, page, &s_world);
}

/* Vehicles carry the previous physics step as their previous transform;
 * everything else animates per clock tick. */
/* Where each car was drawn in the last frame (its first part), for checks. */
static float s_presented[DRIVER_SEAT_LIMIT][3];
EMSCRIPTEN_KEEPALIVE const float *rw_presented(int seat) {
    return seat >= 0 && seat < DRIVER_SEAT_LIMIT ? s_presented[seat] : s_presented[0];
}

static void InterpolateVehicles(float t) {
    int seen[DRIVER_SEAT_LIMIT] = {0};
    for (uint32_t i = 0; i < s_world.instanceCount; ++i) {
        RenderMeshInstance *instance = &s_world.instances[i];
        if (instance->entity >= DRIVER_SEAT_LIMIT) continue;
        RenderTransform mixed;
        RenderInterpolateTransform(&instance->previousTransform, &instance->transform, t, &mixed);
        instance->transform = mixed;
        if (!seen[instance->entity]) {
            seen[instance->entity] = 1;
            s_presented[instance->entity][0] = mixed.position.x;
            s_presented[instance->entity][1] = mixed.position.y;
            s_presented[instance->entity][2] = mixed.position.z;
        }
        /* update_camera.c draws the player's car only outside the car view. */
        if (instance->entity == (uint32_t)s_view.viewSeat && s_view.viewCurrent == WEB_VIEW_CAR)
            instance->flags |= RAGE_RENDER_INSTANCE_RAY_ONLY;
        /* A finished car is gone once it has faded out. */
        if (s_view.run[instance->entity].steps > FINISH_FADE_STEPS)
            instance->flags |= RAGE_RENDER_INSTANCE_RAY_ONLY;
    }
}

static void StoreShadow(void) {
    RenderShadowMap shadow;
    const Vec3 center = RenderShadowCenter(&s_world);
    s_frame.shadowValid = RenderBuildDirectionalShadowMap(
        &center, &s_world.light.direction, RAGE_RENDER_VEHICLE_SHADOW_EXTENT,
        RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION, &shadow);
    memset(s_frame.shadow, 0, sizeof(s_frame.shadow));
    if (!s_frame.shadowValid) return;
    StoreVec3(&s_frame.shadow[0], shadow.position);
    StoreVec3(&s_frame.shadow[4], shadow.row0);
    StoreVec3(&s_frame.shadow[8], shadow.row1);
    StoreVec3(&s_frame.shadow[12], shadow.row2);
    s_frame.shadow[16] = shadow.scaleX;
    s_frame.shadow[17] = shadow.scaleY;
    s_frame.shadow[18] = shadow.depthScale;
    s_frame.shadow[19] = shadow.depthOffset;
}

static void PackSpanFields(float t) {
    for (uint32_t i = 0; i < SpanTotal(); ++i) {
        const RageNativeDrawSpan *span = &s_spans[i];
        uint32_t *out = &s_spanFields[i * WEB_SPAN_FIELDS];
        out[0] = span->firstVertex;
        out[1] = span->vertexCount;
        out[2] = span->material;
        out[3] = (uint32_t)span->assetSet;
        out[4] = (uint32_t)span->assetSource;
        out[5] = span->assetKey;
        out[6] = span->materialVariant;
        out[7] = span->hasCarPaint;
        out[8] = span->carPaintColor1;
        out[9] = span->carPaintColor2;
        out[10] = span->instanceFlags;
        out[11] = span->materialFlags;
        out[12] = span->depthDecal;
        out[WEB_SPAN_ALPHA] = SpanAlpha(span, t);
    }
}

static void StoreLight(void) {
    StoreVec3(&s_frame.light[0], s_world.light.direction);
    StoreVec3(&s_frame.light[4], s_world.light.ambientColor);
    StoreVec3(&s_frame.light[8], s_world.light.diffuseColor);
    StoreVec3(&s_frame.light[12], s_world.camera.skyTopColor);
    StoreVec3(&s_frame.light[16], s_world.camera.skyHorizonColor);
    StoreVec3(&s_frame.light[20], s_world.camera.skyBottomColor);
}

static int DrawSubmitted(float aspect, float t, int withMirror);

/* Builds the scene presented `t` (0..1) of the way from the previous physics
 * step to the latest one, with the native sequence, and expands it into
 * world-space triangles. Returns the vertex count, or -1 on failure. */
EMSCRIPTEN_KEEPALIVE int rw_build_frame(float aspect, float t) {
    RenderDirectionalLight light;
    RenderCamera camera;
    if (!s_race || !s_view.haveStep || !(aspect > 0.0f)) return -1;
    const PlayerCarRuntime *viewed = &s_view.poseCurrent[s_view.viewSeat];
    s_frame.page = viewed->trackSection >= s_race->look.textureSectionLo &&
                   viewed->trackSection < s_race->look.textureSectionHi;
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    RenderWorldBeginFrame(&s_world, ++s_frame.number);
    RenderInterpolateCamera(&s_view.cameraPrevious, &s_view.cameraCurrent, t, &camera);
    ApplyEnvironment(&camera, &s_race->env);
    RenderWorldSetCamera(&s_world, &camera);
    RenderDirectionalLightFromSky(&camera, &light);
    RenderWorldSetDirectionalLight(&s_world, &light);
    if (!SubmitScene(s_frame.page)) return -1;
    RenderWorldFocus(&s_world, (uint32_t)s_view.viewSeat);
    InterpolateVehicles(t);
    return DrawSubmitted(aspect, t, 1);
}

/* Expands the submitted scene into the packed draw buffers and the uniform
 * blocks the renderer reads. Returns the vertex count, or -1. */
static int DrawSubmitted(float aspect, float t, int withMirror) {
    StoreShadow();
    s_frame.vertexCount = RenderBuildNativePassDraws(
        &s_world, RAGE_RENDER_PASS_MAIN, aspect, ResolveMesh, s_race,
        s_vertices, WEB_VERTEX_CAPACITY, s_spans, WEB_SPAN_CAPACITY, &s_frame.spanCount);
    if (withMirror) BuildMirror(t);
    PackSpanFields(t);
    if (!BuildCameraUniform(&s_world.camera, aspect, s_frame.camera)) return -1;
    StoreLight();
    WebSkyUniform(&s_world.camera, aspect, s_frame.sky);
    return (int)s_frame.vertexCount;
}

/* The garage preview: a race prepared for one car (rw_start_race, alone on
 * the grid), drawn without its field from a camera circling the car,
 * `angle` degrees round it. The paint follows rw_set_showroom_paint. */
EMSCRIPTEN_KEEPALIVE void rw_set_showroom_paint(int first, int second) {
    if (!s_race || !s_race->view) return;
    RaceCarLook *look = &s_race->view->looks[0];
    look->hasPaint = first >= 0 && first < RAGE_CAR_PAINT_COLOR_COUNT &&
                     second >= 0 && second < RAGE_CAR_PAINT_COLOR_COUNT;
    look->paint.paintColor1 = (u8)(look->hasPaint ? first : 0);
    look->paint.paintColor2 = (u8)(look->hasPaint ? second : 0);
}

EMSCRIPTEN_KEEPALIVE int rw_build_showroom(float aspect, float angle) {
    enum { RADIUS = 330, HEIGHT = 105, FOCUS_HEIGHT = 30 };
    if (!s_race || !(aspect > 0.0f)) return -1;
    PlayerCarRuntime cars[DRIVER_SEAT_LIMIT];
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) cars[seat] = s_race->sim.drivers[seat].car;
    const PlayerCarRuntime *car = &cars[0];
    s_frame.page = car->trackSection >= s_race->look.textureSectionLo &&
                   car->trackSection < s_race->look.textureSectionHi;
    /* Game coordinates: y points down, so the eye is HEIGHT above the focus
     * when it is that far negative; the view angles are the retail chase
     * camera's (yaw and pitch from the eye-to-focus direction). */
    const float radians = angle * 0.017453292519943295f;
    const Vec3 toFocus = {-RADIUS * sinf(radians), HEIGHT, -RADIUS * cosf(radians)};
    const Vec3 focus = {(float)car->x, (float)car->y - FOCUS_HEIGHT, (float)car->z};
    const Vec3 eye = {focus.x - toFocus.x, focus.y - toFocus.y, focus.z - toFocus.z};
    const s32 horizontal = (s32)lroundf(sqrtf(toFocus.x * toFocus.x + toFocus.z * toFocus.z));
    const s32 yaw = 0x400 - (Atan2((s32)lroundf(toFocus.x), (s32)lroundf(toFocus.z)) & ANGLE_MASK);
    const s32 pitch = 0x400 - (Atan2((s32)lroundf(toFocus.y), horizontal) & ANGLE_MASK);
    RenderCamera camera = CameraFromView(eye, pitch, yaw, 0, 30.0f, 0);
    RenderWorldBeginFrame(&s_world, ++s_frame.number);
    ApplyEnvironment(&camera, &s_race->env);
    RenderWorldSetCamera(&s_world, &camera);
    RenderDirectionalLight light;
    RenderDirectionalLightFromSky(&camera, &light);
    RenderWorldSetDirectionalLight(&s_world, &light);
    if (!SubmitClientTerrain(s_race, s_frame.page, &s_world) ||
        !SubmitRaceViewPoses(&s_race->sim, s_race->view, cars, cars, s_race->rivals,
                             s_race->primaryMesh.cached.assetKey, 0, &s_world)) return -1;
    RenderWorldFocus(&s_world, 0);
    return DrawSubmitted(aspect, 1.0f, 0); /* no interpolation: the poses are the same */
}

/* FNV-1a step. */
static uint32_t FnvMix(uint32_t hash, uint32_t value) { return (hash ^ value) * 16777619u; }

static uint32_t PaletteHash(void) {
    const uint8_t *bytes = (const uint8_t *)s_race->env.clut;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < sizeof(s_race->env.clut); ++i) hash = FnvMix(hash, bytes[i]);
    return hash;
}

/* The native sky uniform block (web_sky.h) for the last built frame. */
EMSCRIPTEN_KEEPALIVE float *rw_sky(void) { return s_frame.sky; }

/* The 512x256 cloud panorama for the last built frame, as the native backend
 * uploads it; 0 when it cannot be decoded (the gradient still draws). */
EMSCRIPTEN_KEEPALIVE int rw_decode_sky(uint8_t *rgba) {
    return s_race && s_view.haveStep &&
           WebSkyDecode(s_race, &s_world.camera, s_frame.page, rgba, WEB_SKY_WIDTH * WEB_SKY_HEIGHT * 4);
}

/* Changes whenever the panorama's inputs do (palette, texture page, cloud
 * row), so the browser knows when to decode it again. */
EMSCRIPTEN_KEEPALIVE uint32_t rw_sky_revision(void) {
    return s_race ? FnvMix(FnvMix(PaletteHash(), (uint32_t)s_frame.page), s_world.camera.skyCloudRow) : 0;
}
EMSCRIPTEN_KEEPALIVE int rw_sky_width(void) { return WEB_SKY_WIDTH; }
EMSCRIPTEN_KEEPALIVE int rw_sky_height(void) { return WEB_SKY_HEIGHT; }

/* WEB_SPAN_FIELDS words per span of the main view, followed by the mirror's
 * (rw_mirror_span_count; rw_decode_texture_mips takes indices into both). */
EMSCRIPTEN_KEEPALIVE uint32_t *rw_spans(void) { return s_spanFields; }
EMSCRIPTEN_KEEPALIVE int rw_span_count(void) { return (int)s_frame.spanCount; }
EMSCRIPTEN_KEEPALIVE int rw_mirror_span_count(void) { return (int)s_mirror.spanCount; }
/* Mirror state of the last built frame (see struct Mirror), its camera
 * uniform (as rw_camera) and sky uniform (as rw_sky, for the mirror target). */
EMSCRIPTEN_KEEPALIVE float *rw_mirror(void) { return s_mirror.state; }
EMSCRIPTEN_KEEPALIVE float *rw_mirror_camera(void) { return s_mirror.uniform; }
EMSCRIPTEN_KEEPALIVE float *rw_mirror_sky(void) { return s_mirror.sky; }
EMSCRIPTEN_KEEPALIVE float *rw_camera(void) { return s_frame.camera; }
EMSCRIPTEN_KEEPALIVE float *rw_light(void) { return s_frame.light; }
EMSCRIPTEN_KEEPALIVE int rw_packed_floats(void) { return WEB_PACKED_FLOATS; }
EMSCRIPTEN_KEEPALIVE float *rw_shadow(void) { return s_frame.shadowValid ? s_frame.shadow : NULL; }
EMSCRIPTEN_KEEPALIVE int rw_shadow_resolution(void) { return RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION; }

/* The draw vertex mixes floats with a byte colour; WebGL wants one typed
 * buffer per upload, so this repacks the frame into floats only:
 * position 3, uv 2, colour 4 (0..255), normal 3, fog 4 (colour, weight),
 * lighting 1, environment light 3, depth bias 1, shadow reception 1. */
EMSCRIPTEN_KEEPALIVE float *rw_pack_vertices(void) {
    for (uint32_t i = 0; i < s_frame.vertexCount + s_mirror.vertexCount; ++i) {
        const RageNativeDrawVertex *v = &s_vertices[i];
        float *out = &s_packed[(size_t)i * WEB_PACKED_FLOATS];
        memcpy(out, v->position, sizeof(v->position));
        memcpy(out + 3, v->uv, sizeof(v->uv));
        for (int c = 0; c < 4; ++c) out[5 + c] = (float)v->color[c];
        memcpy(out + 9, v->normal, sizeof(v->normal));
        memcpy(out + 12, v->fog, sizeof(v->fog));
        out[16] = v->lighting;
        memcpy(out + 17, v->environmentLight, sizeof(v->environmentLight));
        out[20] = v->depthBias;
        out[21] = v->shadowReception;
    }
    for (uint32_t i = 0; i < SpanTotal(); ++i) {
        const RageNativeDrawSpan *span = &s_spans[i];
        /* Fading cars: their colour alpha scales every texel (drawn blended). */
        const uint32_t alpha = s_spanFields[i * WEB_SPAN_FIELDS + WEB_SPAN_ALPHA];
        /* Vehicles cast the shadow map but do not sample it on themselves:
         * the native backend traces those rays instead, and a map lookup on
         * their low-poly surfaces is all acne (see RageNativeDrawVertex). */
        const int vehicle = span->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
                            span->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
        if (alpha == 255 && !vehicle) continue;
        for (uint32_t v = 0; v < span->vertexCount; ++v) {
            float *out = &s_packed[(size_t)(span->firstVertex + v) * WEB_PACKED_FLOATS];
            if (alpha != 255) out[8] *= (float)alpha / 255.0f;
            if (vehicle) out[21] = 0.0f;
        }
    }
    return s_packed;
}

/* 256x256 RGBA for one span's material, exactly as the native backend
 * reconstructs it; the palette is the race's current environment CLUT. */
static int DecodeSpanTexture(int spanIndex, uint8_t *rgba) {
    RenderMeshInstance instance;
    const RageNativeDrawSpan *span;
    if (!s_race || spanIndex < 0 || (uint32_t)spanIndex >= SpanTotal() || !rgba) return 0;
    span = &s_spans[spanIndex];
    if (span->material == UINT32_MAX) return 0;
    memset(&instance, 0, sizeof(instance));
    instance.assetKey = span->assetKey;
    instance.assetSet = span->assetSet;
    instance.assetSource = span->assetSource;
    instance.hasCarPaint = span->hasCarPaint;
    instance.carPaintColor1 = span->carPaintColor1;
    instance.carPaintColor2 = span->carPaintColor2;
    instance.materialVariant = span->materialVariant;
    return DecodeClientMaterial(s_race, &instance, span->material, s_frame.page, s_race->env.clut,
                                rgba, WEB_TEXTURE_BYTES);
}

/* The same premultiplied atlas mip chain the native backend uploads: PS1
 * material pages are dense atlases, so only RAGE_TEXTURE_ATLAS_MIP_LEVELS
 * levels exist; smaller ones would blend unrelated entries. `scratch` takes
 * the 256x256 RGBA base level. Returns the chain (levels back to back, see
 * rw_texture_level_offset) or NULL. */
EMSCRIPTEN_KEEPALIVE uint8_t *rw_decode_texture_mips(int spanIndex, uint8_t *scratch) {
    const size_t size = TextureMipChainSizeRGBA8(256, 256, RAGE_TEXTURE_ATLAS_MIP_LEVELS);
    if (!s_mipChain) s_mipChain = malloc(size);
    if (!s_mipChain || !DecodeSpanTexture(spanIndex, scratch) ||
        !TextureBuildMipChainRGBA8(scratch, 256, 256, RAGE_TEXTURE_ATLAS_MIP_LEVELS, s_mipChain, size))
        return NULL;
    return s_mipChain;
}

/* 0 or 1: the track texture page of the last built frame. */
EMSCRIPTEN_KEEPALIVE int rw_texture_page(void) { return s_frame.page; }

EMSCRIPTEN_KEEPALIVE int rw_texture_levels(void) { return RAGE_TEXTURE_ATLAS_MIP_LEVELS; }
EMSCRIPTEN_KEEPALIVE int rw_texture_level_offset(int level) {
    return (int)TextureMipLevelOffsetRGBA8(256, 256, (uint32_t)level);
}

/* Changes whenever the environment palette (time of day) changes, so the
 * browser knows when palette-dependent textures must be decoded again. */
EMSCRIPTEN_KEEPALIVE uint32_t rw_palette_hash(void) { return s_race ? PaletteHash() : 0; }

/* A seat's current lap, capped at the race's laps once it has finished. */
static int SeatLap(int seat) {
    const int lap = s_race->sim.drivers[seat].car.lap;
    return lap > s_race->sim.laps ? s_race->sim.laps : lap;
}

/* phase, countdown ticks left, lap, laps, place, entrants, race time ms,
 * speed (retail units), gear, status, finish place (unused by the browser),
 * tick, then the viewed car's exact x, y, z and body yaw (used to check
 * physics parity). */
EMSCRIPTEN_KEEPALIVE int32_t *rw_hud(void) {
    const SimDriver *driver;
    const int seat = s_view.viewSeat;
    int entrants = 0;
    if (!s_race) return NULL;
    driver = &s_race->sim.drivers[seat];
    for (int other = 0; other < DRIVER_SEAT_LIMIT; ++other)
        entrants += s_race->sim.drivers[other].status != SIM_EMPTY;
    s_hud[0] = (int32_t)s_race->sim.phase;
    s_hud[1] = (int32_t)s_race->sim.countdown;
    s_hud[2] = SeatLap(seat);
    s_hud[3] = s_race->sim.laps;
    s_hud[4] = RacePosition(&s_race->sim, seat);
    s_hud[5] = entrants;
    s_hud[6] = RaceTime(&s_race->sim, seat);
    s_hud[7] = driver->car.speed * 160 / 1168; /* km/h, as the retail readout */
    s_hud[8] = driver->car.drive.gear;
    s_hud[9] = (int32_t)driver->status;
    s_hud[10] = driver->place;
    s_hud[11] = (int32_t)s_race->sim.tick;
    s_hud[12] = driver->car.x;
    s_hud[13] = driver->car.y;
    s_hud[14] = driver->car.z;
    s_hud[15] = driver->car.bodyYaw;
    return s_hud;
}

EMSCRIPTEN_KEEPALIVE int rw_car_variants(void) { return CAR_MODEL_VARIANT_COUNT; }

/* Standings for any seat: place (1-based, 0 when out), current lap and
 * SimDriverStatus. The HUD lists the online players with these. */
EMSCRIPTEN_KEEPALIVE int rw_seat_place(int seat) {
    return s_race && seat >= 0 && seat < DRIVER_SEAT_LIMIT ? RacePosition(&s_race->sim, seat) : 0;
}
EMSCRIPTEN_KEEPALIVE int rw_seat_lap(int seat) {
    return s_race && seat >= 0 && seat < DRIVER_SEAT_LIMIT ? SeatLap(seat) : 0;
}
EMSCRIPTEN_KEEPALIVE int rw_seat_status(int seat) {
    return s_race && seat >= 0 && seat < DRIVER_SEAT_LIMIT ? (int)s_race->sim.drivers[seat].status : 0;
}

/* The retail tachometer (web_hud.c): its sprite atlas for the prepared race,
 * and this game frame's packet fields for the local car. */
EMSCRIPTEN_KEEPALIVE const uint8_t *rw_hud_atlas(void) { return WebHudAtlas(); }
EMSCRIPTEN_KEEPALIVE int rw_hud_atlas_width(void) { return WEB_HUD_ATLAS_WIDTH; }
EMSCRIPTEN_KEEPALIVE int rw_hud_atlas_height(void) { return WEB_HUD_ATLAS_HEIGHT; }
EMSCRIPTEN_KEEPALIVE const int32_t *rw_tachometer(void) {
    return WebHudTachometer(s_race, s_view.viewSeat == s_localSeat ? s_localSeat : -1);
}
EMSCRIPTEN_KEEPALIVE int rw_tachometer_words(void) { return WEB_HUD_TACHO_WORDS; }

/* ---- audio (web_audio.c) ---------------------------------------------------- */
/* Turns the race's PCM output on once the page has an audio output. */
EMSCRIPTEN_KEEPALIVE void rw_audio_enable(int enabled) { WebAudioEnable(enabled); }
static const int16_t *s_audioPending;
/* Frames (44.1 kHz interleaved stereo int16) rendered since the last call;
 * rw_audio_data() points at them until the next tick. */
EMSCRIPTEN_KEEPALIVE int rw_audio_take(void) {
    int frames;
    s_audioPending = WebAudioPending(&frames);
    return frames;
}
EMSCRIPTEN_KEEPALIVE const int16_t *rw_audio_data(void) { return s_audioPending; }
EMSCRIPTEN_KEEPALIVE int rw_audio_engine_rpm(void) { return WebAudioEngineRpm(); }
