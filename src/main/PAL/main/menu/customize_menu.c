#include "game/menu.h"
#include "game/menu_internal.h"

s32 DrawCustomizeScreen(s32 step) {
    return AdvanceCarSpecPanel(&g_CustomizeFadeAccum, step);
}
