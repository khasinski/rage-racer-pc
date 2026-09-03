#include "game/car.h"
#include "game/diagnostics.h"
#include "game/state.h"

void TraceCarMotion(const char *phase, PlayerCarRuntime *car) {
    static int enabled = -1;
    static int timer = -1;

    if (enabled < 0) {
        enabled = DiagnosticsEnabled("car.motion_trace");
        timer = DiagnosticsIntValue("car.motion_trace_timer", -1);
    }
    if (!enabled || phase == NULL || car == NULL ||
        (timer >= 0 && timer != g_SceneTimer)) return;

    Trace("car-motion", "phase=%s timer=%d x=%d z=%d rotation=%d,%d,%d "
           "roll_velocity=%d kick=%d,%d,%d,%d motion=%d,%d "
           "knockback=%d,%d,%d,%d point=%d progress=%d lateral=%d speed=%d",
           phase, g_SceneTimer, car->x, car->z, car->bodyYaw, car->bodyPitch,
           car->bodyRoll, car->bodyRollVelocity, car->motionMode,
           car->motionModeTimer, car->motionValue.value, car->bodyKickOffset,
           car->motionX, car->motionZ,
           car->motionActive, car->motionTimer, car->velocityX, car->velocityZ,
           car->trackPointIndex, car->trackProgress, car->trackLateralOffset,
           car->speed);
}

void TraceCarStates(void) {
    static int enabled = -1;
    static int timerMin = -1;
    static int timerMax = -1;
    int index;

    if (enabled < 0) {
        enabled = DiagnosticsEnabled("car.state_trace");
        timerMin = DiagnosticsIntValue("car.state_trace_timer_min", -1);
        timerMax = DiagnosticsIntValue("car.state_trace_timer_max", -1);
    }
    if (!enabled || (timerMin >= 0 && g_SceneTimer < timerMin) ||
        (timerMax >= 0 && g_SceneTimer > timerMax)) return;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *opponent = &g_Cars[index];
        Trace("car-state", "timer=%d index=%d x=%d z=%d progress=%d lateral=%d "
               "speed=%d point=%d yaw=%d active=%d collision=%d",
               g_SceneTimer, index, opponent->x, opponent->z,
               opponent->trackProgress, opponent->trackLateralOffset,
               opponent->speed, opponent->trackPointIndex, opponent->bodyYaw,
               opponent->activeFlag, opponent->collisionFlag);
    }
}
