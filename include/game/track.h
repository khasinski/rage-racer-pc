#ifndef GAME_TRACK_H
#define GAME_TRACK_H

#include "common.h"

#include "game/vector.h"
#include "game/player_car_aliases.h"
#include "game/visibility.h"

union GameEnvColor;
struct GameRenderObject;
struct PathSceneryRotationData;
typedef struct GameEnvironmentCue GameEnvironmentCue;


/*
 * One centreline point, 0x18 bytes. `leftHalfWidth` / `rightHalfWidth` are the left and
 * right half-widths (SteerCarAlongRoute clamps the lateral offset to
 * [-leftHalfWidth, rightHalfWidth]); the surface fields are interpolated between
 * a segment's two endpoints by UpdateCarTrackState and its non-clamping
 * twin ResetCarTrackState.
 */
typedef struct GameTrackPoint {
    s32 x;
    s32 z;
    s16 y;
    s16 angle;
    /* +0x0C the pitch component of the surface tilt: interpolated, then paired
     * with the cross-slope angle derived from crossSlope and rotated by the car's
     * track-relative heading to give the two tilt words at obj +0x20 / +0x28. */
    s16 surfacePitch;
    /* +0x0E cross-slope gradient in 1/128 of a unit per unit of lateral
     * offset. `surfaceY = interp(y) + (interp(crossSlope) * lateral >> 7)` in
     * GetTrackSurfaceHeight, SampleTrackSurfaceHeight and
     * UpdateCarTrackState alike. */
    s16 crossSlope;
    s16 leftHalfWidth;
    s16 rightHalfWidth;
    /* +0x14 arc reference, read as one u16 and split: bits 0..1 select the
     * cornering model (0 = straight, the arc block is skipped entirely; 2
     * negates the lateral offset, so it is the mirrored hand), and bits 4..15
     * are a signed index into g_TrackArcCenters (`(s16)arcRef >> 4`). Bits 2..3
     * are never read. */
    u16 arcRef;
    u16 segmentLength;
} GameTrackPoint;

typedef struct GameTrackPointHalfwordView {
    u16 x;
    u16 reserved02;
    u16 z;
    u16 reserved06;
    s16 y;
    s16 angle;
    u8 reserved0C[2];
    s16 crossSlope;
    u8 reserved10[6];
    u16 segmentLength;
} GameTrackPointHalfwordView;

typedef struct TrackAiSpeedKey {
    s16 progress;
    u16 pitch;
    s16 targetSpeeds[4];
} TrackAiSpeedKey;

typedef struct TrackRivalStart {
    s32 x;
    s32 z;
    s16 trackPointIndex;
    s16 modelId;
} TrackRivalStart;

typedef struct TrackRivalAiConfig {
    s16 speed;
    u16 accelerationStep;
    u16 boostAccelerationThreshold;
    u16 collisionBoostDuration;
    u16 boostAcceleration;
    u16 minimumSpeed;
    u16 initialEngineRpm;
    u16 reserved;
} TrackRivalAiConfig;

typedef struct TrackZone {
    s32 start;
    s32 end;
    s16 code;
    s16 value;
} TrackZone;

typedef union TrackZoneAddress {
    s32 value;
    TrackZone *pointer;
} TrackZoneAddress;

typedef struct TrackEventOffsets {
    s32 routeScenery;
    s32 raceIntroCamera;
    s32 pathSceneryPosition;
    s32 pathSceneryRotation;
    s32 reserved;
    s32 flybyScenery;
} TrackEventOffsets;

typedef union TrackEventOffsetBase {
    volatile TrackEventOffsets *offsets;
    u8 *bytes;
    struct PathSceneryRotationData *pathSceneryRotation;
} TrackEventOffsetBase;

s32 InterpolateTrackAngle(s32 pointIndex, s32 weight);
s32 LerpColorChannel(s32 from, s32 to, s32 blend);

typedef struct TrackAmbienceZone {
    s32 start;
    s32 end;
    u16 value;
    u16 flags;
} TrackAmbienceZone;

typedef struct TrackRacingLineHint {
    s16 start;
    s16 end;
    s16 minHeight;
    s16 maxHeight;
    u16 heightAdjustment;
    u16 reserved;
} TrackRacingLineHint;

