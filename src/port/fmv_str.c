#include "fmv_str.h"

#include <stdint.h>
#include <string.h>

enum {
    STR_HEADER_OFFSET = 24,
    STR_PAYLOAD_OFFSET = 32,
};

static const unsigned int STR_MAGIC = 0x80010160u;

static unsigned int ReadLe16(const unsigned char *data) {
    return (unsigned int)data[0] | ((unsigned int)data[1] << 8);
}

static unsigned int ReadLe32(const unsigned char *data) {
    return ReadLe16(data) | (ReadLe16(data + 2) << 16);
}

int HostFmvAssembleStrFrame(const unsigned char *sectors, size_t sectorCount,
                            size_t *sectorCursor, unsigned char *bitstream,
                            size_t bitstreamCapacity,
                            HostFmvStrFrame *frame) {
    size_t size = 0;
    unsigned int chunkCount = 0;
    unsigned int chunksSeen = 0;
    unsigned int width = 0;
    unsigned int height = 0;

    if (frame == NULL) return 0;
    frame->bitstreamSize = 0;
    frame->width = 0;
    frame->height = 0;
    if (sectors == NULL || sectorCursor == NULL || bitstream == NULL ||
        sectorCount > SIZE_MAX / HOST_FMV_SECTOR_SIZE) {
        return 0;
    }

    while (*sectorCursor < sectorCount) {
        const unsigned char *body =
            sectors + *sectorCursor * HOST_FMV_SECTOR_SIZE + STR_HEADER_OFFSET;
        unsigned int chunk;
        unsigned int declaredChunkCount;

        (*sectorCursor)++;
        if (ReadLe32(body) != STR_MAGIC) {
            continue;
        }

        chunk = ReadLe16(body + 4);
        declaredChunkCount = ReadLe16(body + 6);
        if (size == 0 && chunk != 0) {
            continue;
        }
        if (chunk == 0) {
            size = 0;
            chunksSeen = 0;
            chunkCount = declaredChunkCount;
            width = ReadLe16(body + 16);
            height = ReadLe16(body + 18);
        } else if (chunk != chunksSeen || declaredChunkCount != chunkCount) {
            size = 0;
            chunksSeen = 0;
            chunkCount = 0;
            continue;
        }
        if (size > bitstreamCapacity ||
            HOST_FMV_PAYLOAD_SIZE > bitstreamCapacity - size) {
            return 0;
        }

        memcpy(bitstream + size, body + STR_PAYLOAD_OFFSET,
               HOST_FMV_PAYLOAD_SIZE);
        size += HOST_FMV_PAYLOAD_SIZE;
        chunksSeen++;
        if (chunkCount != 0 && chunksSeen >= chunkCount) {
            size_t declaredSize = 8 + (size_t)ReadLe16(bitstream) * 4;

            /* The BS word count is an upper bound consumed by the VLC
             * decoder, not necessarily the amount stored in the STR chunks.
             * Retail frames can reach their end marker before that bound. */
            if (declaredSize > bitstreamCapacity) {
                return 0;
            }
            if (declaredSize > size) {
                memset(bitstream + size, 0, declaredSize - size);
            }
            frame->bitstreamSize = declaredSize > size ? declaredSize : size;
            frame->width = width;
            frame->height = height;
            return 1;
        }
    }

    return 0;
}
