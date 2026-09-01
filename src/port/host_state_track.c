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
#include <stddef.h>

#include "common.h"

typedef struct SceneryMotionData SceneryMotionData;
typedef struct PathSceneryRotationData PathSceneryRotationData;
typedef struct PathSceneryPositionData PathSceneryPositionData;

unsigned char g_StartGridSceneryStep[8] __attribute__((aligned(16))) = {0x48,0x00,0x04,0x00,0xbc,0xff,0xf2,0xff};
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
unsigned char g_SpinningSceneryRate[8] __attribute__((aligned(16))) =
    " \0"
    "@";
unsigned char g_ShuttlePathPoints[96] __attribute__((aligned(16))) = {0xac,0x51,0x00,0x00,0x74,0x0c,0x00,0x00,0x3b,0x94,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x6b,0x00,0x00,0xfa,0x00,0x00,0x00,0xdf,0x84,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x1b,0x00,0x00,0xf7,0x04,0x00,0x00,0xb3,0x79,0x00,0x00,0x00,0x00,0x00,0x00,0xfb,0x25,0x00,0x00,0x0b,0x06,0x00,0x00,0x06,0x72,0x00,0x00,0x00,0x00,0x00,0x00,0x22,0x26,0x00,0x00,0x1e,0x06,0x00,0x00,0x20,0x71,0x00,0x00,0x00,0x00,0x00,0x00,0xe7,0x1a,0x00,0x00,0xfb,0x04,0x00,0x00,0x05,0x79,0x00,0x00,0x00,0x00,0x00,0x00};
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
unsigned char g_SkyTileMap[160] __attribute__((aligned(16))) = {0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00};
unsigned char g_SkyTileUV[88] __attribute__((aligned(16))) = {0x00,0x00,0x40,0x00,0x00,0x7f,0x40,0x7f,0x40,0x00,0x80,0x00,0x40,0x7f,0x80,0x7f,0x80,0x00,0xc0,0x00,0x80,0x7f,0xc0,0x7f,0xc0,0x00,0xff,0x00,0xc0,0x7f,0xff,0x7f,0x00,0x80,0x40,0x80,0x00,0xff,0x40,0xff,0x40,0x80,0x80,0x80,0x40,0xff,0x80,0xff,0x80,0x80,0xc0,0x80,0x80,0xff,0xc0,0xff,0xc0,0x80,0xff,0x80,0xc0,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x16,0x01,0x80,0x2c,0x16,0x01,0x80};
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
/* GameEnvironmentColors: 112 bytes. Writing through the declared type ran 2
 * bytes past this object, into whatever the linker placed next. */
unsigned char g_EnvironmentColors[112] __attribute__((aligned(16)));
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
/* One host object, not a chain of PS1-address aliases.  The recovered game
 * accesses this symbol as FlybySceneryState (52 bytes). */
unsigned char g_FlybyScenery[52] __attribute__((aligned(16)));
s32 g_RouteSceneryClock;
s32 g_RouteSceneryFrame;
s16 g_RouteSceneryArmed;
s16 g_RouteSceneryKeyIndex;
s32 g_RouteSceneryRotX;
s32 g_RouteSceneryRotY;
unsigned char g_RouteSceneryRotZ[12] __attribute__((aligned(16)));
unsigned char g_FlybySceneryKeyframe[8] __attribute__((aligned(16)));
SceneryMotionData *g_FlybySceneryData;
unsigned char g_CourseObjects[8] __attribute__((aligned(16)));
unsigned char g_CellVisibilityTable[8] __attribute__((aligned(16)));
PathSceneryPositionData *g_PathSceneryPosData;
s32 g_CourseObjectCount;
unsigned char g_VisibleCellList[8] __attribute__((aligned(16)));
unsigned char g_PathSceneryClock[8] __attribute__((aligned(16)));
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
