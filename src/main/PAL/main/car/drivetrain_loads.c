#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

/*
 * Work out how much steering grip is available this frame.
 *
 * On the ground, camber adjusts the grip target and the current value settles
 * halfway towards it. Just after takeoff, the car instead remembers whether
 * its expected curve still agrees with the track below it.
 */
void UpdateCarSteeringGrip(PlayerCarRuntime *car, const GameCarSpec *spec,
                           s32 gripBudget) {
    GameCarDrive *drive = &car->drive;
    const GameTrackPoint *trackPoint;
    s16 curveModeNow;
    s32 camber;
    s32 camberLean;

    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        s16 driveCurveMode = drive->trackCurveMode;
        s32 pointCurveMode = TrackPoint(car->trackPointIndex)->arcRef & 3;
        s16 steerBias;

        if (driveCurveMode != 0) {
            drive->trackCurveBias = (u16)drive->trackCurveBias +
                                    (driveCurveMode == pointCurveMode ? 2 : -1);
        }
        steerBias = drive->trackCurveBias;
        if (steerBias >= 0x1F) {
            drive->trackCurveBias = 0x1E;
        } else if (steerBias < -0x1E) {
            drive->trackCurveBias = -0x1E;
        }
        gripBudget += spec->baseSteeringGrip - drive->trackCurveBias * 0xA;
        drive->steeringGrip = (s16)gripBudget;
        return;
    }

    trackPoint = TrackPoint(car->trackPointIndex);
    curveModeNow = drive->trackCurveMode;
    if (curveModeNow != (trackPoint->arcRef & 3) && curveModeNow != 0) {
        camber = trackPoint->crossSlope;
        if (camber < -0x32) {
            camber = -0x32;
        } else if (camber >= 0x33) {
            camber = 0x32;
        }
        camberLean = (trackPoint->arcRef & 3) == 1
            ? -(camber * 0x3C) / 20
            : camber * 3;
        gripBudget += camberLean;
    }
    drive->steeringGrip =
        (s16)((drive->steeringGrip +
               (gripBudget * drive->steeringGripResponse) / 1000) / 2);
}

static void UpdateLongitudinalResistance(CarDrivetrainLoads *loads,
                                         const GameCarDrive *drive,
                                         s32 netTorque) {
    s32 throttleTorque = netTorque * drive->acceleratorInput.value *
                         drive->drivetrainCoupled;

    if (throttleTorque < 0) {
        throttleTorque += 0xFF;
    }
    loads->throttleAcceleration = throttleTorque >> 8;

    if (g_GripLossTimer > 0) {
        g_GripLossTimer--;
    } else {
        g_GripLossTimer = 0;
    }
    loads->accelerationResistance +=
        drive->brakeInput * drive->engineRpm / 8192;
    if (netTorque > 0) {
        if (drive->acceleratorInput.value < 0x7F) {
            loads->accelerationResistance += netTorque / 2;
        }
    } else {
        loads->accelerationResistance -= netTorque / 2;
    }
}

static void UpdateSteeringResistance(CarDrivetrainLoads *loads,
                                     const PlayerCarRuntime *car,
                                     GameCarDrive *drive,
                                     const GameCarSpec *spec) {
    s32 headingError = GetAngleDistance(car->bodyYaw, car->headingAngle);

    drive->steeringLoadAngle = headingError <= ANGLE_QUARTER_TURN
        ? headingError
        : ANGLE_HALF_TURN - headingError;
    loads->steeringResistance += drive->steeringLoadAngle / 256;
    if (drive->motionState != CAR_MOTION_TAKEOFF && g_PadType == 0x41) {
        s32 assistStep = spec->negconSteeringAssistScale *
                         drive->steeringGripResponse / 1000;
        s32 steerPosition = drive->steerPos;

        if (assistStep <= 0) {
            assistStep = 1;
        }
        if (steerPosition >= 0) {
            loads->steeringResistance +=
                ((steerPosition * 5) / 6) / assistStep;
        } else {
            loads->steeringResistance -=
                ((steerPosition * 5) / 6) / assistStep;
        }
    }
}

