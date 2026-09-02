#include "common.h"
#include "game/asset.h"
#include "game/car.h"

#include <stdio.h>
#include <string.h>

u8 *g_AssetBase;
u8 *g_AssetLoadCursor;
CarModelAsset *g_CarModelAsset;

static CarModelAsset *s_serializedAsset;
static CarModelAsset s_nativeAsset;
static CarModelAsset *s_installedAsset;
static s32 s_installedSlot;
static s32 s_selectedSlot;
static ModelBankHeader *s_registeredBank;
static s32 s_registeredSlot;
static s32 s_failures;

CarModelAsset *FindSerializedCarModelAsset(CarModelAsset *nativeAsset) {
    (void)nativeAsset;
    return s_serializedAsset;
}
s32 InstallSerializedCarModelSlot(CarModelAsset *asset, s32 slot) {
    s_installedAsset = asset;
    s_installedSlot = slot;
    return 1;
}
void SelectCarModelSlot(s32 slot) {
    s_selectedSlot = slot;
    g_CarModelAsset = &s_nativeAsset;
}
void RegisterModelBank(ModelBankHeader *bank, s32 slot) {
    s_registeredBank = bank;
    s_registeredSlot = slot;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

int main(void) {
    enum { MODEL_DATA_SIZE = 16, TOTAL_SIZE =
        SERIALIZED_CAR_MODEL_HEADER_SIZE + MODEL_DATA_SIZE };
    union {
        max_align_t alignment;
        u8 bytes[TOTAL_SIZE];
    } source;
    union {
        max_align_t alignment;
        u8 bytes[TOTAL_SIZE + 16];
    } destination;
    s32 i;

    for (i = 0; i < TOTAL_SIZE; i++) source.bytes[i] = (u8)(i + 1);
    memset(destination.bytes, 0xCC, sizeof(destination.bytes));
    s_serializedAsset = (CarModelAsset *)(void *)source.bytes;
    s_serializedAsset->serializedModelSize = MODEL_DATA_SIZE;
    g_CarModelAsset = &s_nativeAsset;
    g_AssetBase = destination.bytes;

    RelocateCarModel();

    Check(memcmp(destination.bytes, source.bytes, TOTAL_SIZE) == 0,
          "serialized model bytes copied");
    Check(destination.bytes[TOTAL_SIZE] == 0xCC,
          "copy stops at serialized model size");
    Check(g_AssetLoadCursor == destination.bytes + TOTAL_SIZE,
          "asset cursor follows relocated model");
    Check(s_installedAsset == (CarModelAsset *)(void *)destination.bytes &&
              s_installedSlot == 0 && s_selectedSlot == 0,
          "relocated model occupies slot zero");
    Check(s_nativeAsset.modelData.pointer ==
              destination.bytes + SERIALIZED_CAR_MODEL_HEADER_SIZE,
          "native model points at relocated model bank");
    Check(s_registeredBank ==
              (ModelBankHeader *)(void *)(destination.bytes +
                                          SERIALIZED_CAR_MODEL_HEADER_SIZE) &&
              s_registeredSlot == 0,
          "relocated model bank registered");

    s_serializedAsset = NULL;
    s_installedAsset = NULL;
    g_AssetLoadCursor = NULL;
    RelocateCarModel();
    Check(s_installedAsset == NULL && g_AssetLoadCursor == NULL,
          "unknown native model is not treated as serialized bytes");

    s_serializedAsset = (CarModelAsset *)(void *)source.bytes;
    s_serializedAsset->serializedModelSize = -1;
    RelocateCarModel();
    Check(s_installedAsset == NULL && g_AssetLoadCursor == NULL,
          "negative serialized model size is rejected");

    s_serializedAsset->serializedModelSize = CAR_MODEL_SLOT_SIZE;
    RelocateCarModel();
    Check(s_installedAsset == NULL && g_AssetLoadCursor == NULL,
          "serialized model larger than its slot is rejected");

    if (s_failures != 0) return 1;
    puts("car model relocation copies its named serialized payload");
    return 0;
}
