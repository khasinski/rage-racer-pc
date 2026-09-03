#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/player_car_internal.h"

enum {
    SPEED_INTERNAL_SCALE = 1168,
    SPEED_DISPLAY_SCALE = 160,
    GEAR_LOAD_ACCELERATION_PERCENT = 6,
    PERCENT_SCALE = 100,
    GEAR_LOAD_FIXED_SCALE = 0x20000,
    RPM_FIXED_SCALE = 10000,
    BASE_TORQUE_CURVE_DIVISOR = 20,
    TORQUE_BAND_HALFWORD_STRIDE = 2,
    MINIMUM_VALID_STEERING_GRIP = 2,
    FALLBACK_STEERING_GRIP = 1,
    TORQUE_BAND_RPM_STEP = 1000,
    LAUNCH_ENERGY_THRESHOLD_SCALE = 0xE,
};

static s32 ClampPositiveInt64ToS32(int64_t value) {
    return value > INT32_MAX ? INT32_MAX : (s32)value;
}

static s32 CalculatePackedGearLoad(s32 gearRatio) {
    int64_t scaledGearRatio =
        (int64_t)gearRatio * SPEED_INTERNAL_SCALE / SPEED_DISPLAY_SCALE;
    int64_t load =
        (scaledGearRatio * GEAR_LOAD_ACCELERATION_PERCENT / PERCENT_SCALE) *
        GEAR_LOAD_FIXED_SCALE / RPM_FIXED_SCALE;

    return ClampPositiveInt64ToS32(load);
}

static s32 CalculateTorqueCurveDivisor(s32 torqueScale, s32 gearRatio) {
    int64_t divisor =
        (int64_t)torqueScale * gearRatio / PERCENT_SCALE;

    if (divisor <= 0) {
        return gearRatio;
    }
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
        (int64_t)spec->tachometer.speedScale * SPEED_INTERNAL_SCALE) /
        SPEED_DISPLAY_SCALE;

    for (index = 0; index < CAR_TORQUE_CURVE_SAMPLE_COUNT; index++) {
        g_GearTorqueCurve[0].values[index] =
            spec->torqueCurve[index] / BASE_TORQUE_CURVE_DIVISOR;
        if (peakOutput < g_GearTorqueCurve[0].values[index]) {
            peakIndex = index;
            peakOutput = g_GearTorqueCurve[0].values[index];
        }
    }
    g_PeakOutputValue = peakOutput;
    g_PeakOutputRpm = WrapSigned16(
        spec->torqueBand.halves[
            peakIndex * TORQUE_BAND_HALFWORD_STRIDE]);
    g_RedlineToPeakRpmHalf =
        (g_PeakOutputRpm - spec->redline) /
        TORQUE_BAND_HALFWORD_STRIDE;
    g_PeakToRevLimitRpmHalf =
        (spec->revLimit - g_PeakOutputRpm) /
        TORQUE_BAND_HALFWORD_STRIDE;

    for (gear = 0; gear < CAR_FORWARD_GEAR_COUNT; gear++) {
        s32 gearRatio = GetPositiveCarGearRatio(spec, gear + 1);
        s32 divisor =
            CalculateTorqueCurveDivisor(spec->torqueScale[gear], gearRatio);

        SetCarGearLoad(spec, gear + 1, CalculatePackedGearLoad(gearRatio));
        for (index = 0; index < CAR_TORQUE_CURVE_SAMPLE_COUNT; index++) {
            g_GearTorqueCurve[gear + 1].values[index] =
                spec->torqueCurve[index] / divisor;
        }
    }

    if (spec->baseSteeringGrip < MINIMUM_VALID_STEERING_GRIP) {
        spec->baseSteeringGrip = FALLBACK_STEERING_GRIP;
    }
    for (index = 0, speedThreshold = TORQUE_BAND_RPM_STEP;
         index < CAR_TORQUE_BAND_COUNT;
         index++, speedThreshold += TORQUE_BAND_RPM_STEP) {
        s32 band = FindFirstBandAtOrAbove(
            spec->torqueBand.values, CAR_TORQUE_CURVE_SAMPLE_COUNT,
            speedThreshold);
        s32 lossBand = FindFirstLossBandAtOrAbove(spec, speedThreshold);

        g_TorqueBandEnd[index] = (s16)(band >= 0 ? band : 0);
        g_TorqueLossBandEnd[index] = (s16)(lossBand >= 0 ? lossBand : 0);
    }

    drive->launchEnergyThreshold =
        g_LaunchEnergyThresholds[
            NormalizeCarLaunchThresholdIndex(drive->launchThresholdIndex)] *
        LAUNCH_ENERGY_THRESHOLD_SCALE;
    drive->steeringGripResponse = spec->steeringGripResponse;
}
