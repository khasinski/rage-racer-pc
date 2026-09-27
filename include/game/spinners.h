#ifndef GAME_SPINNERS_H
#define GAME_SPINNERS_H
#include "common.h"
#include "game/scenery.h"
typedef SceneryPlacement SpinningSceneryPlacement;
_Static_assert(sizeof(SpinningSceneryPlacement) == 16,
               "SpinningSceneryPlacement must match the retail layout");
/* Copies authored placement; invalid input preserves output. */
int RetailSpinner(s32 index, SpinningSceneryPlacement *placement);
typedef struct Spinners {
    s16 angles[4];
    u16 rates[2];
} Spinners;
/* Normalizes selected angles while paused. Active playback changes angles,
 * then refreshes rates at 512-frame boundaries with a local random state. */
void TickSpinners(Spinners *spinners, int multiple, u32 timer, u32 seed, int animate);
#endif
