#include "game/car_asset.h"
#include <stdint.h>
#include <string.h>

int ReadCarShape(const void *data, size_t size, CarShape *shape) {
    if (data == NULL || shape == NULL || size < sizeof(*shape)) return 0;
    memcpy(shape, data, sizeof(*shape));
    return 1;
}

int ReadCarSpec(const void *data, size_t size, GameCarSpec *spec) {
    RaceCarAssetHeader header;
    if (data == NULL || spec == NULL || size < sizeof(header) || size > INT32_MAX)
        return 0;
    memcpy(&header, data, sizeof(header));
    if (header.specificationOffset < (s32)sizeof(header) ||
        header.audioHeaderOffset <= header.specificationOffset ||
        header.audioHeaderOffset - header.specificationOffset < (s32)sizeof(*spec) ||
        header.audioSequenceOffset <= header.audioHeaderOffset ||
        header.audioBodyOffset <= header.audioSequenceOffset ||
        header.imageOffset <= header.audioBodyOffset ||
        (size_t)header.imageOffset >= size)
        return 0;
    memcpy(spec, (const u8 *)data + header.specificationOffset, sizeof(*spec));
    return 1;
}
