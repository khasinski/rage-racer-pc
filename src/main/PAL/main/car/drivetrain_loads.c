#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

static void SettleSteeringGrip(GameCarDrive *drive, s32 gripBudget) {
    s32 response = WrapSigned32(
        (int64_t)gripBudget * drive->steeringGripResponse) / 1000;

    drive->steeringGrip = WrapSigned16(
        WrapSigned32((int64_t)drive->steeringGrip + response) / 2);
}

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
    TrackCurveMode curveModeNow;
    s32 camber;
    s32 camberLean;
    const int hasTrack = g_TrackPoints != NULL && g_TrackPointCount > 0;

    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        TrackCurveMode driveCurveMode =
            (TrackCurveMode)drive->trackCurveMode;
        s16 steerBias;

        if (hasTrack && driveCurveMode != TRACK_CURVE_NONE) {
            TrackCurveMode pointCurveMode =
                TrackPointCurveMode(TrackPoint(car->trackPointIndex));

            drive->trackCurveBias = WrapSigned16(
                (s32)drive->trackCurveBias +
                (driveCurveMode == pointCurveMode ? 2 : -1));
        }
        steerBias = drive->trackCurveBias;
        if (steerBias >= 0x1F) {
            drive->trackCurveBias = 0x1E;
        } else if (steerBias < -0x1E) {
            drive->trackCurveBias = -0x1E;
        }
        gripBudget = WrapSigned32(
            (int64_t)gripBudget + spec->baseSteeringGrip -
            drive->trackCurveBias * 0xA);
        drive->steeringGrip = WrapSigned16(gripBudget);
        return;
    }

    curveModeNow = (TrackCurveMode)drive->trackCurveMode;
    if (!hasTrack) {
        SettleSteeringGrip(drive, gripBudget);
        return;
    }

    trackPoint = TrackPoint(car->trackPointIndex);
    if (curveModeNow != TrackPointCurveMode(trackPoint) &&
        curveModeNow != TRACK_CURVE_NONE) {
        camber = trackPoint->crossSlope;
        if (camber < -0x32) {
            camber = -0x32;
        } else if (camber >= 0x33) {
            camber = 0x32;
        }
        camberLean = TrackPointCurveMode(trackPoint) == TRACK_CURVE_PRIMARY
            ? -(camber * 0x3C) / 20
            : camber * 3;
        gripBudget = WrapSigned32((int64_t)gripBudget + camberLean);
    }
    SettleSteeringGrip(drive, gripBudget);
}

static void UpdateLongitudinalResistance(CarDrivetrainLoads *loads,
                                         const GameCarDrive *drive,
                                         s32 netTorque) {
    s32 throttleTorque = WrapSigned32(
        (int64_t)netTorque * drive->acceleratorInput.value);

    throttleTorque = WrapSigned32(
        (int64_t)throttleTorque * drive->drivetrainCoupled);

    if (throttleTorque < 0) {
        throttleTorque += 0xFF;
    }
    loads->throttleAcceleration = throttleTorque >> 8;

    if (g_GripLossTimer > 0) {
        g_GripLossTimer--;
    } else {
        g_GripLossTimer = 0;
    }
    loads->longitudinalResistance = WrapSigned32(
        (int64_t)loads->longitudinalResistance +
        WrapSigned32(
            (int64_t)drive->brakeInput * drive->engineRpm) / 8192);
    if (netTorque > 0) {
        if (drive->acceleratorInput.value < 0x7F) {
            loads->longitudinalResistance = WrapSigned32(
                (int64_t)loads->longitudinalResistance + netTorque / 2);
        }
    } else {
        loads->longitudinalResistance = WrapSigned32(
            (int64_t)loads->longitudinalResistance - netTorque / 2);
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
    loads->motionResistance = WrapSigned32(
        (int64_t)loads->motionResistance +
        drive->steeringLoadAngle / 256);
    if (drive->motionState != CAR_MOTION_TAKEOFF &&
        g_PadType == PAD_TYPE_DIGITAL) {
        s32 assistStep = WrapSigned32(
            (int64_t)spec->negconSteeringAssistScale *
            drive->steeringGripResponse) / 1000;
        s32 steerPosition = drive->steerPos;

        if (assistStep <= 0) {
            assistStep = 1;
        }
        if (steerPosition >= 0) {
            loads->motionResistance = WrapSigned32(
                (int64_t)loads->motionResistance +
                (WrapSigned32((int64_t)steerPosition * 5) / 6) /
                    assistStep);
        } else {
            loads->motionResistance = WrapSigned32(
                (int64_t)loads->motionResistance -
                (WrapSigned32((int64_t)steerPosition * 5) / 6) /
                    assistStep);
        }
    }
}

static void UpdateRoadGradeResistance(CarDrivetrainLoads *loads,
                                      const PlayerCarRuntime *car) {
    const GameTrackPoint *trackPoint;
    const GameTrackPoint *nextTrackPoint;
    s32 alongSegment = car->segmentFraction;
    s32 trackHeadingError;
    s32 pitchSum;
    s32 roadGradeProduct;
    s32 roadGrade;
    s32 sideForce;

    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        g_RoadGrade = 0;
        return;
    }

    trackPoint = TrackPoint(car->trackPointIndex);
    nextTrackPoint = TrackPoint(car->trackPointIndex + 1);

    trackHeadingError = GetAngleDistance(
        car->headingAngle,
        ANGLE_THREE_QUARTER_TURN - trackPoint->angle);
    pitchSum = WrapSigned32(
        (int64_t)trackPoint->surfacePitch *
        WrapSigned32(INT64_C(0x400) - alongSegment));
    pitchSum = WrapSigned32(
        (int64_t)pitchSum +
        WrapSigned32(
            (int64_t)nextTrackPoint->surfacePitch * alongSegment));
    if (pitchSum < 0) {
        pitchSum += 0x3FF;
    }
    roadGrade = pitchSum >> 10;
    roadGradeProduct = WrapSigned32(
        (int64_t)roadGrade * rcos(trackHeadingError));
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
    loads->motionResistance = WrapSigned32(
        (int64_t)loads->motionResistance +
        (roadGrade < 0 ? sideForce : sideForce / 10));
}