typedef struct TrackCrestEvent {
    s32 progress;
    s32 motionValue;
} TrackCrestEvent;

typedef struct TrackEventSoundZone {
    s16 start;
    s16 end;
    s16 flags;
    u16 reserved;
} TrackEventSoundZone;

typedef union TrackEventSoundZoneAddress {
    s32 value;
    TrackEventSoundZone *pointer;
} TrackEventSoundZoneAddress;

/*
 * A sound that plays from one spot beside the track rather than filling a
 * stretch of it. It is audible between `start` and `end` along the track,
 * fading in and out over the two distances, and it is panned by where the
 * camera stands relative to (sourceX, sourceZ).
 */
typedef struct TrackPointAmbienceZone {
    s32 start;
    s32 end;
    u16 fadeInDistance;
    u16 fadeOutDistance;
    s32 sourceX;
    s32 sourceZ;
    s32 cue; /* 1 picks one sound, anything else the other; sign unused */
} TrackPointAmbienceZone;

typedef struct TrackFinishCue {
    s16 trackSection;
    s16 reserved;
} TrackFinishCue;

typedef struct TrackSpeedCue {
    s16 trackSection;
    s16 speedPercent;
} TrackSpeedCue;

typedef struct TrackRaceCueData {
    TrackFinishCue finish[2];
    u8 reserved08[8];
    TrackSpeedCue speed[2][3];
} TrackRaceCueData;

typedef union TrackRaceCueAddress {
    s32 value;
    u8 *bytePointer;
    TrackFinishCue *finishPointer;
    TrackRaceCueData *pointer;
} TrackRaceCueAddress;

typedef struct TrackEventData {
    s32 trackWalkStart;
    TrackCrestEvent crestEvents[2][8];
    TrackRacingLineHint racingLineHints[2][30];
    TrackRivalStart rivalStarts[2][12];
    TrackAiSpeedKey aiSpeedKeys[2][48];
    TrackRivalAiConfig rivalAiConfigs[2][12];
    TrackZone zones[20];
    TrackEventOffsets offsets;
    u8 reservedB7C[0x1000];
    TrackEventSoundZone eventSoundZones[30];
    TrackPointAmbienceZone pointAmbienceZones[2];
    TrackAmbienceZone ambienceZones[4];
    TrackRaceCueData raceCues;
} TrackEventData;

typedef union TrackEventDataAddress {
    s32 value;
    u8 *bytePointer;
    TrackEventData *pointer;
    TrackRivalStart *rivalStart;
} TrackEventDataAddress;

/*
 * One corner's centre of curvature. `GameTrackPoint.arcRef >> 4` indexes this
 * array, which InstallTrackPoints publishes at `g_TrackArcCenters`
 * (g_TrackArcCenters) immediately after the point table. The stride is 12, proven by
 * three independent `* 0xC` sites (UpdateCarDrivetrain, and 8003237C /
 * UpdateCarTrackState); the third word is never read anywhere in the image.
 *
 * The canonical global declaration lives in track_internal.h because the
 * table is installed and consumed only by track/car internals.
 */
typedef struct GameTrackArcCenter {
    s32 x;      /* +0x00 */
    s32 z;      /* +0x04 */
    s32 reserved08;  /* +0x08 never read */
} GameTrackArcCenter;

typedef s32 TrackPointOffset;

typedef union TrackPointTableAddress {
    TrackPointOffset pointOffset;
    s32 value;
    GameTrackPoint *pointPointer;
    GameTrackPointHalfwordView *halfwordPointer;
    GameTrackArcCenter *arcCenterPointer;
} TrackPointTableAddress;

typedef struct TrackPointTable {
    s32 count;
    GameTrackPoint points[1];
} TrackPointTable;

/* Track centreline points of the loaded course, g_TrackPointCount of them;
 * walked cyclically. */
extern GameTrackPoint *g_TrackPoints;

/* Valid entries in g_TrackPoints; every walker wraps with `% this`. */
extern s32 g_TrackPointCount;

