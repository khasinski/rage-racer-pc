#ifndef RAGE_MODERN_PRESENTATION_CLOCK_H
#define RAGE_MODERN_PRESENTATION_CLOCK_H

#include "modern_frame_pacer.h"

#include <stdint.h>

/* Presentation can observe a completed logic frame from either the game hook
 * or the present callback. Keep that timing policy independent from SDL, GPU
 * resources and the renderer so both paths share one clock. */
typedef struct ModernPresentationClock {
    uint64_t tickTimeNs;
    uint64_t tickIntervalNs;
    uint32_t tickFrame;
    ModernFramePacer pacer;
} ModernPresentationClock;

void ModernPresentationClockReset(ModernPresentationClock *clock);
void ModernPresentationClockObserveFrame(ModernPresentationClock *clock,
                                         uint32_t frame, uint64_t now);
float ModernPresentationClockFraction(const ModernPresentationClock *clock,
                                      uint64_t now);
int ModernPresentationClockDue(const ModernPresentationClock *clock,
                               uint64_t now, uint64_t interval);
void ModernPresentationClockPresented(ModernPresentationClock *clock,
                                      uint64_t now, uint64_t interval);

#endif
