#ifndef GAME_CAR_H
#define GAME_CAR_H

#include "common.h"
#include "game/car_runtime_state.h"
#include "game/vector.h"

struct PlayerCarRuntime;
struct GameRenderObject;

enum {
    GAME_CAR_COUNT = 13,
    CUSTOM_PAINT_CAR_COUNT = 10,
    RACE_CAR_SLOT_COUNT = 11,
    RIVAL_CONTENDER_COUNT = 4,
    CAR_MODEL_VARIANT_COUNT = 32,
    CAR_TIRE_COMPOUND_COUNT = 5,
    CAR_BODY_KICK_INACTIVE = 0,
    CAR_BODY_KICK_LANDING = 1,
    CAR_BODY_KICK_CORNERING = 2,
    CAR_WHEEL_BLUR_FLAG = 0x1000,
};

_Static_assert(RACE_GRID_STORAGE_COUNT == RACE_CAR_SLOT_COUNT + 1,
               "starting grid needs one sentinel after the live cars");

/*
 * Per-car entry. The two setup bytes are what the CUSTOMIZE screen edits and
 * the save file keeps; whether the transmission row can be opened at all is a
 * property of the car's own loaded data (byte 8 of it), not of this entry.
 */
typedef struct CarEntry {
    u8 modelVariant;
    u8 tireCompound;  /* +0x01 CUSTOMIZE row 0, five settings */
    u8 transmission;  /* +0x02 CUSTOMIZE row 1, 0 automatic, 1 manual */
    u8 paintColor1;
    u8 paintColor2;
    u8 enabled;
    u8 pad6[2];
} CarEntry;

_Static_assert(sizeof(CarEntry) * GAME_CAR_COUNT == 104,
               "saved car table ABI changed");

typedef union CarSlideInput {
    s32 value;
    struct {
        s16 low;
        s16 high;
    } halves;
} CarSlideInput;

typedef union CarTrackHeading {
    s32 value;
    struct {
        u16 low;
        u16 high;
    } half;
} CarTrackHeading;

typedef struct CarTrackLimits {
    s16 rightInset;
    s16 leftInset;
    s16 rightKnockbackMode;
    s16 leftKnockbackMode;
} CarTrackLimits;

typedef union CarMotionValue {
    s16 value;
    u16 unsignedValue;
} CarMotionValue;

