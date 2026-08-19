#include <libgpu.h>
#include <libgte.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/asset.h"

#include "modern/scene_capture.h"
#include "native_geometry_diagnostics.h"
#include "native_geometry_interpolation.h"
#include "runtime_config.h"

extern int g_CourseModelCount;
extern int g_AnimTimer;
extern int g_SceneTimer;
void DpqColor(CVECTOR *source, long depthCue, CVECTOR *destination);
void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *base,
                     CVECTOR *out0, CVECTOR *out1, CVECTOR *out2);

/* Portable C counterpart of the hand-written MIPS/GTE model dispatcher in
 * main/render/terrain_submission.c.  Asset records stay in their retail
 * fixed-width format; only packet links and pointers use native types. */

enum {
    RAGE_MODEL_F4 = 0,
    RAGE_MODEL_FT4 = 1,
    RAGE_MODEL_G4 = 2,
    RAGE_MODEL_GT4 = 3,
};

unsigned long long g_RageGt4FacesEmitted;
unsigned g_RageGt4ColorMaximum;
int g_RageGt4DepthMinimum = 0x7fffffff;
int g_RageGt4DepthMaximum = -0x7fffffff;
static int g_RageSubmittedModelIndex;
static int g_RageSubmittedModelType;
static int g_RageInsideModelProjection;
unsigned long long g_RageGt4ClipPositive;
unsigned long long g_RageGt4ClipNegative;
unsigned long long g_RageGt4RejectOffscreen;
unsigned long long g_RageGt4RejectBackface;
unsigned long long g_RageGt4RejectDepth;
unsigned long long g_RageModelRejectBackface;
unsigned long long g_RageTerrainSecondTriangleVisible;
unsigned long long g_RageTerrainChildRejectBackface;
unsigned long long g_RageTerrainChildSecondTriangleVisible;
static int g_RageTerrainClip0;
static int g_RageTerrainClip1;
static int g_RageProjectionReject;
static int g_RageProjectionFlag;
static RageGeometryDiagnostics s_diagnostics;
#define g_RageTerrainTraceEnabled s_diagnostics.terrainTraceEnabled
#define g_RageTerrainTraceTimer s_diagnostics.terrainTraceTimer
#define g_RageTerrainTraceClut s_diagnostics.terrainTraceClut
#define g_RageTerrainTraceTpage s_diagnostics.terrainTraceTpage
#define g_RageTerrainDecisionTraceEnabled s_diagnostics.terrainDecisionTraceEnabled
#define g_RageTerrainDecisionTraceTimer s_diagnostics.terrainDecisionTraceTimer
#define g_RageTerrainDecisionTraceLimit s_diagnostics.terrainDecisionTraceLimit
#define g_RageTerrainDecisionTraceCount s_diagnostics.terrainDecisionTraceCount
#define g_RageCourseTraceEnabled s_diagnostics.courseTraceEnabled
#define g_RageCourseTraceTimer s_diagnostics.courseTraceTimer
#define g_RageCourseTraceClut s_diagnostics.courseTraceClut
#define g_RageCourseTraceTpage s_diagnostics.courseTraceTpage
#define g_RageModelTraceEnabled s_diagnostics.modelTraceEnabled
#define g_RageModelTraceTimer s_diagnostics.modelTraceTimer
static long g_RageCourseVertexDepth[4];




