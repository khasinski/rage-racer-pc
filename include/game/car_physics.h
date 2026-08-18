#ifndef GAME_CAR_PHYSICS_H
#define GAME_CAR_PHYSICS_H

#include "common.h"

typedef struct CarTorqueSample {
    s32 torque;
    s32 lossPercent;
} CarTorqueSample;

s16 CarUpdatePedalLatch(s16 latch, s32 input);
s32 CarCalculateGripBudget(s32 acceleratorInput, s32 brakeInput);
s32 CarCalculateLoadResistance(s32 motionState, s32 gearTorque,
                               s32 drivetrainTorque);
s32 CarCalculateThrottleAcceleration(s32 netTorque, s32 acceleratorInput,
                                     s32 drivetrainCoupled);
s32 CarIntegrateEngineRpm(s32 engineRpm, s32 throttleAcceleration,
                         s32 resistance, s32 steeringLoad,
                         s32 jumpTimer, s32 clutch);
CarTorqueSample CarSampleTorqueCurves(
    s32 engineRpm, s32 revLimit, s32 redline, s32 gear,
    s32 fallbackTorque, const s32 *gearCurve,
    const s32 *torqueBandRpm, const s16 *torqueBandEnd,
    const s32 *lossRpm, const s32 *lossValue,
    const s16 *lossBandEnd);

#endif
