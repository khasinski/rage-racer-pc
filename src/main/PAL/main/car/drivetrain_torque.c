#include "game/car.h"
#include "game/car_internal.h"

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
        s32 weightedTorque;

        if (engineRpm < segmentStart || segmentEnd < engineRpm) {
            continue;
        }
        segmentLength = segmentEnd - segmentStart;
        if (segmentLength <= 0) {
            segmentLength = 1;
        }
        weightedTorque = (engineRpm - segmentStart) * gearCurve[slot + 1] +
                         (segmentEnd - engineRpm) * gearCurve[slot];
        torque = weightedTorque / (segmentLength * 0xA);
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

        if (engineRpm < segmentStart) {
            continue;
        }
        segmentEnd = GetCarTorqueLossBoundary(spec, slot + 1);
        if (segmentEnd < engineRpm) {
            continue;
        }
        segmentLength = segmentEnd - segmentStart;
        if (segmentLength <= 0) {
            segmentLength = 1;
        }
        braking = ((engineRpm - segmentStart) *
                       spec->torqueLossValue[slot + 1] +
                   (segmentEnd - engineRpm) *
                       spec->torqueLossValue[slot]) /
                  segmentLength;
        break;
    }
    if (braking >= 0x64) {
        braking = 0x64;
    } else if (braking <= 0) {
        braking = 0;
    }
    if (gear == 1 && engineRpm < spec->redline) {
        braking *= 2;
    }
    return braking;
}

void ReadCarEngineTorque(const GameCarDrive *drive, const GameCarSpec *spec,
                         const s32 *gearCurve, s32 *netTorque,
                         s32 *bandScale) {
    s32 bandIndex;

    if (drive->engineRpm >= spec->revLimit) {
        *bandScale = 0;
        *netTorque = ((spec->revLimit - drive->engineRpm) * 4) / 5;
        return;
    }
    bandIndex = drive->engineRpm / 1000;
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
    s32 gearTorque = gearRatio * drive->engineRpm;
    s32 netLoad = gearTorque - drive->drivetrainTorque;
    s32 roundedLoad;

    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        roundedLoad = netLoad < 0 ? netLoad + 0xFFF : netLoad;
        return roundedLoad >> 12;
    }
    if (netLoad < -0x30D3) {
        if (drive->motionState == CAR_MOTION_STANDING_START) {
            return netLoad / 768;
        }
        roundedLoad = netLoad + 0x7FF;
        return roundedLoad >> 11;
    }
    if (netLoad > 0x186A0) {
        roundedLoad = netLoad < 0 ? netLoad + 0xFF : netLoad;
        return ((roundedLoad >> 8) * 0x46) / 200;
    }
    return 0;
}