/*
 * Reach a centreline point. The track is a closed ring, so every index into it
 * wraps; this is the one place that does it.
 *
 * The rule used to be written in the comment above and applied by hand at each
 * call site, which meant it was applied at most of them. UpdateFinishCamera
 * wrapped the index it passed to InterpolateTrackPoint and, two lines later,
 * handed a raw one to UpdateCarTrackState, which read past the array and took
 * the process down with it.
 */
static inline GameTrackPoint *TrackPoint(s32 index) {
    s32 count = g_TrackPointCount;
    if (count <= 0) return g_TrackPoints;
    index %= count;
    if (index < 0) index += count;
    return &g_TrackPoints[index];
}

/*
 * Animated course scenery (func_8003Dxxx / func_8003Fxxx). All four courses
 * share one coordinate space, so prop positions are one static table at
 * 0x8007E2C0 and each prop culls itself against the visible-terrain bitmask
 * g_VisibleCellMask.
 */

/* Per-frame update+draw of the current course's props, dispatched on the course
 * index. ...Scenery is the race copy (course passed in), ...Scenery2 the copy
 * for the replay/attract scenes (reads g_CourseIndex); they keep separate
 * animation state. `animate` == 0 draws a frozen frame. */
void DrawCourseScenery(s32 course, s32 timer, s32 animate);
void DrawCourseScenery2(s32 timer, s32 animate);

/* The two-part animated prop at g_AnimSceneryPos[0..1]: a 16-phase model swap plus a
 * companion part. Grand Prix only, nothing drawn in class 5. */
void DrawAnimatedScenery(s32 timer, s32 instance);
void DrawAnimatedScenery2(s32 timer, s32 instance, s32 isReplay, s32 animate);

/* The spinners: 1 on MYTHICAL COAST, 3 on OVER PASS CITY from class 2 up. A
 * 12-bit angle in g_SpinningSceneryAngle[] spins about Z at a rate re-randomised every
 * 512 frames. */
void DrawSpinningScenery(s32 timer, s32 animate);

/* The static landmark at g_StaticSceneryPos (40594, 6002, 11940), on all four courses;
 * pass 1 for THE EXTREME OVAL's +0x5000 z shift. Model 0x3A or 0x3B depending
 * on g_IsEnvironmentMode4. */
void DrawStaticScenery(s32 shifted);

/* A second static landmark at (29266, 6039, 45612): MYTHICAL COAST only, from
 * g_GrandPrixClass >= 4, and the one prop with no visibility cull. */
void DrawHighClassScenery(void);

typedef struct ShuttlePath {
    Vec4 endpoint[2];
} ShuttlePath;

extern ShuttlePath g_ShuttlePathPoints[3];

/* State of a shuttling prop: it runs between the two endpoints of its path in
 * g_ShuttlePathPoints, dwells, then reverses. */
typedef struct GameShuttleScenery {
    s32 dwellCounter;  /* +0x00 frames waited at the endpoint, capped at g_ShuttlePathDwellMax[path] */
    s32 reserved04;
    s32 travelStep;    /* +0x08 progress along the leg, capped at g_ShuttlePathTravelMax[path] */
    s16 startEndpoint; /* +0x0C which of the path's two endpoints this leg started from */
    s16 pathIndex;     /* +0x0E path: 0 OVER PASS CITY, 1 and 2 LAKESIDE GATE */
    Vec4 position;     /* +0x10 interpolated world position; x/z also select the visibility cell */
    s32 angleX;        /* +0x20 seeded from g_ShuttlePathAngles, never read by the drawer */
    s32 angleY;        /* +0x24 Y rotation (BuildRotMatrixY) */
    s32 angleZ;        /* +0x28 Z rotation (BuildRotMatrixZ) */
    u8 pad2C[8];
} GameShuttleScenery;

_Static_assert(sizeof(GameShuttleScenery) == 0x34,
               "GameShuttleScenery must match the retail layout");

/* The two shuttle instances. Instance 1 used to carry eight split symbols of
 * its own, g_Shuttle1DwellCounter..g_Shuttle1AngleZ; they were this array's
 * second element all along and are addressed as g_ShuttleScenery[1] now. */
extern GameShuttleScenery g_ShuttleScenery[2];

