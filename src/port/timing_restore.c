#include <math.h>

#include "timing_control.h"
#include "content/region_profile.h"

/* The game logic is the PAL release's, tuned for 50 Hz menus and 25 Hz races
 * (its physics steps and its race clock both assume 25 ticks a second). A
 * Japanese disc only supplies data, so at NTSC rates the race runs 30 ticks a
 * second and everything - car speed, lap times, countdowns - comes out 20%
 * fast. Its movies are paced by their own frame count and play at 30 fps either
 * way, so the Japanese disc keeps PAL timing. */
RageTimingStandard TimingStandardForRegion(const char *region) {
    const RageRegionProfile *profile = RageRegionByName(region);
    if (profile->id == RAGE_REGION_NTSC_J) return RAGE_TIMING_PAL;
    return profile->baseHz == 60 ? RAGE_TIMING_NTSC : RAGE_TIMING_PAL;
}

/* Whether the platform's frame time has drifted off the standard the game was
 * started in. It resets itself to the NTSC figure when it creates its video
 * device, which happens after the standard is chosen and again on display mode
 * changes, so this is asked every frame. The one microsecond of slack keeps
 * the platform's own rounding from triggering a restore, which would reset its
 * drift compensation continuously. */
int TimingNeedsRestore(double currentFrameTimeUs, int baseHz) {
    double wanted;

    if (!isfinite(currentFrameTimeUs) || currentFrameTimeUs <= 0.0 ||
        baseHz <= 0) {
        return 0;
    }
    wanted = 1000000.0 / (double)baseHz;
    return currentFrameTimeUs > wanted + 1.0 || currentFrameTimeUs < wanted - 1.0;
}