typedef struct GameCarRuntime {
    s32 x;
    /* +0x04 32 bits wide, not 16: `lw`/`sw` at nine sites in each of
     * UpdateRaceCars / UpdateAttractCars, `s32 unk04` in
     * UpdatePlayerCar's own layout, and the `*(s32 *)&ent->y` cast in
     * InitRivalCar that the old `s16` forced. */
    s32 y;
    s32 z;
    /* +0x10..+0x18: per-frame motion, measured in-race. Not the world
     * velocity GameCarDrive documents at +0xC8/+0xD0. */
    s32 positionW;
    s32 motionX;
    s32 motionY;
    s32 motionZ;
    s32 field_1C;
    s32 bodyPitch;
    s32 bodyYaw;
    s32 bodyRoll;
    s32 bodyRotationW;
    s32 trackPointIndex;
    s32 trackLateralOffset;
    s32 segmentFraction;
    s32 normalizedLateralOffset;
    s32 reserved40;
    s32 steeringAngle;
    s32 wheelRotation;
    s32 field_4C;
    s32 modelPitch;
    s32 modelYaw;
    s32 modelRoll;
    s32 field_5C;
    s32 modelY;
    s32 bodyRollVelocity;
    s32 progressA;
    s32 progressB;
    s32 trackProgress;
    s32 previousTrackProgress;
    s16 trackSection;
    s16 field_7A;
    s16 velocityX;
    s16 velocityZ;
    s16 motionActive;
    u16 motionTimer;
    s16 motionMode;
    s16 motionModeTimer;
    CarMotionValue motionValue;
    s16 collisionFlag;
    s16 tiltCounter;
    s16 reserved8E;
    s16 verticalPitch;
    s16 bodyKickOffset;
    s16 verticalRoll;
    s16 reserved96;
    s16 verticalMotionState;
    s16 verticalMotionTimer;
    s16 verticalMotionRate;
    s16 verticalTargetY;
    s32 headingAngle;
    s32 speed;        /* +0xA4 longitudinal speed; km/h readout is speed * 160 / 1168 */
    s32 acceleration; /* +0xA8 per-frame acceleration ramp / drivetrain force */
    s16 activeFlag;
    s16 modelIndex;
    s32 initializedFlag;
    CarTrackHeading trackHeading;
    /* +0xB8 0 = travelling with the course, 1 = against it. Seeded to
     * g_RaceSeries for every car by BuildStartingGrid and recomputed each
     * frame for the player from IsCarFacingBackwards; `!= g_RaceSeries`
     * is the wrong-way test. */
    s16 facingBackwards;
    u8 padBA[2];
    s32 aiEnabled;
    s32 field_C0;
    s32 reservedC4;
    s32 worldVelocityX;
    s32 reservedCC;
    s32 worldVelocityZ;
    s32 field_D4;
    s32 reservedD8;
    s32 reservedDC;
    s32 reservedE0;
    s32 renderDepth;
    s16 field_E8;
    s16 field_EA;
    s32 targetYaw;
    CarSlideInput slideInput;
    s32 yawRate;
    s32 reservedF8;
    s32 initialLateralOffset;
    s32 routeIndex;
    s16 avoidanceActive;
    u8 pad106[2];
    s32 baseBodyYaw;
    s16 nearbyCarCount;
    s16 reserved10E;
    s16 reserved110;
    s16 field_112;
    s16 field_114;
    s16 reserved116;
    s16 gridTargetProgress;
    s16 field_11A;
    s16 aiLateralOffset;
    s16 avoidanceTargetOffset;
    s16 avoidanceStep;
    s16 rivalModelId;
    s16 targetSpeed;
    s16 accelerationStep;
    s16 boostAccelerationThreshold;
    s16 collisionBoostDuration;
    s16 boostAcceleration;
    s16 boostTimer;
    s16 accelerationLimit;
    s16 currentGear;
    s32 engineRpm;
    s16 routeMarkerIndex;
    s16 routeMarkerActive;
    s32 field_13C;
    u8 pad140[0xC];
    u8 field_14C;
    u8 field_14D;
    u8 field_14E;
    u8 field_14F;
    s32 field_150;
    s32 field_154;
    u8 pad158[4];
    s16 acceleratorInput;
    s16 brakeInput;
    s16 field_160;
    s16 field_162;
    s32 previousTrackPointIndex;
    s16 field_168;
    u8 pad16A[0x32];
} GameCarRuntime;

static inline struct GameRenderObject *GetCarRenderObject(
    GameCarRuntime *car) {
    union {
        GameCarRuntime *runtime;
        struct GameRenderObject *renderObject;
    } view;

    view.runtime = car;
    return view.renderObject;
}

typedef struct CarSurfaceSampleView {
    u16 x;
    u16 reserved02;
    s32 surfaceY;
    u16 z;
    u16 reserved0A;
    u8 reserved0C[0x24];
    s32 trackPointIndex;
    u8 reserved34[0x2C];
    s32 modelY;
    u8 reserved64[0x34];
    s16 verticalMotionState;
} CarSurfaceSampleView;

typedef struct CarCollisionPoint {
    s16 x;
    s16 z;
} CarCollisionPoint;

static inline s32 GetCarCollisionPointPacked(const CarCollisionPoint *point) {
    return (s32)((u32)(u16)point->x | ((u32)(u16)point->z << 16));
}

typedef struct CarHullPoint {
    s16 x;
    s16 z;
} CarHullPoint;

_Static_assert(sizeof(CarHullPoint) == 4,
               "car hull points must retain their retail stride");

typedef union CarPaintPalette {
    /* The uploaded car image is a 64x256 halfword rectangle (0x8000 bytes),
     * and this palette begins at byte 0x7060. */
    u16 entries[(0x8000 - 0x7060) / sizeof(u16)];
    struct {
        u16 reserved[0x81];
        u16 bodyColor1;
    } fixed;
    struct {
        u16 reserved[0x2C1];
        u16 bodyColor1Gradient[5];
        u16 bodyColor2Gradient[5];
    } gradients;
} CarPaintPalette;

typedef struct CarImageData {
    u8 reserved[0x7060];
    CarPaintPalette paintPalette;
} CarImageData;

_Static_assert(sizeof(CarImageData) == 0x8000,
               "car image must match its 64x256 upload rectangle");

/*
 * The player's car and a rival's share their first 0xBC bytes field for field,
 * which is what lets the track, the crest hop, the knockback and the body kick
 * work on either of them. Reading the player's car through a rival's view of
 * that prefix is legitimate; doing it silently, through a pointer the compiler
 * has been told not to look at, is not. This is the one place the conversion
 * is written down, and every call that needs it says so.
 */
