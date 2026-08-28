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

/* Retain a defensive runtime check for older platform implementations that
 * initialize video after main() has selected the standard. Current PsyZ keeps
 * an explicit rate across video initialization and display-mode changes. */
void RageTimingApply(void) {
    /* If a backend ever loses the selected standard, restore it before game
     * logic advances. PAL running at sixty is unmistakable in replay/attract. */
    PsyzVideoStats stats;
    if (Psyz_VideoStats(&stats) != 0) return;
    if (RageTimingNeedsRestore(stats.target_frame_time_us, RageTimingBaseHz())) {
        static int announced;
        Psyz_VideoSetTargetFramerate((double)RageTimingBaseHz());
        if (!announced) {
            announced = 1;
            fprintf(stderr,
                    "rage-port: frame pacing restored to %d Hz\n",
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

int RageTimingBaseHz(void) { return s_standard == RAGE_TIMING_NTSC ? 60 : 50; }
