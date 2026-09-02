#ifndef GAME_TRACK_H
#define GAME_TRACK_H

#include "common.h"

#include "game/vector.h"
#include "game/shuttle_scenery.h"
#include "game/visibility.h"
#include "game/visible_cell_scan.h"

union GameEnvColor;
struct GameRenderObject;
struct PathSceneryRotationData;
typedef struct GameEnvironmentCue GameEnvironmentCue;


/*
 * One centreline point, 0x18 bytes. `leftHalfWidth` / `rightHalfWidth` are the left and
 * right half-widths (SteerCarAlongRoute clamps the lateral offset to
 * [-leftHalfWidth, rightHalfWidth]); the surface fields are interpolated between
 * a segment's two endpoints by UpdateCarTrackState and its non-clamping
 * replay reconstruction in ReconstructReplayCarTrackState.
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

typedef enum TrackCurveMode {
    TRACK_CURVE_NONE,
    TRACK_CURVE_PRIMARY,
    TRACK_CURVE_MIRRORED,
} TrackCurveMode;

static inline TrackCurveMode TrackPointCurveMode(
    const GameTrackPoint *point) {
    return (TrackCurveMode)(point->arcRef & 3);
}

static inline s32 TrackPointArcIndex(const GameTrackPoint *point) {
    return (s16)point->arcRef >> 4;
}

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
    s16 slotTargetSpeeds[4];
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

typedef struct TrackEventOffsets {
    s32 routeScenery;
    s32 raceIntroCamera;
    s32 pathSceneryPosition;
    s32 pathSceneryRotation;
    s32 reserved;
    s32 flybyScenery;
} TrackEventOffsets;

s32 InterpolateTrackAngle(s32 pointIndex, s32 weight);

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

enum {
    TRACK_SERIES_COUNT = 2,
    TRACK_CREST_EVENT_COUNT = 8,
    TRACK_RACING_LINE_HINT_COUNT = 30,
    TRACK_RIVAL_COUNT = 12,
    TRACK_AI_SPEED_KEY_COUNT = 48,
    TRACK_ZONE_COUNT = 20,
    TRACK_POINT_AMBIENCE_ZONE_COUNT = 2,
    TRACK_AMBIENCE_ZONE_COUNT = 4,
    TRACK_EVENT_SOUND_ZONE_COUNT = 30,
    TRACK_SPEED_CUE_COUNT = 3,
};

static inline s32 TrackPositionForSeries(s32 position, s32 trackLength,
                                         s32 series) {
    return series != 0 ? trackLength - position : position;
}

typedef struct TrackFinishCue {
    s16 trackSection;
    s16 reserved;
} TrackFinishCue;

typedef struct TrackSpeedCue {
    s16 trackSection;
    s16 speedPercent;
} TrackSpeedCue;

typedef struct TrackRaceCueData {
    TrackFinishCue finish[TRACK_SERIES_COUNT];
    u8 reserved08[8];
    TrackSpeedCue speed[TRACK_SERIES_COUNT][TRACK_SPEED_CUE_COUNT];
} TrackRaceCueData;

typedef struct TrackEventData {
    s32 trackWalkStart;
    TrackCrestEvent crestEvents[TRACK_SERIES_COUNT][TRACK_CREST_EVENT_COUNT];
    TrackRacingLineHint
        racingLineHints[TRACK_SERIES_COUNT][TRACK_RACING_LINE_HINT_COUNT];
    TrackRivalStart rivalStarts[TRACK_SERIES_COUNT][TRACK_RIVAL_COUNT];
    TrackAiSpeedKey
        aiSpeedKeys[TRACK_SERIES_COUNT][TRACK_AI_SPEED_KEY_COUNT];
    TrackRivalAiConfig
        rivalAiConfigs[TRACK_SERIES_COUNT][TRACK_RIVAL_COUNT];
    TrackZone zones[TRACK_ZONE_COUNT];
    TrackEventOffsets offsets;
    u8 reservedB7C[0x1000];
    TrackEventSoundZone eventSoundZones[TRACK_EVENT_SOUND_ZONE_COUNT];
    TrackPointAmbienceZone pointAmbienceZones[TRACK_POINT_AMBIENCE_ZONE_COUNT];
    TrackAmbienceZone ambienceZones[TRACK_AMBIENCE_ZONE_COUNT];
    TrackRaceCueData raceCues;
} TrackEventData;

_Static_assert(__builtin_offsetof(TrackEventData, reservedB7C) == 0xB7C,
               "track event table layout changed");

/*
 * One corner's centre of curvature. `GameTrackPoint.arcRef >> 4` indexes this
 * array, which InstallTrackPoints publishes at `g_TrackArcCenters`
 * immediately after the point table. The stride is 12, proven by three
 * independent `* 0xC` sites (UpdateCarDrivetrain, and 8003237C /
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

typedef struct TrackPointTable {
    s32 count;
    GameTrackPoint points[];
} TrackPointTable;

/* The retail asset stores its variable-length arc-centre table immediately
 * after the declared number of centreline points. Keep that format arithmetic
 * at the asset boundary instead of repeating a layout cast in consumers. */
