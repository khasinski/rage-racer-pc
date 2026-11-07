#include "force_feedback_device.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analog_pad.h"
#include "force_feedback.h"
#include "platform_paths.h"
#include "runtime_config.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/track.h"
#ifndef __ANDROID__
#include "../online/model.h"
#endif

enum {
    FFB_GAIN_STEP = 5,
    FFB_PULSE_FRAMES = 6,
    FFB_PLAYER_LINE = 64
};

static ForceFeedbackTuning s_tuning;
static int s_ready;
static int s_enabled;
static int s_gainPercent;
static int s_harnessLock;
static int s_pulseFrames;
static float s_smoothed;
static float s_previousAxis;
static unsigned s_enginePhase;

static SDL_Haptic *s_haptic;
static SDL_HapticEffect s_effect;
static SDL_HapticEffectID s_effectId;
static int s_effectReady;
static SDL_Joystick *s_boundWheel;
static SDL_Joystick *s_rejectedWheel;
static Sint16 s_sentLevel;
static int s_rumbleNoted;
static int s_updateNoted;
static Uint16 s_lastLow;
static Uint16 s_lastHigh;
static int s_rumbleLatched;
static SDL_JoystickID s_rumbleDevice;
static int s_hapticRumbleTried;
static int s_hapticRumbleReady;
static int s_hapticRumblePlaying;

static int EnvironmentSet(const char *name) {
    const char *value = getenv(name);
    return value != NULL && value[0] != '\0';
}

static float ParseUnit(const char *text, float fallback, const char *key) {
    char *end;
    float value;

    if (text == NULL || text[0] == '\0') return fallback;
    errno = 0;
    value = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || value != value ||
        value < 0.0f || value > 1.0f) {
        fprintf(stderr,
                "rage-port: ignoring %s=%s (expected 0..1); using %.2f\n",
                key, text, fallback);
        return fallback;
    }
    return value;
}

static float UnitSetting(const char *key, float fallback) {
    return ParseUnit(RuntimeConfigGet(key), fallback, key);
}

static int PercentOf(float gain) {
    int percent = (int)(gain * 100.0f + 0.5f);
    if (percent < 0) return 0;
    if (percent > 100) return 100;
    return percent;
}

static int PlayerPath(char *path, size_t size) {
    return PlatformUserConfigPath("force-feedback", path, size);
}

static void ReadPlayerFile(void) {
    char path[4096];
    char line[FFB_PLAYER_LINE];
    FILE *file;
    int sawEnabled = 0;
    int sawGain = 0;
    int enabled = s_enabled;
    int gain = s_gainPercent;

    if (!PlayerPath(path, sizeof(path))) return;
    file = fopen(path, "r");
    if (file == NULL) return;
    while (fgets(line, sizeof(line), file) != NULL) {
        int value;
        if (sscanf(line, "enabled=%d", &value) == 1) {
            enabled = value != 0;
            sawEnabled = 1;
        } else if (sscanf(line, "gain=%d", &value) == 1 && value >= 0 &&
                   value <= 100) {
            gain = value;
            sawGain = 1;
        }
    }
    fclose(file);
    if (sawEnabled) s_enabled = enabled;
    if (sawGain) s_gainPercent = gain;
}

static void WritePlayerFile(void) {
    char directory[4096];
    char path[4096];
    FILE *file;

    if (s_harnessLock) return;
    if (!PlatformUserConfigDirectory(directory, sizeof(directory)) ||
        !PlatformEnsureDirectory(directory) ||
        !PlayerPath(path, sizeof(path))) {
        return;
    }
    file = fopen(path, "w");
    if (file == NULL) return;
    fprintf(file, "enabled=%d\ngain=%d\n", s_enabled ? 1 : 0, s_gainPercent);
    fclose(file);
}

