#include "game/car_asset.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    u8 transmission[10] = {0};
    int automatic = -1;
    CHECK(ReadCarTransmission(transmission + 1, 9, &automatic) && automatic == 0);
    transmission[9] = 1;
    CHECK(ReadCarTransmission(transmission + 1, 9, &automatic) && automatic == 1);
    for (size_t size = 0; size < 9; ++size) {
        CHECK(!ReadCarTransmission(transmission + 1, size, &automatic));
        CHECK(automatic == 1);
    }
    for (int flag = 2; flag <= 255; ++flag) {
        transmission[9] = (u8)flag;
        CHECK(!ReadCarTransmission(transmission + 1, 9, &automatic));
        CHECK(automatic == 1);
    }
    CHECK(!ReadCarTransmission(NULL, 9, &automatic));
    CHECK(!ReadCarTransmission(transmission, sizeof(transmission), NULL));
    CarShape placement = {101, -23, 345, -67}, shape, savedShape;
    u8 prefix[sizeof(placement) + 1];
    memcpy(prefix + 1, &placement, sizeof(placement));
    CHECK(ReadCarShape(prefix + 1, sizeof(placement), &shape));
    CHECK(memcmp(&shape, &placement, sizeof(shape)) == 0);
    savedShape = shape;
    for (size_t size = 0; size < sizeof(shape); size++) {
        CHECK(!ReadCarShape(prefix + 1, size, &shape));
        CHECK(memcmp(&shape, &savedShape, sizeof(shape)) == 0);
    }
    memset(prefix, 0, sizeof(prefix));
    CHECK(memcmp(&shape, &placement, sizeof(shape)) == 0);
    CHECK(!ReadCarShape(NULL, sizeof(shape), &shape));
    CHECK(!ReadCarShape(prefix, sizeof(prefix), NULL));
    u8 data[sizeof(RaceCarAssetHeader) + sizeof(GameCarSpec) + 4];
    RaceCarAssetHeader header = {sizeof(header), sizeof(header) + sizeof(GameCarSpec),
        sizeof(header) + sizeof(GameCarSpec) + 1,
        sizeof(header) + sizeof(GameCarSpec) + 2,
        sizeof(header) + sizeof(GameCarSpec) + 3};
    GameCarSpec source, first, second, saved;
    memset(&source, 0x15, sizeof(source));
    memset(data, 0, sizeof(data));
    memcpy(data, &header, sizeof(header));
    memcpy(data + header.specificationOffset, &source, sizeof(source));
    CHECK(ReadCarSpec(data, sizeof(data), &first));
    CHECK(memcmp(&first, &source, sizeof(first)) == 0);
    saved = first;
    for (size_t size = 0; size < sizeof(data); size++) {
        CHECK(!ReadCarSpec(data, size, &first));
        CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    }
    data[header.specificationOffset] ^= 1;
    CHECK(ReadCarSpec(data, sizeof(data), &second));
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    CHECK(memcmp(&first, &second, sizeof(first)) != 0);
    header.specificationOffset = -1;
    memcpy(data, &header, sizeof(header));
    CHECK(!ReadCarSpec(data, sizeof(data), &first));
    CHECK(!ReadCarSpec(NULL, sizeof(data), &first));
    CHECK(!ReadCarSpec(data, sizeof(data), NULL));
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    u8 wire[CAR_SPEC_WIRE_SIZE + 1], again[CAR_SPEC_WIRE_SIZE];
    source = (GameCarSpec){.torqueCurve = {0x12345678, -2},
        .automaticAccelerationScale = 1000, .shiftPoints = {{-123, 456}},
        .tachometer = {.needleX = -3, .needleQuad = {1, 2, 3, 4}, .speedScale = 0x10203040}};
    CHECK(EncodeCarSpec(&source, wire + 1, CAR_SPEC_WIRE_SIZE));
    CHECK(memcmp(wire + 1, (u8[]){0x78, 0x56, 0x34, 0x12, 0xFE, 0xFF, 0xFF, 0xFF}, 8) == 0);
    CHECK(wire[1 + 258] == 0xE8 && wire[1 + 259] == 3);
    CHECK(memcmp(wire + 1 + 288, (u8[]){0x85, 0xFF, 0xC8, 1}, 4) == 0);
    CHECK(memcmp(wire + 1 + 348, (u8[]){0x40, 0x30, 0x20, 0x10}, 4) == 0);
    CHECK(DecodeCarSpec(wire + 1, CAR_SPEC_WIRE_SIZE, &first));
    CHECK(first.torqueCurve[1] == -2 && first.automaticAccelerationScale == 1000);
    CHECK(first.shiftPoints[0].downshiftSpeed == -123 && first.shiftPoints[0].upshiftSpeed == 456);
    CHECK(first.tachometer.needleX == -3 && first.tachometer.needleQuad[3] == 4);
    saved = first;
    for (size_t size = 0; size <= sizeof(wire); ++size) {
        if (size == CAR_SPEC_WIRE_SIZE) continue;
        memset(again, 0xA5, sizeof(again));
        CHECK(!EncodeCarSpec(&source, again, size));
        for (size_t i = 0; i < sizeof(again); ++i) CHECK(again[i] == 0xA5);
        CHECK(!DecodeCarSpec(wire + 1, size, &first));
        CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    }
    CHECK(!EncodeCarSpec(NULL, again, sizeof(again)));
    CHECK(!EncodeCarSpec(&source, NULL, sizeof(again)));
    CHECK(!DecodeCarSpec(NULL, CAR_SPEC_WIRE_SIZE, &first));
    CHECK(!DecodeCarSpec(wire + 1, CAR_SPEC_WIRE_SIZE, NULL));
    // Every wire byte is represented, including signed/unsigned extremes.
    for (size_t i = 0; i < CAR_SPEC_WIRE_SIZE; ++i) wire[i + 1] = (u8)(i * 73 + 19);
    CHECK(DecodeCarSpec(wire + 1, CAR_SPEC_WIRE_SIZE, &first));
    CHECK(EncodeCarSpec(&first, again, sizeof(again)));
    CHECK(memcmp(again, wire + 1, sizeof(again)) == 0);
    return 0;
}
