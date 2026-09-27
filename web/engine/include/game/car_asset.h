#ifndef GAME_CAR_ASSET_H
#define GAME_CAR_ASSET_H
#include "game/car.h"
#include <stddef.h>

#define SERIALIZED_CAR_MODEL_HEADER_SIZE 0x28

typedef struct SerializedCarModelAssetHeader {
    u8 metadata[0x20];
    s32 modelOffset;
    s32 imageOffset;
} SerializedCarModelAssetHeader;

typedef struct RaceCarAssetHeader {
    s32 specificationOffset;
    s32 audioHeaderOffset;
    s32 audioSequenceOffset;
    s32 audioBodyOffset;
    s32 imageOffset;
} RaceCarAssetHeader;

/* Model placement metadata, independent of loaded geometry/textures. */
typedef struct CarShape {
    s16 offsetX, offsetY, offsetZ, horizon;
} CarShape;

/* Reads only the placement prefix, not the model stream or image payload. */
int ReadCarShape(const void *data, size_t size, CarShape *shape);
/* Retail model metadata: whether this variant offers automatic transmission.
 * Invalid/truncated metadata preserves the output. */
int ReadCarTransmission(const void *data, size_t size, int *automatic);

/* Copy retail values before performance preparation or catalog overrides.
 * Failure leaves the destination unchanged. Audio/image contents are not parsed. */
int ReadCarSpec(const void *data, size_t size, GameCarSpec *spec);
/* Complete specification, before performance preparation. Explicit little-endian
 * fields, no native padding. Packet versioning belongs to the caller. These
 * codecs preserve numeric values; the race setup still validates physics. */
enum { CAR_SPEC_WIRE_SIZE = 352 };
int EncodeCarSpec(const GameCarSpec *spec, u8 *bytes, size_t size);
int DecodeCarSpec(const u8 *bytes, size_t size, GameCarSpec *spec);
#endif