static inline GameCarRuntime *AsRivalCar(struct PlayerCarRuntime *car) {
    union {
        struct PlayerCarRuntime *player;
        GameCarRuntime *rival;
    } view;

    view.player = car;
    return view.rival;
}

extern CarHullPoint g_PlayerHullPoints[6];
extern CarHullPoint g_OpponentHullCorners[4];
extern CarHullPoint g_CarCornerOffsets[4];
extern CarCollisionPoint g_CarCollisionCorners[4];

/* Per-car runtime state, player in slot 0. Individual slots and single fields
 * also have their own split symbols. */
extern GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
extern GameCarRuntime g_CameraCar;

/* The four contenders ordered by race progress (`progressA + progressB`), best
 * first; re-sorted every frame by RankContenders to rubber-band the AI. */
extern GameCarRuntime *g_RankedCars[RIVAL_CONTENDER_COUNT];
/* Per-ranked-rival delay before another proximity cue may play. */
extern s16 g_RivalCueCooldowns[RIVAL_CONTENDER_COUNT];

/* Active car-entry table; repointed at one of the three 13-entry tables below
 * per title-menu row, so it is a pointer rather than a fixed array. */
extern CarEntry *g_CarTable;

/* The three saved car-entry tables, one per title-menu race row (0 GRAND PRIX,
 * 1 EXTRA GRAND PRIX, 2 TIME ATTACK); save block +0x50 / +0xC0 / +0x128. The
 * shops raise the Time Attack row to the best spec reached in either GP file. */
extern CarEntry g_GrandPrixCars[];
extern CarEntry g_ExtraGrandPrixCars[];
extern CarEntry g_TimeAttackCars[];

/* g_Cars index the replay / attract camera is following. */
extern s32 g_CameraCarIndex;

/* Index into g_CarTable of the car the player drives; selects the model and
 * texture pack to install. Distinct from g_CarListCursor. */
extern s32 g_PlayerCarIndex;

/* Cursor of the car list being browsed in the shop; steps to the next entry
 * with `enabled == 0`. Buying it copies it into g_PlayerCarIndex. */
extern s32 g_CarListCursor;

/* Index of each car model's first grade in the 32-entry asset list; thirteen
 * entries, one per model. GetCarAssetIndex adds the owned grade to it. */
extern u8 g_CarModelBaseIndex[GAME_CAR_COUNT];

/* Per-model base of the progress level a purchase requires; the level needed is
 * this plus the grade being bought (GetCarUnlockLevel). */
extern u8 g_CarModelUnlockBase[GAME_CAR_COUNT];

/* One automatic-gearbox shift point; `spec->shiftPoints[gear - 1]`. */
typedef struct GameCarSpecShiftPoint {
    s16 downshiftSpeed;
    s16 upshiftSpeed;
} GameCarSpecShiftPoint;

typedef union CarTorqueBand {
    s32 values[16];
    u16 halves[32];
} CarTorqueBand;

typedef struct CarTachometerSpec {
    s16 needleX;
    s16 needleY;
    u16 faceDX;
    u16 faceDY;
    u16 digitsX;
    u16 digitsY;
    s16 gearDigitDX;
    s16 gearDigitDY;
    u16 shiftLightDX;
    u16 shiftLightDY;
    u8 needleQuad[4];
    s16 angleMin;
    s16 angleMax;
    u8 needleColor[4];
    u8 needleColorAlt[4];
    s32 speedScale;
} CarTachometerSpec;

/* The loaded car's spec block (`g_CarSpec`), from its asset pack. */
typedef struct GameCarSpec {
    s32 torqueCurve[16];      /* +0x00 engine torque samples */
    CarTorqueBand torqueBand; /* +0x40 interpolation boundaries */
    s32 torqueLossValue[10];  /* +0x80 loss curve samples */
    s32 torqueLossRpm[9];     /* +0xA8 loss interpolation boundaries */
    s32 gearLoad[6];      /* +0xCC engine-load divisor per gear */
    s32 gearRatio[7];     /* +0xE4 final-drive ratio per gear (rpm divisor) */
    s16 revLimit;         /* +0x100 rev ceiling; the tacho and cut-out use it */
    s16 automaticAccelerationScale; /* +0x102, per-thousand automatic gearbox scale */
    s16 topGear;          /* +0x104 highest selectable gear */
    s16 redline;          /* +0x106 redline warning rpm */
    s16 steeringGripResponse;
    u16 steerResponse;    /* +0x10A divisor of the AI heading correction */
    s16 referenceTurnRadius; /* +0x10C, baseline for curved-track grip scaling */
    s16 negconSteeringAssistScale; /* +0x10E */
    s16 speedDragDivisor; /* +0x110, denominator of the speed-squared drag term */
    s16 baseSteeringGrip; /* +0x112 */
    s16 torqueScale[6];
    GameCarSpecShiftPoint shiftPoints[6]; /* +0x120, index = gear - 1 */
    CarTachometerSpec tachometer; /* +0x138 */
} GameCarSpec;

