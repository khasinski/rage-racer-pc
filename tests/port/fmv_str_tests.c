#include "fmv_str.h"

#include <stdio.h>
#include <string.h>

enum {
    STR_HEADER_OFFSET = 24,
    STR_PAYLOAD_OFFSET = 32,
};

static void WriteLe16(unsigned char *data, unsigned int value) {
    data[0] = (unsigned char)value;
    data[1] = (unsigned char)(value >> 8);
}

static void WriteLe32(unsigned char *data, unsigned int value) {
    WriteLe16(data, value);
    WriteLe16(data + 2, value >> 16);
}

static void MakeChunk(unsigned char *sector, unsigned int chunk,
                      unsigned int chunkCount, unsigned int width,
                      unsigned int height, unsigned char payload) {
    unsigned char *body = sector + STR_HEADER_OFFSET;

    memset(sector, 0, HOST_FMV_SECTOR_SIZE);
    WriteLe32(body, 0x80010160u);
    WriteLe16(body + 4, chunk);
    WriteLe16(body + 6, chunkCount);
    WriteLe16(body + 16, width);
    WriteLe16(body + 18, height);
    memset(body + STR_PAYLOAD_OFFSET, payload, HOST_FMV_PAYLOAD_SIZE);
    if (chunk == 0) {
        WriteLe16(body + STR_PAYLOAD_OFFSET, 1);
    }
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int TestAssemblyAndResync(void) {
    unsigned char sectors[4][HOST_FMV_SECTOR_SIZE];
    unsigned char bitstream[HOST_FMV_PAYLOAD_SIZE * 2];
    HostFmvStrFrame frame = {0};
    size_t cursor = 0;

    memset(sectors, 0, sizeof(sectors));
    MakeChunk(sectors[1], 1, 2, 320, 192, 0xEE);
    MakeChunk(sectors[2], 0, 2, 320, 192, 0x11);
    MakeChunk(sectors[3], 1, 2, 0, 0, 0x22);

    CHECK(HostFmvAssembleStrFrame(&sectors[0][0], 4, &cursor, bitstream,
                                  sizeof(bitstream), &frame));
    CHECK(cursor == 4);
    CHECK(frame.bitstreamSize == sizeof(bitstream));
    CHECK(frame.width == 320 && frame.height == 192);
    CHECK(bitstream[8] == 0x11);
    CHECK(bitstream[HOST_FMV_PAYLOAD_SIZE] == 0x22);
    return 0;
}

static int TestIncompleteAndOverflow(void) {
    unsigned char sectors[2][HOST_FMV_SECTOR_SIZE];
    unsigned char bitstream[HOST_FMV_PAYLOAD_SIZE * 2];
    HostFmvStrFrame frame = {0};
    size_t cursor = 0;

    MakeChunk(sectors[0], 0, 2, 320, 192, 0x11);
    memset(sectors[1], 0, HOST_FMV_SECTOR_SIZE);
    CHECK(!HostFmvAssembleStrFrame(&sectors[0][0], 2, &cursor, bitstream,
                                   sizeof(bitstream), &frame));
    CHECK(cursor == 2 && frame.bitstreamSize == 0);

    cursor = 0;
    CHECK(!HostFmvAssembleStrFrame(&sectors[0][0], 2, &cursor, bitstream,
                                   HOST_FMV_PAYLOAD_SIZE - 1, &frame));
    CHECK(cursor == 1);
    return 0;
}

static int TestDeclaredBitstreamLength(void) {
    unsigned char sector[HOST_FMV_SECTOR_SIZE];
    unsigned char bitstream[HOST_FMV_PAYLOAD_SIZE + 8];
    HostFmvStrFrame frame = {0};
    size_t cursor = 0;

    MakeChunk(sector, 0, 1, 320, 192, 0x11);
    WriteLe16(sector + STR_HEADER_OFFSET + STR_PAYLOAD_OFFSET,
              HOST_FMV_PAYLOAD_SIZE / 4);
    memset(bitstream, 0xAA, sizeof(bitstream));
    CHECK(HostFmvAssembleStrFrame(sector, 1, &cursor, bitstream,
                                  sizeof(bitstream), &frame));
    CHECK(frame.bitstreamSize == sizeof(bitstream));
    CHECK(bitstream[HOST_FMV_PAYLOAD_SIZE] == 0);
    CHECK(bitstream[sizeof(bitstream) - 1] == 0);

    cursor = 0;
    WriteLe16(sector + STR_HEADER_OFFSET + STR_PAYLOAD_OFFSET, 0xFFFF);
    CHECK(!HostFmvAssembleStrFrame(sector, 1, &cursor, bitstream,
                                   sizeof(bitstream), &frame));
    CHECK(cursor == 1 && frame.bitstreamSize == 0);
    return 0;
}

static int TestInvalidArguments(void) {
    unsigned char sector[HOST_FMV_SECTOR_SIZE] = {0};
    unsigned char bitstream[HOST_FMV_PAYLOAD_SIZE];
    HostFmvStrFrame frame = {0};
    size_t cursor = 0;

    CHECK(!HostFmvAssembleStrFrame(NULL, 1, &cursor, bitstream,
                                   sizeof(bitstream), &frame));
    CHECK(!HostFmvAssembleStrFrame(sector, 1, NULL, bitstream,
                                   sizeof(bitstream), &frame));
    CHECK(!HostFmvAssembleStrFrame(sector, 1, &cursor, NULL,
                                   sizeof(bitstream), &frame));
    CHECK(!HostFmvAssembleStrFrame(sector, 1, &cursor, bitstream,
                                   sizeof(bitstream), NULL));
    return 0;
}

int main(void) {
    CHECK(TestAssemblyAndResync() == 0);
    CHECK(TestIncompleteAndOverflow() == 0);
    CHECK(TestDeclaredBitstreamLength() == 0);
    CHECK(TestInvalidArguments() == 0);
    puts("STR frame assembly resynchronizes and rejects incomplete data");
    return 0;
}
