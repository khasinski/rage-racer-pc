#include "game/car_physics.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    const s32 torqueRpm[] = {0, 1000, 2000, 3000};
    const s32 gearCurve[] = {100, 200, 300, 400};
    const s16 torqueEnds[] = {2, 3, 4, 4};
    const s32 lossRpm[] = {0, 1000, 2000, 3000};
    const s32 lossValue[] = {0, 20, 80, 100};
    const s16 lossEnds[] = {2, 3, 4, 4};
    CarTorqueSample sample;
    const s32 gearRatios[] = {1000, 2000, 3000, 4000, 5000, 6000, 7000};
    CarTransmissionInput transmissionInput = {1168, -1, -240, gearRatios};
    CarTransmissionState transmission;
    CarGroundSpeedInput groundInput;
    CarGroundSpeedOutput groundOutput;
    EXPECT_EQ(0, CarUpdatePedalLatch(0, 0x84));
    EXPECT_EQ(1, CarUpdatePedalLatch(0, 0x85));
    EXPECT_EQ(2, CarUpdatePedalLatch(1, 0));
    EXPECT_EQ(2, CarUpdatePedalLatch(2, 0x7C));
    EXPECT_EQ(0, CarUpdatePedalLatch(2, 0x7B));

    EXPECT_EQ(380, CarCalculateGripBudget(0, 0));
    EXPECT_EQ(380, CarCalculateGripBudget(256, 256));
    EXPECT_EQ(380, CarCalculateGripBudget(-1, 0));
    EXPECT_EQ(10, CarInterpolateSurfacePitch(10, 20, 0));
    EXPECT_EQ(15, CarInterpolateSurfacePitch(10, 20, 512));
    EXPECT_EQ(-15, CarInterpolateSurfacePitch(-10, -20, 512));

    groundInput = (CarGroundSpeedInput){1000, 0x40000, 7, 800,
                                        0, 0, 0, 1, 0};
    groundOutput = CarCalculateGroundSpeed(&groundInput);
    EXPECT_EQ(940, groundOutput.speed);
    EXPECT_EQ(2, groundOutput.acceleration);
    groundInput.manual = 0;
    groundOutput = CarCalculateGroundSpeed(&groundInput);
    EXPECT_EQ(1, groundOutput.acceleration);
    groundInput.clutch = 1;
    groundInput.engineLoad = 9;
    groundInput.gripLossActive = 1;
    groundOutput = CarCalculateGroundSpeed(&groundInput);
    EXPECT_EQ(4, groundOutput.acceleration);
    groundInput.shiftState = 1;
    groundOutput = CarCalculateGroundSpeed(&groundInput);
    EXPECT_EQ(999, groundOutput.speed);
    EXPECT_EQ(0, groundOutput.acceleration);

    EXPECT_EQ(2, CarCalculateLoadResistance(1, 0x2000, 0));
    EXPECT_EQ(-2, CarCalculateLoadResistance(1, -0x2000, 0));
    EXPECT_EQ(0, CarCalculateLoadResistance(0, 100000, 0));
    EXPECT_EQ(273, CarCalculateLoadResistance(0, 200000, 0));
    EXPECT_EQ(-16, CarCalculateLoadResistance(3, -13000, 0));
    EXPECT_EQ(-6, CarCalculateLoadResistance(0, -13000, 0));

    EXPECT_EQ(100, CarCalculateThrottleAcceleration(200, 128, 1));
    EXPECT_EQ(-100, CarCalculateThrottleAcceleration(-200, 128, 1));
    EXPECT_EQ(0, CarCalculateThrottleAcceleration(200, 128, 0));

    EXPECT_EQ(1100, CarIntegrateEngineRpm(1000, 200, 50, 50, 0, 0));
    EXPECT_EQ(1000, CarIntegrateEngineRpm(1000, 200, 50, 50, 1, 0));
    EXPECT_EQ(0, CarIntegrateEngineRpm(10, 0, 20, 0, 0, 0));
    EXPECT_EQ(0x3A98,
              CarIntegrateEngineRpm(0x3A98, 1, 0, 0, 0, 0));

    sample = CarSampleTorqueCurves(
        500, 4000, 3500, 2, 77, gearCurve, torqueRpm, torqueEnds,
        lossRpm, lossValue, lossEnds);
    EXPECT_EQ(15, sample.torque);
    EXPECT_EQ(10, sample.lossPercent);
    sample = CarSampleTorqueCurves(
        1500, 4000, 3500, 1, 77, gearCurve, torqueRpm, torqueEnds,
        lossRpm, lossValue, lossEnds);
    EXPECT_EQ(25, sample.torque);
    EXPECT_EQ(100, sample.lossPercent);
    sample = CarSampleTorqueCurves(
        4500, 4000, 3500, 2, 77, gearCurve, torqueRpm, torqueEnds,
        lossRpm, lossValue, lossEnds);
    EXPECT_EQ(-400, sample.torque);
    EXPECT_EQ(0, sample.lossPercent);

    transmission = (CarTransmissionState){1, 2, 2, 5, 7, 0, 1,
                                           0, 0, 1000, 0, 0, 0};
    EXPECT_EQ(0, CarUpdateTransmission(&transmission, &transmissionInput));
    EXPECT_EQ(0, transmission.jumpTimer);
    EXPECT_EQ(0, transmission.clutch);

    transmission = (CarTransmissionState){0, 4, 3, 0, 0, 1, 1,
                                           0, 0, 1000, 0, 0, 0};
    EXPECT_EQ(1, CarUpdateTransmission(&transmission, &transmissionInput));
    EXPECT_EQ(10, transmission.clutch);
    EXPECT_EQ(0, transmission.drivetrainCoupled);
    EXPECT_EQ(313, transmission.targetSpeed);
    EXPECT_EQ(0, transmission.engineLoad);

    transmission.displayedGear = transmission.gear;
    transmission.clutch = 10;
    transmission.shiftSpeedDelta = 150;
    transmission.targetSpeed = 1000;
    CarUpdateTransmission(&transmission, &transmissionInput);
    EXPECT_EQ(9, transmission.clutch);
    EXPECT_EQ(910, transmission.engineRpm);
    return 0;
}
