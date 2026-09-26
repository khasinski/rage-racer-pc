#include "game/car_drive.h"
#include "game/integer.h"

#include <limits.h>
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

static void IndependentEngineTests(void) {
    GameCarSpec firstSpec = {0};
    GameCarSpec secondSpec;
    firstSpec.topGear = 6;
    firstSpec.revLimit = 16000;
    firstSpec.redline = 8000;
    firstSpec.baseSteeringGrip = 100;
    for (int i = 0; i < CAR_TORQUE_CURVE_SAMPLE_COUNT; i++) {
        firstSpec.torqueCurve[i] = (i + 1) * 20000;
        firstSpec.torqueBand.values[i] = i * 1000;
    }
    for (int i = 0; i < 6; i++) {
        firstSpec.gearRatio[i + 1] = 100;
        firstSpec.torqueScale[i] = 100;
    }
    secondSpec = firstSpec;
    for (int i = 0; i < CAR_TORQUE_CURVE_SAMPLE_COUNT; i++) {
        secondSpec.torqueCurve[i] *= 2;
        secondSpec.torqueBand.values[i] /= 2;
    }
    GameCarDrive first = {.gear = 1, .engineRpm = 1500};
    GameCarDrive second = first;
    CarPerformance firstPerformance;
    CarPerformance secondPerformance;
    memset(&firstPerformance, 0, sizeof(firstPerformance));
    memset(&secondPerformance, 0, sizeof(secondPerformance));
    PrepareCarPerformance(&first, &firstSpec, &firstPerformance);
    CarPerformance saved = firstPerformance;
    s32 expectedTorque = 0;
    s32 expectedBraking = 0;
    ReadCarEngineTorque(&first, &firstSpec, &firstPerformance,
                        firstPerformance.curves[1].values,
                        &expectedTorque, &expectedBraking);
    PrepareCarPerformance(&second, &secondSpec, &secondPerformance);
    CHECK_EQ(memcmp(&firstPerformance, &saved, sizeof(saved)), 0,
             "preparing a second engine leaves first engine tables unchanged");
    for (int i = 0; i < 10; i++) {
        s32 torque = 0;
        s32 braking = 0;
        ReadCarEngineTorque(&second, &secondSpec, &secondPerformance,
                            secondPerformance.curves[1].values, &torque, &braking);
        CHECK_EQ(torque > expectedTorque, 1, "second engine uses its own curve");
        ReadCarEngineTorque(&first, &firstSpec, &firstPerformance,
                            firstPerformance.curves[1].values, &torque, &braking);
        CHECK_EQ(torque, expectedTorque, "interleaved engines retain torque");
        CHECK_EQ(braking, expectedBraking, "interleaved engines retain braking");
    }
}