void UpdateShuttleScenery(s32 instance);
void DrawShuttleScenery(s32 instance);
void InitShuttleScenery(void);

/* Lap distance: the sum of every g_TrackPoints[].segmentLength. Cars' along-
 * track progress uses the same units. */
extern s32 g_TrackLength;

/* Base of the course's event/marker block (InstallTrackEventData installs it). Starts
 * with the s32 track-walk start index; sub-table offsets are at +0xB64..+0xB78
 * and the per-series marker rows at + g_RaceSeries * 576 + 0x474. */
extern TrackEventData *g_TrackEventData;

/* Declared identically by 73 translation units before this
 * header carried them. */

extern s32 g_CameraCarSeedYaw;
extern s32 g_CameraCarAngleY;
extern s32 g_CameraCarHeading;
extern s32 g_CameraCarSpeed;
extern s32 g_CameraCarStepX;
extern s32 g_CameraCarStepZ;
extern s32 g_CameraCarY;
extern s32 g_CameraCarZ;
extern s32 g_CourseModelCount;
extern s16 g_EnvLerpDuration;
extern GameEnvironmentCue *g_EnvScriptCues;
typedef union EnvironmentScriptLocation {
    s32 time;
    GameEnvironmentCue *pointer;
} EnvironmentScriptLocation;
extern s16 g_EnvSpareFrom;
extern s16 g_EnvSpareLerp;
extern s16 g_EnvSpareTo;
extern s16 g_RaceCueDelay;
extern s32 g_RaceCueFlags;
extern volatile s32 g_RouteSceneryFrame;
extern s32 g_RouteSceneryRotX;
extern s32 g_RouteSceneryRotZ;
/*
 * The route prop's position: the four words g_RouteSceneryX/Y/Z/W at
 * 0x801E4340.  Both seeders assign the whole thing at once out of the course
 * record -- `*(Vec4 *)&g_RouteSceneryX = *(Vec4 *)(src + 0x10)` -- which is
 * what puts the address in a register with the four stores hanging off it,
 * and DrawRouteScenery hands the same address to SetGteObjectMatrix.
 *
 * The four names stay.  Declared as one Vec4 the components would be marked
 * as living in an aggregate, and gcc 2.6.3 keeps a repeated in-aggregate
 * address in a register instead of re-materialising the symbol; retail's
 * UpdateRouteScenery re-materialises it for each of the three `+=` in its
 * tail, so its component accesses were not member accesses.
 */
extern Vec4 g_RouteSceneryPosition;
#define g_RouteSceneryX g_RouteSceneryPosition.x
#define g_RouteSceneryY g_RouteSceneryPosition.y
#define g_RouteSceneryZ g_RouteSceneryPosition.z

typedef union RouteSceneryPositionAddress {
    s32 *x;
    Vec4 *position;
} RouteSceneryPositionAddress;

static inline void SetRouteSceneryPosition(const Vec4 *position) {
    RouteSceneryPositionAddress address;

    address.position = &g_RouteSceneryPosition;
    *address.position = *position;
}

extern s16 g_ShuttlePathDwellMax[];

void InterpolateTrackPoint(s32 pointIndex, s32* out, s32 weight);

/* Declared identically by 113 translation units before this
 * header carried them. */

extern volatile s16 g_RouteSceneryArmed;
extern s16 g_AnimScenery2Frame;
extern s32 g_AnimScenery2Tint;
extern s16 g_AnimScenery2Variant;
extern s16 g_AnimSceneryFrame;
extern s16 g_AnimSceneryPitch[];
extern s16 g_AnimSceneryRacePosition;
extern s32 g_AnimSceneryTint;
extern s16 g_AnimSceneryVariant;
/* The orientation quads, same three-group shape as the offsets: delta at
 * 0x8009B1E8, start at +0x10, current at +0x20. Elements 0..2 are pitch, yaw
 * and roll -- 12-bit angles, wrapped to +-0x800 on load and masked with 0xFFF
 * on store -- and element 3 is the pull-back distance, a plain length. */