static void UpdateRoadGradeResistance(CarDrivetrainLoads *loads,
                                      const PlayerCarRuntime *car) {
    const GameTrackPoint *trackPoint = TrackPoint(car->trackPointIndex);
    const GameTrackPoint *nextTrackPoint =
        TrackPoint(car->trackPointIndex + 1);
    s32 alongSegment = car->segmentFraction;
    s32 trackHeadingError;
    s32 pitchSum;
    s32 roadGradeProduct;
    s32 roadGrade;
    s32 sideForce;

    trackHeadingError = GetAngleDistance(
        car->headingAngle,
        ANGLE_THREE_QUARTER_TURN - trackPoint->angle);
    pitchSum = trackPoint->surfacePitch * (0x400 - alongSegment);
    pitchSum += nextTrackPoint->surfacePitch * alongSegment;
    if (pitchSum < 0) {
        pitchSum += 0x3FF;
    }
    roadGrade = pitchSum >> 10;
    roadGradeProduct = roadGrade * rcos(trackHeadingError);
    roadGrade = roadGradeProduct < 0
        ? (roadGradeProduct + 0xFFF) >> 12
        : roadGradeProduct >> 12;
    if (roadGrade < -0xEE) {
        roadGrade = -0xEE;
    } else if (roadGrade >= 0xEF) {
        roadGrade = 0xEE;
    }
    g_RoadGrade = roadGrade;
    sideForce = -rsin(roadGrade) * 0x708 / 0xA000;
    loads->steeringResistance +=
        roadGrade < 0 ? sideForce : sideForce / 10;
}

static void ApplyTransientDriveLoads(CarDrivetrainLoads *loads,
                                     const GameCarDrive *drive) {
    if (g_RacePhase == 2 &&
        drive->motionState == CAR_MOTION_STANDING_START) {
        loads->steeringResistance += (g_StandingStartSpin & 0x1F) * 5;
    }
    if (g_DriveBoostTimer > 0) {
        loads->steeringResistance += 0xC8 + g_DriveBoostTimer * 0x14;
        g_DriveBoostTimer--;
    }
    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        loads->throttleAcceleration =
            loads->throttleAcceleration * 4 / 5;
    }
}

static void ApplyAerodynamicResistance(CarDrivetrainLoads *loads,
                                       const PlayerCarRuntime *car,
                                       const GameCarSpec *spec,
                                       s32 bandScale) {
    s32 roadSpeed = car->speed * 0xA0 / 1168;
    s32 dragScale = (s16)g_DragScale;
    s32 dragDivisor;

    if (dragScale <= 0) {
        dragScale = 1;
    }
    dragDivisor = spec->speedDragDivisor * 0x3E8 / dragScale;
    if (dragDivisor <= 0) {
        dragDivisor = 1;
    }
    loads->steeringResistance += roadSpeed * roadSpeed / dragDivisor;
    g_DragScale = 0x3E8;
    if (car->verticalMotionState == 0) {
        loads->steeringResistance =
            loads->steeringResistance * (0x64 - bandScale) / 100;
    } else {
        loads->throttleAcceleration *= 2;
        loads->steeringResistance = 0;
    }
}

CarDrivetrainLoads CalculateCarDrivetrainLoads(
    PlayerCarRuntime *car, const GameCarSpec *spec, s32 netTorque,
    s32 bandScale, s32 initialAcceleration) {
    GameCarDrive *drive = &car->drive;
    CarDrivetrainLoads loads = {
        .accelerationResistance = initialAcceleration,
        .steeringResistance = car->verticalMotionState == 0
            ? drive->engineRpm / 256
            : 0,
        .throttleAcceleration = 0,
    };

    UpdateLongitudinalResistance(&loads, drive, netTorque);
    UpdateSteeringResistance(&loads, car, drive, spec);
    UpdateRoadGradeResistance(&loads, car);
    ApplyTransientDriveLoads(&loads, drive);
    ApplyAerodynamicResistance(&loads, car, spec, bandScale);
    return loads;
}
