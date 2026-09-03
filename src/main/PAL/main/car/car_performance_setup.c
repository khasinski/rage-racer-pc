#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/player_car_internal.h"

static s32 ClampPositiveInt64ToS32(int64_t value) {
    return value > INT32_MAX ? INT32_MAX : (s32)value;
}

static s32 CalculatePackedGearLoad(s32 gearRatio) {
    int64_t scaledGearRatio = (int64_t)gearRatio * 1168 / 160;
    int64_t load = (scaledGearRatio * 6 / 100) * 0x20000 / 10000;

    return ClampPositiveInt64ToS32(load);
}

static s32 CalculateTorqueCurveDivisor(s32 torqueScale, s32 gearRatio) {
    int64_t divisor = (int64_t)torqueScale * gearRatio / 100;

    if (divisor <= 0) return gearRatio;
    return ClampPositiveInt64ToS32(divisor);
}

static s32 FindFirstBandAtOrAbove(const s32 *values, s32 count,
                                  s32 speedThreshold) {
    s32 index;

    for (index = 0; index < count; index++) {
        if (values[index] >= speedThreshold) {
            return index;
        }
    }
    return -1;
}

static s32 FindFirstLossBandAtOrAbove(const GameCarSpec *spec,
                                      s32 speedThreshold) {
    s32 index;

    for (index = 0; index < CAR_TORQUE_LOSS_BOUNDARY_COUNT; index++) {
        if (GetCarTorqueLossBoundary(spec, index) >= speedThreshold) {
            return index;
        }
    }
    return -1;
}

void PrepareCarPerformance(GameCarDrive *drive) {
    GameCarSpec *spec = g_CarSpec;
    s32 peakOutput = 0;
    s32 peakIndex = 0;
    s32 speedThreshold;
    s32 gear;
    s32 index;

    if (spec->topGear < 1 || spec->topGear > CAR_FORWARD_GEAR_COUNT) {
        spec->topGear = CAR_FORWARD_GEAR_COUNT;
    }
    drive->speedScale = WrapSigned32(
        (int64_t)spec->tachometer.speedScale * 0x490) / 160;

    for (index = 0; index < 16; index++) {
        g_GearTorqueCurve[0].values[index] = spec->torqueCurve[index] / 20;
        if (peakOutput < g_GearTorqueCurve[0].values[index]) {
            peakIndex = index;
            peakOutput = g_GearTorqueCurve[0].values[index];
        }
    }
    g_PeakOutputValue = peakOutput;
    g_PeakOutputRpm = WrapSigned16(
        spec->torqueBand.halves[peakIndex * 2]);
    g_RedlineToPeakRpmHalf = (g_PeakOutputRpm - spec->redline) / 2;
    g_PeakToRevLimitRpmHalf = (spec->revLimit - g_PeakOutputRpm) / 2;

    for (gear = 0; gear < CAR_FORWARD_GEAR_COUNT; gear++) {
        s32 gearRatio = GetPositiveCarGearRatio(spec, gear + 1);
        s32 divisor =
            CalculateTorqueCurveDivisor(spec->torqueScale[gear], gearRatio);

        SetCarGearLoad(spec, gear + 1, CalculatePackedGearLoad(gearRatio));
        for (index = 0; index < 16; index++) {
            g_GearTorqueCurve[gear + 1].values[index] =
                spec->torqueCurve[index] / divisor;
        }
    }

    if (spec->baseSteeringGrip < 2) {
        spec->baseSteeringGrip = 1;
    }
    for (index = 0, speedThreshold = 0x3E8;
         index < CAR_TORQUE_BAND_COUNT;
         index++, speedThreshold += 0x3E8) {
        s32 band = FindFirstBandAtOrAbove(spec->torqueBand.values, 16,
                                          speedThreshold);
        s32 lossBand = FindFirstLossBandAtOrAbove(spec, speedThreshold);

        g_TorqueBandEnd[index] = (s16)(band >= 0 ? band : 0);
        g_TorqueLossBandEnd[index] = (s16)(lossBand >= 0 ? lossBand : 0);
    }

    drive->launchEnergyThreshold =
        g_LaunchEnergyThresholds[
            NormalizeCarLaunchThresholdIndex(drive->launchThresholdIndex)] *
        0xE;
    drive->steeringGripResponse = spec->steeringGripResponse;
}
