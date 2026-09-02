#include "game/car.h"
#include "game/car_internal.h"
#include "game/player_car_internal.h"

static s32 FindFirstPositiveBand(const s32 *values, s32 count,
                                 s32 speedThreshold) {
    s32 index;

    for (index = 0; index < count; index++) {
        if (values[index] / speedThreshold > 0) {
            return index;
        }
    }
    return -1;
}

static s32 FindFirstPositiveLossBand(const GameCarSpec *spec,
                                     s32 speedThreshold) {
    s32 index;

    for (index = 0; index < CAR_TORQUE_LOSS_BOUNDARY_COUNT; index++) {
        if (GetCarTorqueLossBoundary(spec, index) / speedThreshold > 0) {
            return index;
        }
    }
    return -1;
}

void PrepareCarPerformance(GameCarDrive *drive) {
    GameCarSpec *spec = g_CarSpec;
    s32 peakOutput = 0;
    s32 speedThreshold;
    s32 gear;
    s32 index;

    if (spec->topGear <= 0 || spec->topGear >= 6) {
        spec->topGear = 6;
    }
    drive->speedScale = (spec->tachometer.speedScale * 0x490) / 160;

    for (index = 0; index < 16; index++) {
        g_GearTorqueCurve[0].values[index] = spec->torqueCurve[index] / 20;
        if (peakOutput < g_GearTorqueCurve[0].values[index]) {
            g_PeakOutputRpm = index;
            peakOutput = g_GearTorqueCurve[0].values[index];
        }
    }
    g_PeakOutputValue = peakOutput;
    g_PeakOutputRpm = spec->torqueBand.halves[g_PeakOutputRpm * 2];
    g_RedlineToPeakRpmHalf = ((s16)g_PeakOutputRpm - spec->redline) / 2;
    g_PeakToRevLimitRpmHalf = (spec->revLimit - (s16)g_PeakOutputRpm) / 2;

    for (gear = 0; gear < 6; gear++) {
        s32 gearRatio = GetPositiveCarGearRatio(spec, gear + 1);
        s32 scaledGearRatio = gearRatio * 0x490 / 160;
        s32 divisor = spec->torqueScale[gear] * gearRatio / 100;

        SetCarGearLoad(spec, gear + 1,
                       (((scaledGearRatio * 6) / 100) << 17) / 10000);
        if (divisor <= 0) {
            divisor = gearRatio;
        }
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
        s32 band = FindFirstPositiveBand(spec->torqueBand.values, 16,
                                         speedThreshold);
        s32 lossBand = FindFirstPositiveLossBand(spec, speedThreshold);

        g_TorqueBandEnd[index] = (s16)(band >= 0 ? band : 0);
        g_TorqueLossBandEnd[index] = (s16)(lossBand >= 0 ? lossBand : 0);
    }

    drive->launchEnergyThreshold =
        g_LaunchEnergyThresholds[
            NormalizeCarLaunchThresholdIndex(drive->launchThresholdIndex)] *
        0xE;
    drive->steeringGripResponse = spec->steeringGripResponse;
}
