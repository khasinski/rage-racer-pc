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

s32 CarInterpolateSurfacePitch(s32 currentPitch, s32 nextPitch,
                               s32 segmentFraction) {
    s32 weighted = currentPitch * (0x400 - segmentFraction) +
                   nextPitch * segmentFraction;
    if (weighted < 0) weighted += 0x3FF;
    return weighted >> 10;
}

CarGroundSpeedOutput CarCalculateGroundSpeed(
    const CarGroundSpeedInput *input) {
    CarGroundSpeedOutput output;

    if (input->shiftState != 0) {
        output.acceleration = 0;
        output.speed = (input->speed * 0x3E7) / 1000;
        return output;
    }

    if (input->clutch > 0 || input->jumpTimer > 0) {
        output.acceleration = input->engineLoad;
    } else {
        s32 torque = input->drivetrainTorque;
        if (torque < 0) torque += 0x1FFFF;
        output.acceleration = torque >> 17;
        if (input->manual == 0) {
            output.acceleration = input->automaticAccelerationScale *
                                  output.acceleration / 1000;
        }
    }
    if (input->gripLossActive) output.acceleration /= 2;
    output.speed = (input->speed * 0x5E) / 100;
    return output;
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

s32 CarCaptureShiftEngineLoad(s32 acceleration) {
    /* Retail stores the signed acceleration through an unsigned halfword.
     * Negative collision acceleration therefore becomes a large positive
     * clutch load; manual shifts can use it to recover momentum rapidly. */
    return (u16)acceleration;
}

s32 CarManualUpshiftGradeScale(s32 gear, s32 roadGrade) {
    s32 penalty;
    if (gear < 4 || roadGrade >= 0) return 100;
    if (gear == 4) penalty = (-roadGrade) / 120;
    else if (gear == 5) penalty = (-roadGrade) / 48;
    else penalty = (roadGrade * -7) / 240;
    return 100 - penalty;
}

s32 CarUpdateTransmission(CarTransmissionState *state,
                          const CarTransmissionInput *input) {
    s32 suppressResistance = 0;

    if (state->motionState == 1 || state->motionState == 3) {
        state->jumpTimer = 0;
        state->clutch = 0;
        return 0;
    }

    if (state->motionState == 2 && state->jumpTimer >= 0) {
        state->jumpTimer--;
        suppressResistance = 1;
        if (state->jumpTimer < 0) state->jumpTimer = 0;
        if (state->displayedGear != state->gear) {
            state->targetRpm =
                (((input->speed * 160) / 1168) * 10000) /
                input->gearRatios[state->gear];
            state->shiftRpmDelta =
                (s16)((u16)state->targetRpm - (u16)state->engineRpm);
        }
        state->engineRpm = state->targetRpm +
                           state->shiftRpmDelta * state->jumpTimer / 20;
        return suppressResistance;
    }

    if (state->displayedGear != state->gear) {
        s32 targetSpeed = (input->speed * 10000) /
                          (input->gearRatios[state->gear] * 1168 / 160);
        u32 wheelSpeed = (u32)CarCaptureShiftEngineLoad(input->acceleration);

        state->engineLoad = (s32)wheelSpeed;
        state->targetSpeed = targetSpeed;
        if (state->manual != 0 && state->displayedGear < state->gear &&
            input->roadGrade < 0 && state->gear >= 4) {
            s32 gradeScale = CarManualUpshiftGradeScale(
                state->gear, input->roadGrade);
            s32 signedWheelSpeed = (s16)wheelSpeed;
            state->engineLoad =
                (u16)((signedWheelSpeed * gradeScale) / 100);
            state->targetSpeed = (gradeScale * targetSpeed) / 100;
        }
        suppressResistance = 1;
        if (state->displayedGear > state->gear) state->targetSpeed += 500;
        state->clutch = 10;
        state->drivetrainCoupled = 0;
        state->shiftSpeedDelta =
            (s16)((u16)state->targetSpeed - (u16)state->engineRpm);
        return suppressResistance;
    }

    state->clutch--;
    if (state->clutch <= 0) {
        state->drivetrainCoupled = 1;
        state->engineLoad = 0;
        state->clutch = 0;
    } else if (state->manual != 0) {
        state->engineRpm = state->targetSpeed -
            state->shiftSpeedDelta * state->clutch / 15;
    } else {
        state->engineRpm = state->targetSpeed -
            state->shiftSpeedDelta * state->clutch / 10;
    }
    return suppressResistance;
}
