#include "force_feedback_device.h"
#include "analog_pad.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/track.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

void PortUpdateForceFeedback(void);
static int onlineDriving;
int OnlineForceFeedback(int32_t values[8]){int32_t sample[8]={1000,800,200,2,24,80,10,5000};memcpy(values,sample,sizeof(sample));return onlineDriving;}
PlayerCarRuntime g_PlayerCar;
s16 g_GripLossTimer, g_RacePhase;
s32 g_RacePaused, g_SceneId, g_TrackPointCount;
const GameTrackPoint *g_TrackPoints;

static int wheelToken, padToken, hapticToken;
static SDL_Joystick *wheel = (SDL_Joystick *)&wheelToken;
static SDL_Gamepad *pad;
static int opens, closes, creates, destroys, runs, updates, rumbles;
static int failRun, failUpdate, constantForce = 1, failOpen;
static Sint16 lastTorque;
static SDL_JoystickID lastRumbleDevice;
static Uint16 lastLow, lastHigh;
static float axis = 0.5f;
static Uint32 lastDuration;
static int activeEffects, joystickRumbles, conflictingRumbles, rumbleSupported = 1;

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int AnalogWheelActive(void) { return wheel != NULL; }
SDL_Joystick *AnalogWheelJoystick(void) { return wheel; }
SDL_Gamepad *AnalogActiveGamepad(void) { return pad; }
float AnalogSteeringDeflection(void) { return axis; }
const char *RuntimeConfigGet(const char *key) { (void)key; return NULL; }
int RuntimeConfigEnabled(const char *key) { (void)key; return 0; }
int PlatformUserConfigPath(const char *name, char *out, size_t size) {
    (void)name; (void)out; (void)size; return 0;
}
int PlatformUserConfigDirectory(char *out, size_t size) {
    (void)out; (void)size; return 0;
}
int PlatformEnsureDirectory(const char *path) { (void)path; return 0; }

bool SDL_InitSubSystem(SDL_InitFlags flags) { (void)flags; return true; }
const char *SDL_GetError(void) { return "mock device error"; }
const char *SDL_GetPlatform(void) { return "Mock"; }
const char *SDL_GetJoystickName(SDL_Joystick *device) { (void)device; return "test wheel"; }
Uint16 SDL_GetJoystickVendor(SDL_Joystick *device) { (void)device; return 0x046d; }
Uint16 SDL_GetJoystickProduct(SDL_Joystick *device) { (void)device; return 0xc29b; }
SDL_JoystickID SDL_GetJoystickID(SDL_Joystick *device) {
    return device == (SDL_Joystick *)&wheelToken ? 1 : 2;
}
SDL_Joystick *SDL_GetJoystickFromID(SDL_JoystickID id) {
    return id == 1 ? (SDL_Joystick *)&wheelToken :
           id == 2 ? (SDL_Joystick *)&padToken : NULL;
}
SDL_Joystick *SDL_GetGamepadJoystick(SDL_Gamepad *device) {
    return device != NULL ? (SDL_Joystick *)&padToken : NULL;
}
SDL_Haptic *SDL_OpenHapticFromJoystick(SDL_Joystick *device) {
    (void)device; opens++;
    return failOpen ? NULL : (SDL_Haptic *)&hapticToken;
}
Uint32 SDL_GetHapticFeatures(SDL_Haptic *haptic) {
    (void)haptic;
    return SDL_HAPTIC_AUTOCENTER | (constantForce ? SDL_HAPTIC_CONSTANT : 0);
}
bool SDL_SetHapticAutocenter(SDL_Haptic *haptic, int value) {
    (void)haptic; (void)value; return true;
}
SDL_HapticEffectID SDL_CreateHapticEffect(SDL_Haptic *haptic, const SDL_HapticEffect *effect) {
    (void)haptic; (void)effect; creates++; activeEffects++; return 0;
}
bool SDL_RunHapticEffect(SDL_Haptic *haptic, SDL_HapticEffectID effect, Uint32 iterations) {
    (void)haptic; (void)effect; (void)iterations; runs++; return !failRun;
}
void SDL_DestroyHapticEffect(SDL_Haptic *haptic, SDL_HapticEffectID effect) {
    (void)haptic; (void)effect; destroys++; activeEffects--; lastTorque = 0;
}
void SDL_CloseHaptic(SDL_Haptic *haptic) { (void)haptic; closes++; }
bool SDL_UpdateHapticEffect(SDL_Haptic *haptic, SDL_HapticEffectID effect, const SDL_HapticEffect *data) {
    (void)haptic; (void)effect; updates++;
    if (failUpdate) return false;
    lastTorque = data->constant.level; return true;
}
bool SDL_RumbleJoystick(SDL_Joystick *device, Uint16 low, Uint16 high, Uint32 duration) {
    joystickRumbles++;
    if (activeEffects && device == (SDL_Joystick *)&wheelToken) conflictingRumbles++;
    rumbles++; lastRumbleDevice = SDL_GetJoystickID(device);
    lastLow = low; lastHigh = high; lastDuration = duration; return true;
}
bool SDL_HapticRumbleSupported(SDL_Haptic *haptic) { (void)haptic; return rumbleSupported != 0; }
bool SDL_InitHapticRumble(SDL_Haptic *haptic) { (void)haptic; return true; }
bool SDL_PlayHapticRumble(SDL_Haptic *haptic, float strength, Uint32 duration) {
    (void)haptic; rumbles++; lastRumbleDevice = 1;
    lastLow = lastHigh = (Uint16)(strength * 65535.0f); lastDuration = duration;
    return true;
}
bool SDL_StopHapticRumble(SDL_Haptic *haptic) {
    (void)haptic; rumbles++; lastLow = lastHigh = 0; lastDuration = 0; return true;
}