#define CAMPATH_PITCH 0
#define CAMPATH_YAW 1
#define CAMPATH_ROLL 2
#define CAMPATH_DIST 3
extern s32 g_CamPathAngle[4];
extern s32 g_CamPathAngleDelta[4];
#define g_ChaseYawPrev g_CamPathAngleDelta[CAMPATH_YAW]
extern s32 g_CamPathAngleStart[4];
extern s32 g_CamPathFrame;
extern s32 g_CamPathNode;
extern s32 g_CamPathOffset[3];
extern s32 g_CamPathOffsetDelta[3];
extern s32 g_CamPathOffsetStart[3];
extern s32 g_CameraCarSpeedRamp;
extern s32 g_CameraCarTrackPoint;
extern u8 g_CameraModePrev;
extern s32 g_CameraNodeIndex;
extern s8 g_CellScanOffsets[0x1000];
#define g_CellScanOffsetX g_CellScanOffsets
#define g_CellScanOffsetY (g_CellScanOffsets + 1)
extern s32 g_ChaseCameraPreset;
extern s32 g_ChaseCarSpeed;
extern s32 g_ChaseTargetYaw;
extern s32 g_ChaseYaw;
extern s32 g_ChaseYawDamping;
extern s32 g_ChaseYawLag;
extern s32 g_ChaseYawRampNeg;
extern s32 g_ChaseYawRampPos;
extern s32 g_ChaseYawStep;
extern s32 g_ChaseYawStepLimit;
extern s16 g_EnvLerpFrame;
extern u8 g_EnvScriptEnabled;
typedef struct SceneryMotionKeyframe {
    s16 rotationX;
    s16 rotationY;
    s16 rotationZ;
    s16 duration;
    s16 speed;
    s16 reserved;
} SceneryMotionKeyframe;

typedef struct SceneryMotionStart {
    Vec4 position;
    s32 reserved[4];
} SceneryMotionStart;

typedef struct SceneryMotionData {
    s16 triggerSection[2][2];
    s16 firstKeyframe[2][2];
    SceneryMotionStart start[2];
    SceneryMotionKeyframe keyframes[1];
} SceneryMotionData;

typedef struct FlybySceneryState {
    s32 timer;
    s32 soundEnabled;
    s32 keyframeTime;
    s16 lap;
    s16 keyframeIndex;
    Vec4 position;
    s32 rotationX;
    s32 rotationY;
    s32 rotationZ;
    s32 reserved2C;
    s32 volume;
} FlybySceneryState;

extern SceneryMotionKeyframe *g_FlybySceneryKeyframe;
extern s32 g_FogNear;
extern s16 g_FreeCameraAngleOffset[];
extern unsigned char g_StaticSceneryState[32];
#define g_HighClassSceneryYaw (*(s32 *)(void *)(g_StaticSceneryState + 28))
extern s32 g_OrbitCameraDistance;
extern s32 g_OrbitCameraYaw;
extern s16 g_PathSceneryHalfDelta[3];
typedef union PathSceneryPositionKey {
    struct {
        s32 x;
        s32 y;
        s32 z;
        u16 loopIndex;
        u16 reserved;
        s16 span;
        s16 rate;
    } fields;
    Block16 position;
} PathSceneryPositionKey;

typedef union PathSceneryRotationKey {
    struct {
        s16 x;
        s16 y;
        s16 z;
        u16 loopIndex;
        s16 span;
        s16 rate;
    } fields;
    SVec rotation;
} PathSceneryRotationKey;

typedef struct PathSceneryPositionData {
    s16 firstKey[2];
    PathSceneryPositionKey keys[1];
} PathSceneryPositionData;

typedef union SpinningSceneryAngleAddress {
    s32 value;
    s16 *pointer;
} SpinningSceneryAngleAddress;

typedef struct PathSceneryRotationData {
    s16 firstKey[2];
    PathSceneryRotationKey keys[1];
} PathSceneryRotationData;

typedef union PathSceneryKeyAddress {
    s32 value;
    PathSceneryPositionData *positionData;
    PathSceneryRotationData *rotationData;
    PathSceneryPositionKey *positionPointer;
    PathSceneryRotationKey *rotationPointer;
} PathSceneryKeyAddress;

static inline PathSceneryPositionKey *GetPathSceneryPositionKey(
    PathSceneryPositionData *data, s32 index) {
    return &data->keys[index];
}

