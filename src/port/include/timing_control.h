#ifndef RAGE_TIMING_CONTROL_H
#define RAGE_TIMING_CONTROL_H

typedef enum RageTimingStandard {
    RAGE_TIMING_PAL = 0,
    RAGE_TIMING_NTSC = 1
} RageTimingStandard;

void RageTimingInit(void);
RageTimingStandard RageTimingGetStandard(void);
void RageTimingSetStandard(RageTimingStandard standard);
int RageTimingBaseHz(void);

#endif
