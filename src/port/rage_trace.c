#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "game/car.h"
#include "game/diagnostics.h"
#include "game/race.h"
#include "game/state.h"
#include "rage/trace.h"

void TraceCarMotion(const char *phase, PlayerCarRuntime *car) {
    static int enabled = -1;
    static int timer = -1;

    if (enabled < 0) {
        const char *text = DiagnosticsValue("car.motion_trace_timer");
        enabled = DiagnosticsEnabled("car.motion_trace");
        timer = text != NULL ? (int)strtol(text, NULL, 0) : -1;
    }
    if (!enabled || (timer >= 0 && timer != g_SceneTimer)) return;

    printf("car-motion phase=%s timer=%d x=%d z=%d rotation=%d,%d,%d "
           "roll_velocity=%d kick=%d,%d,%d,%d motion=%d,%d "
           "knockback=%d,%d,%d,%d point=%d progress=%d lateral=%d speed=%d\n",
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
        const char *minText = DiagnosticsValue("car.state_trace_timer_min");
        const char *maxText = DiagnosticsValue("car.state_trace_timer_max");
        enabled = DiagnosticsEnabled("car.state_trace");
        timerMin = minText != NULL ? (int)strtol(minText, NULL, 0) : -1;
        timerMax = maxText != NULL ? (int)strtol(maxText, NULL, 0) : -1;
    }
    if (!enabled || (timerMin >= 0 && g_SceneTimer < timerMin) ||
        (timerMax >= 0 && g_SceneTimer > timerMax)) return;

    for (index = 0; index < 11; index++) {
        GameCarRuntime *opponent = &g_Cars[index];
        printf("car-state timer=%d index=%d x=%d z=%d progress=%d lateral=%d "
               "speed=%d point=%d yaw=%d active=%d collision=%d\n",
               g_SceneTimer, index, opponent->x, opponent->z,
               opponent->trackProgress, opponent->trackLateralOffset,
               opponent->speed, opponent->trackPointIndex, opponent->bodyYaw,
               opponent->activeFlag, opponent->collisionFlag);
    }
}
