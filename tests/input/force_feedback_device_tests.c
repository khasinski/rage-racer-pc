#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "force_feedback_device.h"
#include "game/player_car_internal.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/track.h"

void PortUpdateForceFeedback(void);
PlayerCarRuntime g_PlayerCar;
s32 g_SceneId;
s32 g_RacePaused;
s16 g_RacePhase;
s16 g_GripLossTimer;
s32 g_TrackPointCount;
const GameTrackPoint *g_TrackPoints;

static int wheelToken, padTokens[2], hapticToken;
static SDL_Joystick *wheel;
static SDL_Gamepad *pad;
static int opens, closes, creates, destroys, runs, gain, failures;
static int failRun;
static int rumbleCalls[2];
static Uint16 low[2], high[2];
static SDL_HapticEffect lastEffect;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); failures++; } } while (0)

float AnalogSteeringDeflection(void) { return 0.5f; }
int AnalogWheelActive(void) { return wheel != NULL; }
SDL_Joystick *AnalogWheelJoystick(void) { return wheel; }
SDL_Gamepad *AnalogActiveGamepad(void) { return pad; }
const char *RuntimeConfigGet(const char *key) {
    if (!strcmp(key, "input.ffb_gain")) return "1";
    return NULL;
}
int RuntimeConfigEnabled(const char *key) { (void)key; return 0; }
int PlatformUserConfigPath(const char *name, char *out, size_t size) {
    (void)name; (void)out; (void)size; return 0;
}
int PlatformUserConfigDirectory(char *out, size_t size) { (void)out; (void)size; return 0; }
int PlatformEnsureDirectory(const char *path) { (void)path; return 0; }
SDL_InitFlags SDL_WasInit(SDL_InitFlags flags) { return flags; }
bool SDL_InitSubSystem(SDL_InitFlags flags) { (void)flags; return true; }
const char *SDL_GetError(void) { return "simulated failure"; }
const char *SDL_GetJoystickName(SDL_Joystick *j) { (void)j; return "test wheel"; }
SDL_Haptic *SDL_OpenHapticFromJoystick(SDL_Joystick *j) {
    (void)j; opens++; return (SDL_Haptic *)&hapticToken;
}
Uint32 SDL_GetHapticFeatures(SDL_Haptic *h) {
    (void)h; return SDL_HAPTIC_CONSTANT | SDL_HAPTIC_GAIN | SDL_HAPTIC_AUTOCENTER;
}
bool SDL_SetHapticGain(SDL_Haptic *h, int value) { (void)h; gain = value; return true; }
bool SDL_SetHapticAutocenter(SDL_Haptic *h, int value) { (void)h; (void)value; return true; }
SDL_HapticEffectID SDL_CreateHapticEffect(SDL_Haptic *h, const SDL_HapticEffect *effect) {
    (void)h; creates++; lastEffect = *effect; return 7;
}
bool SDL_RunHapticEffect(SDL_Haptic *h, SDL_HapticEffectID id, Uint32 iterations) {
    (void)h; (void)id; (void)iterations; runs++; return !failRun;
}
bool SDL_UpdateHapticEffect(SDL_Haptic *h, SDL_HapticEffectID id, const SDL_HapticEffect *effect) {
    (void)h; (void)id; lastEffect = *effect; return true;
}
void SDL_DestroyHapticEffect(SDL_Haptic *h, SDL_HapticEffectID id) { (void)h; (void)id; destroys++; }
void SDL_CloseHaptic(SDL_Haptic *h) { (void)h; closes++; }
SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad *p) {
    return p == (SDL_Gamepad *)&padTokens[0] ? 1 : 2;
}
SDL_Gamepad *SDL_GetGamepadFromID(SDL_JoystickID id) {
    return id >= 1 && id <= 2 ? (SDL_Gamepad *)&padTokens[id - 1] : NULL;
}
bool SDL_RumbleGamepad(SDL_Gamepad *p, Uint16 l, Uint16 r, Uint32 duration) {
    int i = (int)SDL_GetGamepadID(p) - 1;
    (void)duration; rumbleCalls[i]++; low[i] = l; high[i] = r; return true;
}

int main(void) {
    wheel = (SDL_Joystick *)&wheelToken;
    /* Menu frames must not create/destroy a haptic device every frame. */
    PortUpdateForceFeedback();
    PortUpdateForceFeedback();
    CHECK(opens == 0);
    g_SceneId = GAME_SCENE_RACE;
    g_PlayerCar.initializedFlag = 1;
    g_PlayerCar.drive.steerPos = 2048;
    g_PlayerCar.speed = 800;
    PortUpdateForceFeedback();
    CHECK(opens == 1 && creates == 1 && runs == 1 && gain == 100);
    CHECK(lastEffect.constant.direction.type == SDL_HAPTIC_STEERING_AXIS);
    CHECK(lastEffect.constant.level < 0);
    PortUpdateForceFeedback();
    CHECK(opens == 1);
    g_SceneId = 0;
    PortUpdateForceFeedback();
    CHECK(closes == 1 && destroys == 1);
    /* A failed run still owns an effect ID that must be destroyed. */
    g_SceneId = GAME_SCENE_RACE;
    failRun = 1;
    PortUpdateForceFeedback();
    CHECK(closes == 2 && destroys == 2);
    ForceFeedbackDetachJoystick(wheel);
    wheel = NULL;
    pad = (SDL_Gamepad *)&padTokens[0];
    g_PlayerCar.motionActive = 1;
    g_PlayerCar.velocityX = 64;
    g_PlayerCar.drive.steeringLoadAngle = 1024;
    PortUpdateForceFeedback();
    CHECK(low[0] > 0 && high[0] > 0);
    int calls = rumbleCalls[0];
    PortUpdateForceFeedback();
    CHECK(rumbleCalls[0] == calls + 1); /* refresh a sustained slide */
    pad = (SDL_Gamepad *)&padTokens[1];
    PortUpdateForceFeedback();
    CHECK(low[0] == 0 && high[0] == 0);
    CHECK(low[1] > 0 && high[1] > 0);
    pad = NULL;
    PortUpdateForceFeedback();
    CHECK(low[1] == 0 && high[1] == 0);
    puts("force feedback device lifecycle and rumble routing checked");
    return failures != 0;
}
