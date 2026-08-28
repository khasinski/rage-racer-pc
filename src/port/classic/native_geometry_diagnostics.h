#ifndef RAGE_CLASSIC_GEOMETRY_DIAGNOSTICS_H
#define RAGE_CLASSIC_GEOMETRY_DIAGNOSTICS_H

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

void GeometryDiagnosticsInit(RageGeometryDiagnostics *diagnostics);

#endif
