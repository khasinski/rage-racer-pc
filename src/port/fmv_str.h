#ifndef RAGE_FMV_STR_H
#define RAGE_FMV_STR_H

#include <stddef.h>

enum {
    HOST_FMV_SECTOR_SIZE = 2352,
    HOST_FMV_PAYLOAD_SIZE = 0x7E0,
};

typedef struct HostFmvStrFrame {
    size_t bitstreamSize;
    unsigned int width;
    unsigned int height;
} HostFmvStrFrame;

int HostFmvAssembleStrFrame(const unsigned char *sectors, size_t sectorCount,
                            size_t *sectorCursor, unsigned char *bitstream,
                            size_t bitstreamCapacity,
                            HostFmvStrFrame *frame);

#endif
