#include "game/menu.h"
#include "game/menu_internal.h"

s32 DrawEngineerShopScreen(s32 step) {
    return AdvanceCarSpecPanel(&g_EngineSpecStep, step);
}