enum {
    CAR_FORWARD_GEAR_COUNT = 6,
    CAR_TORQUE_LOSS_BOUNDARY_COUNT = 10,
    CAR_KNOCKBACK_VECTOR_MODE = 4,
};

/* The retail asset stores these two logical tables across adjacent named
 * fields. Keep access within the declared C arrays while preserving that
 * packed format. */
static inline s32 GetCarTorqueLossBoundary(const GameCarSpec *spec,
                                           s32 index) {
    return index == CAR_TORQUE_LOSS_BOUNDARY_COUNT - 1
               ? spec->gearLoad[0]
               : spec->torqueLossRpm[index];
}

static inline s32 GetCarGearLoad(const GameCarSpec *spec, s32 gear) {
    return gear == CAR_FORWARD_GEAR_COUNT ? spec->gearRatio[0]
                                         : spec->gearLoad[gear];
}

static inline s32 GetPositiveCarGearRatio(const GameCarSpec *spec, s32 gear) {
    s32 ratio = spec->gearRatio[gear];

    return ratio > 0 ? ratio : 1;
}

static inline void SetCarGearLoad(GameCarSpec *spec, s32 gear, s32 value) {
    if (gear == CAR_FORWARD_GEAR_COUNT) {
        spec->gearRatio[0] = value;
    } else {
        spec->gearLoad[gear] = value;
    }
}

static inline GameCarSpec *GetGameCarSpec(void *data) {
    union {
        void *data;
        GameCarSpec *spec;
    } view;

    view.data = data;
    return view.spec;
}

extern GameCarSpec *g_CarSpec;

/* Per-gear torque curve, one 16-entry row per gear: row 0 is the engine's own
 * curve, rows 1..6 are it divided by each gear's ratio. */
typedef struct GearCurveRow {
    s32 values[16];
} GearCurveRow;

extern GearCurveRow g_GearTorqueCurve[];

typedef enum CarMotionState {
    CAR_MOTION_DRIVING,
    CAR_MOTION_TAKEOFF,
    CAR_MOTION_AIRBORNE,
    CAR_MOTION_STANDING_START,
} CarMotionState;

typedef enum CarVerticalMotionState {
    CAR_VERTICAL_GROUNDED,
    CAR_VERTICAL_RISING,
    CAR_VERTICAL_AT_CREST,
    CAR_VERTICAL_FALLING,
} CarVerticalMotionState;

typedef struct CarInputValue {
    s16 value;
} CarInputValue;

/* Drivetrain / input block beginning at +0xBC; the player physics code addresses
 * the car's second half through this rather than through GameCarRuntime.
 *
 * Calibrated on g_PlayerCar (D_8009E6D4), which is a different 0x19C object from
 * g_Cars: it shares the stride but not the meaning of every byte.
 * +0x30, +0x38, +0x74 and +0x76 are 16-bit gearDisp/jumpTimer/manual/gear on the
 * player object and 32-bit / AI-speed fields on the rival cars, so use
 * GameCarAiBlock for a g_Cars[] element. */
