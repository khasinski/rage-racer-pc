#include "fmv_stream_index.h"

#include <stdint.h>
#include <stdio.h>

enum { TEST_ENTRY_COUNT = 11 };

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    GameCdLoadEntry entries[TEST_ENTRY_COUNT] = {{0}};
    GameCdLoadEntry foreign = {0};
    const unsigned char *bytes = (const unsigned char *)entries;

    CHECK(HostFmvStreamIndex(entries, TEST_ENTRY_COUNT, &entries[0]) == 0);
    CHECK(HostFmvStreamIndex(entries, TEST_ENTRY_COUNT, &entries[7]) == 7);
    CHECK(HostFmvStreamIndex(entries, TEST_ENTRY_COUNT, &entries[10]) == 10);
    CHECK(HostFmvStreamIndex(entries, TEST_ENTRY_COUNT, NULL) == -1);
    CHECK(HostFmvStreamIndex(NULL, TEST_ENTRY_COUNT, &entries[0]) == -1);
    CHECK(HostFmvStreamIndex(entries, 0, &entries[0]) == -1);
    CHECK(HostFmvStreamIndex(entries, SIZE_MAX, &entries[0]) == -1);
    CHECK(HostFmvStreamIndex(entries, TEST_ENTRY_COUNT, &foreign) == -1);
    CHECK(HostFmvStreamIndex(entries, TEST_ENTRY_COUNT,
                             (const GameCdLoadEntry *)(bytes + 1)) == -1);
    CHECK(HostFmvStreamIndex(entries, TEST_ENTRY_COUNT,
                             &entries[TEST_ENTRY_COUNT]) == -1);

    puts("FMV stream indexes accept only entries from the stream table");
    return 0;
}