static inline PathSceneryRotationKey *GetPathSceneryRotationKey(
    PathSceneryRotationData *data, s32 index) {
    return &data->keys[index];
}

extern PathSceneryPositionKey *g_PathSceneryPosKeys;
typedef union PathSceneryRate {
    u16 value;
    s16 signedValue;
} PathSceneryRate;

typedef union PathSceneryPhase {
    u16 value;
    s16 signedValue;
} PathSceneryPhase;

typedef struct PathSceneryCursors {
    PathSceneryPhase posPhase;
    PathSceneryPhase rotPhase;
    s16 posSpan;
    s16 rotSpan;
    PathSceneryRate posRate;
    PathSceneryRate rotRate;
    s16 posIndex;
    s16 rotIndex;
} PathSceneryCursors;

extern PathSceneryCursors g_PathSceneryCursors;
extern s16 g_PathSceneryRotHalfDelta[3];
extern PathSceneryRotationKey *g_PathSceneryRotKeys;
extern s32 g_PathSceneryVolume;
#define g_ShuttlePath2Points g_ShuttlePathPoints[2]
extern s16 g_ShuttlePathTravelMax[];
extern s16 g_SkyTileMap[][16];
extern s16 g_SpinningSceneryAngle[];
extern u16 g_SpinningSceneryRate[];
typedef struct SpinningSceneryOrientation {
    s32 yaw;
    u8 reserved[12];
} SpinningSceneryOrientation;
extern u8 g_SpinningSceneryRecords[64];
#define g_SpinningSceneryPos ((Vec4 *)(void *)g_SpinningSceneryRecords)
#define g_SpinningSceneryYaw ((SpinningSceneryOrientation *)(void *)(g_SpinningSceneryRecords + 12))
typedef union SpinningSceneryDataAddress {
    s32 value;
    u8 *bytes;
    SpinningSceneryOrientation *orientationPointer;
    Vec4 *positionPointer;
    void *pointer;
} SpinningSceneryDataAddress;
extern s32 g_StartGridSceneryAngle[];
#define g_StaticSceneryYaw (*(s32 *)(void *)(g_StaticSceneryState + 12))

s32 BlendAngle(s32 angleA, s32 angleB, s32 weight);
extern s32 FindNearestTrackCamera(struct GameRenderObject *car);
void LerpEnvColor(union GameEnvColor *from, union GameEnvColor *to,
                  union GameEnvColor *out, s32 blend);
void LoadEnvironmentCue(GameEnvironmentCue *cue);
void UpdateTrackEventSound(s16 trackSection);

/* Declared identically by 5 translation units before this
 * header carried them. */

extern Vec4 g_AnimSceneryPos[];
extern SVec g_ShuttlePathAngles[];
extern Vec4 g_StartGridSceneryPos[];
#define g_StaticSceneryPos (*(Vec4 *)(void *)g_StaticSceneryState)

/* The two tables InstallTerrainCellData splits out of sub-block 7: the
 * 32x32 cell grid (clut index in the low 10 bits) and the per-cell
 * visibility rows read by GetCellVisibility. */
extern u16 *g_TerrainCellGrid;
extern CellVisibilityRow *g_CellVisibilityTable;

/*
 * Their byte sizes, which is all InstallTerrainCellData needs them for - it
 * steps the sub-block pointer past each in turn. Both fall straight out of how
 * track/visible_cells.c indexes them, and both are fixed, not per-course:
 *
 *   grid       g_TerrainCellGrid[y * 32 + x], a u16 per cell
 *              -> 32 * 32 * 2 = 0x800
 *   visibility *(u32 *)(base + y * 0x80 + x * 4), bit = region id
 *              -> a u32 per cell, 32 * 32 * 4 = 0x1000
 *
 * The row stride the code uses is the proof of the second: `cellZ << 7` is
 * 32 u32 entries per row, and 32 such rows are 0x1000. It also caps region ids
 * at 32, even though the grid word has room for 64 in its top six bits.
 */
#define TERRAIN_CELL_GRID_SIZE      0x800
#define CELL_VISIBILITY_TABLE_SIZE  0x1000

#endif
