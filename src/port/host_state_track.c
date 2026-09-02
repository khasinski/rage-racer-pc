/*
 * Retail state describing the course and what stands beside it: the terrain
 * cell grid and the visibility lists built from it, the track's arc centres
 * and camera positions, and the scenery animation state for the route,
 * flyby, path, shuttle and spinning objects.
 *
 * Several of these are read by the port's own shims as often as by the track
 * code, which is why the reader tally alone would have left them in
 * host_state.c. They are here because they describe the course; a shim
 * copying a cell list across does not own it. Order is retail's address
 * order.
 */

#include "game/vector.h"
#include "game/environment.h"
#include <stddef.h>

#include "common.h"

typedef struct SceneryMotionData SceneryMotionData;
typedef struct PathSceneryRotationData PathSceneryRotationData;
typedef struct PathSceneryPositionData PathSceneryPositionData;
typedef struct SceneryMotionKeyframe SceneryMotionKeyframe;
typedef struct StartGridSceneryStep {
    s16 x;
    s16 y;
} StartGridSceneryStep;
typedef struct PathSceneryClock {
    s16 posFrame;
    s16 rotFrame;
} PathSceneryClock;
typedef struct SpinningSceneryPlacement {
    LVec position;
    s32 yaw;
} SpinningSceneryPlacement;
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
typedef struct SceneryPlacement {
    LVec position;
    s32 yaw;
} SceneryPlacement;
typedef struct StaticSceneryState {
    SceneryPlacement standard;
    SceneryPlacement highClass;
} StaticSceneryState;
typedef struct ShuttlePath {
    Vec4 endpoint[2];
} ShuttlePath;
typedef union SkyUV {
    struct {
        u8 u;
        u8 v;
    } bytes;
    u16 packed;
} SkyUV;
typedef struct SkyTileUV {
    SkyUV corner[4];
} SkyTileUV;

enum {
    SKY_TILE_COUNT = 8,
};

StartGridSceneryStep g_StartGridSceneryStep[2]
    __attribute__((aligned(16))) = {
        {72, 4},
        {-68, -14},
    };
StaticSceneryState g_StaticSceneryState = {
    {{40594, 6002, 11940}, 0x440},
    {{29266, 6039, 45612}, 0x655},
};
Vec4 g_StartGridSceneryPos[2] __attribute__((aligned(16))) = {
    {46685, 6010, 12495, 0},
    {39567, 5782, 11986, 0}
};
s32 g_StartGridSceneryAngle[2] __attribute__((aligned(16))) = {
    2926, 5078
};
Vec4 g_AnimSceneryPos[2] __attribute__((aligned(16))) = {
    {39491, 5473, 11857, 3150},
    {20717, 5792, 10467, 2691}
};
s16 g_AnimSceneryPitch[4] __attribute__((aligned(16))) = {
    -111, 0, 0, 0
};
s32 g_AnimSceneryTint;
s16 g_AnimSceneryRacePosition;
s16 g_AnimSceneryFrame;
s32 g_AnimScenery2Tint;
s16 g_AnimScenery2Variant;
s16 g_AnimScenery2Frame;
u16 g_SpinningSceneryRate[4] __attribute__((aligned(16))) = {
    32, 64, 0, 0
};
s16 g_SpinningSceneryAngle[4] __attribute__((aligned(16))) = {
    0, 64, 128, 256
};
SpinningSceneryPlacement g_SpinningSceneryPlacements[4]
    __attribute__((aligned(16))) = {
        {{17805, 5646, 44714}, 590},
        {{30065, 3143, 40558}, 1995},
        {{30171, 3054, 38836}, 2007},
        {{30888, 2954, 37357}, 2005},
    };
