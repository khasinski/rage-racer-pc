#include "game/car.h"
#include "game/car_internal.h"
#include "game/player_car_internal.h"

#include <stdio.h>

enum {
    LAUNCH_THRESHOLD_COUNT = 5,
};

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

static s32 WrapLaunchThresholdIndex(s32 index) {
    index %= LAUNCH_THRESHOLD_COUNT;
    return index < 0 ? index + LAUNCH_THRESHOLD_COUNT : index;
}

void PrepareCarPerformance(GameCarDrive *drive) {
    GameCarSpec *spec = g_CarSpec;
    /* Both retail tables deliberately include one adjacent word: the tenth
     * loss boundary is gearLoad[0], and load slot 6 is gearRatio[0]. */
    s32 *lossBoundaries = (s32 *)(void *)&spec->torqueLossRpm[0];
    s32 *gearLoads = (s32 *)(void *)&spec->gearLoad[0];
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

    printf("%s", g_MsgInit1b);
    printf(g_FmtDecimalLine, spec->topGear);
    for (gear = 0; gear < 6; gear++) {
        s32 scaledGearRatio = (spec->gearRatio[gear + 1] * 0x490) / 160;
        s32 divisor = (spec->torqueScale[gear] *
                       spec->gearRatio[gear + 1]) / 100;

        /* Slot 6 is the adjacent gearRatio[0] word in the packed retail asset. */
        gearLoads[gear + 1] = (((scaledGearRatio * 6) / 100) << 17) / 10000;
        if (divisor <= 0) {
            divisor = spec->gearRatio[gear + 1];
        }
        for (index = 0; index < 16; index++) {
            g_GearTorqueCurve[gear + 1].values[index] =
                spec->torqueCurve[index] / divisor;
        }
    }

    if (spec->baseSteeringGrip < 2) {
        spec->baseSteeringGrip = 1;
    }
    printf("%s", g_MsgInit2);

    for (index = 0, speedThreshold = 0x3E8;
         index < 10;
         index++, speedThreshold += 0x3E8) {
        s32 band = FindFirstPositiveBand(spec->torqueBand.values, 16,
                                         speedThreshold);
        s32 lossBand = FindFirstPositiveBand(lossBoundaries, 10,
                                              speedThreshold);

        if (band >= 0) {
            g_TorqueBandEnd[index] = (s16)band;
        }
        if (lossBand >= 0) {
            g_TorqueLossBandEnd[index] = (s16)lossBand;
        }
    }

    printf("%s", g_MsgInit4);
    drive->launchEnergyThreshold =
        g_LaunchEnergyThresholds[
            WrapLaunchThresholdIndex(drive->launchThresholdIndex)] * 0xE;
    drive->steeringGripResponse = spec->steeringGripResponse;
}
