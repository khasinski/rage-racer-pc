#include "game/save_internal.h"

u32 CalculateSaveBlockChecksum(const GameSaveBlock *block) {
    const u8 *bytes = (const u8 *)block;
    u32 sum = 0;
    s32 offset;

    for (offset = 0; offset < MC_BLOCK_CHECKSUM_OFS; offset += 2) {
        sum += (u32)bytes[offset] | ((u32)bytes[offset + 1] << 8);
    }
    return ~sum;
}