typedef struct GameCarDrive {
    s32 unk00;
    s32 unk04;
    s32 accelPos;    /* +0x08 */
    s32 reserved0C;
    s32 brakePos;    /* +0x10 */
    s32 unk14;
    s32 reserved18;
    s32 steerPos;    /* +0x1C */
    s32 reserved20;
    s32 unk24;
    s32 launchThresholdIndex;
    s16 engineLoad;
    s16 drivetrainCoupled;
    s16 gearDisp;    /* +0x30 */
    s16 steeringGrip;
    s16 clutch;      /* +0x34 */
    s16 shiftSpeedDelta;
    s16 jumpTimer;
    s16 unk3A;
    s16 shiftRpmDelta;
    s16 bodyLiftOffset;
    s16 trackCurveMode;
    s16 trackCurveBias;
    s32 coastFrames;
    s32 launchEnergy;
    s32 steeringLoadAngle;
    s32 spinRate;
    s32 launchDirection;
    s32 launchHeading;
    s32 launchSpeed;
    s32 yawOffset;
    s32 unk64;
    s32 standingStartBounceY;
    s32 standingStartBounceX;
    s16 unk70;
    s16 unk72;
    s16 manual;      /* +0x74 */
    s16 gear;        /* +0x76 */
    s32 engineRpm;   /* +0x78 */
    s32 reserved7C;
    s32 reserved80;
    s32 launchEnergyThreshold;
    s32 steeringGripResponse; /* +0x88 */
    s32 speedScale;
    s32 targetHeading;
    s32 drivetrainTorque; /* +0x94 */
    CarMotionState motionState; /* +0x98 */
    s16 acceleratorLatch;
    s16 brakeLatch;
    CarInputValue acceleratorInput; /* +0xA0 */
    s16 brakeInput;       /* +0xA2 */
    s16 racePosition;
    s16 hudLapHighlightRow;
} GameCarDrive;

typedef union PlayerLapTimes {
    struct {
        s32 frameCounts[6];
        s32 milliseconds[6];
    } table;
    s32 words[12];
} PlayerLapTimes;

/*
 * The player's 0x19C-byte race object. Everything up to +0xBC is laid out
 * exactly as a rival car's, field for field, which is why the track, crest and
 * knockback code takes either; from +0xBC on this carries the player
 * drivetrain block where a rival carries its AI view.
 *
 * The four fields at +0x98 were once called verticalMotionState, verticalMotionTimer, verticalMotionRate
 * and verticalTargetY, which read as gearbox state and are nothing of the kind:
 * UpdateCarCrestHop writes them for the player as well as for the rivals,
 * through exactly that shared prefix, and what they hold is the hop over a
 * crest. They now carry the names the rival side already used.
 */
typedef struct PlayerCarRuntime {
    s32 x;
    s32 y;
    s32 z;
    s32 positionW;
    s32 motionX;
    s32 motionY;
    s32 motionZ;
    s32 field_1C;
    s32 bodyPitch;
    s32 bodyYaw;
    s32 bodyRoll;
    s32 bodyRotationW;
    s32 trackPointIndex;
    s32 trackLateralOffset;
    s32 segmentFraction;
    s32 normalizedLateralOffset;
    s32 reserved40;
    s32 steeringAngle;
    s32 wheelRotation;
    s32 field_4C;
    union {
        struct {
            s32 modelPitch;
            s32 modelYaw;
            s32 modelRoll;
            s32 field_5C;
        };
        Vec4 modelRotation;
    };
    s32 modelY;
    s32 bodyRollVelocity;
    s32 progressA;
    s32 progressB;
    s32 trackProgress;
    s32 previousTrackProgress;
    s16 trackSection;
    s16 field_7A;
    s16 velocityX;
    s16 velocityZ;
    s16 motionActive;
    u16 motionTimer;
    s16 motionMode;
    s16 motionModeTimer;
    CarMotionValue motionValue;
    s16 collisionFlag;
    s16 tiltCounter;
    s16 reserved8E;
    s16 verticalPitch;
    s16 bodyKickOffset;
    s16 verticalRoll;
    s16 reserved96;
    s16 verticalMotionState;
    s16 verticalMotionTimer;
    s16 verticalMotionRate;
    s16 verticalTargetY;
    s32 headingAngle;
    s32 speed;
    s32 acceleration;
    s16 activeFlag;
    s16 modelIndex;
    s32 initializedFlag;
    CarTrackHeading trackHeading;
    s16 facingBackwards;
    u8 padBA[2];
    /* The showroom reuses +0xE4 for its tire selection. It is a retail view
     * of the same storage occupied by GameCarDrive during a race. */
    union {
        GameCarDrive drive;
        struct {
            u8 reservedDrive00[0x28];
            s32 showroomTireCompound;
        };
    };
    s32 previousTrackPointIndex;
    s16 lap;
    s16 field_16A;
    PlayerLapTimes lapTimes;
} PlayerCarRuntime;

_Static_assert(__builtin_offsetof(PlayerCarRuntime, segmentFraction) == 0x38,
               "player segment weight must retain its retail alias offset");
