#ifndef GAME_MODEL_STREAM_H
#define GAME_MODEL_STREAM_H

#include "common.h"
#include <stddef.h>

static inline s32 ModelPrimitiveStride(s32 primitive) {
    switch (primitive) {
    case 0:
        return 0x10;
    case 1:
    case 2:
        return 0x18;
    case 3:
        return 0x20;
    default:
        return 0;
    }
}

static inline s32 CoursePrimitiveStride(s32 primitive) {
    switch (primitive) {
    case 0:
        return 0x10;
    case 1:
        return 0x1C;
    case 2:
    case 3:
        return 0x20;
    default:
        return 0;
    }
}



static inline u16 ReadAssetU16(const u8 *bytes) {
    return (u16)(bytes[0] | (u16)bytes[1] << 8);
}

static inline s32 PrimitiveStreamIsValid(const u8 *base, size_t size, s32 offset,
                                  s32 (*primitiveStride)(s32),
                                  s32 vertexCount, s32 normalCount) {
    size_t cursor = (size_t)offset;

    while (cursor <= size && size - cursor >= sizeof(u32)) {
        const u16 primitive = ReadAssetU16(&base[cursor]);
        const u16 count = ReadAssetU16(&base[cursor + 2]);
        const s32 stride = primitiveStride(primitive);
        u16 record;

        cursor += sizeof(u32);
        if (count == 0) return 1;
        if (stride == 0 || count > (size - cursor) / (size_t)stride) {
            return 0;
        }
        if (vertexCount >= 0) {
            for (record = 0; record < count; record++) {
                const u8 *face = &base[cursor + (size_t)record * stride];
                s32 corner;

                for (corner = 0; corner < 4; corner++) {
                    if (ReadAssetU16(&face[corner * sizeof(u16)]) >=
                        vertexCount) {
                        return 0;
                    }
                }
            }
        }
if (normalCount >= 0 && primitive >= 2) {
    for (record = 0; record < count; record++) {
        const u8 *face = &base[cursor + (size_t)record * stride];
        for (s32 corner = 0; corner < 4; corner++)
            if (ReadAssetU16(face + 8 + corner * sizeof(u16)) >= normalCount)
                return 0;
    }
}
cursor += (size_t)count * (size_t)stride;

    }
    return 0;
}

static inline s32 TerrainPrimitiveStride(s32 primitive) {
    switch (primitive) {
    case 0:
    case 2:
    case 3:
        return 0x20;
    case 1:
    case 4:
    case 5:
        return 0x24;
    default:
        return 0;
    }
}

#endif
