#include "modern_profiler.h"

#include <string.h>

void ModernProfilerInit(ModernProfiler *profiler, uint32_t reportInterval) {
    memset(profiler, 0, sizeof(*profiler));
    profiler->reportInterval = reportInterval == 0 ? 1 : reportInterval;
}

int ModernProfilerAdd(ModernProfiler *profiler,
                      const ModernProfileSample *sample,
                      ModernProfileReport *report) {
    double frames;

    profiler->buildNs += sample->buildNs;
    profiler->submitNs += sample->submitNs;
    profiler->faces += sample->faces;
    profiler->vertices += sample->vertices;
    profiler->spans += sample->spans;
    profiler->frames++;
    if (profiler->frames < profiler->reportInterval) return 0;

    frames = (double)profiler->frames;
    report->frames = profiler->frames;
    report->buildMs = (double)profiler->buildNs / frames / 1000000.0;
    report->submitMs = (double)profiler->submitNs / frames / 1000000.0;
    report->faces = (double)profiler->faces / frames;
    report->vertices = (double)profiler->vertices / frames;
    report->spans = (double)profiler->spans / frames;
    ModernProfilerInit(profiler, profiler->reportInterval);
    return 1;
}
