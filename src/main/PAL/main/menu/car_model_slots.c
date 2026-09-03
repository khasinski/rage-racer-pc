#include "game/car.h"
#include "game/asset.h"
#include "game/render.h"

s32 ActivateShowroomCarModel(s32 slot) {
    if ((u32)slot >= CAR_ASSET_SLOT_COUNT ||
        g_CarModelSlots[slot] == NULL) {
        return 0;
    }
    SelectCarModelSlot(slot);
    SelectModelBank(slot);
    UploadCarImage(slot);
    g_CarModelSlot = (u32)slot;
    return 1;
}
