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

void UpdateOwnedCarNeighbours(CarBrowse *browse) {
    if (g_CarTable == NULL || (u32)g_PlayerCarIndex >= GAME_CAR_COUNT) {
        browse->previous = -1;
        browse->next = -1;
        return;
    }
    browse->previous = FindOwnedCar(g_PlayerCarIndex - 1, -1);
    browse->next = FindOwnedCar(g_PlayerCarIndex + 1, 1);
}

void EnterCarSelectScreen(void) {
    ActivateShowroomCarModel((s32)g_CarModelSlot);
    MenuActivateScreen(MENU_SCREEN_CAR_SELECT);
    g_UiScriptProgress = 0;
    UpdateOwnedCarNeighbours(MenuCarBrowse());
    DrawCarNamePlate(MenuWidgetState());
    DrawMenuCarView();
    DrawMenuLightBurst(MenuWidgetState(), -9);
}