static inline GameTrackArcCenter *TrackPointTableArcCenters(
    TrackPointTable *table) {
    return (GameTrackArcCenter *)(table->points + table->count);
}

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
static inline s32 WrapTrackPointIndex(s32 index) {
    s32 count = g_TrackPointCount;

    if (count <= 0) return 0;
    index %= count;
    if (index < 0) index += count;
    return index;
}

static inline GameTrackPoint *TrackPoint(s32 index) {
    if (g_TrackPointCount <= 0) return g_TrackPoints;
    index = WrapTrackPointIndex(index);
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
void DrawPresentationCourseScenery(s32 timer, s32 animate);
void BuildVisibleCells(s32 near, s32 far);
void DrawCourseObjects(void);
void DrawTerrainCells(void);
void DrawTerrainCellsWide(void);

/* Update (when animate != 0) and draw the route/flyby/path prop layers enabled
 * by the current Grand Prix class. Class 5 wraps to the class-0 route layer. */
void DrawScriptedScenery(s32 animate);
void DrawStartGridScenery(s32 timer);
void InitTrackScene(void);
void InitPathScenery(void);
void SeedFlybyScenery(void);
void SeedRouteScenery(void);
void TriggerRaceCues(void);
void UpdatePointAmbience(s32 trackPosition);

/* The static landmark at g_StaticSceneryPos (40594, 6002, 11940), on all four courses;
 * pass 1 for THE EXTREME OVAL's +0x5000 z shift. Model 0x3A or 0x3B depending
 * on g_IsEnvironmentMode4. */
void DrawStaticScenery(s32 shiftForSeriesCourse);

/* A second static landmark at (29266, 6039, 45612): MYTHICAL COAST only, from
 * g_GrandPrixClass >= 4, and the one prop with no visibility cull. */
void DrawHighClassScenery(void);

typedef struct ShuttlePath {
    Vec4 endpoint[2];
} ShuttlePath;

enum {
    SHUTTLE_INSTANCE_COUNT = 2,
    SHUTTLE_PATH_COUNT = 3,
    SHUTTLE_ENDPOINT_COUNT = 2,
};

extern ShuttlePath g_ShuttlePathPoints[SHUTTLE_PATH_COUNT];

/* The two shuttle instances. Instance 1 used to carry eight split symbols of
 * its own, g_Shuttle1DwellCounter..g_Shuttle1AngleZ; they were this array's
 * second element all along and are addressed as g_ShuttleScenery[1] now. */
extern GameShuttleScenery g_ShuttleScenery[SHUTTLE_INSTANCE_COUNT];

void InitShuttleScenery(void);

/* Lap distance: the sum of every g_TrackPoints[].segmentLength. Cars' along-
 * track progress uses the same units. */
extern s32 g_TrackLength;

/* Base of the course's event/marker block (InstallTrackEventData installs it). Starts
 * with the s32 track-walk start index; sub-table offsets are at +0xB64..+0xB78
 * and the per-series marker rows at + g_RaceSeries * 576 + 0x474. */
extern TrackEventData *g_TrackEventData;

extern s32 g_CameraCarSeedYaw;
extern s32 g_CameraCarHeading;
extern s32 g_CameraCarSpeed;
extern s32 g_CameraCarStepX;
extern s32 g_CameraCarStepZ;
extern s32 g_CameraCarZ;
extern s32 g_CourseModelCount;
extern s16 g_EnvLerpDuration;
extern GameEnvironmentCue *g_EnvScriptCues;
extern s16 g_EnvSpareFrom;
extern s16 g_EnvSpareLerp;
extern s16 g_EnvSpareTo;
extern s16 g_RaceCueDelay;
extern s32 g_RaceCueFlags;
extern s32 g_RouteSceneryFrame;
extern s32 g_RouteSceneryRotX;
extern s32 g_RouteSceneryRotZ;
/* Position of the animated route prop. The component aliases keep the motion
 * update readable while seed/reset code can copy the complete vector. */
extern Vec4 g_RouteSceneryPosition;
#define g_RouteSceneryX g_RouteSceneryPosition.x
#define g_RouteSceneryY g_RouteSceneryPosition.y
#define g_RouteSceneryZ g_RouteSceneryPosition.z

extern s16 g_ShuttlePathDwellMax[];

void InterpolateTrackPoint(s32 pointIndex, s32* out, s32 weight);

extern s16 g_PresentationSceneryFrame;
extern s32 g_PresentationSceneryTint;
extern s16 g_PresentationSceneryVariant;
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
extern s32 g_CameraCarTrackPoint;
extern u8 g_CameraModePrev;
extern s32 g_CameraNodeIndex;
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
typedef struct SceneryPlacement {
    LVec position;
    s32 yaw;
} SceneryPlacement;

typedef struct StaticSceneryState {
    SceneryPlacement standard;
    SceneryPlacement highClass;
} StaticSceneryState;

extern StaticSceneryState g_StaticSceneryState;
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

typedef struct PathSceneryRotationData {
    s16 firstKey[2];
    PathSceneryRotationKey keys[1];
} PathSceneryRotationData;

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
enum {
    SKY_TILE_MAP_ROWS = 5,
    SKY_TILE_MAP_COLUMNS = 16,
};
extern s16 g_SkyTileMap[SKY_TILE_MAP_ROWS][SKY_TILE_MAP_COLUMNS];
extern s16 g_SpinningSceneryAngle[];
extern u16 g_SpinningSceneryRate[];
typedef struct SpinningSceneryPlacement {
    LVec position;
    s32 yaw;
} SpinningSceneryPlacement;
_Static_assert(sizeof(SpinningSceneryPlacement) == 16,
               "SpinningSceneryPlacement must match the retail layout");
extern SpinningSceneryPlacement g_SpinningSceneryPlacements[4];
extern s32 g_StartGridSceneryAngle[];

s32 BlendAngle(s32 angleA, s32 angleB, s32 weight);
extern s32 FindNearestTrackCamera(struct GameRenderObject *car);
void LerpEnvColor(union GameEnvColor *from, union GameEnvColor *to,
                  union GameEnvColor *out, s32 blend);
void LoadEnvironmentCue(GameEnvironmentCue *cue);
void UpdateTrackEventSound(s16 trackSection);

extern Vec4 g_AnimSceneryPos[];
extern SVec g_ShuttlePathAngles[];
extern Vec4 g_StartGridSceneryPos[];

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
#define TERRAIN_CELL_GRID_BYTES                                            \
    (TERRAIN_CELL_GRID_SIZE * TERRAIN_CELL_GRID_SIZE * sizeof(u16))
#define CELL_VISIBILITY_TABLE_SIZE                                         \
    (TERRAIN_CELL_GRID_SIZE * sizeof(CellVisibilityRow))

/* Whether the visibility mask holds the cell a world point falls in. The mask
 * is one bit per 2048-unit cell, a word per row of z and a bit per column of
 * x. */
int TrackCellVisible(s32 x, s32 z);

#endif
