#ifndef RAGE_TIMING_CONTROL_H
#define RAGE_TIMING_CONTROL_H

typedef enum RageTimingStandard {
    RAGE_TIMING_PAL = 0,
    RAGE_TIMING_NTSC = 1
} RageTimingStandard;

void TimingInit(void);

/* Re-apply the standard after the platform has created its video device. */
void TimingApply(void);

/* Whether a measured frame time has drifted off the standard enough to restore.
 * A frame time of zero means nothing has been measured yet. */
int TimingNeedsRestore(double currentFrameTimeUs, int baseHz);
RageTimingStandard TimingStandardForRegion(const char *region);
void TimingSetStandard(RageTimingStandard standard);
int TimingBaseHz(void);

#endif