_Static_assert(__builtin_offsetof(PlayerCarRuntime, normalizedLateralOffset) == 0x3C,
               "player field 3C must retain its retail alias offset");
_Static_assert(__builtin_offsetof(PlayerCarRuntime, steeringAngle) == 0x44,
               "player steering must retain its retail alias offset");
_Static_assert(__builtin_offsetof(PlayerCarRuntime, wheelRotation) == 0x48,
               "player wheel angle must retain its retail alias offset");
_Static_assert(__builtin_offsetof(PlayerCarRuntime, modelPitch) == 0x50,
               "player render rotation must retain its retail alias offset");
_Static_assert(__builtin_offsetof(PlayerCarRuntime, modelRotation) == 0x50,
               "player render rotation view must retain the retail offset");
_Static_assert(__builtin_offsetof(PlayerCarRuntime, modelY) == 0x60,
               "player render Y must retain its retail alias offset");
_Static_assert(__builtin_offsetof(PlayerCarRuntime, facingBackwards) == 0xB8,
               "player facing flag must retain its retail alias offset");
_Static_assert(
    __builtin_offsetof(PlayerCarRuntime, showroomTireCompound) == 0xE4,
    "showroom tire selection must retain its retail alias offset");
_Static_assert(
    __builtin_offsetof(PlayerCarRuntime, drive) +
        __builtin_offsetof(GameCarDrive, manual) == 0x130,
    "player transmission must retain its retail alias offset");
_Static_assert(
    __builtin_offsetof(PlayerCarRuntime, drive) +
        __builtin_offsetof(GameCarDrive, engineRpm) == 0x134,
    "player target RPM must retain its retail alias offset");
_Static_assert(
    __builtin_offsetof(PlayerCarRuntime, drive) +
        __builtin_offsetof(GameCarDrive, racePosition) == 0x160,
    "race position must retain its retail alias offset");
_Static_assert(
    __builtin_offsetof(PlayerCarRuntime, drive) +
        __builtin_offsetof(GameCarDrive, hudLapHighlightRow) == 0x162,
    "HUD lap highlight must retain its retail alias offset");

static inline void CopyPlayerBodyRotationToModel(PlayerCarRuntime *car) {
    car->modelPitch = car->bodyPitch;
    car->modelYaw = car->bodyYaw;
    car->modelRoll = car->bodyRoll;
    car->field_5C = car->bodyRotationW;
}

static inline void CopyCarBodyRotationToModel(GameCarRuntime *car) {
    car->modelPitch = car->bodyPitch;
    car->modelYaw = car->bodyYaw;
    car->modelRoll = car->bodyRoll;
    car->field_5C = car->bodyRotationW;
}

/* A second, halfword-wide view of that same block, for the code that loads
 * 0x104..0x134 as s16 where GameCarDrive declares s32. */
typedef struct GameCarAiBlock {
    s32 enabled;
    u8 pad4[8];
    s32 worldVelocityX;   /* +0x0C world velocity x, sin(headingAngle) * speed / 256 */
    u8 pad10[4];
    s32 worldVelocityZ;   /* +0x14 world velocity z, cos(headingAngle) * speed / 256 */
    u8 pad18[0x18];
    s32 targetYaw;   /* +0x30 target angle: bodyYaw += GetAngleDelta(bodyYaw, this) / 5 */
    CarSlideInput slideInput; /* +0x34 */
    s32 yawRate;   /* +0x38 yaw rate, added to both steeringAngle and bodyYaw */
    u8 pad3C[8];
    s32 routeIndex; /* +0x44 */
    s16 avoidanceActive;  /* set to 1 while another car blocks this one */
    u8 pad4A[6];
    u16 nearbyCarCount;  /* count of cars close enough to matter this frame */
    s16 reserved10E;
    u8 pad54[8];
    s32 gridTargetProgress;  /* grid-seeded target progress (g_TrackLength / 12 steps) */
    /* These two are physically the same halfwords as the signed fields in
     * GameCarRuntime. The recovered AI arithmetic reads them unsigned before
     * narrowing its results back to 16 bits; changing this view to s16 changes
     * rival avoidance behaviour. Cast explicitly at sites that need a signed
     * lateral comparison. */
    u16 aiLateralOffset;
    s16 avoidanceTargetOffset;  /* traffic-avoidance target, +-0x50 */
    u16 avoidanceStep;
    u8 pad66[2];
    s16 targetSpeed;  /* grid-seeded speed, clamped >= 0 */
    u8 pad6A[4];
    s16 collisionBoostDuration;
    s16 boostAcceleration;  /* clamped to 0..15; the boost-branch step of acceleration */
    s16 boostTimer;  /* +0x72 slipstream-boost countdown, decremented while > 0 */
    s16 accelerationLimit;  /* damped while boxed in; caps acceleration */
    s16 minimumSpeed;  /* clamped to >= 0x3C */
    s16 engineRpmLow;  /* low half of GameCarRuntime.engineRpm, clamped to >= 0 */
    u8 pad7A[2];
    u16 markerCounter; /* +0x7C */
    s16 markerDirection; /* +0x7E */
    u8 pad80[0x22];
    s16 brakeInput;  /* +0xA2 */
    u8 padA4[0x3C];
} GameCarAiBlock;

