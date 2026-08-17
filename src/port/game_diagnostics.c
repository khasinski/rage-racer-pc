#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "game/diagnostics.h"
#include "runtime_config.h"

typedef struct RageDiagnosticKey {
    const char *key;
    const char *legacyEnvironment;
} RageDiagnosticKey;

static const RageDiagnosticKey s_keys[] = {
    {"car.motion_trace", "RAGE_PORT_CAR_MOTION_TRACE"},
    {"car.motion_trace_timer", "RAGE_PORT_CAR_MOTION_TRACE_TIMER"},
    {"car.state_trace", "RAGE_PORT_CAR_STATE_TRACE"},
    {"car.state_trace_timer_min", "RAGE_PORT_CAR_STATE_TRACE_TIMER_MIN"},
    {"car.state_trace_timer_max", "RAGE_PORT_CAR_STATE_TRACE_TIMER_MAX"},
    {"car.track_trace", "RAGE_PORT_CAR_TRACK_TRACE"},
    {"car.track_trace_timer", "RAGE_PORT_CAR_TRACK_TRACE_TIMER"},
    {"car.track_trace_timer_min", "RAGE_PORT_CAR_TRACK_TRACE_TIMER_MIN"},
    {"car.track_trace_timer_max", "RAGE_PORT_CAR_TRACK_TRACE_TIMER_MAX"},
    {"car.knockback_trace", "RAGE_PORT_CAR_KNOCKBACK_TRACE"},
    {"car.knockback_trace_timer", "RAGE_PORT_CAR_KNOCKBACK_TRACE_TIMER"},
    {"car.knockback_trace_timer_min", "RAGE_PORT_CAR_KNOCKBACK_TRACE_TIMER_MIN"},
    {"car.knockback_trace_timer_max", "RAGE_PORT_CAR_KNOCKBACK_TRACE_TIMER_MAX"},
    {"car.collision_trace", "RAGE_PORT_CAR_COLLISION_TRACE"},
    {"car.collision_trace_timer", "RAGE_PORT_CAR_COLLISION_TRACE_TIMER"},
    {"render.car_draw_trace", "RAGE_PORT_CAR_DRAW_TRACE"},
    {"render.car_draw_trace_timer", "RAGE_PORT_CAR_DRAW_TRACE_TIMER"},
    {"render.tachometer_trace", "RAGE_PORT_TACHO_TRACE"},
    {"input.debug", "RAGE_PORT_INPUT_DEBUG"},
    {"random.trace", "RAGE_PORT_RANDOM_TRACE"},
};

static const RageDiagnosticKey *RageFindDiagnostic(const char *key) {
    size_t index;
    for (index = 0; index < sizeof(s_keys) / sizeof(s_keys[0]); index++) {
        if (strcmp(s_keys[index].key, key) == 0) return &s_keys[index];
    }
    return NULL;
}

const char *RageDiagnosticsValue(const char *key) {
    const RageDiagnosticKey *entry = RageFindDiagnostic(key);
    char fullKey[128];
    if (entry == NULL ||
        snprintf(fullKey, sizeof(fullKey), "diagnostics.%s", key) >=
            (int)sizeof(fullKey))
        return NULL;
    return RageRuntimeConfigGetLegacy(fullKey, entry->legacyEnvironment);
}

int RageDiagnosticsEnabled(const char *key) {
    const RageDiagnosticKey *entry = RageFindDiagnostic(key);
    char fullKey[128];
    if (entry == NULL ||
        snprintf(fullKey, sizeof(fullKey), "diagnostics.%s", key) >=
            (int)sizeof(fullKey))
        return 0;
    return RageRuntimeConfigEnabled(fullKey, entry->legacyEnvironment);
}
