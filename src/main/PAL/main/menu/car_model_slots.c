#include "game/car.h"
#include "game/asset.h"
#include "game/render.h"

void ActivateShowroomCarModel(void) {
    SelectCarModelSlot(g_CarModelSlot);
    SelectModelBank(g_CarModelSlot);
    UploadCarImage(g_CarModelSlot);
}
