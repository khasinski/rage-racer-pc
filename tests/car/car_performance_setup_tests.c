#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/player_car_internal.h"

#include <stdio.h>
#include <string.h>

static int s_failures;

#define CHECK_EQ(actual, expected, label) do { \
    s32 actualValue = (s32)(actual); \
    s32 expectedValue = (s32)(expected); \
    if (actualValue != expectedValue) { \
        printf("FAIL %s: got %d, expected %d\n", \
               label, actualValue, expectedValue); \
        s_failures++; \
    } \
} while (0)

static s32 FirstBand(const s32 *values, int count, s32 threshold) {
    int index;

    for (index = 0; index < count; index++) {
        if (values[index] / threshold > 0) {
            return index;
        }
    }
    return -1;
}

int main(void) {
    GameCarSpec spec;
    GameCarDrive drive;
    int index;
    int gear;

    memset(&spec, 0, sizeof(spec));
    memset(&drive, 0, sizeof(drive));
    memset(g_GearTorqueCurve, 0, sizeof(GearCurveRow) * 7);
    memset(g_TorqueBandEnd, -1, sizeof(s16) * 10);
    memset(g_TorqueLossBandEnd, -1, sizeof(s16) * 10);

    spec.topGear = 0;
    spec.redline = 12000;
    spec.revLimit = 18000;
    spec.baseSteeringGrip = 0;
    spec.steeringGripResponse = 73;
    spec.tachometer.speedScale = 160;
    for (index = 0; index < 16; index++) {
        spec.torqueCurve[index] = (index + 1) * 200;
        spec.torqueBand.values[index] = (index + 1) * 1500;
    }
    for (index = 0; index < 9; index++) {
        spec.torqueLossRpm[index] = (index + 1) * 2500;
    }
    spec.gearLoad[0] = 25000;
    for (gear = 0; gear < 6; gear++) {
        spec.gearRatio[gear + 1] = (gear + 2) * 100;
        spec.torqueScale[gear] = 100;
    }
    drive.launchThresholdIndex = 2;
    g_CarSpec = &spec;

    PrepareCarPerformance(&drive);

    CHECK_EQ(spec.topGear, 6, "invalid top gear is repaired");
    CHECK_EQ(spec.baseSteeringGrip, 1, "minimum steering grip");
    CHECK_EQ(drive.speedScale, 0x490, "speed scale");
    CHECK_EQ(drive.steeringGripResponse, 73, "steering response");
    CHECK_EQ(drive.launchEnergyThreshold,
             g_LaunchEnergyThresholds[2] * 0xE,
             "launch threshold");
    CHECK_EQ(g_PeakOutputValue, spec.torqueCurve[15] / 20,
             "peak output");
    CHECK_EQ(g_PeakOutputRpm, spec.torqueBand.halves[30],
             "peak rpm");
    CHECK_EQ(g_RedlineToPeakRpmHalf,
             ((s16)g_PeakOutputRpm - spec.redline) / 2,
             "redline distance");
    CHECK_EQ(g_PeakToRevLimitRpmHalf,
             (spec.revLimit - (s16)g_PeakOutputRpm) / 2,
             "rev-limit distance");

    for (index = 0; index < 16; index++) {
        CHECK_EQ(g_GearTorqueCurve[0].values[index],
                 spec.torqueCurve[index] / 20, "base torque curve");
        for (gear = 0; gear < 6; gear++) {
            s32 divisor = spec.torqueScale[gear] *
                          spec.gearRatio[gear + 1] / 100;
            CHECK_EQ(g_GearTorqueCurve[gear + 1].values[index],
                     spec.torqueCurve[index] / divisor,
                     "gear torque curve");
        }
    }
    for (gear = 0; gear < 6; gear++) {
        s32 scaled = spec.gearRatio[gear + 1] * 0x490 / 160;

        CHECK_EQ(GetCarGearLoad(&spec, gear + 1),
                 (((scaled * 6) / 100) << 17) / 10000,
                 "packed gear load");
    }
    for (index = 0; index < 10; index++) {
        s32 threshold = (index + 1) * 1000;
        s32 expected = FirstBand(spec.torqueBand.values, 16, threshold);
        s32 lossBoundaries[CAR_TORQUE_LOSS_BOUNDARY_COUNT];
        s32 expectedLoss;
        s32 lossIndex;

        for (lossIndex = 0;
             lossIndex < CAR_TORQUE_LOSS_BOUNDARY_COUNT;
             lossIndex++) {
            lossBoundaries[lossIndex] =
                GetCarTorqueLossBoundary(&spec, lossIndex);
        }
        expectedLoss = FirstBand(lossBoundaries,
                                 CAR_TORQUE_LOSS_BOUNDARY_COUNT,
                                 threshold);

        CHECK_EQ(g_TorqueBandEnd[index], expected, "torque band");
        CHECK_EQ(g_TorqueLossBandEnd[index], expectedLoss,
                 "torque loss band");
    }

    drive.launchThresholdIndex = -1;
    PrepareCarPerformance(&drive);
    CHECK_EQ(drive.launchEnergyThreshold,
             g_LaunchEnergyThresholds[4] * 0xE,
             "negative launch threshold wraps safely");

    memset(spec.torqueBand.values, 0, sizeof(spec.torqueBand.values));
    memset(spec.torqueLossRpm, 0, sizeof(spec.torqueLossRpm));
    spec.gearLoad[0] = 0;
    PrepareCarPerformance(&drive);
    for (index = 0; index < CAR_TORQUE_BAND_COUNT; index++) {
        CHECK_EQ(g_TorqueBandEnd[index], 0,
                 "missing torque band clears previous car value");
        CHECK_EQ(g_TorqueLossBandEnd[index], 0,
                 "missing loss band clears previous car value");
    }

    spec.gearRatio[1] = 0;
    PrepareCarPerformance(&drive);
    CHECK_EQ(g_GearTorqueCurve[1].values[0], spec.torqueCurve[0],
             "zero gear ratio uses a safe unit divisor");

    spec.gearRatio[1] = INT32_MAX;
    spec.torqueScale[0] = INT16_MAX;
    spec.torqueCurve[0] = INT32_MAX;
    PrepareCarPerformance(&drive);
    CHECK_EQ(GetCarGearLoad(&spec, 1), INT32_MAX,
             "large gear load saturates without signed overflow");
    CHECK_EQ(g_GearTorqueCurve[1].values[0], 1,
             "large torque divisor saturates without signed overflow");

    memset(spec.torqueCurve, 0, sizeof(spec.torqueCurve));
    spec.torqueBand.halves[0] = 4321;
    g_PeakOutputRpm = 12345;
    g_PeakOutputValue = 12345;
    PrepareCarPerformance(&drive);
    CHECK_EQ(g_PeakOutputRpm, 4321,
             "empty torque curve uses the first rpm band");
    CHECK_EQ(g_PeakOutputValue, 0,
             "empty torque curve clears stale peak output");

    if (s_failures != 0) {
        printf("%d car performance checks failed\n", s_failures);
        return 1;
    }
    puts("car performance setup passed");
    return 0;
}
