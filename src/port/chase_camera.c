#include "rage/chase_camera.h"

#include <stdio.h>
#include <stdlib.h>

#include "runtime_config.h"

enum { RAGE_CHASE_FULL_LOCK_YAW = 341 };

static int s_initialized;
static float s_lookahead;

static void ChaseCameraInit(void) {
    const char *text;
    char *end;
    float value;

    if (s_initialized) return;
    s_initialized = 1;
    s_lookahead = 0.0f;
    text = RuntimeConfigGet("camera.chase_turn_lookahead");
    if (text == NULL || text[0] == '\0') return;
    value = strtof(text, &end);
    if (*end != '\0' || value < 0.0f || value > 1.0f) {
        fprintf(stderr,
                "rage-port: ignoring camera.chase_turn_lookahead=%s "
                "(expected 0..1); using 0\n",
                text);
        return;
    }
    s_lookahead = value;
}

int ChaseCameraYawOffset(int steeringAngle) {
    int offset;

    ChaseCameraInit();
    if (steeringAngle > 4096) steeringAngle = 4096;
    if (steeringAngle < -4096) steeringAngle = -4096;
    offset = (int)((float)steeringAngle * s_lookahead / 12.0f);
    if (offset > RAGE_CHASE_FULL_LOCK_YAW) offset = RAGE_CHASE_FULL_LOCK_YAW;
    if (offset < -RAGE_CHASE_FULL_LOCK_YAW) offset = -RAGE_CHASE_FULL_LOCK_YAW;
    return offset;
}
