#include "archive_index.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void WriteLe32(unsigned char *data, uint32_t value) {
    data[0] = (unsigned char)value;
    data[1] = (unsigned char)(value >> 8);
    data[2] = (unsigned char)(value >> 16);
    data[3] = (unsigned char)(value >> 24);
}

static int TestDecode(void) {
    unsigned char data[16] = {0};
    RageArchiveIndexEntry entries[2] = {{0}};

    WriteLe32(data, 3);
    WriteLe32(data + 4, 127);
    WriteLe32(data + 8, 9);
    WriteLe32(data + 12, 2048);
    CHECK(RageArchiveDecodeIndex(data, sizeof(data), entries, 2));
    CHECK(entries[0].byteOffset == 3 * 2048 && entries[0].size == 127);
    CHECK(entries[1].byteOffset == 9 * 2048 && entries[1].size == 2048);
    return 0;
}

static int TestInvalidIndexIsAtomic(void) {
    unsigned char data[16] = {0};
    RageArchiveIndexEntry entries[2] = {{11, 12}, {13, 14}};
    RageArchiveIndexEntry original[2];

    memcpy(original, entries, sizeof(entries));
    WriteLe32(data, 3);
    WriteLe32(data + 4, 127);
    WriteLe32(data + 8, UINT32_MAX / 2048 + 1);
    WriteLe32(data + 12, 1);
    CHECK(!RageArchiveDecodeIndex(data, sizeof(data), entries, 2));
    CHECK(memcmp(entries, original, sizeof(entries)) == 0);

    WriteLe32(data + 8, UINT32_MAX / 2048);
    WriteLe32(data + 12, 2049);
    CHECK(!RageArchiveDecodeIndex(data, sizeof(data), entries, 2));
    CHECK(memcmp(entries, original, sizeof(entries)) == 0);
    return 0;
}

static int TestArguments(void) {
    unsigned char data[8] = {0};
    RageArchiveIndexEntry entry;

    CHECK(!RageArchiveDecodeIndex(NULL, sizeof(data), &entry, 1));
    CHECK(!RageArchiveDecodeIndex(data, sizeof(data), NULL, 1));
    CHECK(!RageArchiveDecodeIndex(data, sizeof(data), &entry, 0));
    CHECK(!RageArchiveDecodeIndex(data, sizeof(data) - 1, &entry, 1));
    CHECK(!RageArchiveDecodeIndex(
        data, sizeof(data), &entry, RAGE_ARCHIVE_INDEX_ENTRY_COUNT + 1));
    return 0;
}

int main(void) {
    CHECK(TestDecode() == 0);
    CHECK(TestInvalidIndexIsAtomic() == 0);
    CHECK(TestArguments() == 0);
    puts("RAGE.BIN indexes decode atomically with checked byte ranges");
    return 0;
}
