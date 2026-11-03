#include "rage/menu_animation.h"

/* Individual menu tests can select the clock without loading host settings. */
int g_TestMenuAnimationTicks = 1;
int PortMenuAnimationTicks(void) { return g_TestMenuAnimationTicks; }
