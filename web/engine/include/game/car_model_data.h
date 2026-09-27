#ifndef GAME_CAR_MODEL_DATA_H
#define GAME_CAR_MODEL_DATA_H
#include "game/car_asset.h"
#include "game/model_bank.h"

/* Owned model/image source, immutable after preparation. Keep it alive while
 * its bank or images are borrowed. Never copy by struct assignment. */
typedef struct CarModelData {
    CarShape shape;
    NativeModelBank bank;
    const CarImageData *image;
    size_t size;
    u16 sharedImage[64 * 256];
    u16 logoPalette[16];
    int hasSharedImage;
    u8 bytes[];
} CarModelData;

/* Copies the first car pack only; shared artwork is prepared separately. */
CarModelData *CopyCarModelData(const void *data, size_t size);
/* Validated shared boot artwork, copied independently of archive/VRAM. */
int ReadCarModelImages(CarModelData *model, const void *data, size_t size);
void FreeCarModelData(CarModelData *model);
#endif
