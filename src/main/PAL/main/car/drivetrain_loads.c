#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

enum {
    PER_THOUSAND_SCALE = 1000,
    PERCENT_SCALE = 100,
    GRIP_SETTLING_DIVISOR = 2,
    MATCHING_CURVE_BIAS_STEP = 2,
    MISMATCHED_CURVE_BIAS_STEP = -1,
    CURVE_BIAS_UPPER_TRIGGER = 0x1F,
    CURVE_BIAS_LIMIT = 0x1E,
    CURVE_BIAS_GRIP_SCALE = 0xA,
    CAMBER_LIMIT = 0x32,
    PRIMARY_CAMBER_GRIP_SCALE = 0x3C,
    PRIMARY_CAMBER_GRIP_DIVISOR = 20,
    SECONDARY_CAMBER_GRIP_SCALE = 3,
    PEDAL_FIXED_SHIFT = 8,
    PEDAL_FIXED_ROUNDING_BIAS = 0xFF,
    BRAKE_RESISTANCE_DIVISOR = 8192,
    RELEASED_ACCELERATOR_THRESHOLD = 0x7F,
    COAST_TORQUE_DIVISOR = 2,
    HEADING_RESISTANCE_DIVISOR = 256,
    MINIMUM_STEERING_ASSIST_STEP = 1,
    STEERING_ASSIST_NUMERATOR = 5,
    STEERING_ASSIST_DENOMINATOR = 6,
    TRACK_SEGMENT_FRACTION_SCALE = 0x400,
    TRACK_SEGMENT_FRACTION_SHIFT = 10,
    ROAD_GRADE_TRIG_SHIFT = 12,
    ROAD_GRADE_LIMIT = 0xEE,
    ROAD_GRADE_SIDE_FORCE_SCALE = 0x708,
    ROAD_GRADE_SIDE_FORCE_DIVISOR = 0xA000,
    UPHILL_SIDE_FORCE_DIVISOR = 10,
    ACTIVE_RACE_PHASE = 2,
    STANDING_START_SPIN_MASK = 0x1F,
    STANDING_START_RESISTANCE_SCALE = 5,
    DRIVE_BOOST_BASE_RESISTANCE = 0xC8,
    DRIVE_BOOST_RESISTANCE_STEP = 0x14,
    TAKEOFF_THROTTLE_NUMERATOR = 4,
    TAKEOFF_THROTTLE_DENOMINATOR = 5,
    SPEED_DISPLAY_SCALE = 0xA0,
    SPEED_INTERNAL_SCALE = 1168,
    DEFAULT_DRAG_SCALE = 0x3E8,
    AIRBORNE_THROTTLE_MULTIPLIER = 2,
};