static void RageTraceTerrainDecision(
    int cell, int face, uint16_t clut, uint16_t tpage, int projected,
    const uint16_t vertexIndices[4], const VECTOR *translation,
    int clip0, int clip1, const int sxy[4], int rawDepth, int depth) {
    const char *reason;
    if (!g_RageTerrainDecisionTraceEnabled ||
        (g_RageTerrainDecisionTraceTimer >= 0 &&
         g_RageTerrainDecisionTraceTimer != g_SceneTimer) ||
        g_RageTerrainDecisionTraceCount >= g_RageTerrainDecisionTraceLimit)
        return;
    reason = g_RageProjectionReject == 1 ? "offscreen" :
        g_RageProjectionReject == 2 ? "backface" :
        g_RageProjectionReject == 3 ? "depth" : "unknown";
    if (projected)
        fprintf(stderr,
                "terrain-decision timer=%d index=%d cell=%d face=%d mirror=%d "
                "clut=%04x tpage=%04x vertices=%u,%u,%u,%u "
                "translation=%d,%d,%d clip=%d,%d "
                "sxy=%d,%d/%d,%d/%d,%d/%d,%d bounds=%d,%d,%d,%d "
                "raw=%d depth=%d result=%s\n",
                g_SceneTimer, g_RageTerrainDecisionTraceCount, cell, face,
                RENDER_MIRROR, clut, tpage & 0x9ff,
                vertexIndices[0], vertexIndices[1], vertexIndices[2],
                vertexIndices[3], translation->vx, translation->vy,
                translation->vz, clip0, clip1,
                (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                (int16_t)sxy[3], (int16_t)(sxy[3] >> 16),
                g_RenderWorkspace.x0, g_RenderWorkspace.x1,
                g_RenderWorkspace.y0, g_RenderWorkspace.y1,
                rawDepth, depth, "submit");
    else
        fprintf(stderr,
                "terrain-decision timer=%d index=%d cell=%d face=%d mirror=%d "
                "clut=%04x tpage=%04x vertices=%u,%u,%u,%u "
                "translation=%d,%d,%d clip=%d,%d "
                "sxy=%d,%d/%d,%d/%d,%d/%d,%d bounds=%d,%d,%d,%d "
                "raw=na depth=na result=reject "
                "reason=%s\n",
                g_SceneTimer, g_RageTerrainDecisionTraceCount, cell, face,
                RENDER_MIRROR, clut, tpage & 0x9ff,
                vertexIndices[0], vertexIndices[1], vertexIndices[2],
                vertexIndices[3], translation->vx, translation->vy,
                translation->vz, clip0, clip1,
                (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                (int16_t)sxy[3], (int16_t)(sxy[3] >> 16),
                g_RenderWorkspace.x0, g_RenderWorkspace.x1,
                g_RenderWorkspace.y0, g_RenderWorkspace.y1, reason);
    g_RageTerrainDecisionTraceCount++;
}


static int RagePrimitiveSpaceAvailable(const uint8_t *cursor, size_t size) {
    int i;
    for (i = 0; i < 2; i++) {
        const uint8_t *begin = g_FrameContexts[i].layout.primitiveBuffer;
        const uint8_t *end = begin + sizeof(g_FrameContexts[i].layout.primitiveBuffer);
        if (cursor >= begin && cursor <= end) {
            if (size <= (size_t)(end - cursor)) return 1;
            fprintf(stderr,
                    "rage geometry: primitive buffer exhausted (used=%zu capacity=%zu)\n",
                    (size_t)(cursor - begin), (size_t)(end - begin));
            return 0;
        }
    }
    fprintf(stderr, "rage geometry: primitive cursor outside frame buffers\n");
    return 0;
}

static uint16_t RageReadU16(const uint8_t *p) {
    uint16_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static uint32_t RageReadU32(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static int RageCourseTraceMatches(int timer, int type, const uint8_t *face) {
    uint16_t clut;
    uint16_t tpage;
    if (!g_RageCourseTraceEnabled || type == 0) return 0;
    clut = RageReadU16(face + 14);
    tpage = RageReadU16(face + 18) & 0x9ff;
    return (g_RageCourseTraceTimer < 0 || g_RageCourseTraceTimer == timer) &&
           (g_RageCourseTraceClut < 0 || g_RageCourseTraceClut == clut) &&
           (g_RageCourseTraceTpage < 0 || g_RageCourseTraceTpage == tpage);
}

static void RageStoreSxy(short *x, short *y, int packed) {
    *x = (short)(packed & 0xffff);
    *y = (short)((uint32_t)packed >> 16);
}

static int RageProjectQuad(
    const SVECTOR *v0, const SVECTOR *v1, const SVECTOR *v2,
    const SVECTOR *v3, int sxy[4], int *depth, int *fog, int *rawDepth,
    int terrainQuad) {
    int p;
    int flag;
    long otz;

    otz = RotAverage4((SVECTOR *)v0, (SVECTOR *)v1, (SVECTOR *)v2,
                      (SVECTOR *)v3, &sxy[0], &sxy[1], &sxy[2], &sxy[3],
                      &p, &flag);
    g_RageProjectionFlag = flag;
    g_RageProjectionReject = 0;
    if (fog != NULL) *fog = p;
    if (rawDepth != NULL) *rawDepth = (int)otz;
    {
        int x[4], y[4], i;
        int allLeft = 1, allRight = 1, allAbove = 1, allBelow = 1;
        /* A widened modern view accepts faces the 4:3 screen rect would
         * cull; the compat image is unchanged because the PS1 drawing area
         * still clips them. Never widen the mirror's deliberate bounds. */
        int marginX = RENDER_MIRROR ? 0 : RageModernCullMarginX();
        for (i = 0; i < 4; i++) {
            x[i] = (int16_t)(sxy[i] & 0xffff);
            y[i] = (int16_t)((uint32_t)sxy[i] >> 16);
            allLeft &= x[i] < g_RenderWorkspace.x0 - marginX;
            allRight &= x[i] > g_RenderWorkspace.x1 + marginX;
            allAbove &= y[i] < g_RenderWorkspace.y0;
            allBelow &= y[i] > g_RenderWorkspace.y1;
        }
        if (allLeft || allRight || allAbove || allBelow) {
            g_RageProjectionReject = 1;
            return 0;
        }
    }
    {
        int clip0 = NormalClip(sxy[0], sxy[1], sxy[2]);
        int clip1 = terrainQuad
            ? NormalClip(sxy[1], sxy[2], sxy[3]) : clip0;
        if (terrainQuad) {
            g_RageTerrainClip0 = clip0;
            g_RageTerrainClip1 = clip1;
        }
        if (g_RageInsideModelProjection && g_RageSubmittedModelIndex == 0 &&
            g_RageSubmittedModelType == RAGE_MODEL_GT4) {
            if (clip0 > 0) g_RageGt4ClipPositive++;
            if (clip0 < 0) g_RageGt4ClipNegative++;
        }
        /* Model faces use the single NCLIP performed by SubmitModelFaces.
         * The terrain dispatcher is different: after RTPT it runs NCLIP for
         * v0/v1/v2, projects v3 with RTPS, then runs NCLIP again on the GTE
         * FIFO (v1/v2/v3).  That second triangle has the opposite winding in
         * the retail four-corner order.  Retail keeps the quad when either
         * triangle faces the camera.  Applying the model rule to terrain
         * drops whole quads whenever only their first half is degenerate. */
        if (terrainQuad &&
            ((!RENDER_MIRROR && clip0 <= 0 && clip1 < 0) ||
             (RENDER_MIRROR && clip0 >= 0 && clip1 > 0))) {
            if (terrainQuad == 2)
                g_RageTerrainChildSecondTriangleVisible++;
            else
                g_RageTerrainSecondTriangleVisible++;
        }
        /* SubmitModelFaces has only one NCLIP result.  Reflection reverses
         * projected winding, so the mirror pass must accept the opposite
         * sign from the main pass.  The ordering flag only selects the OT;
         * it must not turn back-facing model polygons into front faces.
         * Do not feed the duplicated clip0 into
         * the terrain two-half expression: that reduces both rejection
         * tests to clip0 == 0 and submits every back-facing model face. */
        *depth = ((int)otz >> RENDER_OT_SHIFT);
        if ((!terrainQuad && !RageModelFaceVisible(RENDER_MIRROR, clip0)) ||
            (terrainQuad &&
             ((!RENDER_MIRROR && clip0 <= 0 && clip1 >= 0) ||
              (RENDER_MIRROR && clip0 >= 0 && clip1 <= 0)))) {
            g_RageProjectionReject = 2;
            if (!terrainQuad) g_RageModelRejectBackface++;
            if (terrainQuad == 2) g_RageTerrainChildRejectBackface++;
            return 0;
        }
    }
    /* Subdivided terrain children inherit the parent's OT slot.  Retail
     * projects them with RTPT/NCLIP only and never runs AVSZ4 per child. */
    if (terrainQuad != 2 && (*depth <= 0 || *depth >= 448)) {
        g_RageProjectionReject = 3;
        return 0;
    }
    return 1;
}

static int RageProjectModelFace(
    const uint8_t *face, const SVECTOR *vertices, int sxy[4], int *depth,
    int *fog) {
    return RageProjectQuad(
        &vertices[RageReadU16(face + 0)], &vertices[RageReadU16(face + 2)],
        &vertices[RageReadU16(face + 4)], &vertices[RageReadU16(face + 6)],
        sxy, depth, fog, NULL, 0);
}

static int RageProjectCourseFace(
    const uint8_t *face, const SVECTOR *vertices, int sxy[4], int *depth,
    int *fog, int *rawDepth) {
    int p = 0;
    int flag = 0;
    int vertexFlag;
    long vertexDepth[4];
    int vertex;
    for (vertex = 0; vertex < 4; vertex++) {
        vertexDepth[vertex] = RotTransPers(
            (SVECTOR *)&vertices[RageReadU16(face + vertex * 2)],
            &sxy[vertex], &p, &vertexFlag);
        flag |= vertexFlag;
        g_RageCourseVertexDepth[vertex] = vertexDepth[vertex];
    }
    /* The retail course transform first stores each GTE Z in its 16-bit
     * scratch table at 1/8 of RotTransPers' result.  The face loop then adds
     * the already-quantized first and fourth values and shifts by three
     * (0x8002a37c..0x8002a43c).  Quantize before adding: averaging the wider
     * values first rounds some faces into the next OT bucket. */
    long otz = (vertexDepth[0] + vertexDepth[3]) >> 1;
    long retailDepth = ((vertexDepth[0] >> 3) +
                        (vertexDepth[3] >> 3)) >> 3;
    int clip = NormalClip(sxy[0], sxy[1], sxy[2]);
    int i;
    int allLeft = 1, allRight = 1, allAbove = 1, allBelow = 1;
    (void)flag;
    g_RageProjectionReject = 0;
    if ((!RENDER_MIRROR && clip <= 0) || (RENDER_MIRROR && clip >= 0)) {
        g_RageProjectionReject = 2;
        return 0;
    }
    {
        int marginX = RENDER_MIRROR ? 0 : RageModernCullMarginX();
        for (i = 0; i < 4; i++) {
            int x = (int16_t)sxy[i];
            int y = (int16_t)(sxy[i] >> 16);
            allLeft &= x < g_RenderWorkspace.x0 - marginX;
            allRight &= x > g_RenderWorkspace.x1 + marginX;
            allAbove &= y < g_RenderWorkspace.y0;
            allBelow &= y > g_RenderWorkspace.y1;
        }
    }
    if (allLeft || allRight || allAbove || allBelow) {
        g_RageProjectionReject = 1;
        return 0;
    }
    *depth = (int)retailDepth;
    if (fog != NULL) *fog = p;
    if (rawDepth != NULL) *rawDepth = (int)otz;
    if (*depth <= 0 || *depth >= 448) {
        g_RageProjectionReject = 3;
        return 0;
    }
    return 1;
}

static int RageCourseScreenQuadVisible(const int sxy[4]) {
    int i;
    int clip0 = NormalClip(sxy[0], sxy[1], sxy[2]);
    int clip1 = NormalClip(sxy[3], sxy[1], sxy[2]);
    int allLeft = 1, allRight = 1, allAbove = 1, allBelow = 1;
    /* The second half is evaluated as v3/v1/v2, i.e. with the opposite
     * winding from v0/v1/v2.  Reject only when both halves are back-facing.
     * The old comparisons did the inverse and discarded every front-facing
     * child as soon as a course quad entered its near subdivision path. */
    if (!RageCourseQuadVisible(RENDER_MIRROR, clip0, clip1))
        return 0;
    for (i = 0; i < 4; i++) {
        int x = (int16_t)sxy[i];
        int y = (int16_t)(sxy[i] >> 16);
        allLeft &= x < g_RenderWorkspace.x0;
        allRight &= x > g_RenderWorkspace.x1;
        allAbove &= y < g_RenderWorkspace.y0;
        allBelow &= y > g_RenderWorkspace.y1;
    }
    return !(allLeft || allRight || allAbove || allBelow);
}

static uint8_t *RageEmitTerrainFt4(
    uint8_t *cursor, OT_TYPE *ot, int depth, int fog, int dispatch,
    const int sxy[4], const uint8_t uv[8], uint16_t clut, uint16_t tpage,
    const uint8_t color[3], uint32_t textureWindow, int subdivided) {
    POLY_FT4 *poly;
    DR_TWIN *window;
    DR_TWIN *reset;
    CVECTOR base;
    CVECTOR shaded;
    size_t packetSize = sizeof(POLY_FT4);
    if (dispatch >= 2 && !subdivided)
        packetSize += sizeof(DR_TWIN) * 2;
    if (!RagePrimitiveSpaceAvailable(cursor, packetSize)) return NULL;
    poly = (POLY_FT4 *)cursor;
    SetPolyFT4(poly);
    poly->r0 = color[0]; poly->g0 = color[1]; poly->b0 = color[2];
    if ((poly->r0 | poly->g0 | poly->b0) == 0)
        poly->r0 = poly->g0 = poly->b0 = 0x80;
    if (dispatch == 0 || dispatch == 2) {
        base.r = poly->r0; base.g = poly->g0; base.b = poly->b0;
        base.cd = POLY_FT4_CODE;
        DpqColor(&base, fog, &shaded);
        poly->r0 = shaded.r; poly->g0 = shaded.g; poly->b0 = shaded.b;
    }
    poly->u0=uv[0]; poly->v0=uv[1]; poly->u1=uv[2]; poly->v1=uv[3];
    poly->u2=uv[4]; poly->v2=uv[5]; poly->u3=uv[6]; poly->v3=uv[7];
    poly->clut = (uint16_t)(clut + ((dispatch & 1) != 0));
    poly->tpage = tpage;
    RageStoreSxy(&poly->x0,&poly->y0,sxy[0]);
    RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
    RageStoreSxy(&poly->x2,&poly->y2,sxy[2]);
    RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
    cursor += sizeof(*poly);
    if (dispatch < 2) {
        AddPrim(&ot[depth], poly);
        return cursor;
    }
    if (subdivided) {
        AddPrim(&ot[depth], poly);
        return cursor;
    }

    reset = (DR_TWIN *)cursor;
    cursor += sizeof(*reset);
    setlen(reset, 2);
    reset->code[0] = 0xE2000000u;
    reset->code[1] = 0;

    window = (DR_TWIN *)cursor;
    cursor += sizeof(*window);
    setlen(window, 2);
    window->code[0] = 0xE2000000u;
    window->code[1] = 0xE2000000u | (textureWindow & 0x000FFFFFu);

    setaddr(reset, getaddr(&ot[depth]));
    setaddr(poly, reset);
    setaddr(window, poly);
    setaddr(&ot[depth], window);
    return cursor;
}

static uint8_t *RageEmitTerrainSubdivisionLines(
    uint8_t *cursor, OT_TYPE *ot, int depth, const int sxy[4],
    uint32_t command) {
    LINE_F3 *first;
    LINE_F3 *second;
    if (!RagePrimitiveSpaceAvailable(cursor, sizeof(LINE_F3) * 2))
        return NULL;
    first = (LINE_F3 *)cursor;
    second = first + 1;
    SetLineF3(first);
    SetLineF3(second);
    memcpy(&first->r0, &command, sizeof(command));
    memcpy(&second->r0, &command, sizeof(command));
    first->x0 = (int16_t)sxy[0];
    first->y0 = (int16_t)(sxy[0] >> 16);
    first->x1 = (int16_t)sxy[1];
    first->y1 = (int16_t)(sxy[1] >> 16);
    first->x2 = (int16_t)sxy[3];
    first->y2 = (int16_t)(sxy[3] >> 16);
    second->x0 = first->x0;
    second->y0 = first->y0;
    second->x1 = (int16_t)sxy[2];
    second->y1 = (int16_t)(sxy[2] >> 16);
    second->x2 = first->x2;
    second->y2 = first->y2;
    first->pad = 0x55555555;
    second->pad = 0x55555555;
    AddPrim(&ot[depth + 64], first);
    AddPrim(&ot[depth + 64], second);
    return cursor + sizeof(LINE_F3) * 2;
}

static uint8_t *RageEmitCourseFt4(
    uint8_t *cursor, OT_TYPE *ot, int depth, const int sxy[4],
    const uint8_t uv[8], uint16_t clut, uint16_t tpage,
    const uint8_t color[3], uint32_t textureWindow, int windowed) {
    POLY_FT4 *poly;
    DR_TWIN *window;
    DR_TWIN *reset;
    size_t packetSize = sizeof(POLY_FT4);
    if (windowed) packetSize += sizeof(DR_TWIN) * 2;
    if (!RagePrimitiveSpaceAvailable(cursor, packetSize)) return NULL;
    poly = (POLY_FT4 *)cursor;
    SetPolyFT4(poly);
    poly->r0 = color[0]; poly->g0 = color[1]; poly->b0 = color[2];
    poly->u0=uv[0]; poly->v0=uv[1]; poly->u1=uv[2]; poly->v1=uv[3];
    poly->u2=uv[4]; poly->v2=uv[5]; poly->u3=uv[6]; poly->v3=uv[7];
    poly->clut = clut;
    poly->tpage = tpage;
    RageStoreSxy(&poly->x0,&poly->y0,sxy[0]);
    RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
    RageStoreSxy(&poly->x2,&poly->y2,sxy[2]);
    RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
    cursor += sizeof(*poly);
    if (!windowed) {
        AddPrim(&ot[depth], poly);
        return cursor;
    }
    reset = (DR_TWIN *)cursor;
    cursor += sizeof(*reset);
    setlen(reset, 2);
    reset->code[0] = 0xE2000000u;
    reset->code[1] = 0;
    window = (DR_TWIN *)cursor;
    cursor += sizeof(*window);
    setlen(window, 2);
    window->code[0] = 0xE2000000u | (textureWindow & 0x000FFFFFu);
    window->code[1] = 0;
    setaddr(reset, getaddr(&ot[depth]));
    setaddr(poly, reset);
    setaddr(window, poly);
    setaddr(&ot[depth], window);
    return cursor;
}

static void RageCopyFt4UvWithMode(
    POLY_FT4 *poly, const uint8_t *record, uint32_t mode) {
    uint32_t uv0 = RageReadU32(record + 0) + mode;
    uint32_t uv1 = RageReadU32(record + 4);
    uint32_t uv23 = RageReadU32(record + 8);
    poly->u0 = (uint8_t)uv0;
    poly->v0 = (uint8_t)(uv0 >> 8);
    poly->clut = (uint16_t)(uv0 >> 16);
    poly->u1 = (uint8_t)uv1;
    poly->v1 = (uint8_t)(uv1 >> 8);
    poly->tpage = (uint16_t)(uv1 >> 16);
    poly->u2 = (uint8_t)uv23;
    poly->v2 = (uint8_t)(uv23 >> 8);
    poly->u3 = (uint8_t)(uv23 >> 16);
    poly->v3 = (uint8_t)(uv23 >> 24);
}

static void RageCopyGt4UvWithMode(
    POLY_GT4 *poly, const uint8_t *record, uint32_t mode) {
    uint32_t uv0 = RageReadU32(record + 0) + mode;
    uint32_t uv1 = RageReadU32(record + 4);
    uint32_t uv23 = RageReadU32(record + 8);
    poly->u0 = (uint8_t)uv0;
    poly->v0 = (uint8_t)(uv0 >> 8);
    poly->clut = (uint16_t)(uv0 >> 16);
    poly->u1 = (uint8_t)uv1;
    poly->v1 = (uint8_t)(uv1 >> 8);
    poly->tpage = (uint16_t)(uv1 >> 16);
    poly->u2 = (uint8_t)uv23;
    poly->v2 = (uint8_t)(uv23 >> 8);
    poly->u3 = (uint8_t)(uv23 >> 16);
    poly->v3 = (uint8_t)(uv23 >> 24);
}

static void RageSubmitModelFaces(
    int type, int count, const uint8_t *faces, const SVECTOR *vertices,
    const SVECTOR *normals) {
    static const uint8_t strides[4] = {16, 24, 24, 32};
    uint8_t *cursor = RENDER_PRIM_CURSOR_AS(uint8_t);
    /* Retail seeds t5 with scratch OT + 0x200 bytes before dispatching any
     * model face.  On PS1 that is 128 four-byte OT entries. */
    OT_TYPE *ot = RENDER_OT_BASE_AS(OT_TYPE) + 128;
    int i;

    RageGeometryDiagnosticsInit(&s_diagnostics);

    if ((unsigned)type >= 4 || count <= 0 || faces == NULL || vertices == NULL)
        return;

    for (i = 0; i < count; i++, faces += strides[type]) {
        int sxy[4];
        int depth;
        if (type == RAGE_MODEL_GT4 && g_RageSubmittedModelIndex == 0) {
            int vertex;
            for (vertex = 0; vertex < 4; vertex++) {
                VECTOR transformed;
                int flag;
                RotTrans((SVECTOR *)&vertices[RageReadU16(faces + vertex * 2)],
                         &transformed, &flag);
                if (transformed.vz < g_RageGt4DepthMinimum)
                    g_RageGt4DepthMinimum = transformed.vz;
                if (transformed.vz > g_RageGt4DepthMaximum)
                    g_RageGt4DepthMaximum = transformed.vz;
            }
        }
        g_RageSubmittedModelType = type;
        g_RageInsideModelProjection = 1;
        if (!RageProjectModelFace(faces, vertices, sxy, &depth, NULL)) {
            g_RageInsideModelProjection = 0;
            if (type == RAGE_MODEL_GT4 && g_RageSubmittedModelIndex == 0) {
                if (g_RageProjectionReject == 1) g_RageGt4RejectOffscreen++;
                if (g_RageProjectionReject == 2) g_RageGt4RejectBackface++;
                if (g_RageProjectionReject == 3) g_RageGt4RejectDepth++;
            }
            continue;
        }
        g_RageInsideModelProjection = 0;
        if (depth <= 0 || depth >= 448) continue;
        if (g_RageModelTraceEnabled &&
            (g_RageModelTraceTimer < 0 ||
             g_RageModelTraceTimer == g_SceneTimer)) {
            char recordBytes[65];
            int byteIndex;
            for (byteIndex = 0; byteIndex < strides[type]; byteIndex++)
                snprintf(recordBytes + byteIndex * 2, 3, "%02x", faces[byteIndex]);
            fprintf(stderr,
                    "model-face timer=%d model=%d type=%d face=%d depth=%d bias=%d "
                    "packet=%p mode=%08x record=%p record_bytes=%s "
                    "indices=%u,%u,%u,%u "
                    "sxy=%d,%d/%d,%d/%d,%d/%d,%d\n",
                    g_SceneTimer, g_RageSubmittedModelIndex, type, i, depth,
                    (int8_t)faces[strides[type] - 3],
                    (void *)cursor, (unsigned)RENDER_ENV_MODE4,
                    (const void *)faces, recordBytes,
                    RageReadU16(faces), RageReadU16(faces + 2),
                    RageReadU16(faces + 4), RageReadU16(faces + 6),
                    (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                    (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                    (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                    (int16_t)sxy[3], (int16_t)(sxy[3] >> 16));
        }
        /* Every model record ends with a signed OT adjustment.  Apply it
         * only after retail's range test of the projected parent depth. */
        depth += (int8_t)faces[strides[type] - 3];
        if (type == RAGE_MODEL_F4) {
            if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_F4))) break;
            POLY_F4 *poly = (POLY_F4 *)cursor;
            SetPolyF4(poly);
            poly->r0 = faces[8]; poly->g0 = faces[9]; poly->b0 = faces[10];
            RageStoreSxy(&poly->x0, &poly->y0, sxy[0]);
            RageStoreSxy(&poly->x1, &poly->y1, sxy[1]);
            RageStoreSxy(&poly->x2, &poly->y2, sxy[2]);
            RageStoreSxy(&poly->x3, &poly->y3, sxy[3]);
            AddPrim(&ot[depth], poly);
            cursor += sizeof(*poly);
            {
                RageCaptureFaceInput capture = {0};
                capture.kind = RAGE_CAPTURE_KIND_MODEL;
                capture.klass = 0;
                capture.bias = (int8_t)faces[strides[type] - 3];
                capture.otDepth = depth;
                capture.fog = -1;
                capture.cellSlot = -1;
                capture.v[0] = &vertices[RageReadU16(faces + 0)];
                capture.v[1] = &vertices[RageReadU16(faces + 2)];
                capture.v[2] = &vertices[RageReadU16(faces + 4)];
                capture.v[3] = &vertices[RageReadU16(faces + 6)];
                capture.colors = &poly->r0;
                capture.colorCount = 1;
                RageCaptureFace3D(&capture);
            }
        } else if (type == RAGE_MODEL_FT4) {
            if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_FT4))) break;
            POLY_FT4 *poly = (POLY_FT4 *)cursor;
            SetPolyFT4(poly);
            SetShadeTex(poly, 1);
            poly->r0 = poly->g0 = poly->b0 = 0x80;
            RageCopyFt4UvWithMode(poly, faces + 8,
                                  (uint32_t)RENDER_ENV_MODE4);
            RageStoreSxy(&poly->x0, &poly->y0, sxy[0]);
            RageStoreSxy(&poly->x1, &poly->y1, sxy[1]);
            RageStoreSxy(&poly->x2, &poly->y2, sxy[2]);
            RageStoreSxy(&poly->x3, &poly->y3, sxy[3]);
            AddPrim(&ot[depth], poly);
            cursor += sizeof(*poly);
            {
                uint8_t uv[8] = {poly->u0, poly->v0, poly->u1, poly->v1,
                                 poly->u2, poly->v2, poly->u3, poly->v3};
                RageCaptureFaceInput capture = {0};
                capture.kind = RAGE_CAPTURE_KIND_MODEL;
                capture.klass = 1;
                capture.raw = 1; /* forced command byte 0x2D */
                capture.bias = (int8_t)faces[strides[type] - 3];
                capture.otDepth = depth;
                capture.fog = -1;
                capture.cellSlot = -1;
                capture.v[0] = &vertices[RageReadU16(faces + 0)];
                capture.v[1] = &vertices[RageReadU16(faces + 2)];
                capture.v[2] = &vertices[RageReadU16(faces + 4)];
                capture.v[3] = &vertices[RageReadU16(faces + 6)];
                capture.uv = uv;
                capture.clut = poly->clut;
                capture.tpage = poly->tpage;
                capture.colors = &poly->r0;
                capture.colorCount = 1;
                RageCaptureFace3D(&capture);
            }
        } else if (type == RAGE_MODEL_G4) {
            if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_G4))) break;
            POLY_G4 *poly = (POLY_G4 *)cursor;
            CVECTOR base = *(const CVECTOR *)(faces + 16);
            CVECTOR colors[4];
            SetPolyG4(poly);
            NormalColorCol3(
                (SVECTOR *)&normals[RageReadU16(faces + 8)],
                (SVECTOR *)&normals[RageReadU16(faces + 10)],
                (SVECTOR *)&normals[RageReadU16(faces + 12)], &base,
                &colors[0], &colors[1], &colors[2]);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 14)], &base,
                           &colors[3]);
            if (g_RageModelTraceEnabled &&
                (g_RageModelTraceTimer < 0 ||
                 g_RageModelTraceTimer == g_SceneTimer)) {
                fprintf(stderr,
                        "model-color timer=%d model=%d type=%d face=%d "
                        "normal=%u,%u,%u,%u rgb=%02x%02x%02x/%02x%02x%02x/"
                        "%02x%02x%02x/%02x%02x%02x\n",
                        g_SceneTimer, g_RageSubmittedModelIndex, type, i,
                        RageReadU16(faces + 8), RageReadU16(faces + 10),
                        RageReadU16(faces + 12), RageReadU16(faces + 14),
                        colors[0].r, colors[0].g, colors[0].b,
                        colors[1].r, colors[1].g, colors[1].b,
                        colors[2].r, colors[2].g, colors[2].b,
                        colors[3].r, colors[3].g, colors[3].b);
            }
            poly->r0=colors[0].r; poly->g0=colors[0].g; poly->b0=colors[0].b;
            poly->r1=colors[1].r; poly->g1=colors[1].g; poly->b1=colors[1].b;
            poly->r2=colors[2].r; poly->g2=colors[2].g; poly->b2=colors[2].b;
            poly->r3=colors[3].r; poly->g3=colors[3].g; poly->b3=colors[3].b;
            RageStoreSxy(&poly->x0,&poly->y0,sxy[0]); RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
            RageStoreSxy(&poly->x2,&poly->y2,sxy[2]); RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
            AddPrim(&ot[depth], poly); cursor += sizeof(*poly);
            {
                RageCaptureFaceInput capture = {0};
                capture.kind = RAGE_CAPTURE_KIND_MODEL;
                capture.klass = 2;
                capture.bias = (int8_t)faces[strides[type] - 3];
                capture.otDepth = depth;
                capture.fog = -1;
                capture.cellSlot = -1;
                capture.v[0] = &vertices[RageReadU16(faces + 0)];
                capture.v[1] = &vertices[RageReadU16(faces + 2)];
                capture.v[2] = &vertices[RageReadU16(faces + 4)];
                capture.v[3] = &vertices[RageReadU16(faces + 6)];
                capture.colors = &colors[0].r;
                capture.colorCount = 4;
                RageCaptureFace3D(&capture);
            }
        } else {
            if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_GT4))) break;
            POLY_GT4 *poly = (POLY_GT4 *)cursor;
            CVECTOR base = {RENDER_GT4_R, RENDER_GT4_G, RENDER_GT4_B,
                            RENDER_GT4_CODE};
            CVECTOR colors[4];
            SetPolyGT4(poly);
            RageCopyGt4UvWithMode(poly, faces + 16,
                                  (uint32_t)RENDER_ENV_MODE4);
            NormalColor3(
                (SVECTOR *)&normals[RageReadU16(faces + 8)],
                (SVECTOR *)&normals[RageReadU16(faces + 10)],
                (SVECTOR *)&normals[RageReadU16(faces + 12)],
                &base,
                &colors[0], &colors[1], &colors[2]);
            NormalColor((SVECTOR *)&normals[RageReadU16(faces + 14)], &base,
                        &colors[3]);
            if (g_RageModelTraceEnabled &&
                (g_RageModelTraceTimer < 0 ||
                 g_RageModelTraceTimer == g_SceneTimer)) {
                fprintf(stderr,
                        "model-color timer=%d model=%d type=%d face=%d "
                        "normal=%u,%u,%u,%u rgb=%02x%02x%02x/%02x%02x%02x/"
                        "%02x%02x%02x/%02x%02x%02x\n",
                        g_SceneTimer, g_RageSubmittedModelIndex, type, i,
                        RageReadU16(faces + 8), RageReadU16(faces + 10),
                        RageReadU16(faces + 12), RageReadU16(faces + 14),
                        colors[0].r, colors[0].g, colors[0].b,
                        colors[1].r, colors[1].g, colors[1].b,
                        colors[2].r, colors[2].g, colors[2].b,
                        colors[3].r, colors[3].g, colors[3].b);
            }
            poly->r0=colors[0].r; poly->g0=colors[0].g; poly->b0=colors[0].b;
            poly->r1=colors[1].r; poly->g1=colors[1].g; poly->b1=colors[1].b;
            poly->r2=colors[2].r; poly->g2=colors[2].g; poly->b2=colors[2].b;
            poly->r3=colors[3].r; poly->g3=colors[3].g; poly->b3=colors[3].b;
            {
                int vertex;
                for (vertex = 0; vertex < 4; vertex++) {
                    unsigned value = colors[vertex].r;
                    if (colors[vertex].g > value) value = colors[vertex].g;
                    if (colors[vertex].b > value) value = colors[vertex].b;
                    if (value > g_RageGt4ColorMaximum)
                        g_RageGt4ColorMaximum = value;
                }
                g_RageGt4FacesEmitted++;
            }
            RageStoreSxy(&poly->x0,&poly->y0,sxy[0]); RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
            RageStoreSxy(&poly->x2,&poly->y2,sxy[2]); RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
            AddPrim(&ot[depth], poly); cursor += sizeof(*poly);
            {
                uint8_t uv[8] = {poly->u0, poly->v0, poly->u1, poly->v1,
                                 poly->u2, poly->v2, poly->u3, poly->v3};
                RageCaptureFaceInput capture = {0};
                capture.kind = RAGE_CAPTURE_KIND_MODEL;
                capture.klass = 3;
                capture.bias = (int8_t)faces[strides[type] - 3];
                capture.otDepth = depth;
                capture.fog = -1;
                capture.cellSlot = -1;
                capture.v[0] = &vertices[RageReadU16(faces + 0)];
                capture.v[1] = &vertices[RageReadU16(faces + 2)];
                capture.v[2] = &vertices[RageReadU16(faces + 4)];
                capture.v[3] = &vertices[RageReadU16(faces + 6)];
                capture.uv = uv;
                capture.clut = poly->clut;
                capture.tpage = poly->tpage;
                capture.colors = &colors[0].r;
                capture.colorCount = 4;
                RageCaptureFace3D(&capture);
            }
        }
    }
    RENDER_PRIM_CURSOR_AS(uint8_t) = cursor;
}