static void LoadSettings(void) {
    const char *forcedEnabled;
    const char *forcedGain;

    if (s_ready) return;
    s_ready = 1;
    s_tuning = ForceFeedbackDefaultTuning();
    s_enabled = RuntimeConfigGet("input.force_feedback") == NULL
                    ? 1
                    : RuntimeConfigEnabled("input.force_feedback");
    s_gainPercent = PercentOf(UnitSetting("input.ffb_gain", s_tuning.gain));
    s_tuning.center = UnitSetting("input.ffb_center", s_tuning.center);
    s_tuning.slide = UnitSetting("input.ffb_slide", s_tuning.slide);
    s_tuning.collision = UnitSetting("input.ffb_collision", s_tuning.collision);
    s_tuning.road = UnitSetting("input.ffb_road", s_tuning.road);
    s_tuning.damper = UnitSetting("input.ffb_damper", s_tuning.damper);
    s_tuning.minForce = UnitSetting("input.ffb_min_force", s_tuning.minForce);
    s_tuning.softLock = UnitSetting("input.ffb_soft_lock", s_tuning.softLock);
    s_tuning.engine = UnitSetting("input.ffb_engine", s_tuning.engine);
    s_tuning.invert = RuntimeConfigEnabled("input.ffb_invert");

    s_harnessLock = EnvironmentSet("RAGE_PORT_INPUT_FORCE_FEEDBACK") ||
                    EnvironmentSet("RAGE_PORT_INPUT_FFB_GAIN");
    if (!s_harnessLock) ReadPlayerFile();

    forcedEnabled = getenv("RAGE_PORT_INPUT_FORCE_FEEDBACK");
    if (forcedEnabled != NULL && forcedEnabled[0] != '\0') {
        s_enabled = strcmp(forcedEnabled, "0") != 0 &&
                    strcmp(forcedEnabled, "false") != 0 &&
                    strcmp(forcedEnabled, "off") != 0 &&
                    strcmp(forcedEnabled, "no") != 0;
    }
    forcedGain = getenv("RAGE_PORT_INPUT_FFB_GAIN");
    if (forcedGain != NULL && forcedGain[0] != '\0') {
        s_gainPercent = PercentOf(
            ParseUnit(forcedGain, (float)s_gainPercent / 100.0f,
                      "input.ffb_gain"));
    }
    fprintf(stderr, "rage-port: force feedback enabled=%d gain=%d%%\n",
            s_enabled, s_gainPercent);
}

static void StopJoystickRumble(void) {
    SDL_Joystick *device = s_rumbleDevice != 0
                            ? SDL_GetJoystickFromID(s_rumbleDevice) : NULL;
    if (device != NULL && s_rumbleLatched && (s_lastLow || s_lastHigh))
        SDL_RumbleJoystick(device, 0, 0, 0);
    s_rumbleDevice = 0;
    s_rumbleLatched = 0;
}

static void ReleaseEffect(int restoreCenter) {
    if (s_haptic != NULL && s_hapticRumblePlaying)
        SDL_StopHapticRumble(s_haptic);
    s_hapticRumbleTried = 0;
    s_hapticRumbleReady = 0;
    s_hapticRumblePlaying = 0;
    if (s_haptic != NULL && restoreCenter &&
        (SDL_GetHapticFeatures(s_haptic) & SDL_HAPTIC_AUTOCENTER) != 0) {
        SDL_SetHapticAutocenter(s_haptic, 50);
    }
    if (s_haptic != NULL && s_effectReady) {
        SDL_DestroyHapticEffect(s_haptic, s_effectId);
    }
    s_effectReady = 0;
    s_sentLevel = 0;
    if (s_haptic != NULL) {
        SDL_CloseHaptic(s_haptic);
        s_haptic = NULL;
    }
    s_boundWheel = NULL;
}

void ForceFeedbackDetachJoystick(struct SDL_Joystick *joystick) {
    if (joystick != NULL && SDL_GetJoystickID(joystick) == s_rumbleDevice) {
        StopJoystickRumble();
    }
    if (joystick != NULL && joystick == s_boundWheel) ReleaseEffect(1);
    if (joystick != NULL && joystick == s_rejectedWheel) s_rejectedWheel = NULL;
}

