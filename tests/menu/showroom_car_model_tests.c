#include "common.h"
#include "game/asset.h"
#include "game/menu.h"

#include <stdio.h>

u32 g_CarModelSlot;
CarModelAsset *g_CarModelSlots[CAR_ASSET_SLOT_COUNT];
static CarModelAsset s_model;

static s32 s_selectedModelSlot;
static s32 s_selectedBank;
static s32 s_uploadedImageSlot;

void SelectCarModelSlot(s32 slot) { s_selectedModelSlot = slot; }
void SelectModelBank(s32 bank) { s_selectedBank = bank; }
void UploadCarImage(s32 slot) { s_uploadedImageSlot = slot; }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_CarModelSlot = 0;
    g_CarModelSlots[1] = &s_model;
    s_selectedModelSlot = -1;
    s_selectedBank = -1;
    s_uploadedImageSlot = -1;

    CHECK(ActivateShowroomCarModel(1) == 1);

    CHECK(s_selectedModelSlot == 1);
    CHECK(s_selectedBank == 1);
    CHECK(s_uploadedImageSlot == 1);
    CHECK(g_CarModelSlot == 1);

    CHECK(ActivateShowroomCarModel(CAR_ASSET_SLOT_COUNT) == 0);
    CHECK(g_CarModelSlot == 1);
    CHECK(s_selectedModelSlot == 1);
    CHECK(s_selectedBank == 1);
    CHECK(s_uploadedImageSlot == 1);

    CHECK(ActivateShowroomCarModel(0) == 0);
    CHECK(g_CarModelSlot == 1);
    CHECK(s_selectedModelSlot == 1);
    puts("showroom car model tests passed");
    return 0;
}