ShuttlePath g_ShuttlePathPoints[3] __attribute__((aligned(16))) = {
    {{{20908, 3188, 37947, 0}, {27399, 250, 34015, 0}}},
    {{{6934, 1271, 31155, 0}, {9723, 1547, 29190, 0}}},
    {{{9762, 1566, 28960, 0}, {6887, 1275, 30981, 0}}},
};
SVec g_ShuttlePathAngles[3] __attribute__((aligned(16))) = {
    {0, 3736, -240, 0},
    {0, 10234, 0, 0},
    {0, 10234, 0, 0}
};
s16 g_ShuttlePathTravelMax[4] __attribute__((aligned(16))) = {
    628, 512, 512, 0
};
s16 g_ShuttlePathDwellMax[62] __attribute__((aligned(16))) = {
    300, 128, 128, 0, -17324, 0, 5903, 0, 12361, 0, 0, 0, 200, 180, -19127,
    0, 5903, 0, 12309, 0, 0, 0, 400, 180, -18381, 0, 5903, 0, 12444, 0, 0,
    0, 600, 180, -17324, 0, 5903, 0, 12361, 0, 0, 0, -1, 180, 200, 0, 0, 0,
    100, 100, 180, 2048, 200, 0, 200, 100, 200, 0, 0, 0, -1, 100
};
s16 g_SkyTileMap[5][16] __attribute__((aligned(16))) = {
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
    {4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3},
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
    {4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3},
};
_Static_assert(sizeof(g_SkyTileMap) == 160,
               "sky tile map must preserve the retail data block");
SkyTileUV g_SkyTileUV[SKY_TILE_COUNT] __attribute__((aligned(16))) = {
    {{{.bytes = {0, 0}}, {.bytes = {64, 0}},
      {.bytes = {0, 127}}, {.bytes = {64, 127}}}},
    {{{.bytes = {64, 0}}, {.bytes = {128, 0}},
      {.bytes = {64, 127}}, {.bytes = {128, 127}}}},
    {{{.bytes = {128, 0}}, {.bytes = {192, 0}},
      {.bytes = {128, 127}}, {.bytes = {192, 127}}}},
    {{{.bytes = {192, 0}}, {.bytes = {255, 0}},
      {.bytes = {192, 127}}, {.bytes = {255, 127}}}},
    {{{.bytes = {0, 128}}, {.bytes = {64, 128}},
      {.bytes = {0, 255}}, {.bytes = {64, 255}}}},
    {{{.bytes = {64, 128}}, {.bytes = {128, 128}},
      {.bytes = {64, 255}}, {.bytes = {128, 255}}}},
    {{{.bytes = {128, 128}}, {.bytes = {192, 128}},
      {.bytes = {128, 255}}, {.bytes = {192, 255}}}},
    {{{.bytes = {192, 128}}, {.bytes = {255, 128}},
      {.bytes = {192, 255}}, {.bytes = {255, 255}}}},
};
_Static_assert(sizeof(g_SkyTileUV) == 64,
               "sky UV records must contain exactly eight tiles");
