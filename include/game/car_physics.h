#ifndef GAME_CAR_PHYSICS_H
#define GAME_CAR_PHYSICS_H

#include "common.h"

typedef struct CarTorqueSample {
    s32 torque;
    s32 lossPercent;
} CarTorqueSample;

typedef struct CarTransmissionState {
    s16 motionState;
    s16 gear;
    s16 displayedGear;
    s16 jumpTimer;
    s16 clutch;
    s16 manual;
    s16 drivetrainCoupled;
    s16 shiftRpmDelta;
    s16 shiftSpeedDelta;
    s32 engineRpm;
    s32 engineLoad;
    s32 targetRpm;
    s32 targetSpeed;
} CarTransmissionState;

typedef struct CarTransmissionInput {
    s32 speed;
    s32 acceleration;
    s32 roadGrade;
    const s32 *gearRatios;
} CarTransmissionInput;

typedef struct CarGroundSpeedInput {
    s32 speed;
    s32 drivetrainTorque;
    s32 engineLoad;
    s32 automaticAccelerationScale;
    s16 shiftState;
    s16 clutch;
    s16 jumpTimer;
    s16 manual;
    s16 gripLossActive;
} CarGroundSpeedInput;

typedef struct CarGroundSpeedOutput {
    s32 speed;
    s32 acceleration;
} CarGroundSpeedOutput;

s16 CarUpdatePedalLatch(s16 latch, s32 input);
s32 CarCalculateGripBudget(s32 acceleratorInput, s32 brakeInput);
s32 CarCalculateLoadResistance(s32 motionState, s32 gearTorque,
                               s32 drivetrainTorque);
s32 CarCalculateThrottleAcceleration(s32 netTorque, s32 acceleratorInput,
                                     s32 drivetrainCoupled);
s32 CarCaptureShiftEngineLoad(s32 acceleration);
s32 CarManualUpshiftGradeScale(s32 gear, s32 roadGrade);
s32 CarIntegrateEngineRpm(s32 engineRpm, s32 throttleAcceleration,
                         s32 resistance, s32 steeringLoad,
                         s32 jumpTimer, s32 clutch);
CarTorqueSample CarSampleTorqueCurves(
    s32 engineRpm, s32 revLimit, s32 redline, s32 gear,
    s32 fallbackTorque, const s32 *gearCurve,
    const s32 *torqueBandRpm, const s16 *torqueBandEnd,
    const s32 *lossRpm, const s32 *lossValue,
    const s16 *lossBandEnd);
s32 CarUpdateTransmission(CarTransmissionState *state,
                          const CarTransmissionInput *input);
s32 CarInterpolateSurfacePitch(s32 currentPitch, s32 nextPitch,
                               s32 segmentFraction);
CarGroundSpeedOutput CarCalculateGroundSpeed(
    const CarGroundSpeedInput *input);

#endif
