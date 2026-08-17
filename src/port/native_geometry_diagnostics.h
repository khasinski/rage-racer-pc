#ifndef RAGE_NATIVE_GEOMETRY_DIAGNOSTICS_H
#define RAGE_NATIVE_GEOMETRY_DIAGNOSTICS_H

typedef struct RageGeometryDiagnostics {
    int initialized;
    int terrainTraceEnabled;
    int terrainTraceTimer;
    int terrainTraceClut;
    int terrainTraceTpage;
    int terrainDecisionTraceEnabled;
    int terrainDecisionTraceTimer;
    int terrainDecisionTraceLimit;
    int terrainDecisionTraceCount;
    int courseTraceEnabled;
    int courseTraceTimer;
    int courseTraceClut;
    int courseTraceTpage;
    int modelTraceEnabled;
    int modelTraceTimer;
} RageGeometryDiagnostics;

void RageGeometryDiagnosticsInit(RageGeometryDiagnostics *diagnostics);

#endif
