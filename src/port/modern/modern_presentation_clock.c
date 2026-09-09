#include "modern_presentation_clock.h"

#include <string.h>

void ModernPresentationClockReset(ModernPresentationClock *clock) {
    if (clock == NULL) return;
    memset(clock, 0, sizeof(*clock));
    clock->tickFrame = UINT32_MAX;
}

void ModernPresentationClockObserveFrame(ModernPresentationClock *clock,
                                         uint32_t frame, uint64_t now) {
    uint64_t delta;
    if (clock == NULL || frame == clock->tickFrame) return;
    if (clock->tickTimeNs != 0 && now >= clock->tickTimeNs) {
        delta = now - clock->tickTimeNs;
        /* Ignore startup and debugger/suspend gaps. They are not a game tick
         * and must not stretch the next interpolated presentation. */
        if (delta > 1000000u && delta < 200000000u)
            clock->tickIntervalNs = delta;
    }
    clock->tickFrame = frame;
    clock->tickTimeNs = now;
}

float ModernPresentationClockFraction(const ModernPresentationClock *clock,
                                      uint64_t now) {
    double fraction;
    if (clock == NULL || clock->tickTimeNs == 0 ||
        clock->tickIntervalNs == 0 || now <= clock->tickTimeNs) {
        return 1.0f;
    }
    fraction = (double)(now - clock->tickTimeNs) /
               (double)clock->tickIntervalNs;
    return fraction >= 1.0 ? 1.0f : (float)fraction;
}

int ModernPresentationClockDue(const ModernPresentationClock *clock,
                               uint64_t now, uint64_t interval) {
    return clock != NULL && ModernFrameDue(&clock->pacer, now, interval);
}

void ModernPresentationClockPresented(ModernPresentationClock *clock,
                                      uint64_t now, uint64_t interval) {
    if (clock != NULL) ModernFramePresented(&clock->pacer, now, interval);
}
