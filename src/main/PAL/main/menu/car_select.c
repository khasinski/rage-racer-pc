#include "game/car.h"
#include "game/menu.h"

static s32 FindOwnedCar(s32 from, s32 step) {
    s32 index;

    for (index = from; index >= 0 && index < GAME_CAR_COUNT; index += step) {
        if (g_CarTable[index].enabled == 1) {
            return index;
        }
    }
    return -1;
}

void UpdateOwnedCarNeighbours(void) {
    if (g_CarTable == NULL || (u32)g_PlayerCarIndex >= GAME_CAR_COUNT) {
        g_PrevOwnedCarIndex = -1;
        g_NextOwnedCarIndex = -1;
        return;
    }
    g_PrevOwnedCarIndex = FindOwnedCar(g_PlayerCarIndex - 1, -1);
    g_NextOwnedCarIndex = FindOwnedCar(g_PlayerCarIndex + 1, 1);
}

void EnterCarSelectScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    ActivateShowroomCarModel((s32)g_CarModelSlot);
    g_MenuScreen = MENU_SCREEN_CAR_SELECT;
    g_UiScriptProgress = 0;
    UpdateOwnedCarNeighbours();
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    DrawMenuLightBurst(-9);
}
