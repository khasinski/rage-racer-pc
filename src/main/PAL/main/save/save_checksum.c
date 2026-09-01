#include "game/save_internal.h"

static u32 CalculateLittleEndianHalfwordChecksum(
    const u8 *bytes,
    s32 checksumOffset) {
    u32 sum = 0;
    s32 offset;

    for (offset = 0; offset < checksumOffset; offset += 2) {
        sum += (u32)bytes[offset] | ((u32)bytes[offset + 1] << 8);
    }
    return ~sum;
}

u32 CalculateSaveBlockChecksum(const GameSaveBlock *block) {
    return CalculateLittleEndianHalfwordChecksum(
        (const u8 *)block, MC_BLOCK_CHECKSUM_OFS);
}

u32 CalculateSaveHeaderChecksum(const GameSaveHeaderRow *header) {
    return CalculateLittleEndianHalfwordChecksum(
        header->bytes, MC_HEADER_CHECKSUM_OFS);
}
