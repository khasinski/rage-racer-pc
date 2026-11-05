#ifndef RAGE_MENU_ANIMATION_H
#define RAGE_MENU_ANIMATION_H

#include <limits.h>
#include <stdint.h>

/* Animation time only: input, asset loading and gameplay retain their clocks. */
int PortMenuAnimationTicks(void);

static inline int PortMenuAnimationStep(int step) {
    int64_t scaled = (int64_t)step * PortMenuAnimationTicks();
    return scaled > INT_MAX ? INT_MAX : scaled < INT_MIN ? INT_MIN : (int)scaled;
}

#endif