static void BindWheel(SDL_Joystick *wheel) {
    const char *name;

    if (wheel == s_boundWheel && s_haptic != NULL) return;
    if (wheel == NULL) {
        ReleaseEffect(1);
        return;
    }
    if (wheel == s_rejectedWheel) return;
    ReleaseEffect(1);
    s_rumbleNoted = 0;
    /* DirectInput's joystick rumble initializer resets every effect on the
     * device. Stop that path before creating effects on the haptic handle. */
    if (SDL_GetJoystickID(wheel) == s_rumbleDevice) StopJoystickRumble();
    if (!SDL_InitSubSystem(SDL_INIT_HAPTIC)) {
        s_rejectedWheel = wheel;
        fprintf(stderr, "rage-port: haptic subsystem unavailable (%s)\n",
                SDL_GetError());
        return;
    }
    s_haptic = SDL_OpenHapticFromJoystick(wheel);
    name = SDL_GetJoystickName(wheel);
    if (name == NULL) name = "racing wheel";
    if (s_haptic == NULL) {
        fprintf(stderr, "rage-port: opening force feedback on %s failed (%s); "
                        "joystick vibration remains available if supported\n",
                name, SDL_GetError());
        s_rejectedWheel = wheel;
        return;
    }
    fprintf(stderr, "rage-port: haptic device %s id=%u vid=%04x pid=%04x "
                    "features=0x%x platform=%s\n", name,
            (unsigned)SDL_GetJoystickID(wheel), SDL_GetJoystickVendor(wheel),
            SDL_GetJoystickProduct(wheel), SDL_GetHapticFeatures(s_haptic),
            SDL_GetPlatform());
    if ((SDL_GetHapticFeatures(s_haptic) & SDL_HAPTIC_CONSTANT) == 0) {
        if (SDL_HapticRumbleSupported(s_haptic) && SDL_InitHapticRumble(s_haptic)) {
            s_hapticRumbleTried = 1;
            s_hapticRumbleReady = 1;
            s_boundWheel = wheel;
            fprintf(stderr, "rage-port: %s supports vibration without constant force\n", name);
            return;
        }
        fprintf(stderr, "rage-port: %s has no constant-force feedback; "
                        "trying joystick vibration\n", name);
        if (s_haptic != NULL) SDL_CloseHaptic(s_haptic);
        s_haptic = NULL;
        s_rejectedWheel = wheel;
        return;
    }
    if ((SDL_GetHapticFeatures(s_haptic) & SDL_HAPTIC_AUTOCENTER) != 0) {
        SDL_SetHapticAutocenter(s_haptic, 0);
    }
    memset(&s_effect, 0, sizeof(s_effect));
    s_effect.type = SDL_HAPTIC_CONSTANT;
    s_effect.constant.direction.type = SDL_HAPTIC_CARTESIAN;
    s_effect.constant.direction.dir[0] = 1;
    s_effect.constant.length = SDL_HAPTIC_INFINITY;
    s_effect.constant.level = 0;
    s_effectId = SDL_CreateHapticEffect(s_haptic, &s_effect);
    s_effectReady = s_effectId >= 0;
    if (!s_effectReady || !SDL_RunHapticEffect(s_haptic, s_effectId, 1)) {
        fprintf(stderr, "rage-port: could not start force feedback on %s (%s)\n",
                name, SDL_GetError());
        ReleaseEffect(1);
        s_rejectedWheel = wheel;
        return;
    }
    s_effectReady = 1;
    s_boundWheel = wheel;
    s_sentLevel = 0;
    s_previousAxis = AnalogSteeringDeflection();
    s_updateNoted = 0;
    fprintf(stderr, "rage-port: force feedback on %s\n", name);
}

static void SendTorque(float torque) {
    Sint16 level;
    float scaled;

    if (!s_effectReady || s_haptic == NULL) return;
    scaled = torque * 32767.0f;
    if (scaled > 32767.0f) scaled = 32767.0f;
    if (scaled < -32767.0f) scaled = -32767.0f;
    level = (Sint16)scaled;
    if (level == s_sentLevel) return;
    s_effect.constant.level = level;
    if (!SDL_UpdateHapticEffect(s_haptic, s_effectId, &s_effect)) {
        if (!s_updateNoted) {
            s_updateNoted = 1;
            fprintf(stderr, "rage-port: force feedback update failed (%s)\n",
                    SDL_GetError());
        }
        return;
    }
    s_sentLevel = level;
}

