/*
 * Handing a disc's movie table to the game.
 *
 * This is the only part of the stream-table work that needs the game's own
 * headers, and it is on its own so that disc_stream_table.c does not: that
 * module is driven by a raw-sector callback and is tested without a disc, a
 * game or anything else. Keeping the game headers out of the platform layer
 * also keeps them away from <windows.h>, which names a RECT and a LoadImage
 * of its own.
 */

#include "game/asset.h"

#include "disc_stream_table.h"

void DiscStreamTablePublish(const DiscStreamTable *table) {
    int stream;
    for (stream = 0; stream < RAGE_DISC_STREAM_COUNT; stream++) {
        g_StreamCdEntries[stream].position.sectorOffset = table->offset[stream];
        g_StreamCdEntries[stream].size = table->frames[stream];
    }
}
