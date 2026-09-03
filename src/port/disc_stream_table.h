#ifndef RAGE_DISC_STREAM_TABLE_H
#define RAGE_DISC_STREAM_TABLE_H

#include "disc_iso.h"

/* What a mounted disc says about itself.
 *
 * The port finds RAGE.BIN and RAGE.STR through the ISO directory, so it mounts
 * a disc from any region.  The eleven movies do not sit in the same places on
 * all of them, and the American release encodes them at twice the frame rate,
 * so the PAL stream table compiled into the port cuts every NTSC movie short
 * and starts ten of the eleven in the wrong sector.  Read the table off the
 * disc that is actually mounted instead. */

#define RAGE_DISC_STREAM_COUNT 11

typedef struct DiscStreamTable {
    unsigned int offset[RAGE_DISC_STREAM_COUNT]; /* sectors into RAGE.STR */
    unsigned int frames[RAGE_DISC_STREAM_COUNT]; /* last frame the game shows */
    unsigned int span[RAGE_DISC_STREAM_COUNT];   /* sectors the movie occupies */
} DiscStreamTable;

typedef struct DiscIdentity {
    char boot[16];        /* "SCES_006.50", from SYSTEM.CNF */
    const char *region;   /* "PAL", "NTSC-U", "NTSC-J" or "unknown" */
    DiscStreamTable table;
    int tableValid;
    const char *reason;   /* why the table could not be read, when it could not */
} DiscIdentity;

/* The table transcribed from retail main.exe @ 0x8007C6A8, which is the PAL
 * disc's own answer to what this module reads off whichever disc is mounted.
 * It is what g_StreamCdEntries starts out holding and what it keeps when a
 * disc cannot be read. */
extern const DiscStreamTable g_RetailPalStreamTable;

/* Fills identity from the disc behind the reader.  Returns 1 when the disc was
 * identified at all; identity->tableValid says whether the stream table came
 * with it. */
int DiscIdentify(DiscRawSectorReader read, void *context,
                 DiscIdentity *identity);

/* The region a boot executable name belongs to.  Exposed for the tests. */
const char *DiscRegionForBootName(const char *boot);

/*
 * Publish a table into the game's own stream index. The platform layer used
 * to write it directly, which meant including the game's asset header beside
 * <windows.h>; those two both define a RECT and the build came apart inside
 * the headers. Whoever knows the table writes it.
 */
void DiscStreamTablePublish(const DiscStreamTable *table);

#endif
