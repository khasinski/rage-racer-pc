
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "runtime_config.h"

enum {
    RAGE_CHASE_FULL_LOCK_YAW = 341,
    RAGE_ANGLE_UNITS_PER_TURN = 4096,
};

static int s_initialized;
static float s_lookahead;
static float s_heightScale;
static float s_distanceScale;
static float s_pitchDegrees;

static float ConfigFloat(const char *key, float fallback,
                         float minimum, float maximum) {
    const char *text = RuntimeConfigGet(key);
    char *end;
    float value;

    if (text == NULL || text[0] == '\0') return fallback;
    errno = 0;
    value = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || !isfinite(value) ||
        value < minimum || value > maximum) {
        fprintf(stderr,
                "rage-port: ignoring %s=%s (expected %.2f..%.2f); using %.2f\n",
                key, text, minimum, maximum, fallback);
        return fallback;
    }
    return value;
}

static void ChaseCameraInit(void) {
    if (s_initialized) return;
    s_initialized = 1;
    s_lookahead = ConfigFloat(
        "camera.chase_turn_lookahead", 0.0f, 0.0f, 1.0f);
    s_heightScale = ConfigFloat(
        "camera.chase_height", 1.0f, 0.25f, 4.0f);
    s_distanceScale = ConfigFloat(
        "camera.chase_distance", 1.0f, 0.25f, 4.0f);
    s_pitchDegrees = ConfigFloat(
        "camera.chase_pitch", 0.0f, -45.0f, 45.0f);
}

int ChaseCameraHeight(int authoredHeight) {
    ChaseCameraInit();
    return (int)lroundf((float)authoredHeight * s_heightScale);
}

int ChaseCameraDistance(int authoredDistance) {
    ChaseCameraInit();
    return (int)lroundf((float)authoredDistance * s_distanceScale);
}

int ChaseCameraPitchOffset(void) {
    ChaseCameraInit();
    return (int)lroundf(
        s_pitchDegrees * (float)RAGE_ANGLE_UNITS_PER_TURN / 360.0f);
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
