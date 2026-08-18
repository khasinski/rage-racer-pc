#ifndef RAGE_MODERN_PROFILER_H
#define RAGE_MODERN_PROFILER_H

#include <stdint.h>

typedef struct ModernProfileSample {
    uint64_t buildNs;
    uint64_t submitNs;
    uint32_t faces;
    uint32_t vertices;
    uint32_t spans;
} ModernProfileSample;

typedef struct ModernProfileReport {
    uint32_t frames;
    double buildMs;
    double submitMs;
    double faces;
    double vertices;
    double spans;
} ModernProfileReport;

typedef struct ModernProfiler {
    uint64_t buildNs;
    uint64_t submitNs;
    uint64_t faces;
    uint64_t vertices;
    uint64_t spans;
    uint32_t frames;
    uint32_t reportInterval;
} ModernProfiler;

void ModernProfilerInit(ModernProfiler *profiler, uint32_t reportInterval);
int ModernProfilerAdd(ModernProfiler *profiler,
                      const ModernProfileSample *sample,
                      ModernProfileReport *report);

#endif
