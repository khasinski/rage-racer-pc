#ifndef TRACK_PACK_H
#define TRACK_PACK_H

#include <stddef.h>

enum { TRACK_PACK_BLOCK_COUNT = 11 };
typedef struct TrackBlock {
    const unsigned char *data;
    size_t size;
} TrackBlock;

/* Borrowed spans into original bytes. Failure leaves output untouched.
 * This checks the container; each block still requires its typed validator. */
int TrackPackOpen(const void *data, size_t size,
                  TrackBlock blocks[TRACK_PACK_BLOCK_COUNT]);
const char *TrackBlockName(unsigned index);

#endif