static void SettleSteeringGrip(GameCarDrive *drive, s32 gripBudget) {
    s32 response = WrapSigned32(
        (int64_t)gripBudget * drive->steeringGripResponse) /
        PER_THOUSAND_SCALE;

    drive->steeringGrip = WrapSigned16(
        WrapSigned32((int64_t)drive->steeringGrip + response) /
        GRIP_SETTLING_DIVISOR);
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
                (driveCurveMode == pointCurveMode
                     ? MATCHING_CURVE_BIAS_STEP
                     : MISMATCHED_CURVE_BIAS_STEP));
        }
        steerBias = drive->trackCurveBias;
        if (steerBias >= CURVE_BIAS_UPPER_TRIGGER) {
            drive->trackCurveBias = CURVE_BIAS_LIMIT;
        } else if (steerBias < -CURVE_BIAS_LIMIT) {
            drive->trackCurveBias = -CURVE_BIAS_LIMIT;
        }
        gripBudget = WrapSigned32(
            (int64_t)gripBudget + spec->baseSteeringGrip -
            drive->trackCurveBias * CURVE_BIAS_GRIP_SCALE);
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
        if (camber < -CAMBER_LIMIT) {
            camber = -CAMBER_LIMIT;
        } else if (camber > CAMBER_LIMIT) {
            camber = CAMBER_LIMIT;
        }
        camberLean = TrackPointCurveMode(trackPoint) == TRACK_CURVE_PRIMARY
            ? -(camber * PRIMARY_CAMBER_GRIP_SCALE) /
                  PRIMARY_CAMBER_GRIP_DIVISOR
            : camber * SECONDARY_CAMBER_GRIP_SCALE;
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
        throttleTorque += PEDAL_FIXED_ROUNDING_BIAS;
    }
    loads->throttleAcceleration = throttleTorque >> PEDAL_FIXED_SHIFT;

    if (g_GripLossTimer > 0) {
        g_GripLossTimer--;
    } else {
        g_GripLossTimer = 0;
    }
    loads->longitudinalResistance = WrapSigned32(
        (int64_t)loads->longitudinalResistance +
        WrapSigned32(
            (int64_t)drive->brakeInput * drive->engineRpm) /
            BRAKE_RESISTANCE_DIVISOR);
    if (netTorque > 0) {
        if (drive->acceleratorInput.value <
            RELEASED_ACCELERATOR_THRESHOLD) {
            loads->longitudinalResistance = WrapSigned32(
                (int64_t)loads->longitudinalResistance +
                netTorque / COAST_TORQUE_DIVISOR);
        }
    } else {
        loads->longitudinalResistance = WrapSigned32(
            (int64_t)loads->longitudinalResistance -
            netTorque / COAST_TORQUE_DIVISOR);
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
        drive->steeringLoadAngle / HEADING_RESISTANCE_DIVISOR);
    if (drive->motionState != CAR_MOTION_TAKEOFF &&
        g_PadType == PAD_TYPE_DIGITAL) {
        s32 assistStep = WrapSigned32(
            (int64_t)spec->negconSteeringAssistScale *
            drive->steeringGripResponse) / PER_THOUSAND_SCALE;
        s32 steerPosition = drive->steerPos;

        if (assistStep <= 0) {
            assistStep = MINIMUM_STEERING_ASSIST_STEP;
        }
        if (steerPosition >= 0) {
            loads->motionResistance = WrapSigned32(
                (int64_t)loads->motionResistance +
                (WrapSigned32(
                     (int64_t)steerPosition * STEERING_ASSIST_NUMERATOR) /
                 STEERING_ASSIST_DENOMINATOR) /
                    assistStep);
        } else {
            loads->motionResistance = WrapSigned32(
                (int64_t)loads->motionResistance -
                (WrapSigned32(
                     (int64_t)steerPosition * STEERING_ASSIST_NUMERATOR) /
                 STEERING_ASSIST_DENOMINATOR) /
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
        WrapSigned32(
            (int64_t)TRACK_SEGMENT_FRACTION_SCALE - alongSegment));
    pitchSum = WrapSigned32(
        (int64_t)pitchSum +
        WrapSigned32(
            (int64_t)nextTrackPoint->surfacePitch * alongSegment));
    if (pitchSum < 0) {
        pitchSum += TRACK_SEGMENT_FRACTION_SCALE - 1;
    }
    roadGrade = pitchSum >> TRACK_SEGMENT_FRACTION_SHIFT;
    roadGradeProduct = WrapSigned32(
        (int64_t)roadGrade * rcos(trackHeadingError));
    roadGrade = roadGradeProduct < 0
        ? (roadGradeProduct + ANGLE_MASK) >> ROAD_GRADE_TRIG_SHIFT
        : roadGradeProduct >> ROAD_GRADE_TRIG_SHIFT;
    if (roadGrade < -ROAD_GRADE_LIMIT) {
        roadGrade = -ROAD_GRADE_LIMIT;
    } else if (roadGrade > ROAD_GRADE_LIMIT) {
        roadGrade = ROAD_GRADE_LIMIT;
    }
    g_RoadGrade = roadGrade;
    sideForce = -rsin(roadGrade) * ROAD_GRADE_SIDE_FORCE_SCALE /
                ROAD_GRADE_SIDE_FORCE_DIVISOR;
    loads->motionResistance = WrapSigned32(
        (int64_t)loads->motionResistance +
        (roadGrade < 0
             ? sideForce
             : sideForce / UPHILL_SIDE_FORCE_DIVISOR));
}

static void ApplyTransientDriveLoads(CarDrivetrainLoads *loads,
                                     const GameCarDrive *drive) {
    if (g_RacePhase == ACTIVE_RACE_PHASE &&
        drive->motionState == CAR_MOTION_STANDING_START) {
        loads->motionResistance = WrapSigned32(
            (int64_t)loads->motionResistance +
            (g_StandingStartSpin & STANDING_START_SPIN_MASK) *
                STANDING_START_RESISTANCE_SCALE);
    }
    if (g_DriveBoostTimer > 0) {
        loads->motionResistance = WrapSigned32(
            (int64_t)loads->motionResistance +
            DRIVE_BOOST_BASE_RESISTANCE +
            WrapSigned32(
                (int64_t)g_DriveBoostTimer *
                DRIVE_BOOST_RESISTANCE_STEP));
        g_DriveBoostTimer = WrapSigned32(
            (int64_t)g_DriveBoostTimer - 1);
    }
    if (drive->motionState == CAR_MOTION_TAKEOFF) {
        loads->throttleAcceleration = WrapSigned32(
            (int64_t)loads->throttleAcceleration *
            TAKEOFF_THROTTLE_NUMERATOR) / TAKEOFF_THROTTLE_DENOMINATOR;
    }
}

static void ApplyAerodynamicResistance(CarDrivetrainLoads *loads,
                                       const PlayerCarRuntime *car,
                                       const GameCarSpec *spec,
                                       s32 bandScale) {
    s32 roadSpeed = WrapSigned32(
        (int64_t)car->speed * SPEED_DISPLAY_SCALE) / SPEED_INTERNAL_SCALE;
    s32 dragScale = WrapSigned16(g_DragScale);
    s32 dragDivisor;

    if (dragScale <= 0) {
        dragScale = 1;
    }
    dragDivisor = WrapSigned32(
        (int64_t)spec->speedDragDivisor * DEFAULT_DRAG_SCALE) / dragScale;
    if (dragDivisor <= 0) {
        dragDivisor = 1;
    }
    loads->motionResistance = WrapSigned32(
        (int64_t)loads->motionResistance +
        WrapSigned32((int64_t)roadSpeed * roadSpeed) / dragDivisor);
    g_DragScale = DEFAULT_DRAG_SCALE;
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        loads->motionResistance = WrapSigned32(
            (int64_t)loads->motionResistance *
            WrapSigned32((int64_t)PERCENT_SCALE - bandScale)) /
            PERCENT_SCALE;
    } else {
        loads->throttleAcceleration = WrapSigned32(
            (int64_t)loads->throttleAcceleration *
            AIRBORNE_THROTTLE_MULTIPLIER);
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
            ? drive->engineRpm / HEADING_RESISTANCE_DIVISOR
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
