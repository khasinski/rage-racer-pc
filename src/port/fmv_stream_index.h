#ifndef RAGE_FMV_STREAM_INDEX_H
#define RAGE_FMV_STREAM_INDEX_H

#include <stddef.h>

#include "game/asset.h"

int HostFmvStreamIndex(const GameCdLoadEntry *entries, size_t entryCount,
                       const GameCdLoadEntry *selected);

#endif
