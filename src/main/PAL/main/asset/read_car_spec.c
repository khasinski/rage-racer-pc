#include "game/car_asset.h"
#include <stdint.h>
#include <string.h>

/* Width and count are wire schema, never inferred from native padding. */
#define SPEC_FIELDS(X) \
    X(torqueCurve, 4, 16) X(torqueBand.values, 4, 16) \
    X(torqueLossValue, 4, 10) X(torqueLossRpm, 4, 9) \
    X(gearLoad, 4, 6) X(gearRatio, 4, 7) \
    X(revLimit, 2, 1) X(automaticAccelerationScale, 2, 1) \
    X(topGear, 2, 1) X(redline, 2, 1) X(steeringGripResponse, 2, 1) \
    X(steerResponse, 2, 1) X(referenceTurnRadius, 2, 1) \
    X(negconSteeringAssistScale, 2, 1) X(speedDragDivisor, 2, 1) \
    X(baseSteeringGrip, 2, 1) X(torqueScale, 2, 6) \
    X(shiftPoints, 2, 12) \
    X(tachometer.needleX, 2, 1) X(tachometer.needleY, 2, 1) \
    X(tachometer.faceDX, 2, 1) X(tachometer.faceDY, 2, 1) \
    X(tachometer.digitsX, 2, 1) X(tachometer.digitsY, 2, 1) \
    X(tachometer.gearDigitDX, 2, 1) X(tachometer.gearDigitDY, 2, 1) \
    X(tachometer.shiftLightDX, 2, 1) X(tachometer.shiftLightDY, 2, 1) \
    X(tachometer.needleQuad, 1, 4) X(tachometer.angleMin, 2, 1) \
    X(tachometer.angleMax, 2, 1) X(tachometer.needleColor, 1, 4) \
    X(tachometer.needleColorAlt, 1, 4) X(tachometer.speedScale, 4, 1)
#define FIELD_SIZE(field, width, count) + width * count
_Static_assert(0 SPEC_FIELDS(FIELD_SIZE) == CAR_SPEC_WIRE_SIZE, "car specification wire size");
_Static_assert(sizeof(GameCarSpecShiftPoint) == 4, "shift pair layout");
#undef FIELD_SIZE

int EncodeCarSpec(const GameCarSpec *spec, u8 *bytes, size_t size) {
    if (!spec || !bytes || size != CAR_SPEC_WIRE_SIZE) return 0;
    u8 encoded[CAR_SPEC_WIRE_SIZE];
    size_t offset = 0;
#define WRITE_FIELD(field, width, count) \
    for (size_t i = 0; i < count; ++i) { \
        const u8 *source = (const u8 *)&spec->field + i * width; \
        u32 value; \
        if (width == 4) memcpy(&value, source, 4); \
        else if (width == 2) { u16 half; memcpy(&half, source, 2); value = half; } \
        else value = *source; \
        for (size_t byte = 0; byte < width; ++byte) encoded[offset++] = (u8)(value >> (byte * 8)); \
    }
    SPEC_FIELDS(WRITE_FIELD)
#undef WRITE_FIELD
    memcpy(bytes, encoded, sizeof(encoded));
    return 1;
}

int DecodeCarSpec(const u8 *bytes, size_t size, GameCarSpec *spec) {
    if (!spec || !bytes || size != CAR_SPEC_WIRE_SIZE) return 0;
    GameCarSpec decoded = {0};
    size_t offset = 0;
#define READ_FIELD(field, width, count) \
    for (size_t i = 0; i < count; ++i) { \
        u32 value = 0; \
        for (size_t byte = 0; byte < width; ++byte) value |= (u32)bytes[offset++] << (byte * 8); \
        u8 *target = (u8 *)&decoded.field + i * width; \
        if (width == 4) memcpy(target, &value, 4); \
        else if (width == 2) { u16 half = (u16)value; memcpy(target, &half, 2); } \
        else *target = (u8)value; \
    }
    SPEC_FIELDS(READ_FIELD)
#undef READ_FIELD
    *spec = decoded;
    return 1;
}
#undef SPEC_FIELDS

int ReadCarTransmission(const void *data, size_t size, int *automatic) {
    const u8 *bytes = data;
    if (!bytes || !automatic || size < 9 || bytes[8] > 1) return 0;
    *automatic = bytes[8];
    return 1;
}

int ReadCarShape(const void *data, size_t size, CarShape *shape) {
    if (data == NULL || shape == NULL || size < sizeof(*shape)) return 0;
    memcpy(shape, data, sizeof(*shape));
    return 1;
}

int ReadCarSpec(const void *data, size_t size, GameCarSpec *spec) {
    RaceCarAssetHeader header;
    if (data == NULL || spec == NULL || size < sizeof(header) || size > INT32_MAX)
        return 0;
    memcpy(&header, data, sizeof(header));
    if (header.specificationOffset < (s32)sizeof(header) ||
        header.audioHeaderOffset <= header.specificationOffset ||
        header.audioHeaderOffset - header.specificationOffset < (s32)sizeof(*spec) ||
        header.audioSequenceOffset <= header.audioHeaderOffset ||
        header.audioBodyOffset <= header.audioSequenceOffset ||
        header.imageOffset <= header.audioBodyOffset ||
        (size_t)header.imageOffset >= size)
        return 0;
    memcpy(spec, (const u8 *)data + header.specificationOffset, sizeof(*spec));
    return 1;
}
