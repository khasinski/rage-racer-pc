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
static size_t s_installedSize;
static s32 s_installedSlot;
static s32 s_selectedSlot;
static ModelBankHeader *s_registeredBank;
static s32 s_registeredSlot;
static s32 s_registerResult = 1;
static size_t s_destinationRoom;
static s32 s_failures;

CarModelAsset *FindSerializedCarModelAsset(CarModelAsset *nativeAsset) {
    (void)nativeAsset;
    return s_serializedAsset;
}
s32 IsValidSerializedCarModelAsset(const CarModelAsset *asset, size_t size) {
    return asset != NULL && asset->serializedModelSize >= 0 &&
           size >= SERIALIZED_CAR_MODEL_HEADER_SIZE &&
           asset->serializedModelSize <=
               CAR_MODEL_SLOT_SIZE - SERIALIZED_CAR_MODEL_HEADER_SIZE -
                   (s32)sizeof(CarImageData);
}
s32 IsValidModelBankAsset(const ModelBankHeader *bank, size_t size) {
    (void)bank;
    (void)size;
    return 1;
}
s32 InstallSerializedCarModelSlot(CarModelAsset *asset, size_t size,
                                  s32 slot) {
    s_installedAsset = asset;
    s_installedSize = size;
    s_installedSlot = slot;
    s_nativeAsset.modelData.pointer =
        (u8 *)asset + SERIALIZED_CAR_MODEL_HEADER_SIZE;
    return 1;
}
void SelectCarModelSlot(s32 slot) {
    s_selectedSlot = slot;
    g_CarModelAsset = &s_nativeAsset;
}
s32 RegisterModelBank(ModelBankHeader *bank, size_t size, s32 slot) {
    (void)size;
    s_registeredBank = bank;
    s_registeredSlot = slot;
    return s_registerResult;
}
size_t PortAssetRoomAt(const void *at) {
    return at == g_AssetBase ? s_destinationRoom : 0;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

int main(void) {
    enum {
        MODEL_DATA_SIZE = 16,
        TOTAL_SIZE = SERIALIZED_CAR_MODEL_HEADER_SIZE + MODEL_DATA_SIZE +
                     sizeof(CarImageData),
    };
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
    s_destinationRoom = sizeof(destination.bytes);

    Check(RelocateCarModel() == 1, "valid serialized model is relocated");

    Check(memcmp(destination.bytes, source.bytes, TOTAL_SIZE) == 0,
          "serialized model bytes copied");
    Check(destination.bytes[TOTAL_SIZE] == 0xCC,
          "copy stops at serialized model size");
    Check(g_AssetLoadCursor == destination.bytes + TOTAL_SIZE,
          "asset cursor follows relocated model");
    Check(s_installedAsset == (CarModelAsset *)(void *)destination.bytes &&
              s_installedSize == TOTAL_SIZE && s_installedSlot == 0 &&
              s_selectedSlot == 0,
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
    Check(RelocateCarModel() == 0, "unknown native model is rejected");
    Check(s_installedAsset == NULL && g_AssetLoadCursor == NULL,
          "unknown native model is not treated as serialized bytes");

    s_serializedAsset = (CarModelAsset *)(void *)source.bytes;
    s_serializedAsset->serializedModelSize = -1;
    Check(RelocateCarModel() == 0, "negative model size reports failure");
    Check(s_installedAsset == NULL && g_AssetLoadCursor == NULL,
          "negative serialized model size is rejected");

    s_serializedAsset->serializedModelSize = CAR_MODEL_SLOT_SIZE;
    Check(RelocateCarModel() == 0, "oversized model reports failure");
    Check(s_installedAsset == NULL && g_AssetLoadCursor == NULL,
          "serialized model larger than its slot is rejected");

    s_serializedAsset->serializedModelSize = MODEL_DATA_SIZE;
    s_registerResult = 0;
    s_installedAsset = NULL;
    g_AssetLoadCursor = NULL;
    Check(RelocateCarModel() == 0, "model-bank failure is reported");
    Check(s_installedAsset == NULL && g_AssetLoadCursor == NULL,
          "unregistrable relocated bank is not published");

    s_registerResult = 1;
    s_destinationRoom = TOTAL_SIZE - 1;
    memset(destination.bytes, 0xCC, sizeof(destination.bytes));
    Check(RelocateCarModel() == 0,
          "model exceeding destination storage is rejected");
    Check(destination.bytes[0] == 0xCC && g_AssetLoadCursor == NULL,
          "undersized destination remains untouched");

    if (s_failures != 0) return 1;
    puts("car model relocation copies its named serialized payload");
    return 0;
}
