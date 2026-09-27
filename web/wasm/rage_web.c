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
#include "render/render_mesh_build.h"
#include "render/render_projection.h"
#include "render/render_shadow.h"
#include "render/render_world_frame.h"
#include "render/texture_mipmap.h"
#include "scene_matrix.h"
#include "web_rules.h"

enum {
    WEB_INSTANCE_CAPACITY = 8192,
    WEB_VERTEX_CAPACITY = 600000,
    WEB_SPAN_CAPACITY = 32768,
    WEB_SPAN_FIELDS = 13,
    WEB_TEXTURE_BYTES = 256 * 256 * 4,
    WEB_PACKED_FLOATS = 22,
};

/* chase_camera.c reads optional tuning from the runtime config; the browser
 * has none, so every setting takes its built-in default. */
const char *RuntimeConfigGet(const char *key);
int RuntimeConfigEnabled(const char *key);
const char *RuntimeConfigGet(const char *key) { (void)key; return NULL; }
int RuntimeConfigEnabled(const char *key) { (void)key; return 0; }

static RaceData *s_archive;
/* The seat this player drives, and whether a server steps the race. */
static int s_localSeat, s_net;
static ClientRace *s_race;
static DriverInput s_input;
static int s_pendingShiftUp, s_pendingShiftDown;
static RenderMeshInstance *s_instances;
static RenderWorld s_world;
static RageNativeDrawVertex *s_vertices;
static RageNativeDrawSpan *s_spans;
static uint32_t s_vertexCount, s_spanCount;
static uint32_t s_spanFields[WEB_SPAN_CAPACITY * WEB_SPAN_FIELDS];
static float *s_packed;
static uint64_t s_frame;
/* position, viewRow0, viewRow1, viewRow2, projection, fogColor, fogRange. */
static float s_camera[28];
/* direction, ambient, diffuse, skyTop, skyHorizon, skyBottom. */
static float s_light[24];
static int32_t s_hud[16];
/* Presentation history at the simulation's physics steps (every second
 * 50 Hz tick): the browser draws between the last two, so motion is smooth
 * at any display rate instead of stepping at 25 Hz. */
static PlayerCarRuntime s_posePrevious[DRIVER_SEAT_LIMIT], s_poseCurrent[DRIVER_SEAT_LIMIT];
static RenderCamera s_cameraPrevious, s_cameraCurrent;
static u32 s_lastStepTick;
static int s_haveStep, s_lastTickStepped;
/* Retail far plane (16384) and fog scale; above 1 also draws cells the
 * retail visibility table hides (they stay in the scene as ray geometry). */
static float s_drawDistance = 1.0f;
/* Track texture page the frame was built with (render/track_textures.c:
 * retail swaps the upper VRAM rows while the player is inside the track's
 * texture section range). Terrain and course carry it in their material
 * variant; track model banks decode against it directly. */
static int s_page;
/* Vehicle shadow camera: position, rows 0..2, (scaleX, scaleY, depthScale,
 * depthOffset); zero when no map could be built. */
static float s_shadow[20];
static int s_shadowValid;
static uint8_t *s_mipChain;

/* Retail chase camera state (see RetailChaseView). */
typedef struct WebChase {
    s32 previousYaw, rampNeg, rampPos, yawLag, damping, stepLimit, step;
    int active;
} WebChase;
static WebChase s_chase;

/* race_scene.c: the race starts in the car view; the camera button swaps it
 * with the chase view (chase preset 0, the only one retail selects), and
 * holding down in the chase view looks behind. */
typedef enum WebView { WEB_VIEW_CAR, WEB_VIEW_CHASE, WEB_VIEW_LOOK_BEHIND } WebView;
static WebView s_selectedView, s_viewCurrent;
/* Last pad sample's button bits, and a camera-button press not yet used. */
static u16 s_padHeld;
static int s_pendingCamera;

const RaceData *WebLoadedArchive(void) { return s_archive; }

static void ReleaseRace(void) {
    FreeClientRace(s_race);
    s_race = NULL;
}

