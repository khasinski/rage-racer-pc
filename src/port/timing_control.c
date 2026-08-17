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

void RageTimingInit(void) {
    const char *value = RageRuntimeConfigGet("timing.standard");
    RageTimingSetStandard(value != NULL && strcmp(value, "ntsc") == 0
                              ? RAGE_TIMING_NTSC
                              : RAGE_TIMING_PAL);
}

RageTimingStandard RageTimingGetStandard(void) { return s_standard; }
int RageTimingBaseHz(void) { return s_standard == RAGE_TIMING_NTSC ? 60 : 50; }
