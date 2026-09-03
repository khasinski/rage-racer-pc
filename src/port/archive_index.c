#include "archive_index.h"

#include <string.h>

static uint32_t ReadLe32(const unsigned char *data) {
    return (uint32_t)data[0] | (uint32_t)data[1] << 8 |
           (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

int RageArchiveDecodeIndex(const void *data, size_t dataSize,
                           RageArchiveIndexEntry *entries, size_t entryCount) {
    RageArchiveIndexEntry decoded[RAGE_ARCHIVE_INDEX_ENTRY_COUNT];
    const unsigned char *bytes = data;
    size_t index;

    if (data == NULL || entries == NULL || entryCount == 0 ||
        entryCount > RAGE_ARCHIVE_INDEX_ENTRY_COUNT ||
        dataSize < entryCount * RAGE_ARCHIVE_INDEX_ENTRY_SIZE) {
        return 0;
    }
    for (index = 0; index < entryCount; index++) {
        uint32_t sector = ReadLe32(bytes + index * 8);
        uint32_t size = ReadLe32(bytes + index * 8 + 4);
        uint32_t byteOffset;

        if (sector > UINT32_MAX / RAGE_ARCHIVE_SECTOR_SIZE) return 0;
        byteOffset = sector * RAGE_ARCHIVE_SECTOR_SIZE;
        if (size > UINT32_MAX - byteOffset) return 0;
        decoded[index].byteOffset = byteOffset;
        decoded[index].size = size;
    }
    memcpy(entries, decoded, entryCount * sizeof(decoded[0]));
    return 1;
}