void SubmitModel(void *ctx, int index) {
    uint8_t *stream;
    uint32_t opcode;
    void **models = (void **)RENDER_MODEL_MODELS;
    (void)ctx;
    if (models == NULL || index < 0 || models[index] == NULL) return;
    stream = (uint8_t *)models[index];
    g_RageSubmittedModelIndex = index;
    if (g_RageModelTraceEnabled &&
        (g_RageModelTraceTimer < 0 ||
         g_RageModelTraceTimer == g_SceneTimer))
        fprintf(stderr, "model-submit timer=%d model=%d table=%p stream=%p\n",
                g_SceneTimer, index, (void *)models, (void *)stream);
    RageCaptureModelBegin(RAGE_CAPTURE_KIND_MODEL, index, 0);
    while ((opcode = RageReadU32(stream)) != 0) {
        int type = opcode & 0xffff;
        int count = (int)(opcode >> 16);
        stream += 4;
        RageSubmitModelFaces(type, count, stream,
                             (const SVECTOR *)RENDER_MODEL_TABLE1,
                             (const SVECTOR *)RENDER_MODEL_NORMALS);
        if ((unsigned)type >= 4) break;
        stream += count * (const uint8_t[]){16,24,24,32}[type];
    }
    RageCaptureSubmitEnd();
}

