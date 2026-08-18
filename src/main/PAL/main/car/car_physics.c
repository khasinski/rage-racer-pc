#include "game/car_physics.h"

s16 CarUpdatePedalLatch(s16 latch, s32 input) {
    if (latch == 0) {
        if (input >= 0x85) return 1;
    } else if (latch == 1) {
        return 2;
    } else if (input < 0x7C) {
        return 0;
    }
    return latch;
}

s32 CarCalculateGripBudget(s32 acceleratorInput, s32 brakeInput) {
    s32 frontLoad = acceleratorInput * 0x64;
    s32 frontLoadScaled = frontLoad >> 8;
    if (frontLoad < 0) frontLoadScaled = (frontLoad + 0xFF) >> 8;
    return 0x17C - frontLoadScaled + (brakeInput * 0x64) / 256;
}

s32 CarCalculateLoadResistance(s32 motionState, s32 gearTorque,
                               s32 drivetrainTorque) {
    s32 netTorque = gearTorque - drivetrainTorque;
    if (motionState == 1)
        return (netTorque < 0 ? netTorque + 0xFFF : netTorque) >> 12;
    if (netTorque >= -0x30D3) {
        if (netTorque > 0x186A0)
            return (((netTorque < 0 ? netTorque + 0xFF : netTorque) >> 8) *
                    0x46) / 200;
        return 0;
    }
    if (motionState == 3) return netTorque / 768;
    return (netTorque < 0 ? netTorque + 0x7FF : netTorque) >> 11;
}

s32 CarCalculateThrottleAcceleration(s32 netTorque, s32 acceleratorInput,
                                     s32 drivetrainCoupled) {
    s32 torque = netTorque * acceleratorInput * drivetrainCoupled;
    if (torque < 0) torque += 0xFF;
    return torque >> 8;
}

s32 CarIntegrateEngineRpm(s32 engineRpm, s32 throttleAcceleration,
                         s32 resistance, s32 steeringLoad,
                         s32 jumpTimer, s32 clutch) {
    if (jumpTimer <= 0 && clutch <= 0)
        engineRpm += throttleAcceleration - resistance - steeringLoad;
    if (engineRpm < 0) return 0;
    if (engineRpm >= 0x3A99) return 0x3A98;
    return engineRpm;
}
