#include "game/race_clock.h"

s32 RaceClockFramesToMilliseconds(s32 frames, s32 subframeMilliseconds) {
    s32 seconds = frames / 25;
    return seconds * 1000 + (frames - seconds * 25) * 40 +
           subframeMilliseconds;
}

s32 RaceClockSaturateMilliseconds(s32 milliseconds) {
    return milliseconds >= RACE_CLOCK_MAX_TIME_MS
               ? RACE_CLOCK_MAX_TIME_MS
               : milliseconds;
}

RaceLapClock RaceClockTickLap(s32 frameCount, s32 subframeMilliseconds) {
    RaceLapClock clock;

    frameCount++;
    if (frameCount > 0xFFFF) frameCount = RACE_CLOCK_MAX_FRAMES;
    clock.frameCount = frameCount;
    clock.milliseconds = RaceClockFramesToMilliseconds(
        frameCount, subframeMilliseconds);
    clock.saturated = clock.milliseconds >= RACE_CLOCK_MAX_TIME_MS;
    clock.milliseconds = RaceClockSaturateMilliseconds(clock.milliseconds);
    return clock;
}

s32 RaceClockTickCountdown(s32 remaining, s32 running) {
    return running ? remaining - 1 : remaining;
}