static void SendRumble(float low, float high) {
    SDL_Gamepad *pad = AnalogActiveGamepad();
    SDL_Joystick *device = AnalogWheelActive() ? AnalogWheelJoystick()
                            : pad != NULL ? SDL_GetGamepadJoystick(pad) : NULL;
    SDL_JoystickID id;
    Uint16 lowLevel;
    Uint16 highLevel;

    if (low < 0.0f) low = 0.0f;
    if (high < 0.0f) high = 0.0f;
    if (low > 1.0f) low = 1.0f;
    if (high > 1.0f) high = 1.0f;
    if (device != NULL && device == s_boundWheel && s_haptic != NULL) {
        float strength = low > high ? low : high;
        StopJoystickRumble();
        /* Reuse the haptic handle: SDL_RumbleJoystick can reset the constant
         * effect on DirectInput devices, even for a zero-strength command. */
        if (strength == 0.0f) {
            if (s_hapticRumblePlaying) SDL_StopHapticRumble(s_haptic);
            s_hapticRumblePlaying = 0;
            return;
        }
        if (!s_hapticRumbleTried) {
            s_hapticRumbleTried = 1;
            s_hapticRumbleReady = SDL_HapticRumbleSupported(s_haptic) &&
                                  SDL_InitHapticRumble(s_haptic);
            if (!s_hapticRumbleReady)
                fprintf(stderr, "rage-port: separate haptic vibration unavailable on %s (%s); "
                                "constant force remains active\n",
                        SDL_GetJoystickName(device), SDL_GetError());
        }
        if (s_hapticRumbleReady) {
            if (SDL_PlayHapticRumble(s_haptic, strength, 80))
                s_hapticRumblePlaying = 1;
            else if (!s_rumbleNoted) {
                s_rumbleNoted = 1;
                fprintf(stderr, "rage-port: haptic vibration failed on %s (%s)\n",
                        SDL_GetJoystickName(device), SDL_GetError());
            }
        }
        return;
    }
    id = device != NULL ? SDL_GetJoystickID(device) : 0;
    if (id != s_rumbleDevice) {
        StopJoystickRumble();
        s_rumbleDevice = id;
        s_rumbleNoted = 0;
    }
    if (device == NULL) return;
    lowLevel = (Uint16)(low * 65535.0f);
    highLevel = (Uint16)(high * 65535.0f);
    /* A zero command can be sent once. A live rumble has a short duration, so
     * the same strength has to be posted again every frame or the pad goes
     * quiet while the car is still sliding. */
    if (lowLevel == 0 && highLevel == 0) {
        if (!s_rumbleLatched || (s_lastLow == 0 && s_lastHigh == 0)) return;
        s_lastLow = 0;
        s_lastHigh = 0;
        s_rumbleLatched = 1;
        SDL_RumbleJoystick(device, 0, 0, 0);
        return;
    }
    s_lastLow = lowLevel;
    s_lastHigh = highLevel;
    s_rumbleLatched = 1;
    if (!SDL_RumbleJoystick(device, lowLevel, highLevel, 80) && !s_rumbleNoted) {
        s_rumbleNoted = 1;
        fprintf(stderr, "rage-port: vibration unavailable on %s (%s)\n",
                SDL_GetJoystickName(device), SDL_GetError());
    }
}

static int Driving(void) {
    return g_SceneId == GAME_SCENE_RACE && g_RacePaused == 0 &&
           g_RacePhase < RACE_PHASE_FINISHED &&
           g_PlayerCar.initializedFlag != 0;
}

static float EngineWave(int rpm) {
    unsigned phase;

    if (rpm < 0) rpm = 0;
    s_enginePhase += (unsigned)rpm;
    phase = (s_enginePhase >> 6) & 255u;
    if (phase < 128u) return (float)phase / 64.0f - 1.0f;
    return (192.0f - (float)phase) / 64.0f;
}

static void FillCar(ForceFeedbackSample *sample) {
    const PlayerCarRuntime *car = &g_PlayerCar;

    sample->steerPos = car->drive.steerPos;
    sample->speed = car->speed;
    sample->steeringLoadAngle = car->drive.steeringLoadAngle;
    sample->gripLossTimer = g_GripLossTimer;
    if (car->motionActive) sample->collisionLateral = car->velocityX;
    sample->roadImpulse = car->bodyKickOffset;
    if (g_TrackPointCount > 0) {
        const GameTrackPoint *point = TrackPoint(car->trackPointIndex);
        if (point != NULL) sample->crossSlope = point->crossSlope;
    }
    sample->engineWave = EngineWave(car->drive.engineRpm);
}