EMSCRIPTEN_KEEPALIVE int rw_load_disc(const char *path) {
    ReleaseRace();
    FreeRaceData(s_archive);
    s_archive = LoadRaceDisc(path);
    return s_archive != NULL;
}

/* Frees the imported disc copy; a prepared race keeps its own data. */
EMSCRIPTEN_KEEPALIVE void rw_release_disc(void) {
    FreeRaceData(s_archive);
    s_archive = NULL;
}

/* Loads the field in setup and prepares the local presentation. The server
 * and every player build it from the same rules (web_rules.c WebBuildField). */
static int PrepareRace(int classIndex, int course, int reverse, int laps, int rivals,
                       const WebSeat *humans, int humanCount, int localSeat) {
    RaceSetup setup;
    if (!s_archive || laps < 1 || laps > WEB_MAX_LAPS || localSeat < 0 ||
        localSeat >= humanCount) return 0;
    ReleaseRace();
    memset(&setup, 0, sizeof(setup));
    setup.classIndex = classIndex;
    setup.courseIndex = course;
    setup.laps = laps;
    setup.reverse = reverse ? 1 : 0;
    if (!WebBuildField(s_archive, classIndex, course, reverse, humans, humanCount, rivals,
                       setup.entrants)) return 0;
    for (int seat = 0; seat < humanCount; ++seat) setup.looks[seat].variant = humans[seat].variant;
    s_race = LoadClientRace(s_archive, &setup, NULL);
    if (!s_race) return 0;
    if (!StartRaceSim(&s_race->sim, WEB_COUNTDOWN_TICKS)) {
        ReleaseRace();
        return 0;
    }
    s_localSeat = localSeat;
    if (!s_instances) s_instances = calloc(WEB_INSTANCE_CAPACITY, sizeof(*s_instances));
    if (!s_vertices) s_vertices = calloc(WEB_VERTEX_CAPACITY, sizeof(*s_vertices));
    if (!s_spans) s_spans = calloc(WEB_SPAN_CAPACITY, sizeof(*s_spans));
    if (!s_packed) s_packed = calloc((size_t)WEB_VERTEX_CAPACITY * WEB_PACKED_FLOATS, sizeof(*s_packed));
    if (!s_instances || !s_vertices || !s_spans || !s_packed) {
        ReleaseRace();
        return 0;
    }
    RenderWorldInit(&s_world, s_instances, WEB_INSTANCE_CAPACITY);
    memset(&s_input, 0, sizeof(s_input));
    s_input.steering.mode = STEERING_DIGITAL;
    s_pendingShiftUp = s_pendingShiftDown = 0;
    s_frame = 0;
    memset(&s_chase, 0, sizeof(s_chase));
    s_selectedView = s_viewCurrent = WEB_VIEW_CAR;
    s_padHeld = 0;
    s_pendingCamera = 0;
    s_haveStep = s_lastTickStepped = 0;
    s_shadowValid = 0;
    return 1;
}

/* Offline race: the local player alone, with or without the retail AI. */
EMSCRIPTEN_KEEPALIVE int rw_start_race(int classIndex, int course, int car, int manual,
                                       int reverse, int laps, int rivals) {
    const WebSeat human = {RACE_SEAT_HUMAN, car, manual ? 1 : 0};
    s_net = 0;
    return PrepareRace(classIndex, course, reverse, laps, rivals, &human, 1, 0);
}

/* Networked race as the server announced it: humanSeats holds (variant,
 * manual) per human in seat order. The server's frames then drive it. */
EMSCRIPTEN_KEEPALIVE int rw_start_net_race(int classIndex, int course, int reverse, int laps,
                                           int rivals, int humanCount, const int32_t *humanSeats,
                                           int localSeat) {
    WebSeat humans[DRIVER_SEAT_LIMIT];
    if (!humanSeats || humanCount < 1 || humanCount > DRIVER_SEAT_LIMIT) return 0;
    for (int seat = 0; seat < humanCount; ++seat)
        humans[seat] = (WebSeat){RACE_SEAT_HUMAN, humanSeats[seat * 2], humanSeats[seat * 2 + 1]};
    s_net = 1;
    return PrepareRace(classIndex, course, reverse, laps, rivals, humans, humanCount, localSeat);
}

