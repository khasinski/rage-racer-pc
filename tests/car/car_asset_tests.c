#include "game/car_asset.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
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
    return 0;
}
