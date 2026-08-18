#include "modern/modern_profiler.h"

#include <assert.h>

int main(void) {
    ModernProfiler profiler;
    ModernProfileReport report;
    const ModernProfileSample first = {1000000, 3000000, 10, 20, 2};
    const ModernProfileSample second = {3000000, 5000000, 30, 40, 4};

    ModernProfilerInit(&profiler, 2);
    assert(!ModernProfilerAdd(&profiler, &first, &report));
    assert(ModernProfilerAdd(&profiler, &second, &report));
    assert(report.frames == 2);
    assert(report.buildMs == 2.0);
    assert(report.submitMs == 4.0);
    assert(report.faces == 20.0);
    assert(report.vertices == 30.0);
    assert(report.spans == 3.0);
    assert(profiler.frames == 0 && profiler.reportInterval == 2);

    ModernProfilerInit(&profiler, 0);
    assert(ModernProfilerAdd(&profiler, &first, &report));
    assert(report.frames == 1);
    return 0;
}