void PortUpdateForceFeedback(void) {
    ForceFeedbackSample sample = {0};
    ForceFeedbackTuning tuning;
    ForceFeedbackOutput output;
    float axis = AnalogSteeringDeflection();
    int onlineDriving=0;int32_t online[8]={0};
#ifndef __ANDROID__
    onlineDriving=OnlineForceFeedback(online);
#endif

    LoadSettings();
    /* Keep one effect during a race or settings preview. In ordinary menus
     * release it once so the driver's centering spring can take over. */
    BindWheel(AnalogWheelActive() &&
                      (g_SceneId == GAME_SCENE_RACE || onlineDriving || s_pulseFrames > 0)
                  ? AnalogWheelJoystick() : NULL);
    sample.wheelAxis = axis;
    sample.previousWheelAxis = s_previousAxis;
    sample.smoothed = s_smoothed;
    sample.active = Driving();
    if (sample.active) FillCar(&sample);
    if(onlineDriving){
        sample.active=1;sample.steerPos=online[0];sample.speed=online[1];
        sample.steeringLoadAngle=online[2];sample.gripLossTimer=online[3];
        sample.collisionLateral=online[4];sample.roadImpulse=online[5];sample.crossSlope=online[6];
        sample.engineWave=EngineWave(online[7]);
    }

    tuning = s_tuning;
    tuning.enabled = s_enabled;
    tuning.gain = (float)s_gainPercent / 100.0f;
    output = ForceFeedbackEvaluate(&tuning, &sample);
    if (!sample.active || !s_enabled || s_gainPercent == 0)
        output.torque = 0.0f;
    if (s_pulseFrames > 0) {
        s_pulseFrames--;
        if (s_enabled && s_gainPercent > 0) {
            float pulse = 0.30f * tuning.gain;
            if (output.torque < pulse) output.torque = pulse;
            if (output.lowRumble < pulse) output.lowRumble = pulse;
            if (output.highRumble < pulse) output.highRumble = pulse;
        }
    }
    s_smoothed = output.torque;
    s_previousAxis = axis;

    SendTorque(output.torque);
    SendRumble(output.lowRumble, output.highRumble);
}

int PortForceFeedbackEnabled(void) {
    LoadSettings();
    return s_enabled;
}

void PortSetForceFeedbackEnabled(int enabled) {
    LoadSettings();
    if (s_harnessLock) return;
    enabled = enabled != 0;
    if (enabled == s_enabled) return;
    s_enabled = enabled;
    if (s_enabled) s_rejectedWheel = NULL;
    s_pulseFrames = s_enabled ? FFB_PULSE_FRAMES : 0;
    WritePlayerFile();
    fprintf(stderr, "rage-port: force feedback %s\n",
            s_enabled ? "on" : "off");
}

int PortForceFeedbackGainPercent(void) {
    LoadSettings();
    return s_gainPercent;
}

void PortSetForceFeedbackGainPercent(int percent) {
    LoadSettings();
    if (s_harnessLock) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (percent == s_gainPercent) return;
    s_gainPercent = percent;
    WritePlayerFile();
}

void PortToggleForceFeedback(void) {
    LoadSettings();
    s_enabled = !s_enabled;
    if (s_enabled) s_rejectedWheel = NULL;
    if (s_enabled) s_pulseFrames = FFB_PULSE_FRAMES;
    else s_pulseFrames = 0;
    WritePlayerFile();
    fprintf(stderr, "rage-port: force feedback %s\n",
            s_enabled ? "on" : "off");
}

int PortAdjustForceFeedbackGain(int direction) {
    int next;

    LoadSettings();
    if (!s_enabled || direction == 0) return 0;
    next = s_gainPercent + (direction < 0 ? -FFB_GAIN_STEP : FFB_GAIN_STEP);
    if (next < 0) next = 0;
    if (next > 100) next = 100;
    if (next == s_gainPercent) return 0;
    s_gainPercent = next;
    WritePlayerFile();
    return 1;
}

void PortForceFeedbackLabel(char *text, size_t size) {
    if (text == NULL || size == 0) return;
    LoadSettings();
    if (!s_enabled) {
        snprintf(text, size, "FFB OFF");
        return;
    }
    snprintf(text, size, "FFB %3d", s_gainPercent);
}
