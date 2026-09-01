#include "game/car.h"
#include "game/menu.h"

void UpdateOwnedCarNeighbours(void) {
    s32 index;

    g_PrevOwnedCarIndex = -1;
    for (index = g_PlayerCarIndex - 1; index >= 0; index--) {
        if (g_CarTable[index].enabled == 1) {
            g_PrevOwnedCarIndex = index;
            break;
        }
    }

    g_NextOwnedCarIndex = -1;
    for (index = g_PlayerCarIndex + 1; index < GAME_CAR_COUNT; index++) {
        if (g_CarTable[index].enabled == 1) {
            g_NextOwnedCarIndex = index;
            break;
        }
    }
}

void EnterCarSelectScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    InstallCarModelSlot();
    g_MenuScreen = 4;
    g_UiScriptProgress = 0;
    UpdateOwnedCarNeighbours();
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    DrawMenuLightBurst(-9);
}