/* Restores the server's authoritative RaceFrame into the local race. */
EMSCRIPTEN_KEEPALIVE int rw_apply_frame(const uint8_t *wire, int size) {
    static RaceFrame frame;
    if (!s_race || !s_net || !wire || size != RACE_FRAME_WIRE_SIZE ||
        !DecodeRaceFrame(&s_race->sim, wire, (size_t)size, &frame)) return 0;
    return RestoreRaceFrame(&s_race->sim, &frame);
}
EMSCRIPTEN_KEEPALIVE int rw_frame_size(void) { return RACE_FRAME_WIRE_SIZE; }

/* The local controls to send (web_rules.h wire words); gear edges are handed
 * over once, as the server's simulation accumulates them. */
EMSCRIPTEN_KEEPALIVE int32_t *rw_take_input(void) {
    static int32_t words[WEB_INPUT_WORDS];
    DriverInput input = s_input;
    input.shiftUp = s_pendingShiftUp;
    input.shiftDown = s_pendingShiftDown;
    s_pendingShiftUp = s_pendingShiftDown = 0;
    WebEncodeInput(&input, words);
    return words;
}

EMSCRIPTEN_KEEPALIVE int rw_local_seat(void) { return s_localSeat; }

/* Keyboard levels for the local seat. Gear requests are edges: they stay
 * pending until the next simulation tick consumes them. */
EMSCRIPTEN_KEEPALIVE void rw_set_input(int left, int right, int throttle, int brake,
                                       int shiftUp, int shiftDown) {
    s_input.steering.mode = STEERING_DIGITAL;
    s_input.steering.left = left ? 1 : 0;
    s_input.steering.right = right ? 1 : 0;
    s_input.steering.angle = 0;
    s_input.throttle = (s16)(throttle < 0 ? 0 : throttle > 256 ? 256 : throttle);
    s_input.brake = (s16)(brake < 0 ? 0 : brake > 256 ? 256 : brake);
    if (shiftUp) s_pendingShiftUp = 1;
    if (shiftDown) s_pendingShiftDown = 1;
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
    const u16 pressed = buttons & (u16)~s_padHeld;
    s_padHeld = buttons;
    if (pressed & CAMERA_BUTTON) s_pendingCamera = 1;
    if (!gamepad) {
        s_input.steering.mode = STEERING_DIGITAL;
        s_input.steering.left = (buttons & PAD_LEFT) != 0;
        s_input.steering.right = (buttons & PAD_RIGHT) != 0;
        s_input.steering.angle = 0;
        s_input.throttle = buttons & PAD_CROSS ? PEDAL_FULLY_PRESSED : 0;
        s_input.brake = buttons & PAD_SQUARE ? PEDAL_FULLY_PRESSED : 0;
        if (pressed & DIGITAL_SHIFT_UP) s_pendingShiftUp = 1;
        if (pressed & DIGITAL_SHIFT_DOWN) s_pendingShiftDown = 1;
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
        s_input.steering.mode = STEERING_ANALOG;
        s_input.steering.left = s_input.steering.right = 0;
        s_input.steering.angle = CalibrateNegconSteer(twist) * NEGCON_STEERING_SCALE /
                                 NEGCON_DEFAULT_STEER_RANGE;
        s_input.throttle = NegconPedal(analogI);
        s_input.brake = NegconPedal(analogII);
    }
    if (pressed & NEGCON_SHIFT_UP) s_pendingShiftUp = 1;
    if (pressed & NEGCON_SHIFT_DOWN) s_pendingShiftDown = 1;
}

static float Daylight(const ClientRace *race) {
    RenderCamera environment;
    memset(&environment, 0, sizeof(environment));
    ApplyEnvironment(&environment, &race->env);
    return CarLightDaylight(environment.skyTopColor, environment.skyHorizonColor);
}

static RenderCamera BuildRaceCamera(const PlayerCarRuntime *car, WebView view);

