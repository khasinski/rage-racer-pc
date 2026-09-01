#include <stdlib.h>
#include <string.h>

#include "game/save_internal.h"

#define CHECK(condition) do { if (!(condition)) abort(); } while (0)

int main(void) {
    GameSaveBlock block;

    memset(&block, 0, sizeof(block));
    CHECK(CalculateSaveBlockChecksum(&block) == UINT32_MAX);

    block.padMappingIndex = 0x1234;
    block.negconMappingIndex = 0x5678;
    CHECK(CalculateSaveBlockChecksum(&block) == ~(u32)0x68AC);

    block.reserved[sizeof(block.reserved) - 1] = 0xA5;
    CHECK(CalculateSaveBlockChecksum(&block) == ~(u32)0x10DAC);

    block.checksum = 0xDEADBEEF;
    CHECK(CalculateSaveBlockChecksum(&block) == ~(u32)0x10DAC);
    return EXIT_SUCCESS;
}
