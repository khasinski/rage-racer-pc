#include "fmv_stream_index.h"

#include <stdio.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    GameCdLoadEntry entries[11] = {{0}};
    GameCdLoadEntry foreign = {0};
    const unsigned char *bytes = (const unsigned char *)entries;

    CHECK(HostFmvStreamIndex(entries, 11, &entries[0]) == 0);
    CHECK(HostFmvStreamIndex(entries, 11, &entries[7]) == 7);
    CHECK(HostFmvStreamIndex(entries, 11, &entries[10]) == 10);
    CHECK(HostFmvStreamIndex(entries, 11, NULL) == -1);
    CHECK(HostFmvStreamIndex(NULL, 11, &entries[0]) == -1);
    CHECK(HostFmvStreamIndex(entries, 0, &entries[0]) == -1);
    CHECK(HostFmvStreamIndex(entries, 11, &foreign) == -1);
    CHECK(HostFmvStreamIndex(entries, 11,
                             (const GameCdLoadEntry *)(bytes + 1)) == -1);
    CHECK(HostFmvStreamIndex(entries, 11, &entries[11]) == -1);

    puts("FMV stream indexes accept only entries from the stream table");
    return 0;
}