int main(void) {
    GameCarSpec spec;
    CarPerformance performance;
    GameCarDrive drive;
    int index;
    int gear;

    memset(&spec, 0, sizeof(spec));
    memset(&drive, 0, sizeof(drive));
    memset(performance.curves, 0, sizeof(GearCurveRow) * 7);
    memset(performance.torqueBands, -1, sizeof(s16) * 10);
    memset(performance.lossBands, -1, sizeof(s16) * 10);

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

    PrepareCarPerformance(&drive, &spec, &performance);

    CHECK_EQ(spec.topGear, 6, "invalid top gear is repaired");
    CHECK_EQ(spec.baseSteeringGrip, 1, "minimum steering grip");
    CHECK_EQ(drive.speedScale, 0x490, "speed scale");
    CHECK_EQ(drive.steeringGripResponse, 73, "steering response");
    CHECK_EQ(drive.launchEnergyThreshold, 1000 * 0xE,
             "launch threshold");
    CHECK_EQ(performance.peakOutput, spec.torqueCurve[15] / 20,
             "peak output");
    CHECK_EQ(performance.peakRpm, spec.torqueBand.halves[30],
             "peak rpm");
    CHECK_EQ(performance.redlineToPeak,
             ((s16)performance.peakRpm - spec.redline) / 2,
             "redline distance");
    CHECK_EQ(performance.peakToLimit,
             (spec.revLimit - (s16)performance.peakRpm) / 2,
             "rev-limit distance");

    for (index = 0; index < 16; index++) {
        CHECK_EQ(performance.curves[0].values[index],
                 spec.torqueCurve[index] / 20, "base torque curve");
        for (gear = 0; gear < 6; gear++) {
            s32 divisor = spec.torqueScale[gear] *
                          spec.gearRatio[gear + 1] / 100;
            CHECK_EQ(performance.curves[gear + 1].values[index],
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

        CHECK_EQ(performance.torqueBands[index], expected, "torque band");
        CHECK_EQ(performance.lossBands[index], expectedLoss,
                 "torque loss band");
    }

    drive.launchThresholdIndex = -1;
    PrepareCarPerformance(&drive, &spec, &performance);
    CHECK_EQ(drive.launchEnergyThreshold, 1550 * 0xE,
             "negative launch threshold wraps safely");

    memset(spec.torqueBand.values, 0, sizeof(spec.torqueBand.values));
    memset(spec.torqueLossRpm, 0, sizeof(spec.torqueLossRpm));
    spec.gearLoad[0] = 0;
    PrepareCarPerformance(&drive, &spec, &performance);
    for (index = 0; index < CAR_TORQUE_BAND_COUNT; index++) {
        CHECK_EQ(performance.torqueBands[index], 0,
                 "missing torque band clears previous car value");
        CHECK_EQ(performance.lossBands[index], 0,
                 "missing loss band clears previous car value");
    }

    spec.gearRatio[1] = 0;
    PrepareCarPerformance(&drive, &spec, &performance);
    CHECK_EQ(performance.curves[1].values[0], spec.torqueCurve[0],
             "zero gear ratio uses a safe unit divisor");

    spec.gearRatio[1] = INT32_MAX;
    spec.torqueScale[0] = INT16_MAX;
    spec.torqueCurve[0] = INT32_MAX;
    PrepareCarPerformance(&drive, &spec, &performance);
    CHECK_EQ(GetCarGearLoad(&spec, 1), INT32_MAX,
             "large gear load saturates without signed overflow");
    CHECK_EQ(performance.curves[1].values[0], 1,
             "large torque divisor saturates without signed overflow");

    memset(spec.torqueCurve, 0, sizeof(spec.torqueCurve));
    spec.torqueBand.halves[0] = 4321;
    performance.peakRpm = 12345;
    performance.peakOutput = 12345;
    PrepareCarPerformance(&drive, &spec, &performance);
    CHECK_EQ(performance.peakRpm, 4321,
             "empty torque curve uses the first rpm band");
    CHECK_EQ(performance.peakOutput, 0,
             "empty torque curve clears stale peak output");

    spec.tachometer.speedScale = INT_MAX;
    spec.torqueBand.halves[0] = UINT16_MAX;
    spec.redline = INT16_MIN;
    spec.revLimit = INT16_MAX;
    PrepareCarPerformance(&drive, &spec, &performance);
    CHECK_EQ(drive.speedScale,
             WrapSigned32((int64_t)INT_MAX * 0x490) / 160,
             "speed scale preserves 32-bit multiplication");
    CHECK_EQ(performance.peakRpm, -1,
             "peak rpm preserves signed halfword storage");
    CHECK_EQ(performance.redlineToPeak, 16383,
             "redline distance uses signed peak rpm");
    CHECK_EQ(performance.peakToLimit, 16384,
             "rev-limit distance uses signed peak rpm");

    IndependentEngineTests();
    if (s_failures != 0) {
        printf("%d car performance checks failed\n", s_failures);
        return 1;
    }
    puts("car performance setup passed");
    return 0;
}
