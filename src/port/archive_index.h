#ifndef RAGE_ARCHIVE_INDEX_H
#define RAGE_ARCHIVE_INDEX_H

#include <stddef.h>
#include <stdint.h>

enum {
    RAGE_ARCHIVE_INDEX_ENTRY_COUNT = 135,
    RAGE_ARCHIVE_INDEX_ENTRY_SIZE = 8,
    RAGE_ARCHIVE_SECTOR_SIZE = 2048,
};

typedef struct RageArchiveIndexEntry {
    uint32_t byteOffset;
    uint32_t size;
} RageArchiveIndexEntry;

/* Converts little-endian (sector, byte-size) pairs from RAGE.BIN. Output is
 * unchanged when any entry is truncated or its byte range cannot be
 * represented by uint32_t. */
int RageArchiveDecodeIndex(const void *data, size_t dataSize,
                           RageArchiveIndexEntry *entries, size_t entryCount);

#endif
