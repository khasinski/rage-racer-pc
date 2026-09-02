#include "fmv_stream_index.h"

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

int main(void) {
    /* The first member is itself a struct, so a brace-initialiser needs
     * three levels of them to satisfy gcc. Zeroing says the same thing once. */
    GameCdLoadEntry entries[11];
    GameCdLoadEntry foreign;
    const unsigned char *bytes = (const unsigned char *)entries;

    memset(entries, 0, sizeof(entries));
    memset(&foreign, 0, sizeof(foreign));

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
