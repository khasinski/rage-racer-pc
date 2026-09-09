#include "modern_presentation_clock.h"

#include <assert.h>

int main(void) {
    ModernPresentationClock clock;

    ModernPresentationClockReset(&clock);
    assert(clock.tickFrame == UINT32_MAX);
    assert(ModernPresentationClockFraction(&clock, 100) == 1.0f);
    assert(ModernPresentationClockDue(&clock, 100, 10));

    ModernPresentationClockObserveFrame(&clock, 40, 10000000u);
    assert(clock.tickIntervalNs == 0);
    /* Seeing the same world in the present hook must not manufacture a tick. */
    ModernPresentationClockObserveFrame(&clock, 40, 20000000u);
    assert(clock.tickTimeNs == 10000000u);
    assert(clock.tickIntervalNs == 0);

    ModernPresentationClockObserveFrame(&clock, 41, 30000000u);
    assert(clock.tickIntervalNs == 20000000u);
    assert(ModernPresentationClockFraction(&clock, 30000000u) == 1.0f);
    assert(ModernPresentationClockFraction(&clock, 35000000u) == 0.25f);
    assert(ModernPresentationClockFraction(&clock, 50000000u) == 1.0f);
    assert(ModernPresentationClockFraction(&clock, 90000000u) == 1.0f);

    ModernPresentationClockPresented(&clock, 100, 16);
    assert(!ModernPresentationClockDue(&clock, 115, 16));
    assert(ModernPresentationClockDue(&clock, 116, 16));
    ModernPresentationClockPresented(&clock, 116, 16);
    assert(!ModernPresentationClockDue(&clock, 131, 16));
    assert(ModernPresentationClockDue(&clock, 132, 16));

    /* A debugger/suspend gap cannot become the interpolation interval. */
    ModernPresentationClockObserveFrame(&clock, 42, 400000000u);
    assert(clock.tickIntervalNs == 20000000u);
    return 0;
}
