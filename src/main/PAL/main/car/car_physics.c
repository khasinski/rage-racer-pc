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

static s32 CurveBandStart(const s16 *bandEnd, s32 bandIndex) {
    s32 start;
    if (bandIndex == 0) return 0;
    start = bandEnd[bandIndex - 1];
    return start == 0 ? 0 : start - 1;
}

CarTorqueSample CarSampleTorqueCurves(
    s32 engineRpm, s32 revLimit, s32 redline, s32 gear,
    s32 fallbackTorque, const s32 *gearCurve,
    const s32 *torqueBandRpm, const s16 *torqueBandEnd,
    const s32 *lossRpm, const s32 *lossValue,
    const s16 *lossBandEnd) {
    CarTorqueSample sample;
    s32 bandIndex;
    s32 slot;
    s32 end;

    if (engineRpm >= revLimit) {
        sample.torque = ((revLimit - engineRpm) * 4) / 5;
        sample.lossPercent = 0;
        return sample;
    }

    bandIndex = engineRpm / 1000;
    sample.torque = fallbackTorque;
    slot = CurveBandStart(torqueBandEnd, bandIndex);
    end = torqueBandEnd[bandIndex];
    while (slot < end) {
        s32 lowerRpm = torqueBandRpm[slot];
        s32 upperRpm = torqueBandRpm[slot + 1];
        if (engineRpm >= lowerRpm && upperRpm >= engineRpm) {
            s32 span = upperRpm - lowerRpm;
            s32 weighted;
            if (span <= 0) span = 1;
            weighted = (engineRpm - lowerRpm) * gearCurve[slot + 1];
            weighted += (upperRpm - engineRpm) * gearCurve[slot];
            sample.torque = weighted / (span * 10);
            break;
        }
        slot++;
    }
    if (sample.torque < 0) sample.torque = 0;

    sample.lossPercent = 0;
    slot = CurveBandStart(lossBandEnd, bandIndex);
    end = lossBandEnd[bandIndex];
    while (slot < end) {
        s32 lowerRpm = lossRpm[slot];
        if (engineRpm >= lowerRpm) {
            s32 upperRpm = lossRpm[slot + 1];
            slot++;
            if (upperRpm >= engineRpm) {
                s32 span = upperRpm - lowerRpm;
                if (span <= 0) span = 1;
                sample.lossPercent =
                    ((engineRpm - lowerRpm) * lossValue[slot] +
                     (upperRpm - engineRpm) * lossValue[slot - 1]) / span;
                break;
            }
        } else {
            slot++;
        }
    }
    if (sample.lossPercent >= 100) sample.lossPercent = 100;
    else if (sample.lossPercent <= 0) sample.lossPercent = 0;
    if (gear == 1 && engineRpm < redline) sample.lossPercent *= 2;
    return sample;
}
