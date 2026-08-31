#ifndef RAGE_HOST_CLOCK_H
#define RAGE_HOST_CLOCK_H

/*
 * Real time, in nanoseconds since the host started counting.
 *
 * It is one function behind its own header because the callers are compiled
 * for the PS1 layer, with its compatibility header forced in ahead of
 * everything else. SDL's headers reach the toolchain's intrinsics, and on
 * Windows the two come apart there rather than in any code of ours, so the
 * files that need a clock take this instead of including SDL.
 */
unsigned long long HostNanoseconds(void);

#endif
