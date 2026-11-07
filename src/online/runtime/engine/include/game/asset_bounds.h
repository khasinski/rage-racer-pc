#ifndef GAME_ASSET_BOUNDS_H
#define GAME_ASSET_BOUNDS_H
#include <stddef.h>
#include "common.h"

static inline s32 AssetPayloadOffsetIsValid(s32 offset,
                                            size_t payloadOffset,
                                            size_t size) {
    return offset >= 0 && offset % (s32)sizeof(s32) == 0 &&
           (size_t)offset >= payloadOffset && (size_t)offset < size;
}

#endif