static void ApplyTransientDriveLoads(CarDrivetrainLoads *loads,
                                     const GameCarDrive *drive) {
    if (g_RacePhase == 2 &&
        drive->motionState == CAR_MOTION_STANDING_START) {
        loads->motionResistance = WrapSigned32(
            (int64_t)loads->motionResistance +
            (g_StandingStartSpin & 0x1F) * 5);
    }
    if (g_DriveBoostTimer > 0) {
        loads->motionResistance = WrapSigned32(
            (int64_t)loads->motionResistance + 0xC8 +
            WrapSigned32((int64_t)g_DriveBoostTimer * 0x14));
        g_DriveBoostTimer = WrapSigned32(
            (int64_t)g_DriveBoostTimer - 1);
    }
    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        loads->throttleAcceleration = WrapSigned32(
            (int64_t)loads->throttleAcceleration * 4) / 5;
    }
}

static void ApplyAerodynamicResistance(CarDrivetrainLoads *loads,
                                       const PlayerCarRuntime *car,
                                       const GameCarSpec *spec,
                                       s32 bandScale) {
    s32 roadSpeed = WrapSigned32((int64_t)car->speed * 0xA0) / 1168;
    s32 dragScale = WrapSigned16(g_DragScale);
    s32 dragDivisor;

    if (dragScale <= 0) {
        dragScale = 1;
    }
    dragDivisor = WrapSigned32(
        (int64_t)spec->speedDragDivisor * 0x3E8) / dragScale;
    if (dragDivisor <= 0) {
        dragDivisor = 1;
    }
    loads->motionResistance = WrapSigned32(
        (int64_t)loads->motionResistance +
        WrapSigned32((int64_t)roadSpeed * roadSpeed) / dragDivisor);
    g_DragScale = 0x3E8;
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        loads->motionResistance = WrapSigned32(
            (int64_t)loads->motionResistance *
            WrapSigned32(INT64_C(0x64) - bandScale)) / 100;
    } else {
        loads->throttleAcceleration = WrapSigned32(
            (int64_t)loads->throttleAcceleration * 2);
        loads->motionResistance = 0;
    }
}

CarDrivetrainLoads CalculateCarDrivetrainLoads(
    PlayerCarRuntime *car, const GameCarSpec *spec, s32 netTorque,
    s32 bandScale, s32 initialAcceleration) {
    GameCarDrive *drive = &car->drive;
    CarDrivetrainLoads loads = {
        .longitudinalResistance = initialAcceleration,
        .motionResistance =
            car->verticalMotionState == CAR_VERTICAL_GROUNDED
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