/* Course and terrain decoders are implemented next. Keeping these explicit
 * non-rendering bodies here makes the remaining fidelity gap visible and
 * avoids hiding it among generic platform adapters. */
static void RageSubmitCourseModel(int index, int fogged) {
    static const uint8_t strides[4] = {16, 28, 32, 32};
    NativeCourseModel *models = (NativeCourseModel *)RENDER_COURSE_BANK;
    uint8_t *stream;
    uint8_t *cursor = RENDER_PRIM_CURSOR_AS(uint8_t);
    /* The course dispatcher applies the same fixed +0x200-byte OT base. */
    OT_TYPE *ot = RENDER_OT_BASE_AS(OT_TYPE) + 128;
    const SVECTOR *vertices;
    uint32_t opcode;
    RageGeometryDiagnosticsInit(&s_diagnostics);
    if (g_RageCourseTraceEnabled &&
        (g_RageCourseTraceTimer < 0 || g_RageCourseTraceTimer == g_SceneTimer)) {
        fprintf(stderr, "course-model timer=%d model=%d fogged=%d\n",
                g_SceneTimer, index, fogged);
    }
    if (models == NULL || index < 0 || index >= g_CourseModelCount ||
        models[index].geometry == NULL || models[index].model == NULL) return;
    RageCaptureModelBegin(RAGE_CAPTURE_KIND_COURSE, index, fogged);
    vertices = (const SVECTOR *)models[index].geometry;
    stream = (uint8_t *)models[index].model;
    while ((opcode = RageReadU32(stream)) != 0) {
        int type = opcode & 0xffff;
        int count = (int)(opcode >> 16);
        int stride;
        int i;
        stream += 4;
        if ((unsigned)type >= 4 || count <= 0) break;
        stride = strides[type];
        for (i = 0; i < count; i++, stream += stride) {
            int sxy[4] = {0}, depth = 0, fog = 0, rawDepth = 0;
            int projected;
            int trace = RageCourseTraceMatches(g_SceneTimer, type, stream);
            int bias = (int8_t)stream[type == 0 ? 13 : 25];
            uint8_t color[3] = {stream[8], stream[9], stream[10]};
            int extendedDepth = 0;
            projected = RageProjectCourseFace(stream, vertices, sxy, &depth,
                                               &fog, &rawDepth);
            if (trace) {
                int clip = projected ? NormalClip(sxy[0], sxy[1], sxy[2]) : 0;
                fprintf(stderr,
                        "course-face timer=%d model=%d type=%d face=%d "
                        "fogged=%d projected=%d reject=%d mirror=%d "
                        "idx=%u,%u,%u,%u clut=%04x tpage=%04x "
                        "z=%ld,%ld,%ld,%ld raw_depth=%d depth=%d bias=%d clip=%d "
                        "sxy=%d,%d/%d,%d/%d,%d/%d,%d "
                        "uv=%u,%u/%u,%u/%u,%u/%u,%u\n",
                        g_SceneTimer, index, type, i, fogged, projected,
                        projected ? 0 : g_RageProjectionReject,
                        RENDER_MIRROR, RageReadU16(stream + 0),
                        RageReadU16(stream + 2), RageReadU16(stream + 4),
                        RageReadU16(stream + 6), RageReadU16(stream + 14),
                        RageReadU16(stream + 18) & 0x9ff,
                        g_RageCourseVertexDepth[0], g_RageCourseVertexDepth[1],
                        g_RageCourseVertexDepth[2], g_RageCourseVertexDepth[3],
                        projected ? rawDepth : -1,
                        projected ? depth : -1, bias, clip,
                        (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                        (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                        (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                        (int16_t)sxy[3], (int16_t)(sxy[3] >> 16),
                        stream[12], stream[13], stream[16], stream[17],
                        stream[20], stream[21], stream[22], stream[23]);
            }
            if (!projected) {
                /* Beyond-cutoff faces feed only the modern far view. */
                if (g_RageProjectionReject == 3 &&
                    RageModernDepthLimit() > 0 && depth > 0 &&
                    depth < RageModernDepthLimit()) {
                    extendedDepth = 1;
                } else {
                    continue;
                }
            }
            if (fogged) {
                CVECTOR base = {color[0], color[1], color[2], 0};
                CVECTOR shaded;
                DpqColor(&base, fog, &shaded);
                color[0] = shaded.r;
                color[1] = shaded.g;
                color[2] = shaded.b;
            }
            /* Retail validates the un-biased parent OTZ in 1..447, then adds
             * the signed record bias to the already shifted +128 base. */
            depth += bias;
            {
                RageCaptureFaceInput capture = {0};
                uint8_t flat[4] = {color[0], color[1], color[2], 0};
                capture.kind = RAGE_CAPTURE_KIND_COURSE;
                capture.klass = type == 0 ? 0 : 1;
                capture.bias = bias;
                capture.otDepth = depth;
                capture.fogged = fogged;
                capture.fog = fogged ? fog : -1;
                capture.cellSlot = -1;
                capture.v[0] = &vertices[RageReadU16(stream + 0)];
                capture.v[1] = &vertices[RageReadU16(stream + 2)];
                capture.v[2] = &vertices[RageReadU16(stream + 4)];
                capture.v[3] = &vertices[RageReadU16(stream + 6)];
                capture.colors = flat;
                capture.colorCount = 1;
                if (type == 1) {
                    uint32_t uv0 = RageReadU32(stream + 12) +
                                   (uint32_t)RENDER_ENV_MODE4;
                    uint8_t uv[8] = {
                        (uint8_t)uv0, (uint8_t)(uv0 >> 8),
                        stream[16], stream[17],
                        stream[20], stream[21], stream[22], stream[23]
                    };
                    memcpy(capture.uvStorage, uv, sizeof(uv));
                    capture.uv = capture.uvStorage;
                    capture.clut = (uint16_t)(uv0 >> 16);
                    capture.tpage = RageReadU16(stream + 18);
                }
                /* Types 2/3 are captured at their subdivision site where the
                 * scrolled/mode-adjusted UVs are final. */
                if (type <= 1) RageCaptureFace3D(&capture);
            }
            if (extendedDepth && type <= 1) continue;
            if (type == 0) {
                if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_F4))) break;
                POLY_F4 *poly = (POLY_F4 *)cursor;
                SetPolyF4(poly);
                poly->r0=color[0]; poly->g0=color[1]; poly->b0=color[2];
                RageStoreSxy(&poly->x0,&poly->y0,sxy[0]); RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
                RageStoreSxy(&poly->x2,&poly->y2,sxy[2]); RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
                AddPrim(&ot[depth],poly); cursor += sizeof(*poly);
            } else if (type == 1) {
                uint8_t uv[8] = {
                    stream[12], stream[13], stream[16], stream[17],
                    stream[20], stream[21], stream[22], stream[23]
                };
                uint32_t uv0 = RageReadU32(stream + 12) +
                               (uint32_t)RENDER_ENV_MODE4;
                memcpy(&uv[0], &uv0, 2);
                uint8_t *next = RageEmitCourseFt4(
                    cursor, ot, depth, sxy, uv, (uint16_t)(uv0 >> 16),
                    RageReadU16(stream + 18), color, 0, 0);
                if (next == NULL) goto course_buffer_full;
                cursor = next;
            } else {
                uint8_t uvRecord[12];
                uint8_t uv[8];
                uint16_t clut;
                uint16_t tpage;
                uint32_t textureWindow = RageReadU32(stream + 28);
                int uLevel = stream[26] -
                    (rawDepth >> RENDER_FACE_OT_SHIFT);
                int vLevel = stream[27] -
                    (rawDepth >> RENDER_FACE_OT_SHIFT);
                int uSteps, vSteps;
                int sy, sx;
                memcpy(uvRecord, stream + 12, sizeof(uvRecord));
                {
                    uint32_t uv0 = RageReadU32(uvRecord) +
                                   (uint32_t)RENDER_ENV_MODE4;
                    memcpy(uvRecord, &uv0, sizeof(uv0));
                }
                if (type == 3) {
                    uint32_t scroll = (uint32_t)g_AnimTimer & 0x7f;
                    uint32_t uv0 = RageReadU32(uvRecord + 0) + scroll;
                    uint16_t uv1 = (uint16_t)(RageReadU16(uvRecord + 4) +
                                              scroll);
                    uint16_t uv2 = (uint16_t)(RageReadU16(uvRecord + 8) +
                                              scroll);
                    uint16_t uv3 = (uint16_t)(RageReadU16(uvRecord + 10) +
                                              scroll);
                    memcpy(uvRecord + 0, &uv0, sizeof(uv0));
                    memcpy(uvRecord + 4, &uv1, sizeof(uv1));
                    memcpy(uvRecord + 8, &uv2, sizeof(uv2));
                    memcpy(uvRecord + 10, &uv3, sizeof(uv3));
                }
                uv[0]=uvRecord[0]; uv[1]=uvRecord[1];
                uv[2]=uvRecord[4]; uv[3]=uvRecord[5];
                uv[4]=uvRecord[8]; uv[5]=uvRecord[9];
                uv[6]=uvRecord[10]; uv[7]=uvRecord[11];
                clut = RageReadU16(uvRecord + 2);
                tpage = RageReadU16(uvRecord + 6);
                /* Large animated course quads (notably the Lakeside Gate
                 * waterfall sheets) request more subdivision as the camera
                 * approaches.  Dropping the whole record above the native
                 * safety limit makes the texture abruptly disappear.  Keep
                 * the face and saturate tessellation at the highest level we
                 * can emit instead. */
                uLevel = RageClampSubdivisionLevel(uLevel);
                vLevel = RageClampSubdivisionLevel(vLevel);
                uSteps = 1 << uLevel;
                vSteps = 1 << vLevel;
                {
                    RageCaptureFaceInput capture = {0};
                    uint8_t flat[4] = {color[0], color[1], color[2], 0};
                    capture.kind = RAGE_CAPTURE_KIND_COURSE;
                    capture.klass = 1;
                    capture.bias = bias;
                    capture.otDepth = depth;
                    capture.fogged = fogged;
                    capture.fog = fogged ? fog : -1;
                    capture.cellSlot = -1;
                    capture.v[0] = &vertices[RageReadU16(stream + 0)];
                    capture.v[1] = &vertices[RageReadU16(stream + 2)];
                    capture.v[2] = &vertices[RageReadU16(stream + 4)];
                    capture.v[3] = &vertices[RageReadU16(stream + 6)];
                    memcpy(capture.uvStorage, uv, 8);
                    capture.uv = capture.uvStorage;
                    capture.clut = clut;
                    capture.tpage = tpage;
                    capture.textureWindow = textureWindow & 0xFFFFFu;
                    capture.colors = flat;
                    capture.colorCount = 1;
                    RageCaptureFace3D(&capture);
                }
                if (extendedDepth) continue;
                if (uSteps == 1 && vSteps == 1) {
                    uint8_t *next = RageEmitCourseFt4(
                        cursor, ot, depth, sxy, uv, clut, tpage, color,
                        textureWindow, 1);
                    if (next == NULL) goto course_buffer_full;
                    cursor = next;
                    continue;
                }
                for (sy = 0; sy < vSteps; sy++) {
                    for (sx = 0; sx < uSteps; sx++) {
                        int subSxy[4];
                        uint8_t subUv[8];
                        uint8_t *next;
                        subSxy[0] = RageBilerpSxy(sxy,sx,sy,uSteps,vSteps);
                        subSxy[1] = RageBilerpSxy(sxy,sx+1,sy,uSteps,vSteps);
                        subSxy[2] = RageBilerpSxy(sxy,sx,sy+1,uSteps,vSteps);
                        subSxy[3] = RageBilerpSxy(sxy,sx+1,sy+1,uSteps,vSteps);
#define RAGE_COURSE_SUB_UV(out, corner, uu, vv) do { \
    (out)[(corner)*2] = RageBilerpByte(uv[0],uv[2],uv[4],uv[6],(uu),(vv),uSteps,vSteps); \
    (out)[(corner)*2+1] = RageBilerpByte(uv[1],uv[3],uv[5],uv[7],(uu),(vv),uSteps,vSteps); \
} while (0)
                        RAGE_COURSE_SUB_UV(subUv,0,sx,sy);
                        RAGE_COURSE_SUB_UV(subUv,1,sx+1,sy);
                        RAGE_COURSE_SUB_UV(subUv,2,sx,sy+1);
                        RAGE_COURSE_SUB_UV(subUv,3,sx+1,sy+1);
#undef RAGE_COURSE_SUB_UV
                        if (!RageCourseScreenQuadVisible(subSxy)) continue;
                        next = RageEmitCourseFt4(
                            cursor, ot, depth, subSxy, subUv, clut, tpage,
                            color, textureWindow, 1);
                        if (next == NULL) goto course_buffer_full;
                        cursor = next;
                    }
                }
            }
        }
    }
    RENDER_PRIM_CURSOR_AS(uint8_t) = cursor;
    RageCaptureSubmitEnd();
    return;
