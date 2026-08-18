#ifndef GAME_RENDER_WORKSPACE_H
#define GAME_RENDER_WORKSPACE_H

#include "common.h"
#include "game/vector.h"
#include "psyq/gte.h"

/* Native, pointer-safe state shared by render emitters during one frame. */
typedef struct RenderWorkspace {
    void *packetCursor;
    void *primData;
    s32 viewX;
    s32 viewY;
    s32 viewZ;
    s32 reserved14;
    s32 viewAngleX;
    s32 viewAngleY;
    s32 viewAngleZ;
    s32 depth;
    Matrix matrix;
    void *courseBank;
    void *modelModels;
    void *modelTable1;
    void *modelNormals;
    void *cellTable;
    void *cellFaces;
    s32 otShift;
    s32 orderingFlag;
    s32 faceOtShift;
    s32 mode;
    u8 ft4Color[4];
    u8 gt4Color[4];
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    s32 envMode4;
} RenderWorkspace;

/* Temporary transform storage passed between the camera and model submitters. */
typedef struct ObjectMatrixWork {
    s16 relative[3];
    s16 pad06;
    LVec view;
    s32 pad14;
    Matrix mtx;
} ObjectMatrixWork;

typedef union RenderViewCoordinate {
    s32 value;
    struct {
        u16 low;
        u16 high;
    } half;
} RenderViewCoordinate;

typedef struct RenderViewCoordinates {
    RenderViewCoordinate x;
    RenderViewCoordinate y;
    RenderViewCoordinate z;
} RenderViewCoordinates;

typedef union RenderViewPosition {
    RenderViewCoordinates components;
    LVec vector;
} RenderViewPosition;

typedef struct RenderViewState {
    RenderViewPosition position;
    s32 reserved14;
    s32 angleX;
    s32 angleY;
    s32 angleZ;
} RenderViewState;

extern RenderWorkspace g_RenderWorkspace;

#define RENDER_WORKSPACE (&g_RenderWorkspace)
#define RENDER_PRIM_CURSOR_AS(type) (*(type **)&g_RenderWorkspace.packetCursor)
#define RENDER_PRIM_CURSOR RENDER_PRIM_CURSOR_AS(void)
#define RENDER_PRIM_CURSOR_WORD (*(s32 *)&g_RenderWorkspace.packetCursor)
#define RENDER_PRIM_CURSOR_VOLATILE (*(u8 *volatile *)&g_RenderWorkspace.packetCursor)
#define RENDER_PRIM_CURSOR_SLOT (&RENDER_PRIM_CURSOR_VOLATILE)

#define RENDER_OT_BASE_AS(type) (*(type **)&g_RenderWorkspace.primData)
#define RENDER_OT_BASE RENDER_OT_BASE_AS(void)
#define RENDER_OT_BASE_WORD (*(s32 *)&g_RenderWorkspace.primData)
#define RENDER_OT_SHIFT (g_RenderWorkspace.otShift)

#define RENDER_VIEW_X (g_RenderWorkspace.viewX)
#define RENDER_VIEW_Y (g_RenderWorkspace.viewY)
#define RENDER_VIEW_Z (g_RenderWorkspace.viewZ)
#define RENDER_VIEW_ANGLE_X (g_RenderWorkspace.viewAngleX)
#define RENDER_VIEW_ANGLE_Y (g_RenderWorkspace.viewAngleY)
#define RENDER_VIEW_ANGLE_Z (g_RenderWorkspace.viewAngleZ)
#define RENDER_VIEW_POSITION_BLOCK ((Vec4 *)&g_RenderWorkspace.viewX)
#define RENDER_VIEW_STATE ((RenderViewState *)&g_RenderWorkspace.viewX)
#define RENDER_VIEW_MATRIX_GTE (&g_RenderWorkspace.matrix)

#define RENDER_COURSE_BANK (g_RenderWorkspace.courseBank)
#define RENDER_MODEL_MODELS (g_RenderWorkspace.modelModels)
#define RENDER_MODEL_TABLE1 (g_RenderWorkspace.modelTable1)
#define RENDER_MODEL_NORMALS (g_RenderWorkspace.modelNormals)
#define RENDER_CELL_TABLE (g_RenderWorkspace.cellTable)
#define RENDER_CELL_FACES (g_RenderWorkspace.cellFaces)
#define RENDER_FACE_OT_SHIFT (g_RenderWorkspace.faceOtShift)
#define RENDER_MIRROR (g_RenderWorkspace.orderingFlag)

#define RENDER_FT4_R (g_RenderWorkspace.ft4Color[0])
#define RENDER_FT4_G (g_RenderWorkspace.ft4Color[1])
#define RENDER_FT4_B (g_RenderWorkspace.ft4Color[2])
#define RENDER_FT4_CODE (g_RenderWorkspace.ft4Color[3])
#define RENDER_GT4_R (g_RenderWorkspace.gt4Color[0])
#define RENDER_GT4_G (g_RenderWorkspace.gt4Color[1])
#define RENDER_GT4_B (g_RenderWorkspace.gt4Color[2])
#define RENDER_GT4_CODE (g_RenderWorkspace.gt4Color[3])

#define RENDER_CLIP_X0 (*(u16 *)&g_RenderWorkspace.x0)
#define RENDER_CLIP_Y0 (*(u16 *)&g_RenderWorkspace.y0)
#define RENDER_CLIP_X1 (*(u16 *)&g_RenderWorkspace.x1)
#define RENDER_CLIP_Y1 (*(u16 *)&g_RenderWorkspace.y1)
#define RENDER_ENV_MODE4 (g_RenderWorkspace.envMode4)

#endif