static void Drive(void) {
    g_SceneId = GAME_SCENE_RACE;
    g_RacePhase = 0;
    g_PlayerCar.initializedFlag = 1;
    g_PlayerCar.speed = 800;
    g_PlayerCar.drive.steerPos = 2048;
    g_PlayerCar.motionActive = 1;
    g_PlayerCar.velocityX = 32;
    g_GripLossTimer = 10;
}

int main(int argc, char **argv) {
    CHECK(argc == 2);
    Drive();
    if(!strcmp(argv[1],"online")){
        g_SceneId=GAME_SCENE_MENU;onlineDriving=1;
        PortUpdateForceFeedback();
        CHECK(opens==1 && lastTorque!=0 && lastLow>0 && lastHigh>0);
        onlineDriving=0;PortUpdateForceFeedback();
        CHECK(closes==1 && lastLow==0 && lastHigh==0);
        return 0;
    }
    if (!strcmp(argv[1], "rumble_only")) constantForce = 0;
    if (!strcmp(argv[1], "open_failure")) failOpen = 1;
    if (!strcmp(argv[1], "start_failure") || !strcmp(argv[1], "retry")) failRun = 1;
    if (!strcmp(argv[1], "constant_only")) rumbleSupported = 0;
    if (!strcmp(argv[1], "gamepad")) {
        wheel = NULL;
        pad = (SDL_Gamepad *)&padToken;
    }
    if (!strcmp(argv[1], "preview")) {
        g_SceneId = GAME_SCENE_MENU;
        for (int i = 0; i < 8; ++i) PortUpdateForceFeedback();
        CHECK(opens == 0);
        PortSetForceFeedbackEnabled(0);
        PortSetForceFeedbackEnabled(1);
        for (int i = 0; i < 6; ++i) PortUpdateForceFeedback();
        CHECK(opens == 1 && creates == 1 && runs == 1 && destroys == 0);
        CHECK(lastTorque > 0 && lastTorque < 6000 && lastLow > 0);
        PortUpdateForceFeedback();
        CHECK(destroys == 1 && closes == 1 && lastLow == 0 && lastHigh == 0);
        for (int i = 0; i < 8; ++i) PortUpdateForceFeedback();
        CHECK(opens == 1);
        PortSetForceFeedbackEnabled(0);
        PortSetForceFeedbackEnabled(1);
        PortSetForceFeedbackGainPercent(0);
        for (int i = 0; i < 8; ++i) PortUpdateForceFeedback();
        CHECK(opens == 2 && closes == 2 && lastTorque == 0);
        CHECK(conflictingRumbles == 0);
        return 0;
    }
    PortUpdateForceFeedback();
    if (!rumbleSupported) {
        for (int i = 0; i < 10; ++i) PortUpdateForceFeedback();
        CHECK(lastTorque != 0 && joystickRumbles == 0 && opens == 1);
        return 0;
    }
    CHECK(lastLow > 0 && lastHigh > 0 && lastDuration == 80);
    CHECK(lastRumbleDevice == (wheel != NULL ? 1u : 2u));
    for (int i = 0; i < 10; ++i) PortUpdateForceFeedback();
    CHECK(rumbles == 11); /* Identical live output must refresh its duration. */
    CHECK(conflictingRumbles == 0);
    if (!strcmp(argv[1], "retry")) {
        CHECK(opens == 1 && creates == 1 && destroys == 1);
        failRun = 0;
        PortSetForceFeedbackEnabled(0);
        PortSetForceFeedbackEnabled(1);
        for (int i = 0; i < 6; ++i) PortUpdateForceFeedback();
        CHECK(opens == 2 && creates == 2 && destroys == 1 && lastTorque != 0);
        CHECK(conflictingRumbles == 0);
        return 0;
    }
    if (wheel == NULL) CHECK(opens == 0 && creates == 0);
    else if (!constantForce || failOpen || failRun) {
        CHECK(opens == 1); /* Do not retry an unsupported device every frame. */
        if (failRun) CHECK(creates == 1 && destroys == 1 && closes == 1);
    } else {
        CHECK(opens == 1 && creates == 1 && runs == 1 && destroys == 0);
        CHECK(updates > 0 && lastTorque != 0);
        failUpdate = 1;
        axis = -0.5f;
        PortUpdateForceFeedback();
        int attempts = updates;
        failUpdate = 0;
        PortUpdateForceFeedback();
        CHECK(updates == attempts + 1); /* A failed send must not latch the value. */
    }
    g_RacePaused = 1;
    PortUpdateForceFeedback();
    CHECK(lastLow == 0 && lastHigh == 0 && lastTorque == 0);
    if (!strcmp(argv[1], "hotplug")) {
        ForceFeedbackDetachJoystick(wheel);
        wheel = NULL;
        pad = (SDL_Gamepad *)&padToken;
        g_RacePaused = 0;
        PortUpdateForceFeedback();
        CHECK(lastRumbleDevice == 2 && lastLow > 0 && creates == destroys);
        pad = NULL;
        PortUpdateForceFeedback();
        CHECK(lastLow == 0 && lastHigh == 0 && lastDuration == 0);
        return 0;
    }
    g_RacePaused = 0;
    PortSetForceFeedbackEnabled(0);
    PortUpdateForceFeedback();
    CHECK(lastLow == 0 && lastHigh == 0 && lastTorque == 0);
    PortSetForceFeedbackEnabled(1);
    PortSetForceFeedbackGainPercent(0);
    PortUpdateForceFeedback();
    CHECK(lastLow == 0 && lastHigh == 0 && lastTorque == 0);
    if (wheel != NULL) {
        ForceFeedbackDetachJoystick(wheel);
        CHECK(lastRumbleDevice == 1 && lastDuration == 0);
        CHECK(creates == destroys);
    }
    CHECK(conflictingRumbles == 0);
    return 0;
}