course_buffer_full:
    fprintf(stderr, "rage course: primitive buffer exhausted\n");
    RENDER_PRIM_CURSOR_AS(uint8_t) = cursor;
    RageCaptureSubmitEnd();
}

void SubmitCourseModel(void *ctx, int index) {
    (void)ctx;
    RageSubmitCourseModel(index, 0);
}
void SubmitCourseModel2(void *ctx, int index) {
    (void)ctx;
    RageSubmitCourseModel(index, 1);
}
void SubmitTerrainCells(void *ctx, void *cells, int count) {
    static const uint8_t dispatchStride[4] = {32, 32, 36, 36};
    const int32_t *visible = (const int32_t *)cells;
    void **cellTable = (void **)RENDER_CELL_TABLE;
    const SVECTOR *vertices = (const SVECTOR *)RENDER_CELL_FACES;
    uint8_t *cursor = RENDER_PRIM_CURSOR_AS(uint8_t);
    /* func_80028E9C seeds its terrain OT register from scratch+4 + 0x200. */
    OT_TYPE *ot = RENDER_OT_BASE_AS(OT_TYPE) + 128;
    int cell;
    int decodedFaces = 0;
    int emittedFaces = 0;
    (void)ctx;
    RageGeometryDiagnosticsInit(&s_diagnostics);
    if (visible == NULL || cellTable == NULL || vertices == NULL) return;

    /* The hand-written retail dispatcher mirrors the active GTE view by
     * negating RT11, RT12 and RT13 when scratch+0x68 is set.  It intentionally
     * leaves that matrix installed: DrawCourseObjects and DrawCars share it
     * for the remainder of the rear-view pass.  The negation must therefore
     * happen IN PLACE on the shared scratch matrix, not on a local copy:
     * with only the GTE register updated, DrawCar composed rival cars with
     * the un-reflected matrix and the mirror showed them moving opposite to
     * the track.  EndMirrorPass restores the saved camera matrix. */
    if (RENDER_MIRROR) {
        RENDER_VIEW_MATRIX_GTE->m[0][0] = -RENDER_VIEW_MATRIX_GTE->m[0][0];
        RENDER_VIEW_MATRIX_GTE->m[0][1] = -RENDER_VIEW_MATRIX_GTE->m[0][1];
        RENDER_VIEW_MATRIX_GTE->m[0][2] = -RENDER_VIEW_MATRIX_GTE->m[0][2];
        SetRotMatrix(RENDER_VIEW_MATRIX_GTE);
    }
    /* Capture after the mirror matrix install so the batch GTE state is the
     * one the cells are actually projected with. */
    RageCaptureTerrainBegin(cells, count);

    for (cell = 0; cell < count; cell++, visible += 4) {
        uint8_t *stream;
        uint32_t opcode;
        VECTOR translation;
        int cellIndex = visible[3];
        int farCell = visible[2] >= 0xa000;
        if (cellIndex < 0 || cellIndex >= GAME_TERRAIN_CELL_LIMIT ||
            cellTable[cellIndex] == NULL) continue;
        /* The rear-view dispatcher reflects both RT1 and the already
         * transformed cell-center X.  Reflecting only the rotation row moves
         * otherwise correct terrain quads tens of pixels to the right. */
        translation.vx = RENDER_MIRROR ? -visible[0] : visible[0];
        translation.vy = visible[1];
        translation.vz = visible[2];
        translation.pad = 0;
        SetTransVector(&translation);
        stream = (uint8_t *)cellTable[cellIndex];

        while ((opcode = RageReadU32(stream)) != 0) {
            int mode = opcode & 0xffff;
            int faceCount = (int)(opcode >> 16);
            int dispatch;
            int stride;
            int faceIndex;
            stream += 4;
            if ((unsigned)mode >= 6 || faceCount <= 0) break;
            dispatch = mode < 2 ? mode * 2 + (RENDER_ENV_MODE4 != 0)
                                : mode - 2;
            stride = dispatchStride[dispatch];
            decodedFaces += faceCount;
            for (faceIndex = 0; faceIndex < faceCount;
                 faceIndex++, stream += stride) {
                const SVECTOR *v0 = &vertices[RageReadU16(stream + 0)];
                const SVECTOR *v1 = &vertices[RageReadU16(stream + 2)];
                const SVECTOR *v2 = &vertices[RageReadU16(stream + 4)];
                const SVECTOR *v3 = &vertices[RageReadU16(stream + 6)];
                int sxy[4], depth, fog, rawDepth;
                int uLevel, vLevel, uSteps, vSteps;
                uint8_t baseUv[8] = {
                    stream[8], stream[9], stream[12], stream[13],
                    stream[16], stream[17], stream[18], stream[19]
                };
                uint8_t color[3] = {stream[28], stream[29], stream[30]};
                uint16_t clut = RageReadU16(stream + 10);
                uint16_t tpage = RageReadU16(stream + 14);
                uint32_t textureWindow = dispatch >= 2
                    ? RageReadU32(stream + 32) : 0;
                uint16_t vertexIndices[4] = {
                    RageReadU16(stream + 0), RageReadU16(stream + 2),
                    RageReadU16(stream + 4), RageReadU16(stream + 6)
                };
                int bias;
                int extendedDepth = 0;
                if (farCell && (stream[20] & 2) != 0) {
                    /* Retail's far-cell path skips these records entirely;
                     * capture them for the modern renderer's continuous far
                     * road without emitting to the compat stream. */
                    if (RageModernDepthLimit() <= 0) continue;
                    extendedDepth = 1;
                }
                {
                    int projected = RageProjectQuad(
                        v0, v1, v2, v3, sxy, &depth, &fog, &rawDepth, 1);
                    if (g_RageTerrainTraceEnabled &&
                        (g_RageTerrainTraceTimer < 0 ||
                         g_RageTerrainTraceTimer == g_SceneTimer) &&
                        (g_RageTerrainTraceClut < 0 ||
                         g_RageTerrainTraceClut == clut) &&
                        (g_RageTerrainTraceTpage < 0 ||
                         g_RageTerrainTraceTpage == (tpage & 0x9ff))) {
                        fprintf(stderr,
                                "terrain-face timer=%d cell=%d face=%d packet=%p "
                                "mode=%d mirror=%d reject=%d depth=%d raw=%d fog=%d "
                                "bias=%d lod=%u,%u shift=%d "
                                "rgb=%02x%02x%02x clut=%04x tpage=%04x "
                                "window=%05x indices=%u,%u,%u,%u "
                                "translation=%d,%d,%d "
                                "sxy=%d,%d/%d,%d/%d,%d/%d,%d\n",
                                g_SceneTimer, cellIndex, faceIndex, (void *)cursor,
                                dispatch,
                                RENDER_MIRROR,
                                projected ? 0 : g_RageProjectionReject, depth,
                                rawDepth, fog, (int8_t)stream[21], stream[22], stream[23],
                                RENDER_FACE_OT_SHIFT,
                                color[0], color[1], color[2],
                                clut, tpage, textureWindow & 0xfffff,
                                RageReadU16(stream + 0), RageReadU16(stream + 2),
                                RageReadU16(stream + 4), RageReadU16(stream + 6),
                                translation.vx, translation.vy, translation.vz,
                                (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                                (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                                (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                                (int16_t)sxy[3], (int16_t)(sxy[3] >> 16));
                    }
                    RageTraceTerrainDecision(
                        cellIndex, faceIndex, clut, tpage, projected,
                        vertexIndices, &translation,
                        g_RageTerrainClip0, g_RageTerrainClip1, sxy,
                        rawDepth, depth);
                    if (!projected) {
                        /* Faces beyond the retail depth cutoff are captured
                         * for the modern renderer's extended draw distance
                         * but never emitted to the compat stream. Distant
                         * road quads are nearly edge-on, so their NCLIP
                         * orientation flickers; past bucket 96 (the same
                         * z 12288 as the far-field vertex snap) capture
                         * them regardless of facing - the depth buffer
                         * sorts them out. A higher cutoff left multi-row
                         * sky holes across the road where a whole stretch
                         * of strips was NCLIP-rejected in one frame. */
                        int reject = g_RageProjectionReject;
                        if ((reject == 3 || (reject == 2 && depth >= 96)) &&
                            RageModernDepthLimit() > 0 && depth > 0 &&
                            depth < RageModernDepthLimit()) {
                            extendedDepth = 1;
                        } else {
                            continue;
                        }
                    }
                }
                /* The retail cell dispatcher tests bit 0 of the halfword at
                 * +0x14 when OTZ reaches 0x800.  In that case it halves all
                 * four UV pairs before either the direct or subdivided
                 * emitter sees them.  Omitting this selected the other half
                 * of 4-bit terrain pages (and made a 0..127 tile become
                 * 0..254), which looked like missing/black terrain. */
                if (rawDepth >= 0x800 && (stream[20] & 1) != 0) {
                    int uvIndex;
                    for (uvIndex = 0; uvIndex < 8; uvIndex++)
                        baseUv[uvIndex] >>= 1;
                    /* The retail dispatcher applies the same far-face mode
                     * to the packed texture-window command.  This is not a
                     * generic GP0 conversion: it is part of Rage Racer's
                     * terrain record decoder at 0x80028608/0x80028760.
                     * Keeping the source window unchanged selects a
                     * different part of the page even though XY and UV are
                     * otherwise identical. */
                    if (dispatch >= 2) {
                        textureWindow = ((textureWindow >> 1) & 0x0007BFFFu) |
                                        0x00000210u;
                    }
                }
                bias = (int8_t)stream[21];
                uLevel = stream[22] - (rawDepth >> RENDER_FACE_OT_SHIFT);
                vLevel = stream[23] - (rawDepth >> RENDER_FACE_OT_SHIFT);
                if (uLevel < 0) uLevel = 0;
                if (vLevel < 0) vLevel = 0;
                if (g_RageTerrainTraceEnabled &&
                    (g_RageTerrainTraceTimer < 0 ||
                     g_RageTerrainTraceTimer == g_SceneTimer) &&
                    (g_RageTerrainTraceClut < 0 ||
                     g_RageTerrainTraceClut == clut) &&
                    (g_RageTerrainTraceTpage < 0 ||
                     g_RageTerrainTraceTpage == (tpage & 0x9ff))) {
                    fprintf(stderr,
                            "terrain-lod timer=%d cell=%d face=%d mirror=%d "
                            "raw=%d shift=%d source=%u,%u level=%d,%d "
                            "steps=%u,%u\n",
                            g_SceneTimer, cellIndex, faceIndex, RENDER_MIRROR,
                            rawDepth, RENDER_FACE_OT_SHIFT, stream[22],
                            stream[23], uLevel, vLevel,
                            uLevel < 31 ? 1u << uLevel : 0,
                            vLevel < 31 ? 1u << vLevel : 0);
                }
                if (uLevel > 6 || vLevel > 6) continue;
                uSteps = 1 << uLevel;
                vSteps = 1 << vLevel;
                {
                    /* Capture the parent quad exactly as the emitter shades
                     * it: zero colour promotes to 0x80 and odd dispatches
                     * select the adjacent CLUT row. */
                    RageCaptureFaceInput capture = {0};
                    uint8_t flat[4] = {color[0], color[1], color[2], 0};
                    if ((flat[0] | flat[1] | flat[2]) == 0)
                        flat[0] = flat[1] = flat[2] = 0x80;
                    capture.kind = RAGE_CAPTURE_KIND_TERRAIN;
                    capture.klass = 1;
                    capture.fogged = dispatch == 0 || dispatch == 2;
                    if (capture.fogged) {
                        /* Bake the same depth cue the emitter applies so the
                         * captured colour matches the compat packet. */
                        CVECTOR base = {flat[0], flat[1], flat[2],
                                        POLY_FT4_CODE};
                        CVECTOR shaded;
                        DpqColor(&base, fog, &shaded);
                        flat[0] = shaded.r;
                        flat[1] = shaded.g;
                        flat[2] = shaded.b;
                    }
                    capture.bias = bias;
                    capture.otDepth = depth + bias;
                    capture.fog = fog;
                    capture.cellSlot = cell;
                    capture.v[0] = v0;
                    capture.v[1] = v1;
                    capture.v[2] = v2;
                    capture.v[3] = v3;
                    memcpy(capture.uvStorage, baseUv, 8);
                    capture.uv = capture.uvStorage;
                    capture.clut = (uint16_t)(clut + ((dispatch & 1) != 0));
                    capture.tpage = tpage;
                    capture.textureWindow =
                        dispatch >= 2 ? (textureWindow & 0xFFFFFu) : 0;
                    capture.colors = flat;
                    capture.colorCount = 1;
                    RageCaptureFace3D(&capture);
                }
                if (extendedDepth) continue;
                if (uSteps == 1 && vSteps == 1) {
                    uint8_t *next;
                    depth += bias;
                    next = RageEmitTerrainFt4(cursor, ot, depth, fog, dispatch,
                                              sxy, baseUv, clut, tpage, color,
                                              textureWindow, 0);
                    if (next == NULL) goto terrain_buffer_full;
                    cursor = next;
                    emittedFaces++;
                } else {
                    int sy, sx;
                    DR_TWIN *subdivisionWindow = NULL;
                    uint32_t lineCommand = RageReadU32(stream + 24);
                    if (g_RageTerrainDecisionTraceEnabled &&
                        (g_RageTerrainDecisionTraceTimer < 0 ||
                         g_RageTerrainDecisionTraceTimer == g_SceneTimer)) {
                        fprintf(stderr,
                                "terrain-subdivision-lines timer=%d cell=%d face=%d "
                                "sxy=%d,%d/%d,%d/%d,%d/%d,%d flag=%08x "
                                "command=%08x emit=%d depth=%d\n",
                                g_SceneTimer, cellIndex, faceIndex,
                                (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                                (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                                (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                                (int16_t)sxy[3], (int16_t)(sxy[3] >> 16),
                                (uint32_t)g_RageProjectionFlag, lineCommand,
                                (((uint32_t)g_RageProjectionFlag | lineCommand) &
                                 0x80000000u) == 0,
                                depth + bias);
                    }
                    if ((((uint32_t)g_RageProjectionFlag | lineCommand) &
                         0x80000000u) == 0) {
                        uint8_t *next = RageEmitTerrainSubdivisionLines(
                            cursor, ot, depth + bias, sxy, lineCommand);
                        if (next == NULL) goto terrain_buffer_full;
                        cursor = next;
                    }
                    /* Retail's low subdivision byte is the outer v0-v1 /
                     * v2-v3 interpolation.  The high byte is the inner
                     * v0-v2 / v1-v3 row interpolation. */
                    for (sy = 0; sy < uSteps; sy++) {
                        for (sx = 0; sx < vSteps; sx++) {
                            int subSxy[4];
                            int subDepth = depth + bias;
                            int childDepth, childFog, childRawDepth;
                            SVECTOR child[4];
                            uint8_t uv[8];
                            uint8_t *next;
                            child[0] = RageBilerpVertex(
                                v0, v1, v2, v3, sy, sx, uSteps, vSteps);
                            child[1] = RageBilerpVertex(
                                v0, v1, v2, v3, sy + 1, sx,
                                uSteps, vSteps);
                            child[2] = RageBilerpVertex(
                                v0, v1, v2, v3, sy, sx + 1,
                                uSteps, vSteps);
                            child[3] = RageBilerpVertex(
                                v0, v1, v2, v3, sy + 1, sx + 1,
                                uSteps, vSteps);
                            {
                                int childVisible = RageProjectQuad(
                                    &child[0], &child[1], &child[2], &child[3],
                                    subSxy, &childDepth, &childFog,
                                    &childRawDepth, 2);
                                if (g_RageTerrainTraceEnabled &&
                                    (g_RageTerrainTraceTimer < 0 ||
                                     g_RageTerrainTraceTimer == g_SceneTimer) &&
                                    (g_RageTerrainTraceClut < 0 ||
                                     g_RageTerrainTraceClut == clut) &&
                                    (g_RageTerrainTraceTpage < 0 ||
                                     g_RageTerrainTraceTpage == (tpage & 0x9ff))) {
                                    fprintf(stderr,
                                            "terrain-nclip timer=%d cell=%d face=%d "
                                            "child=%d,%d/%d,%d mirror=%d "
                                            "clip=%d,%d visible=%d reject=%d\n",
                                            g_SceneTimer, cellIndex, faceIndex,
                                            sy, sx, uSteps, vSteps,
                                            RENDER_MIRROR, g_RageTerrainClip0,
                                            g_RageTerrainClip1, childVisible,
                                            childVisible ? 0 : g_RageProjectionReject);
                                }
                                if (!childVisible) continue;
                            }
                            /* The retail emitter brackets the complete set of
                             * visible children with one texture-window pair.
                             * Since an OT is LIFO, link the reset before the
                             * first child and the set command after the last. */
                            if (dispatch >= 2 && subdivisionWindow == NULL) {
                                DR_TWIN *reset;
                                if (!RagePrimitiveSpaceAvailable(
                                        cursor, sizeof(DR_TWIN) * 2))
                                    goto terrain_buffer_full;
                                reset = (DR_TWIN *)cursor;
                                subdivisionWindow = reset + 1;
                                cursor += sizeof(DR_TWIN) * 2;
                                setlen(reset, 2);
                                reset->code[0] = 0xE2000000u;
                                reset->code[1] = 0;
                                setlen(subdivisionWindow, 2);
                                subdivisionWindow->code[0] =
                                    0xE2000000u |
                                    (textureWindow & 0x000FFFFFu);
                                subdivisionWindow->code[1] = 0;
                                AddPrim(&ot[subDepth], reset);
                            }
#define RAGE_SUB_UV(out, corner, uu, vv) do { \
    (out)[(corner)*2] = RageBilerpByte(baseUv[0],baseUv[2],baseUv[4],baseUv[6],(uu),(vv),uSteps,vSteps); \
    (out)[(corner)*2+1] = RageBilerpByte(baseUv[1],baseUv[3],baseUv[5],baseUv[7],(uu),(vv),uSteps,vSteps); \
} while (0)
                            RAGE_SUB_UV(uv,0,sy,sx);
                            RAGE_SUB_UV(uv,1,sy+1,sx);
                            RAGE_SUB_UV(uv,2,sy,sx+1);
                            RAGE_SUB_UV(uv,3,sy+1,sx+1);
#undef RAGE_SUB_UV
                            {
                                if (g_RageTerrainTraceEnabled &&
                                    (g_RageTerrainTraceTimer < 0 ||
                                     g_RageTerrainTraceTimer == g_SceneTimer) &&
                                    (g_RageTerrainTraceClut < 0 ||
                                     g_RageTerrainTraceClut == clut) &&
                                    (g_RageTerrainTraceTpage < 0 ||
                                     g_RageTerrainTraceTpage ==
                                         (tpage & 0x9ff))) {
                                    fprintf(stderr,
                                            "terrain-child timer=%d cell=%d "
                                            "face=%d child=%d,%d/%d,%d packet=%p "
                                            "visible=%d mirror=%d depth=%d bias=%d ot=%d "
                                            "rgb=%02x%02x%02x "
                                            "xyz=%d,%d,%d/%d,%d,%d/"
                                            "%d,%d,%d/%d,%d,%d "
                                            "sxy=%d,%d/%d,%d/%d,%d/%d,%d "
                                            "uv=%u,%u/%u,%u/%u,%u/%u,%u\n",
                                            g_SceneTimer, cellIndex, faceIndex,
                                            sy, sx, uSteps, vSteps, (void *)cursor, 1,
                                            RENDER_MIRROR, depth, bias,
                                            subDepth + 128,
                                            color[0], color[1], color[2],
                                            child[0].vx, child[0].vy,
                                            child[0].vz, child[1].vx,
                                            child[1].vy, child[1].vz,
                                            child[2].vx, child[2].vy,
                                            child[2].vz, child[3].vx,
                                            child[3].vy, child[3].vz,
                                            (int16_t)subSxy[0],
                                            (int16_t)(subSxy[0] >> 16),
                                            (int16_t)subSxy[1],
                                            (int16_t)(subSxy[1] >> 16),
                                            (int16_t)subSxy[2],
                                            (int16_t)(subSxy[2] >> 16),
                                            (int16_t)subSxy[3],
                                            (int16_t)(subSxy[3] >> 16),
                                            uv[0], uv[1], uv[2], uv[3],
                                            uv[4], uv[5], uv[6], uv[7]);
                                }
                            }
                            next = RageEmitTerrainFt4(cursor,ot,subDepth,fog,
                                                      dispatch,subSxy,uv,clut,
                                                      tpage,color,textureWindow,
                                                      1);
                            if (next == NULL) goto terrain_buffer_full;
                            cursor = next;
                            emittedFaces++;
                        }
                    }
                    if (subdivisionWindow != NULL)
                        AddPrim(&ot[depth + bias], subdivisionWindow);
                }
            }
        }
    }
    RENDER_PRIM_CURSOR_AS(uint8_t) = cursor;
    RageCaptureSubmitEnd();
    return;
terrain_buffer_full:
    fprintf(stderr, "rage terrain: primitive buffer exhausted decoded=%d emitted=%d\n",
            decodedFaces, emittedFaces);
    RENDER_PRIM_CURSOR_AS(uint8_t) = cursor;
    RageCaptureSubmitEnd();
}
