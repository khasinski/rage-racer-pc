#include "game/car.h"
#include "game/asset.h"
#include "game/render.h"

void ActivateShowroomCarModel(void) {
    if (g_CarModelSlot >= CAR_ASSET_SLOT_COUNT ||
        g_CarModelSlots[g_CarModelSlot] == NULL) {
        return;
    }
    SelectCarModelSlot(g_CarModelSlot);
    SelectModelBank(g_CarModelSlot);
    UploadCarImage(g_CarModelSlot);
}
