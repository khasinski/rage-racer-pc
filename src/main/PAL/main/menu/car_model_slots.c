#include "game/car.h"
#include "game/asset.h"
#include "game/render.h"

/* Re-registers the showroom car after g_CarModelSlot changes. */
void InstallCarModelSlot(void) {
    SelectCarModelSlot(g_CarModelSlot);
    SelectModelBank(g_CarModelSlot);
    UploadCarImage(g_CarModelSlot);
}