/* Snapshots poses and the race camera whenever the field physics stepped.
 * Retail reads the camera button and updates the camera once per game
 * frame, and the chase camera's yaw settling is tuned per frame, so all of
 * it advances only here. */
static void RecordPresentation(void) {
    const RaceSim *sim = &s_race->sim;
    const u32 stepTick = sim->drivers[s_localSeat].stepTick;
    const int racing = sim->phase == SIM_RACING; /* CanToggleRaceCamera */
    WebView view;
    RenderCamera camera;
    s_lastTickStepped = !s_haveStep || stepTick != s_lastStepTick;
    if (!s_lastTickStepped) return;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        s_posePrevious[seat] = s_haveStep ? s_poseCurrent[seat] : sim->drivers[seat].car;
        s_poseCurrent[seat] = sim->drivers[seat].car;
    }
    if (s_pendingCamera && racing)
        s_selectedView = s_selectedView == WEB_VIEW_CAR ? WEB_VIEW_CHASE : WEB_VIEW_CAR;
    s_pendingCamera = 0;
    view = racing && s_selectedView == WEB_VIEW_CHASE && (s_padHeld & PAD_DOWN)
               ? WEB_VIEW_LOOK_BEHIND : s_selectedView;
    camera = BuildRaceCamera(&s_poseCurrent[s_localSeat], view);
    /* A new view cuts; only frames within one view are interpolated. */
    s_cameraPrevious = s_haveStep && view == s_viewCurrent ? s_cameraCurrent : camera;
    s_cameraCurrent = camera;
    s_viewCurrent = view;
    s_lastStepTick = stepTick;
    s_haveStep = 1;
}

/* One 50 Hz simulation tick. Returns the race phase, or -1 on failure. */
EMSCRIPTEN_KEEPALIVE int rw_tick(void) {
    DriverInput input;
    if (!s_race) return -1;
    input = s_input;
    input.shiftUp = s_pendingShiftUp;
    input.shiftDown = s_pendingShiftDown;
    /* Online the server steps the field; its frames arrive via rw_apply_frame. */
    if (!s_net) {
        if (SetRaceInput(&s_race->sim, s_localSeat, &input)) s_pendingShiftUp = s_pendingShiftDown = 0;
        StepRaceSim(&s_race->sim);
    }
    if (!TickClientScenery(s_race)) return -1;
    TickRaceView(s_race->view, &s_race->sim, Daylight(s_race));
    RecordPresentation();
    return (int)s_race->sim.phase;
}

/* 1 when the last tick produced a new presentation snapshot. */
EMSCRIPTEN_KEEPALIVE int rw_last_tick_stepped(void) { return s_lastTickStepped; }

EMSCRIPTEN_KEEPALIVE void rw_set_draw_distance(float multiplier) {
    s_drawDistance = multiplier >= 1.0f && multiplier <= 16.0f ? multiplier : 1.0f;
}

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
    WebChase *chase = &s_chase;
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

/* render_world_game.c's GameRenderWorldBuildCamera for the race view:
 * PAL 320x240 projection, near 1, the verified race depth limit. The chase
 * camera only settles while it is the view, as retail's previousMode check
 * restarts it otherwise. */
