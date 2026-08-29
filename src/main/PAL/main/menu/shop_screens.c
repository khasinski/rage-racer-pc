#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"


u32 DrawEngineerShopScreen(s32 step) {
    s32 value;
    s32 amount;

    if (step == 0) {
        g_EngineSpecStep = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_EngineSpecStep;
        g_EngineSpecStep = value;
        if (value >= 0x1FD) {
            g_EngineSpecStep = 0x1FC;
        }
        amount = 0;
    } else {
        s32 diff = 0x1FC;
        u32 product;

        value = step + g_EngineSpecStep;
        g_EngineSpecStep = value;
        if (value < 0) {
            g_EngineSpecStep = 0;
        }
        diff -= g_EngineSpecStep;
        product = diff * diff;
        amount = product / 2048;
    }

    DrawCarEngineSpec((s16)amount, (u8)(g_EngineSpecStep >> 2));
    return g_EngineSpecStep;
}


void ShopScreenNoOp(void) {
}
