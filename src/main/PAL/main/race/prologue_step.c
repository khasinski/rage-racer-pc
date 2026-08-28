#include "common.h"
#include "game/state.h"
#include "game/race_internal.h"

void TickPrologueStep(void) {
    g_SceneTimer++;
    g_PrologueSteps[g_PrologueStep]();
}
