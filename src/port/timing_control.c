#include "timing_control.h"

#include <stdio.h>
#include <string.h>
#include <psyz/video.h>
#include "runtime_config.h"
#include "host_disc.h"

static RageTimingStandard s_standard = RAGE_TIMING_PAL;

void TimingSetStandard(RageTimingStandard standard) {
    s_standard = standard == RAGE_TIMING_NTSC ? RAGE_TIMING_NTSC
                                               : RAGE_TIMING_PAL;
    Psyz_VideoSetTargetFramerate((double)TimingBaseHz());
    fprintf(stderr, "rage-port: timing=%s base_hz=%d\n",
            s_standard == RAGE_TIMING_NTSC ? "ntsc" : "pal",
            TimingBaseHz());
}

/* Retain a defensive runtime check for older platform implementations that
 * initialize video after main() has selected the standard. Current PsyZ keeps
 * an explicit rate across video initialization and display-mode changes. */
void TimingApply(void) {
    /* If a backend ever loses the selected standard, restore it before game
     * logic advances. PAL running at sixty is unmistakable in replay/attract. */
    PsyzVideoStats stats;
    if (Psyz_VideoStats(&stats) != 0) return;
    if (TimingNeedsRestore(stats.target_frame_time_us, TimingBaseHz())) {
        static int announced;
        Psyz_VideoSetTargetFramerate((double)TimingBaseHz());
        if (!announced) {
            announced = 1;
            fprintf(stderr,
                    "rage-port: frame pacing restored to %d Hz\n",
                    TimingBaseHz());
        }
    }
}

void TimingInit(void) {
    const char *value = RuntimeConfigGet("timing.standard");
    if (value != NULL && strcmp(value, "pal") == 0) {
        TimingSetStandard(RAGE_TIMING_PAL);
    } else if (value != NULL && strcmp(value, "ntsc") == 0) {
        TimingSetStandard(RAGE_TIMING_NTSC);
    } else {
        const char *region = HostDiscRegion();
        fprintf(stderr,
                "rage-port: timing standard selected from disc region=%s\n",
                region != NULL ? region : "unknown");
        TimingSetStandard(TimingStandardForRegion(region));
    }
}

int TimingBaseHz(void) { return s_standard == RAGE_TIMING_NTSC ? 60 : 50; }
