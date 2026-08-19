#include "timing_control.h"

#include <stdio.h>
#include <string.h>
#include <psyz/video.h>
#include "runtime_config.h"

static RageTimingStandard s_standard = RAGE_TIMING_PAL;

void RageTimingSetStandard(RageTimingStandard standard) {
    s_standard = standard == RAGE_TIMING_NTSC ? RAGE_TIMING_NTSC
                                               : RAGE_TIMING_PAL;
    Psyz_VideoSetTargetFramerate((double)RageTimingBaseHz());
    fprintf(stderr, "rage-port: timing=%s base_hz=%d\n",
            s_standard == RAGE_TIMING_NTSC ? "ntsc" : "pal",
            RageTimingBaseHz());
}

/* The platform layer initialises its own timing when the video device is
 * created, which is after main() has chosen the standard, and it starts at the
 * NTSC frame time. Re-applying once the device exists is what makes a PAL game
 * run at fifty rather than sixty: without it every scene, and with them the
 * replay and the attract demo, ran a fifth too fast. */
void RageTimingApply(void) {
    /* The platform layer resets its frame time to NTSC whenever it initialises
     * its video device, which the game triggers again on display mode changes,
     * long after main() chose the standard. Left alone, a PAL game paced itself
     * at sixty rather than fifty and ran a fifth too fast: unmistakable in the
     * replay and the attract demo, where nothing the player does hides it.
     * Restore the standard whenever it has drifted rather than once at boot. */
    PsyzVideoStats stats;
    if (Psyz_VideoStats(&stats) != 0) return;
    if (RageTimingNeedsRestore(stats.target_frame_time_us, RageTimingBaseHz())) {
        static int announced;
        Psyz_VideoSetTargetFramerate((double)RageTimingBaseHz());
        if (!announced) {
            announced = 1;
            fprintf(stderr,
                    "rage-port: frame pacing held at %d Hz; the platform keeps resetting it\n",
                    RageTimingBaseHz());
        }
    }
}

void RageTimingInit(void) {
    const char *value = RageRuntimeConfigGet("timing.standard");
    RageTimingSetStandard(value != NULL && strcmp(value, "ntsc") == 0
                              ? RAGE_TIMING_NTSC
                              : RAGE_TIMING_PAL);
}

RageTimingStandard RageTimingGetStandard(void) { return s_standard; }
int RageTimingBaseHz(void) { return s_standard == RAGE_TIMING_NTSC ? 60 : 50; }
