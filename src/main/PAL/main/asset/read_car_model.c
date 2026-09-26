#include "game/car_model_data.h"
#include <stdlib.h>
#include "game/image_asset.h"
#include <string.h>

_Static_assert(offsetof(CarModelData, bytes) % _Alignof(ModelBankHeader) == 0,
               "owned model storage must be aligned");

CarModelData *CopyCarModelData(const void *data, size_t size) {
    SerializedCarModelAssetHeader header;
    s32 modelSize;
    if (data == NULL || size < sizeof(header) || size > INT32_MAX ||
        size > SIZE_MAX - sizeof(CarModelData)) return NULL;
    memcpy(&header, data, sizeof(header));
    memcpy(&modelSize, header.metadata + 0x18, sizeof(modelSize));
    if (modelSize < 0 || header.modelOffset != sizeof(header) ||
        header.imageOffset < 0 ||
        header.imageOffset % _Alignof(CarImageData) != 0 ||
        (size_t)header.imageOffset != sizeof(header) + (size_t)modelSize ||
        (size_t)header.imageOffset > size ||
        sizeof(CarImageData) > size - (size_t)header.imageOffset) return NULL;

    CarModelData *model = calloc(1, sizeof(*model) + size);
    if (model == NULL) return NULL;
    memcpy(model->bytes, data, size);
    if (!ReadCarShape(model->bytes, size, &model->shape) ||
        !ReadModelBank((const ModelBankHeader *)(const void *)(model->bytes + header.modelOffset),
                       (size_t)modelSize, &model->bank)) {
        free(model);
        return NULL;
    }
    model->size = size;
    model->image = (const CarImageData *)(const void *)(model->bytes + header.imageOffset);
    return model;
}

void FreeCarModelData(CarModelData *model) { free(model); }

int ReadCarModelImages(CarModelData *model, const void *data, size_t size) {
    if (!model || !IsValidImageAsset(data, size)) return 0;
    const ImagePixels shared = {model->sharedImage, 64 * 256, 640, 0, 64, 256};
    const ImagePixels palette = {model->logoPalette, 16, 16, 480, 16, 1};
    memset(model->sharedImage, 0, sizeof(model->sharedImage));
    memset(model->logoPalette, 0, sizeof(model->logoPalette));
    CopyImageAssetPixels(data, size, &shared);
    CopyImageAssetPixels(data, size, &palette);
    model->hasSharedImage = 1;
    return 1;
}