static RenderCamera BuildRaceCamera(const PlayerCarRuntime *car, WebView selected) {
    RenderCamera camera;
    Vec3 eye;
    s32 pitch, yaw, roll;
    SceneMat3 view, converted;

    if (selected == WEB_VIEW_CHASE) {
        RetailChaseView(car, &eye, &pitch, &yaw, &roll);
    } else {
        s_chase.active = 0;
        if (selected == WEB_VIEW_LOOK_BEHIND) RetailLookBehindView(car, &eye, &pitch, &yaw, &roll);
        else RetailCarView(car, &eye, &pitch, &yaw, &roll);
    }
    memset(&camera, 0, sizeof(camera));
    camera.transform.position = (Vec3){eye.x, -eye.y, -eye.z};
    view = SceneMat3Multiply(SceneMat3Multiply(SceneRotationZ(roll), SceneRotationX(pitch)),
                             SceneRotationY(yaw));
    RenderConvertPsxMatrix(view.m, converted.m);
    camera.transform.orientation = SceneQuaternion(SceneMat3Transpose(converted));
    camera.transform.hasOrientation = 1;
    camera.transform.rotation = (Vec3){-AngleToDegrees(pitch), -AngleToDegrees(yaw),
                                       -AngleToDegrees(roll)};
    camera.transform.scale = (Vec3){1.0f, 1.0f, 1.0f};
    camera.verticalFovDegrees = 41.112f;
    camera.nearPlane = 1.0f;
    camera.farPlane = 16384.0f;
    return camera;
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
static int BuildCameraUniform(const RenderCamera *camera, float aspect) {
    static const float axes[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float columns[3][3];
    memset(s_camera, 0, sizeof(s_camera));
    if (!RenderPerspectiveScales(camera, aspect, &s_camera[16], &s_camera[17]) ||
        !RenderPerspectiveDepthTerms(camera, &s_camera[18], &s_camera[19])) return 0;
    s_camera[0] = camera->transform.position.x;
    s_camera[1] = camera->transform.position.y;
    s_camera[2] = camera->transform.position.z;
    for (int axis = 0; axis < 3; ++axis) {
        RotateByCamera(columns[axis], axes[axis], camera);
        s_camera[4 + axis] = columns[axis][0];
        s_camera[8 + axis] = columns[axis][1];
        s_camera[12 + axis] = columns[axis][2];
    }
    s_camera[20] = camera->fogColor.x;
    s_camera[21] = camera->fogColor.y;
    s_camera[22] = camera->fogColor.z;
    if (isfinite(camera->fogNear) && isfinite(camera->fogFar) &&
        camera->fogNear > 0.0f && camera->fogFar > camera->fogNear) {
        s_camera[24] = camera->fogNear;
        s_camera[25] = camera->fogFar;
        s_camera[26] = 1.0f / camera->fogNear;
        s_camera[27] = s_camera[26] - 1.0f / camera->fogFar;
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

/* Builds the scene presented `t` (0..1) of the way from the previous physics
 * step to the latest one, with the native sequence, and expands it into
 * world-space triangles. Returns the vertex count, or -1 on failure. */
EMSCRIPTEN_KEEPALIVE int rw_build_frame(float aspect, float t) {
    RenderDirectionalLight light;
    RenderCamera camera;
    RenderShadowMap shadow;
    Vec3 shadowCenter;
    int page;
    if (!s_race || !s_haveStep || !(aspect > 0.0f)) return -1;
    page = s_poseCurrent[s_localSeat].trackSection >= s_race->look.textureSectionLo &&
           s_poseCurrent[s_localSeat].trackSection < s_race->look.textureSectionHi;
    s_page = page;
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    RenderWorldBeginFrame(&s_world, ++s_frame);
    RenderInterpolateCamera(&s_cameraPrevious, &s_cameraCurrent, t, &camera);
    ApplyEnvironment(&camera, &s_race->env);
    camera.farPlane *= s_drawDistance;
    camera.fogNear *= s_drawDistance;
    camera.fogFar *= s_drawDistance;
    RenderWorldSetCamera(&s_world, &camera);
    RenderDirectionalLightFromSky(&camera, &light);
    RenderWorldSetDirectionalLight(&s_world, &light);
    if (!SubmitClientTerrain(s_race, page, &s_world) ||
        !SubmitClientScenery(s_race, page, &s_world) ||
        !SubmitRaceViewPoses(&s_race->sim, s_race->view, s_poseCurrent, s_posePrevious,
                             s_race->rivals, s_race->primaryMesh.cached.assetKey, 0, &s_world) ||
        !SubmitClientShuttles(s_race, page, &s_world) ||
        !SubmitClientSpinners(s_race, page, &s_world) ||
        !SubmitClientLandmarks(s_race, page, &s_world)) return -1;
    RenderWorldFocus(&s_world, (uint32_t)s_localSeat);
    for (uint32_t i = 0; i < s_world.instanceCount; ++i) {
        RenderMeshInstance *instance = &s_world.instances[i];
        if (instance->entity < DRIVER_SEAT_LIMIT) {
            /* Vehicles carry the previous physics step as their previous
             * transform; everything else animates per clock tick. */
            RenderTransform mixed;
            RenderInterpolateTransform(&instance->previousTransform, &instance->transform, t, &mixed);
            instance->transform = mixed;
            /* update_camera.c draws the player's car only outside the car view. */
            if (instance->entity == (uint32_t)s_localSeat && s_viewCurrent == WEB_VIEW_CAR)
                instance->flags |= RAGE_RENDER_INSTANCE_RAY_ONLY;
        } else if (s_drawDistance > 1.0f) {
            instance->flags &= ~RAGE_RENDER_INSTANCE_RAY_ONLY;
        }
    }
    shadowCenter = RenderShadowCenter(&s_world);
    s_shadowValid = RenderBuildDirectionalShadowMap(
        &shadowCenter, &s_world.light.direction, RAGE_RENDER_VEHICLE_SHADOW_EXTENT,
        RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION, &shadow);
    memset(s_shadow, 0, sizeof(s_shadow));
    if (s_shadowValid) {
        StoreVec3(&s_shadow[0], shadow.position);
        StoreVec3(&s_shadow[4], shadow.row0);
        StoreVec3(&s_shadow[8], shadow.row1);
        StoreVec3(&s_shadow[12], shadow.row2);
        s_shadow[16] = shadow.scaleX;
        s_shadow[17] = shadow.scaleY;
        s_shadow[18] = shadow.depthScale;
        s_shadow[19] = shadow.depthOffset;
    }
    s_vertexCount = RenderBuildNativePassDraws(
        &s_world, RAGE_RENDER_PASS_MAIN, aspect, ResolveMesh, s_race,
        s_vertices, WEB_VERTEX_CAPACITY, s_spans, WEB_SPAN_CAPACITY, &s_spanCount);
    for (uint32_t i = 0; i < s_spanCount; ++i) {
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
    }
    if (!BuildCameraUniform(&s_world.camera, aspect)) return -1;
    StoreVec3(&s_light[0], s_world.light.direction);
    StoreVec3(&s_light[4], s_world.light.ambientColor);
    StoreVec3(&s_light[8], s_world.light.diffuseColor);
    StoreVec3(&s_light[12], s_world.camera.skyTopColor);
    StoreVec3(&s_light[16], s_world.camera.skyHorizonColor);
    StoreVec3(&s_light[20], s_world.camera.skyBottomColor);
    return (int)s_vertexCount;
}

EMSCRIPTEN_KEEPALIVE uint32_t *rw_spans(void) { return s_spanFields; }
EMSCRIPTEN_KEEPALIVE int rw_span_count(void) { return (int)s_spanCount; }
EMSCRIPTEN_KEEPALIVE int rw_span_fields(void) { return WEB_SPAN_FIELDS; }
EMSCRIPTEN_KEEPALIVE float *rw_camera(void) { return s_camera; }
EMSCRIPTEN_KEEPALIVE float *rw_light(void) { return s_light; }
EMSCRIPTEN_KEEPALIVE int rw_packed_floats(void) { return WEB_PACKED_FLOATS; }
EMSCRIPTEN_KEEPALIVE float *rw_shadow(void) { return s_shadowValid ? s_shadow : NULL; }
EMSCRIPTEN_KEEPALIVE int rw_shadow_resolution(void) { return RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION; }

/* The draw vertex mixes floats with a byte colour; WebGL wants one typed
 * buffer per upload, so this repacks the frame into floats only:
 * position 3, uv 2, colour 4 (0..255), normal 3, fog 4 (colour, weight),
 * lighting 1, environment light 3, depth bias 1, shadow reception 1. */
EMSCRIPTEN_KEEPALIVE float *rw_pack_vertices(void) {
    for (uint32_t i = 0; i < s_vertexCount; ++i) {
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
    /* Vehicles cast the shadow map but do not sample it on themselves: the
     * native backend traces those rays instead, and a map lookup on their
     * low-poly surfaces is all acne (see RageNativeDrawVertex). */
    for (uint32_t i = 0; i < s_spanCount; ++i) {
        const RageNativeDrawSpan *span = &s_spans[i];
        if (span->assetSet != RAGE_RENDER_ASSET_MODEL_BANK &&
            span->assetSet != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) continue;
        for (uint32_t v = 0; v < span->vertexCount; ++v)
            s_packed[(size_t)(span->firstVertex + v) * WEB_PACKED_FLOATS + 21] = 0.0f;
    }
    return s_packed;
}

/* 256x256 RGBA for one span's material, exactly as the native backend
 * reconstructs it; the palette is the race's current environment CLUT. */
EMSCRIPTEN_KEEPALIVE int rw_decode_texture(int spanIndex, uint8_t *rgba) {
    RenderMeshInstance instance;
    const RageNativeDrawSpan *span;
    if (!s_race || spanIndex < 0 || (uint32_t)spanIndex >= s_spanCount || !rgba) return 0;
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
    return DecodeClientMaterial(s_race, &instance, span->material, s_page, s_race->env.clut,
                                rgba, WEB_TEXTURE_BYTES);
}

/* The same premultiplied atlas mip chain the native backend uploads: PS1
 * material pages are dense atlases, so only RAGE_TEXTURE_ATLAS_MIP_LEVELS
 * levels exist; smaller ones would blend unrelated entries. Returns the
 * chain (levels back to back, see rw_texture_level_offset) or NULL. */
EMSCRIPTEN_KEEPALIVE uint8_t *rw_decode_texture_mips(int spanIndex, uint8_t *scratch) {
    const size_t size = TextureMipChainSizeRGBA8(256, 256, RAGE_TEXTURE_ATLAS_MIP_LEVELS);
    if (!s_mipChain) s_mipChain = malloc(size);
    if (!s_mipChain || !rw_decode_texture(spanIndex, scratch) ||
        !TextureBuildMipChainRGBA8(scratch, 256, 256, RAGE_TEXTURE_ATLAS_MIP_LEVELS, s_mipChain, size))
        return NULL;
    return s_mipChain;
}

/* 0 or 1: the track texture page of the last built frame. */
EMSCRIPTEN_KEEPALIVE int rw_texture_page(void) { return s_page; }

EMSCRIPTEN_KEEPALIVE int rw_texture_levels(void) { return RAGE_TEXTURE_ATLAS_MIP_LEVELS; }
EMSCRIPTEN_KEEPALIVE int rw_texture_level_offset(int level) {
    return (int)TextureMipLevelOffsetRGBA8(256, 256, (uint32_t)level);
}

/* Changes whenever the environment palette (time of day) changes, so the
 * browser knows when palette-dependent textures must be decoded again. */
EMSCRIPTEN_KEEPALIVE uint32_t rw_palette_hash(void) {
    uint32_t hash = 2166136261u;
    const uint8_t *bytes;
    if (!s_race) return 0;
    bytes = (const uint8_t *)s_race->env.clut;
    for (size_t i = 0; i < sizeof(s_race->env.clut); ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

/* phase, countdown ticks left, lap, laps, place, entrants, race time ms,
 * speed (retail units), gear, status, finish place, tick, then the local
 * car's exact x, y, z and body yaw (used to check physics parity). */
EMSCRIPTEN_KEEPALIVE int32_t *rw_hud(void) {
    const SimDriver *driver;
    int entrants = 0;
    if (!s_race) return NULL;
    driver = &s_race->sim.drivers[s_localSeat];
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat)
        entrants += s_race->sim.drivers[seat].status != SIM_EMPTY;
    s_hud[0] = (int32_t)s_race->sim.phase;
    s_hud[1] = (int32_t)s_race->sim.countdown;
    s_hud[2] = driver->car.lap > s_race->sim.laps ? s_race->sim.laps : driver->car.lap;
    s_hud[3] = s_race->sim.laps;
    s_hud[4] = RacePosition(&s_race->sim, s_localSeat);
    s_hud[5] = entrants;
    s_hud[6] = RaceTime(&s_race->sim, s_localSeat);
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