s32 g_ChaseCameraPreset;
s32 g_OrbitCameraYaw;
s32 g_OrbitCameraDistance = 330;
s16 g_AnimSceneryVariant;
unsigned char g_CamPathOffsetDelta[12] __attribute__((aligned(16)));
unsigned char g_CamPathOffsetStart[12] __attribute__((aligned(16)));
unsigned char g_CamPathOffset[12] __attribute__((aligned(16)));
unsigned char g_CamPathAngleDelta[16] __attribute__((aligned(16)));
unsigned char g_ChaseYawPrev[8] __attribute__((aligned(16)));
unsigned char g_CamPathAngleStart[16] __attribute__((aligned(16)));
unsigned char g_CamPathAngle[16] __attribute__((aligned(16)));
u8 g_CameraModePrev;
s32 g_ChaseTargetYaw;
s32 g_ChaseYaw;
s32 g_ChaseYawLag;
s32 g_ChaseYawRampNeg;
s32 g_ChaseYawRampPos;
s32 g_ChaseYawStepLimit;
s32 g_ChaseYawStep;
s32 g_ChaseYawDamping;
s32 g_ChaseCarSpeed;
s32 g_CameraNodeIndex;
s32 g_CamPathFrame;
s32 g_CamPathNode;
s32 g_FogNear;
unsigned char g_MainVisibleCellList[1024] __attribute__((aligned(16)));
s32 g_EnvScriptLength;
unsigned char g_TrackCameras[8] __attribute__((aligned(16)));
unsigned char g_TrackArcCenters[8] __attribute__((aligned(16)));
unsigned char g_MainVisibleCellMask[128] __attribute__((aligned(16)));
u8 g_EnvScriptEnabled;
s32 g_EnvScriptClock;
u16 g_TrackSectionCount;
unsigned char g_CameraCarZ[28] __attribute__((aligned(16)));
s32 g_CameraCarTrackPoint;
s32 g_CameraCarHeading;
s32 g_CameraCarSpeed;
s32 g_CameraCarStepX;
unsigned char g_CameraCarStepZ[128] __attribute__((aligned(16)));
GameEnvironmentColors g_EnvironmentColors __attribute__((aligned(16)));
s16 g_EnvLerpFrame;
s16 g_EnvLerpDuration;
s16 g_EnvironmentMode;
s16 g_EnvSpareLerp;
s16 g_EnvSpareFrom;
s16 g_EnvSpareTo;
s32 g_IsEnvironmentMode4;
s32 g_CourseModelCount;
unsigned char g_EnvScriptCursor[40] __attribute__((aligned(16)));
SceneryMotionData *g_RouteSceneryData;
unsigned char g_EnvPaletteTable[8] __attribute__((aligned(16)));
s32 g_TerrainCellCount;
PathSceneryRotationData *g_PathSceneryRotData;
unsigned char g_PathSceneryPosKeys[8] __attribute__((aligned(16)));
unsigned char g_PathSceneryRotKeys[8] __attribute__((aligned(16)));
unsigned char g_EnvScriptCues[8] __attribute__((aligned(16)));
FlybySceneryState g_FlybyScenery;
s32 g_RouteSceneryClock;
s32 g_RouteSceneryFrame;
s16 g_RouteSceneryKeyIndex;
s32 g_RouteSceneryRotX;
s32 g_RouteSceneryRotY;
unsigned char g_RouteSceneryRotZ[12] __attribute__((aligned(16)));
SceneryMotionKeyframe *g_FlybySceneryKeyframe;
SceneryMotionData *g_FlybySceneryData;
unsigned char g_CourseObjects[8] __attribute__((aligned(16)));
unsigned char g_CellVisibilityTable[8] __attribute__((aligned(16)));
PathSceneryPositionData *g_PathSceneryPosData;
s32 g_CourseObjectCount;
unsigned char g_VisibleCellList[8] __attribute__((aligned(16)));
PathSceneryClock g_PathSceneryClock;
unsigned char g_PathSceneryTransform[24] __attribute__((aligned(16)));
unsigned char g_PathSceneryRotHalfDelta[8] __attribute__((aligned(16)));
unsigned char g_PathSceneryHalfDelta[8] __attribute__((aligned(16)));
unsigned char g_PathSceneryCursors[16] __attribute__((aligned(16)));
s32 g_PathSceneryVolume;
s32 g_EnvironmentModePrev;
/* The symbol map split the two retail 0x34-byte shuttle records after the
 * first 0x0e bytes.  Game code addresses the complete pair through this base. */
unsigned char g_ShuttleScenery[104] __attribute__((aligned(16)));
unsigned char g_TerrainCellGrid[8] __attribute__((aligned(16)));
unsigned char g_VisibleCellMask[8] __attribute__((aligned(16)));
unsigned char g_RouteSceneryKeyframe[8] __attribute__((aligned(16)));
unsigned char g_EnvironmentClut[208] __attribute__((aligned(16)));
s32 g_RaceCueFlags;
