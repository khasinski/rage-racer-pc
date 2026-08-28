#include "native_geometry_diagnostics.h"

#include <stdlib.h>
#include <string.h>

#include "../runtime_config.h"

static int ParseValue(const char *value, int fallback, int base) {
    char *end;
    long parsed;

    if (value == NULL || value[0] == '\0') return fallback;
    parsed = strtol(value, &end, base);
    return end != value && *end == '\0' ? (int)parsed : fallback;
}

void GeometryDiagnosticsInit(RageGeometryDiagnostics *diagnostics) {
    const char *model;
    const char *modelTimer;
    const char *terrainTimer;
    const char *terrainClut;
    const char *terrainTpage;
    const char *decision;
    const char *decisionTimer;
    const char *decisionLimit;
    const char *courseTimer;
    const char *courseClut;
    const char *courseTpage;

    if (diagnostics == NULL || diagnostics->initialized) return;
    memset(diagnostics, 0, sizeof(*diagnostics));
    diagnostics->initialized = 1;
    diagnostics->modelTraceTimer = -1;
    diagnostics->terrainTraceTimer = -1;
    diagnostics->terrainTraceClut = -1;
    diagnostics->terrainTraceTpage = -1;
    diagnostics->terrainDecisionTraceTimer = -1;
    diagnostics->terrainDecisionTraceLimit = 10000;
    diagnostics->courseTraceTimer = -1;
    diagnostics->courseTraceClut = -1;
    diagnostics->courseTraceTpage = -1;

    model = RuntimeConfigGetLegacy("diagnostics.model_trace", "RAGE_PORT_MODEL_TRACE");
    modelTimer = RuntimeConfigGetLegacy("diagnostics.model_trace_timer", "RAGE_PORT_MODEL_TRACE_TIMER");
    diagnostics->modelTraceEnabled = model != NULL || modelTimer != NULL;
    diagnostics->modelTraceTimer = ParseValue(modelTimer, -1, 0);

    terrainTimer = RuntimeConfigGetLegacy("diagnostics.terrain_trace_timer", "RAGE_PORT_TERRAIN_TRACE_TIMER");
    terrainClut = RuntimeConfigGetLegacy("diagnostics.terrain_trace_clut", "RAGE_PORT_TERRAIN_TRACE_CLUT");
    terrainTpage = RuntimeConfigGetLegacy("diagnostics.terrain_trace_tpage", "RAGE_PORT_TERRAIN_TRACE_TPAGE");
    diagnostics->terrainTraceEnabled = terrainTimer != NULL || terrainClut != NULL || terrainTpage != NULL;
    diagnostics->terrainTraceTimer = ParseValue(terrainTimer, -1, 0);
    diagnostics->terrainTraceClut = ParseValue(terrainClut, -1, 16);
    diagnostics->terrainTraceTpage = ParseValue(terrainTpage, -1, 16);

    decision = RuntimeConfigGetLegacy("diagnostics.terrain_decision_trace", "RAGE_PORT_TERRAIN_DECISION_TRACE");
    decisionTimer = RuntimeConfigGetLegacy("diagnostics.terrain_decision_timer", "RAGE_PORT_TERRAIN_DECISION_TIMER");
    decisionLimit = RuntimeConfigGetLegacy("diagnostics.terrain_decision_limit", "RAGE_PORT_TERRAIN_DECISION_LIMIT");
    diagnostics->terrainDecisionTraceEnabled = decision != NULL || decisionTimer != NULL;
    diagnostics->terrainDecisionTraceTimer = ParseValue(decisionTimer, -1, 0);
    diagnostics->terrainDecisionTraceLimit = ParseValue(decisionLimit, 10000, 0);

    courseTimer = RuntimeConfigGetLegacy("diagnostics.course_trace_timer", "RAGE_PORT_COURSE_TRACE_TIMER");
    courseClut = RuntimeConfigGetLegacy("diagnostics.course_trace_clut", "RAGE_PORT_COURSE_TRACE_CLUT");
    courseTpage = RuntimeConfigGetLegacy("diagnostics.course_trace_tpage", "RAGE_PORT_COURSE_TRACE_TPAGE");
    diagnostics->courseTraceEnabled = courseTimer != NULL || courseClut != NULL || courseTpage != NULL;
    diagnostics->courseTraceTimer = ParseValue(courseTimer, -1, 0);
    diagnostics->courseTraceClut = ParseValue(courseClut, -1, 16);
    diagnostics->courseTraceTpage = ParseValue(courseTpage, -1, 16);
}
