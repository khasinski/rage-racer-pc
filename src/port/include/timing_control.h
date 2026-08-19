#ifndef RAGE_TIMING_CONTROL_H
#define RAGE_TIMING_CONTROL_H

typedef enum RageTimingStandard {
    RAGE_TIMING_PAL = 0,
    RAGE_TIMING_NTSC = 1
} RageTimingStandard;

void RageTimingInit(void);

/* Re-apply the standard after the platform has created its video device. */
void RageTimingApply(void);

/* Whether a measured frame time has drifted off the standard enough to restore.
 * A frame time of zero means nothing has been measured yet. */
int RageTimingNeedsRestore(double currentFrameTimeUs, int baseHz);
RageTimingStandard RageTimingGetStandard(void);
void RageTimingSetStandard(RageTimingStandard standard);
int RageTimingBaseHz(void);

#endif