static inline GameCarAiBlock *GetCarAiBlock(GameCarRuntime *car) {
    return (GameCarAiBlock *)&car->aiEnabled;
}

/*
 * The car pipeline.
 */
/* Race-entry init for the player object: start pose plus the speed/gear lookup
 * tables g_GearTorqueCurve / g_TorqueBandEnd / g_TorqueLossBandEnd. Logs "init_car" .. "init_ok". */
void InitPlayerCar(PlayerCarRuntime *car);
/* Pick the player's gear for this frame. Alternate controllers keep their two
 * shift buttons in the second half of the mapping table. */
void ShiftPlayerGears(PlayerCarRuntime *car, int useAlternateMapping);
/* The two variants of the rival-car driver over all RACE_CAR_SLOT_COUNT
 * slots. Race runs
 * only while `g_RacePhase >= 2 && g_GrandPrixMode`, adds three race-only passes
 * and time-slices cars 4..10; attract has no player so every car runs. */
void UpdateRaceCars(void);
void UpdateAttractCars(void);
/* Player-vs-field collision (detection, response and the crash cue), called
 * only from UpdatePlayerCar; returns the struck sub-quad 1..4 or 0. */
/* update_player_car.c spells the argument void *; the body reads
 * GameCarRuntime *. */
s32 CollidePlayerWithCars(PlayerCarRuntime *car);
/* One row of the AI pairwise sweep: car[index] against car[index + 1 .. 10],
 * push-apart only - no sound, no damage globals, no mode gate. */
s32 CollideRivalCars(GameCarRuntime *car, s32 index);
/* Draws one car, from the DrawCars loop; two LOD tiers plus the mirrored
 * wheel pass, submitted through SubmitModel. */
/* Selects model bank 1 and calls DrawCar for each of the 11 runtime cars
 * whose activeFlag != -1 and aiEnabled == 1. */
void DrawCars(void);
void DrawReplayRivalCar(void);
/*
 * The player's own 0x19C-byte car object.
 */

extern s32 g_DriveBoostTimer;
extern s32 g_EngineRpmSnapshot;
extern s32 g_AutoShiftCooldown;
extern u8 *g_CarModelBuffer;
extern s16 g_DragScale;
extern s32 g_EngineRpm;
extern s32 g_EngineRpmJitter;
extern s16 g_GripLossTimer;
extern u16 g_HudGlyphClut;
extern s16 g_PeakOutputRpm;
extern s16 g_PeakOutputValue;
extern RaceIntroCameraScript *g_RaceIntroCameraScript;
extern s32 g_RoadGrade;
extern s32 g_ShiftSoundLevel;
extern s32 g_ShiftTargetRpm;
extern s32 g_StandingStartSpin;
extern s16 g_SteerHoldFrames;
extern s16 g_TachoNeedleQuad[4][2];
extern s16 g_TrackZoneDark;

/* (model, owned grade) -> index of the CAR_xx asset pair, 0..31. */
/* Progress level needed to buy this model's next grade. */
s32 GetCarUnlockLevel(s32 model);
/* Asset/price-table row for the model variant currently owned. */
s32 GetOwnedCarAssetIndex(s32 model);
struct CarModelAsset;
/* Decode a serialized car asset into a native slot. Returns zero when its
 * offsets do not describe the expected model-plus-image layout. */
s32 InstallSerializedCarModelSlot(struct CarModelAsset *asset, s32 index);
/* Which of the two showroom model slots is live, 0 or 1. */
extern u32 g_CarModelSlot;
/* Point g_CarModelAsset at g_CarModelSlots[index]. */
void SelectCarModelSlot(s32 index);
/* Repaint the loaded car's texture block in the two body colours. */
void ApplyPrimaryBodyColor(u32 colour, CarImageData *imageData);
void ApplySecondaryBodyColor(u32 colour, CarImageData *imageData);
void SetPrimaryBodyColor(s32 colour);
void SetSecondaryBodyColor(s32 colour);
s32 SmoothTrackAngle(s32 pointIndex, s32 weight);
void UpdateRivalRubberBand(void);

