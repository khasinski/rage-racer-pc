#include "fmv_stream_index.h"

#include <stdint.h>

int HostFmvStreamIndex(const GameCdLoadEntry *entries, size_t entryCount,
                       const GameCdLoadEntry *selected) {
    uintptr_t base = (uintptr_t)entries;
    uintptr_t address = (uintptr_t)selected;
    uintptr_t offset;

    if (entries == NULL || selected == NULL || address < base) {
        return -1;
    }

    offset = address - base;
    if (offset % sizeof(*entries) != 0 ||
        offset / sizeof(*entries) >= entryCount) {
        return -1;
    }

    return (int)(offset / sizeof(*entries));
}
