#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"

enum {
    TORQUE_CURVE_SCALE = 10,
    REV_LIMIT_RESPONSE_NUMERATOR = 4,
    REV_LIMIT_RESPONSE_DENOMINATOR = 5,
    TAKEOFF_LOAD_SHIFT = 12,
    DRIVING_LOAD_SHIFT = 11,
    HIGH_LOAD_THRESHOLD = 100000,
    LOW_LOAD_THRESHOLD = -12500,
    MAX_ENGINE_BRAKING_PERCENT = 100,
    FIRST_GEAR_BRAKING_MULTIPLIER = 2,
    RPM_PER_TORQUE_BAND = 1000,
    STANDING_START_LOAD_DIVISOR = 768,
    HIGH_LOAD_COARSE_SHIFT = 8,
    HIGH_LOAD_RESPONSE_NUMERATOR = 70,
    HIGH_LOAD_RESPONSE_DENOMINATOR = 200,
};

static s32 BandStartIndex(const s16 *bandEnds, s32 bandIndex) {
    s16 previousEnd;

    if (bandIndex == 0) {
        return 0;
    }
    previousEnd = bandEnds[bandIndex - 1];
    return previousEnd == 0 ? 0 : previousEnd - 1;
}

static s32 InterpolateDriveTorque(const GameCarSpec *spec,
                                  const s32 *gearCurve, s32 engineRpm,
                                  s32 bandIndex, s32 fallbackTorque) {
    s32 slot;
    s32 torque = fallbackTorque;

    for (slot = BandStartIndex(g_TorqueBandEnd, bandIndex);
         slot < g_TorqueBandEnd[bandIndex]; slot++) {
        s32 segmentStart = spec->torqueBand.values[slot];
        s32 segmentEnd = spec->torqueBand.values[slot + 1];
        s32 segmentLength;
        s32 nextWeight;
        s32 currentWeight;
        s32 weightedTorque;
        s32 scaledLength;

        if (engineRpm < segmentStart || segmentEnd < engineRpm) {
            continue;
        }
        segmentLength = WrapSigned32((int64_t)segmentEnd - segmentStart);
        if (segmentLength <= 0) {
            segmentLength = 1;
        }
        nextWeight = WrapSigned32(
            (int64_t)WrapSigned32((int64_t)engineRpm - segmentStart) *
            gearCurve[slot + 1]);
        currentWeight = WrapSigned32(
            (int64_t)WrapSigned32((int64_t)segmentEnd - engineRpm) *
            gearCurve[slot]);
        weightedTorque = WrapSigned32((int64_t)nextWeight + currentWeight);
        scaledLength = WrapSigned32(
            (int64_t)segmentLength * TORQUE_CURVE_SCALE);
        torque = weightedTorque / scaledLength;
        break;
    }
    return torque < 0 ? 0 : torque;
}

static s32 InterpolateEngineBraking(const GameCarSpec *spec, s32 engineRpm,
                                    s32 bandIndex, s16 gear) {
    s32 slot;
    s32 braking = 0;

    for (slot = BandStartIndex(g_TorqueLossBandEnd, bandIndex);
         slot < g_TorqueLossBandEnd[bandIndex]; slot++) {
        s32 segmentStart = GetCarTorqueLossBoundary(spec, slot);
        s32 segmentEnd;
        s32 segmentLength;
        s32 nextWeight;
        s32 currentWeight;

        if (engineRpm < segmentStart) {
            continue;
        }
        segmentEnd = GetCarTorqueLossBoundary(spec, slot + 1);
        if (segmentEnd < engineRpm) {
            continue;
        }
        segmentLength = WrapSigned32((int64_t)segmentEnd - segmentStart);
        if (segmentLength <= 0) {
            segmentLength = 1;
        }
        nextWeight = WrapSigned32(
            (int64_t)WrapSigned32((int64_t)engineRpm - segmentStart) *
            spec->torqueLossValue[slot + 1]);
        currentWeight = WrapSigned32(
            (int64_t)WrapSigned32((int64_t)segmentEnd - engineRpm) *
            spec->torqueLossValue[slot]);
        braking = WrapSigned32((int64_t)nextWeight + currentWeight) /
                  segmentLength;
        break;
    }
    if (braking >= MAX_ENGINE_BRAKING_PERCENT) {
        braking = MAX_ENGINE_BRAKING_PERCENT;
    } else if (braking <= 0) {
        braking = 0;
    }
    if (gear == CAR_FIRST_FORWARD_GEAR && engineRpm < spec->redline) {
        braking *= FIRST_GEAR_BRAKING_MULTIPLIER;
    }
    return braking;
}

void ReadCarEngineTorque(const GameCarDrive *drive, const GameCarSpec *spec,
                         const s32 *gearCurve, s32 *netTorque,
                         s32 *bandScale) {
    s32 bandIndex;

    if (drive->engineRpm >= spec->revLimit) {
        s32 overRevResponse = WrapSigned32(
            (int64_t)WrapSigned32(
                (int64_t)spec->revLimit - drive->engineRpm) *
            REV_LIMIT_RESPONSE_NUMERATOR);

        *bandScale = 0;
        *netTorque = overRevResponse / REV_LIMIT_RESPONSE_DENOMINATOR;
        return;
    }
    bandIndex = drive->engineRpm / RPM_PER_TORQUE_BAND;
    if (bandIndex < 0) {
        bandIndex = 0;
    } else if (bandIndex >= CAR_TORQUE_BAND_COUNT) {
        bandIndex = CAR_TORQUE_BAND_COUNT - 1;
    }
    *netTorque = InterpolateDriveTorque(
        spec, gearCurve, drive->engineRpm, bandIndex, *netTorque);
    *bandScale = InterpolateEngineBraking(
        spec, drive->engineRpm, bandIndex, drive->gear);
}

s32 CalculateCarInitialAcceleration(const GameCarDrive *drive,
                                    s32 gearRatio) {
    s32 gearTorque = WrapSigned32((int64_t)gearRatio * drive->engineRpm);
    s32 netLoad = WrapSigned32(
        (int64_t)gearTorque - drive->drivetrainTorque);
    s32 roundedLoad;

    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        roundedLoad = netLoad < 0 ? netLoad + ((1 << TAKEOFF_LOAD_SHIFT) - 1)
                                  : netLoad;
        return roundedLoad >> TAKEOFF_LOAD_SHIFT;
    }
    if (netLoad < LOW_LOAD_THRESHOLD) {
        if (drive->motionState == CAR_MOTION_STANDING_START) {
            return netLoad / STANDING_START_LOAD_DIVISOR;
        }
        roundedLoad = netLoad + ((1 << DRIVING_LOAD_SHIFT) - 1);
        return roundedLoad >> DRIVING_LOAD_SHIFT;
    }
    if (netLoad > HIGH_LOAD_THRESHOLD) {
        return ((netLoad >> HIGH_LOAD_COARSE_SHIFT) *
                HIGH_LOAD_RESPONSE_NUMERATOR) /
               HIGH_LOAD_RESPONSE_DENOMINATOR;
    }
    return 0;
}