extern s16 g_LaunchEnergyThresholds[];
extern s16 g_RedlineToPeakRpmHalf;
extern s16 g_PeakToRevLimitRpmHalf;
extern RaceGridSlot g_AttractGridSlots[RACE_GRID_STORAGE_COUNT];
extern u16 g_BodyColorPrimary[];
extern u16 g_BodyColorSecondary[];
extern s16 g_NegconAnalogI;
extern s16 g_NegconAnalogII;
extern s16 g_NegconAnalogL;
extern u16 g_PaintSlots3StopA[];
extern u16 g_PaintSlots3StopB[];
extern u16 g_PaintSlots4Stop[];
extern RaceGridSlot g_RaceGridSlots[RACE_GRID_STORAGE_COUNT];
/*
 * The race-intro camera's offset from the keyframe it is easing away from:
 * the three halfwords at 0x8009AFBC.  All three writers take them from one
 * keyframe's f0/f4/f8 in x/y/z order and the easing reads them back in the
 * same order, one per camera axis.  Retail's codegen does not discriminate
 * here -- no component is touched twice in a block -- so this is a layout
 * claim, not a proof.
 */
extern SVec g_RaceIntroCameraDelta;
extern s32 g_RaceIntroCameraTimer;
extern s32 g_ShiftTargetSpeed;
extern s32 g_TachoNeedleFlash;

typedef enum TachometerLightingMode {
    TACHOMETER_LIGHTING_NORMAL,
    TACHOMETER_LIGHTING_FADE_TO_DARK,
    TACHOMETER_LIGHTING_DARK,
    TACHOMETER_LIGHTING_FADE_FROM_DARK,
} TachometerLightingMode;

void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex);
void BuildStartingGrid(void);
void BuildTachoNeedleQuad(void);
void ClampCarLateralOffset(GameCarRuntime *car, s32 rivalSlot);
void InitRivalCar(GameCarRuntime *car, s32 gridPosition,
                  const RaceGridSlot *grid);
void InitRivalCarAi(GameCarRuntime *car, s32 gridPosition,
                    const RaceGridSlot *grid);
void RankContenders(void);
void SeedCarRouteMarkers(void);
void SlowRivalAhead(s32 rank);
void SteerCarAlongRoute(GameCarRuntime *car);
void SteerCarToTrackLine(PlayerCarRuntime *car);
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 carIndex);
void UpdateCarDrivetrain(PlayerCarRuntime *car);
void UpdateCarDriving(PlayerCarRuntime *car);
void UpdateCarLaunch(PlayerCarRuntime *car);
void UpdateCarAirborne(PlayerCarRuntime *car);
void UpdateCarStandingStart(PlayerCarRuntime *car);
void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex);
void AccumulateLapProgress(GameCarRuntime *car);
void UpdateCarTravelVelocity(GameCarRuntime *car);
void ApplyCarKnockback(GameCarRuntime *car);
s32 FindTrackSegment(GameCarRuntime *car, s32 idx);
s32 IsCarFacingBackwards(const PlayerCarRuntime *car);
void SeedCarLapProgress(GameCarRuntime *car, s32 mode);
void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode);
void StartCarBodyKick(GameCarRuntime *car, s32 mode);
void UpdateCarBodyKick(GameCarRuntime *car);
void UpdateCarBodyRoll(PlayerCarRuntime *car);
void UpdateCarCrestHop(GameCarRuntime *car);
void UpdateCarSlideAngle(GameCarRuntime *car, s32 slideScale);
void UpdatePlayerTilt(PlayerCarRuntime *car);
s32 UpdateCarTrackState(GameCarRuntime *car, s32 trackPointIndex,
                        const CarTrackLimits *limits);
void DrawTachometer(s32 rpm, s32 flash, TachometerLightingMode lighting,
                    s32 blendAmount);
void DrawPlayerTachometer(void);
void BeginCarStandingStart(PlayerCarRuntime *car);
void RunRaceIntroCamera(PlayerCarRuntime *car, s32 mode);
void UpdatePlayerCar(PlayerCarRuntime *car);

#endif
