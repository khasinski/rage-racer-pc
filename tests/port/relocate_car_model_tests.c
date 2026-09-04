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
static const void *s_installedModelData;
static CarImageData *s_installedImageData;
static s32 s_selectedSlot;
static const ModelBankHeader *s_registeredBank;
static s32 s_registeredSlot;
static s32 s_registerResult = 1;
static size_t s_destinationRoom;
static s32 s_failures;

const CarModelAsset *FindSerializedCarModelAsset(
    const CarModelAsset *nativeAsset) {
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
s32 PublishCarModelSlot(const CarModelAsset *serializedAsset,
                        const void *metadata, void *modelData,
                        CarImageData *imageData, s32 slot) {
    (void)metadata;
    s_installedAsset = (CarModelAsset *)(void *)serializedAsset;
    s_installedSlot = slot;
    s_installedModelData = modelData;
    s_installedImageData = imageData;
    s_nativeAsset.modelData.pointer = modelData;
    s_nativeAsset.imageData.carImage = imageData;
    return 1;
}
void SelectCarModelSlot(s32 slot) {
    s_selectedSlot = slot;
    g_CarModelAsset = &s_nativeAsset;
}
s32 RegisterModelBank(const ModelBankHeader *bank, size_t size, s32 slot) {
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
        MODEL_SIZE = SERIALIZED_CAR_MODEL_HEADER_SIZE + MODEL_DATA_SIZE,
        TOTAL_SIZE = MODEL_SIZE + sizeof(CarImageData),
    };
    union {
        max_align_t alignment;
        u8 bytes[TOTAL_SIZE];
    } source;
    union {
        max_align_t alignment;
        u8 bytes[TOTAL_SIZE + 16];
    } destination;
    union {
        max_align_t alignment;
        u8 bytes[TOTAL_SIZE + 8];
    } overlap;
    u8 overlapExpected[TOTAL_SIZE];
    CarModelAsset *overlapSource;
    s32 i;

    for (i = 0; i < TOTAL_SIZE; i++) source.bytes[i] = (u8)(i + 1);
    memset(destination.bytes, 0xCC, sizeof(destination.bytes));
    s_serializedAsset = (CarModelAsset *)(void *)source.bytes;
    s_serializedAsset->serializedModelSize = MODEL_DATA_SIZE;
    GetSerializedCarModelAssetHeader(source.bytes)->modelOffset =
        SERIALIZED_CAR_MODEL_HEADER_SIZE;
    GetSerializedCarModelAssetHeader(source.bytes)->imageOffset = MODEL_SIZE;
    g_CarModelAsset = &s_nativeAsset;
    g_AssetBase = destination.bytes;
    s_destinationRoom = sizeof(destination.bytes);

    Check(RelocateCarModel() == 1, "valid serialized model is relocated");

    Check(memcmp(destination.bytes, source.bytes, MODEL_SIZE) == 0,
          "serialized model bytes copied");
    Check(destination.bytes[MODEL_SIZE] == 0xCC,
          "copy stops at the model; the image stays with the source");
    Check(g_AssetLoadCursor == destination.bytes + MODEL_SIZE,
          "asset cursor follows the relocated model like retail");
    Check(s_installedAsset == (CarModelAsset *)(void *)source.bytes &&
              s_installedSlot == 0 &&
              s_selectedSlot == 0,
          "slot zero keeps the source and its image");
    Check(s_installedModelData ==
              destination.bytes + SERIALIZED_CAR_MODEL_HEADER_SIZE &&
              s_installedImageData ==
                  (CarImageData *)(void *)(source.bytes + MODEL_SIZE),
          "slot publication uses captured model and image addresses");
    Check(s_nativeAsset.modelData.pointer ==
              destination.bytes + SERIALIZED_CAR_MODEL_HEADER_SIZE,
          "native model points at relocated model bank");
    Check(s_registeredBank ==
              (ModelBankHeader *)(void *)(destination.bytes +
                                          SERIALIZED_CAR_MODEL_HEADER_SIZE) &&
              s_registeredSlot == 0,
          "relocated model bank registered");

    for (i = 0; i < (s32)sizeof(overlap.bytes); i++) {
        overlap.bytes[i] = (u8)(i + 17);
    }
    overlapSource = (CarModelAsset *)(void *)(overlap.bytes + 8);
    overlapSource->serializedModelSize = MODEL_DATA_SIZE;
    GetSerializedCarModelAssetHeader(overlapSource)->modelOffset =
        SERIALIZED_CAR_MODEL_HEADER_SIZE;
    GetSerializedCarModelAssetHeader(overlapSource)->imageOffset = MODEL_SIZE;
    memcpy(overlapExpected, overlapSource, sizeof(overlapExpected));
    s_serializedAsset = overlapSource;
    g_AssetBase = overlap.bytes;
    s_destinationRoom = sizeof(overlap.bytes);
    Check(RelocateCarModel() == 1,
          "overlapping serialized model is relocated");
    Check(memcmp(overlap.bytes, overlapExpected, MODEL_SIZE) == 0,
          "overlapping relocation preserves every model byte");
    Check(s_installedModelData ==
              overlap.bytes + SERIALIZED_CAR_MODEL_HEADER_SIZE &&
              s_installedImageData ==
                  (CarImageData *)(void *)(overlap.bytes + 8 + MODEL_SIZE),
          "overlapping relocation does not reread the overwritten header");
    g_AssetBase = destination.bytes;
    s_destinationRoom = sizeof(destination.bytes);

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
    s_destinationRoom = MODEL_SIZE - 1;
    memset(destination.bytes, 0xCC, sizeof(destination.bytes));
    Check(RelocateCarModel() == 0,
          "model exceeding destination storage is rejected");
    Check(destination.bytes[0] == 0xCC && g_AssetLoadCursor == NULL,
          "undersized destination remains untouched");

    if (s_failures != 0) return 1;
    puts("car model relocation copies its named serialized payload");
    return 0;
}
