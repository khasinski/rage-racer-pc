#include <libgpu.h>
#include <libgte.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/asset.h"

extern int g_CourseModelCount;
extern int g_AnimTimer;
extern int g_SceneTimer;
void DpqColor(CVECTOR *source, long depthCue, CVECTOR *destination);

/* Portable C counterpart of the hand-written MIPS/GTE model dispatcher in
 * main/render/terrain_submission.c.  Asset records stay in their retail
 * fixed-width format; only packet links and pointers use native types. */

enum {
    RAGE_MODEL_F4 = 0,
    RAGE_MODEL_FT4 = 1,
    RAGE_MODEL_G4 = 2,
    RAGE_MODEL_GT4 = 3,
};

unsigned long long g_RageNearFacesCrossing;
unsigned long long g_RageNearTrianglesEmitted;
unsigned long long g_RageGt4FacesEmitted;
unsigned long long g_RageNearGt4TrianglesEmitted;
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
static int g_RageTerrainTraceInitialized;
static int g_RageTerrainTraceEnabled;
static int g_RageTerrainTraceTimer = -1;
static int g_RageTerrainTraceClut = -1;
static int g_RageTerrainTraceTpage = -1;
static int g_RageCourseTraceInitialized;
static int g_RageCourseTraceEnabled;
static int g_RageCourseTraceTimer = -1;
static int g_RageCourseTraceClut = -1;
static int g_RageCourseTraceTpage = -1;

static void RageInitializeTerrainTrace(void) {
    const char *timer;
    const char *clut;
    const char *tpage;
    if (g_RageTerrainTraceInitialized) return;
    g_RageTerrainTraceInitialized = 1;
    timer = getenv("RAGE_PORT_TERRAIN_TRACE_TIMER");
    clut = getenv("RAGE_PORT_TERRAIN_TRACE_CLUT");
    tpage = getenv("RAGE_PORT_TERRAIN_TRACE_TPAGE");
    if (timer != NULL) g_RageTerrainTraceTimer = (int)strtol(timer, NULL, 0);
    if (clut != NULL) g_RageTerrainTraceClut = (int)strtol(clut, NULL, 16);
    if (tpage != NULL) g_RageTerrainTraceTpage = (int)strtol(tpage, NULL, 16);
    g_RageTerrainTraceEnabled = timer != NULL || clut != NULL || tpage != NULL;
}

static void RageInitializeCourseTrace(void) {
    const char *timer;
    const char *clut;
    const char *tpage;
    if (g_RageCourseTraceInitialized) return;
    g_RageCourseTraceInitialized = 1;
    timer = getenv("RAGE_PORT_COURSE_TRACE_TIMER");
    clut = getenv("RAGE_PORT_COURSE_TRACE_CLUT");
    tpage = getenv("RAGE_PORT_COURSE_TRACE_TPAGE");
    if (timer != NULL) g_RageCourseTraceTimer = (int)strtol(timer, NULL, 0);
    if (clut != NULL) g_RageCourseTraceClut = (int)strtol(clut, NULL, 16);
    if (tpage != NULL) g_RageCourseTraceTpage = (int)strtol(tpage, NULL, 16);
    g_RageCourseTraceEnabled = timer != NULL || clut != NULL || tpage != NULL;
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
        for (i = 0; i < 4; i++) {
            x[i] = (int16_t)(sxy[i] & 0xffff);
            y[i] = (int16_t)((uint32_t)sxy[i] >> 16);
            allLeft &= x[i] < g_RageScratchpadState.x0;
            allRight &= x[i] > g_RageScratchpadState.x1;
            allAbove &= y[i] < g_RageScratchpadState.y0;
            allBelow &= y[i] > g_RageScratchpadState.y1;
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
            ((!SCRATCH_MIRROR && clip0 <= 0 && clip1 < 0) ||
             (SCRATCH_MIRROR && clip0 >= 0 && clip1 > 0))) {
            if (terrainQuad == 2)
                g_RageTerrainChildSecondTriangleVisible++;
            else
                g_RageTerrainSecondTriangleVisible++;
        }
        /* SubmitModelFaces has only one NCLIP result.  Its retail branches
         * accept clip > 0 in the main pass and clip >= 0 after the mirror
         * ordering flag is toggled.  Do not feed the duplicated clip0 into
         * the terrain two-half expression: that reduces both rejection
         * tests to clip0 == 0 and submits every back-facing model face. */
        if ((!terrainQuad &&
             ((!SCRATCH_MIRROR && clip0 <= 0) ||
              (SCRATCH_MIRROR && clip0 < 0))) ||
            (terrainQuad &&
             ((!SCRATCH_MIRROR && clip0 <= 0 && clip1 >= 0) ||
              (SCRATCH_MIRROR && clip0 >= 0 && clip1 <= 0)))) {
            g_RageProjectionReject = 2;
            if (!terrainQuad) g_RageModelRejectBackface++;
            if (terrainQuad == 2) g_RageTerrainChildRejectBackface++;
            return 0;
        }
    }
    *depth = ((int)otz >> SCRATCH_OT_SHIFT);
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
    int p;
    int flag;
    long otz = RotAverage4(
        (SVECTOR *)&vertices[RageReadU16(face + 0)],
        (SVECTOR *)&vertices[RageReadU16(face + 2)],
        (SVECTOR *)&vertices[RageReadU16(face + 4)],
        (SVECTOR *)&vertices[RageReadU16(face + 6)],
        &sxy[0], &sxy[1], &sxy[2], &sxy[3], &p, &flag);
    int clip = NormalClip(sxy[0], sxy[1], sxy[2]);
    int i;
    int allLeft = 1, allRight = 1, allAbove = 1, allBelow = 1;
    (void)flag;
    g_RageProjectionReject = 0;
    if ((!SCRATCH_MIRROR && clip <= 0) || (SCRATCH_MIRROR && clip >= 0)) {
        g_RageProjectionReject = 2;
        return 0;
    }
    for (i = 0; i < 4; i++) {
        int x = (int16_t)sxy[i];
        int y = (int16_t)(sxy[i] >> 16);
        allLeft &= x < g_RageScratchpadState.x0;
        allRight &= x > g_RageScratchpadState.x1;
        allAbove &= y < g_RageScratchpadState.y0;
        allBelow &= y > g_RageScratchpadState.y1;
    }
    if (allLeft || allRight || allAbove || allBelow) {
        g_RageProjectionReject = 1;
        return 0;
    }
    *depth = (int)otz >> SCRATCH_OT_SHIFT;
    if (*depth <= 0 || *depth >= 448) {
        g_RageProjectionReject = 3;
        return 0;
    }
    if (fog != NULL) *fog = p;
    if (rawDepth != NULL) *rawDepth = (int)otz;
    return 1;
}

static int RageFloorShift12(int64_t value) {
    if (value >= 0) return (int)(value >> 12);
    return -(int)((-value + 0xfff) >> 12);
}

/* GTE INTPL with sf=1/lm=0 as used by EmitSubdividedTerrainQuad.  Retail
 * programs IR0 to 4096-index*(4096/steps), FC to start and IR to end. */
static int RageIntplComponent(int start, int end, int index, int steps) {
    int ir0 = 4096 - index * (4096 / steps);
    int difference = start - end;
    int result;
    if (difference < -32768) difference = -32768;
    if (difference > 32767) difference = 32767;
    result = RageFloorShift12((int64_t)end * 4096 +
                              (int64_t)ir0 * difference);
    if (result < -32768) result = -32768;
    if (result > 32767) result = 32767;
    return result;
}

static int RageBilerpSxy(
    const int sxy[4], int u, int v, int uSteps, int vSteps) {
    int topX = RageIntplComponent((int16_t)sxy[0], (int16_t)sxy[1],
                                  u, uSteps);
    int bottomX = RageIntplComponent((int16_t)sxy[2], (int16_t)sxy[3],
                                     u, uSteps);
    int topY = RageIntplComponent((int16_t)(sxy[0] >> 16),
                                  (int16_t)(sxy[1] >> 16), u, uSteps);
    int bottomY = RageIntplComponent((int16_t)(sxy[2] >> 16),
                                     (int16_t)(sxy[3] >> 16), u, uSteps);
    int x = RageIntplComponent(topX, bottomX, v, vSteps);
    int y = RageIntplComponent(topY, bottomY, v, vSteps);
    return (int)((uint16_t)x | ((uint32_t)(uint16_t)y << 16));
}

static SVECTOR RageBilerpVertex(
    const SVECTOR *v0, const SVECTOR *v1, const SVECTOR *v2,
    const SVECTOR *v3, int outer, int inner, int outerSteps,
    int innerSteps) {
    SVECTOR result;
#define RAGE_BILERP_VERTEX_COMPONENT(field) \
    RageIntplComponent( \
        RageIntplComponent(v0->field, v1->field, outer, outerSteps), \
        RageIntplComponent(v2->field, v3->field, outer, outerSteps), \
        inner, innerSteps)
    result.vx = (short)RAGE_BILERP_VERTEX_COMPONENT(vx);
    result.vy = (short)RAGE_BILERP_VERTEX_COMPONENT(vy);
    result.vz = (short)RAGE_BILERP_VERTEX_COMPONENT(vz);
#undef RAGE_BILERP_VERTEX_COMPONENT
    result.pad = 0;
    return result;
}

static int RageCourseScreenQuadVisible(const int sxy[4]) {
    int i;
    int clip0 = NormalClip(sxy[0], sxy[1], sxy[2]);
    int clip1 = NormalClip(sxy[3], sxy[1], sxy[2]);
    int allLeft = 1, allRight = 1, allAbove = 1, allBelow = 1;
    if ((!SCRATCH_MIRROR && clip0 >= 0 && clip1 <= 0) ||
        (SCRATCH_MIRROR && clip0 <= 0 && clip1 >= 0))
        return 0;
    for (i = 0; i < 4; i++) {
        int x = (int16_t)sxy[i];
        int y = (int16_t)(sxy[i] >> 16);
        allLeft &= x < g_RageScratchpadState.x0;
        allRight &= x > g_RageScratchpadState.x1;
        allAbove &= y < g_RageScratchpadState.y0;
        allBelow &= y > g_RageScratchpadState.y1;
    }
    return !(allLeft || allRight || allAbove || allBelow);
}

static uint8_t RageBilerpByte(
    uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
    int u, int v, int uSteps, int vSteps) {
    int top = RageIntplComponent(c0, c1, u, uSteps);
    int bottom = RageIntplComponent(c2, c3, u, uSteps);
    return (uint8_t)RageIntplComponent(top, bottom, v, vSteps);
}

static uint8_t *RageEmitTerrainFt4(
    uint8_t *cursor, OT_TYPE *ot, int depth, int fog, int dispatch,
    const int sxy[4], const uint8_t uv[8], uint16_t clut, uint16_t tpage,
    const uint8_t color[3], uint32_t textureWindow) {
    POLY_FT4 *poly;
    DR_TWIN *window;
    DR_TWIN *reset;
    CVECTOR base;
    CVECTOR shaded;
    size_t packetSize = sizeof(POLY_FT4);
    if (dispatch >= 2) packetSize += sizeof(DR_TWIN) * 2;
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

static void RageCopyFt4Uv(POLY_FT4 *poly, const uint8_t *record) {
    RageCopyFt4UvWithMode(poly, record, 0);
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

static void RageCopyGt4Uv(POLY_GT4 *poly, const uint8_t *record) {
    RageCopyGt4UvWithMode(poly, record, 0);
}

typedef struct RageNearVertex {
    int64_t x, y, z;
    int u, v;
    int r, g, b;
} RageNearVertex;

static RageNearVertex RageInterpolateNearVertex(
    const RageNearVertex *a, const RageNearVertex *b, int64_t nearZ) {
    RageNearVertex out;
    int64_t numerator = nearZ - a->z;
    int64_t denominator = b->z - a->z;
#define RAGE_LERP(field) \
    (a->field + (int)((b->field - a->field) * numerator / denominator))
    out.x = a->x + (b->x - a->x) * numerator / denominator;
    out.y = a->y + (b->y - a->y) * numerator / denominator;
    out.z = nearZ;
    out.u = RAGE_LERP(u); out.v = RAGE_LERP(v);
    out.r = RAGE_LERP(r); out.g = RAGE_LERP(g); out.b = RAGE_LERP(b);
#undef RAGE_LERP
    return out;
}

static int RageClipTriangleNear(
    const RageNearVertex input[3], RageNearVertex output[4], int64_t nearZ) {
    int outCount = 0;
    int i;
    for (i = 0; i < 3; i++) {
        const RageNearVertex *a = &input[i];
        const RageNearVertex *b = &input[(i + 1) % 3];
        int aInside = a->z >= nearZ;
        int bInside = b->z >= nearZ;
        if (aInside) output[outCount++] = *a;
        if (aInside != bInside)
            output[outCount++] = RageInterpolateNearVertex(a, b, nearZ);
    }
    return outCount;
}

static int RageProjectNearVertex(const RageNearVertex *v) {
    int offsetX, offsetY;
    int64_t screen = ReadGeomScreen();
    int x, y;
    ReadGeomOffset(&offsetX, &offsetY);
    x = offsetX + (int)(v->x * screen / v->z);
    y = offsetY + (int)(v->y * screen / v->z);
    if (x < -1024) x = -1024;
    if (x > 1023) x = 1023;
    if (y < -1024) y = -1024;
    if (y > 1023) y = 1023;
    return (int)((uint16_t)x | ((uint32_t)(uint16_t)y << 16));
}

static uint8_t *RageEmitNearClippedModelFace(
    uint8_t *cursor, OT_TYPE *ot, int type, const uint8_t *face,
    const SVECTOR *vertices, const SVECTOR *normals, int bias) {
    static const int triangles[2][3] = {{0, 1, 2}, {1, 3, 2}};
    RageNearVertex source[4];
    uint16_t indices[4];
    CVECTOR lit[4];
    int uv[4][2] = {{0}};
    uint16_t clut = 0, tpage = 0;
    int i, triangle;
    int anyBehind = 0, anyInFront = 0;
    const int64_t nearZ = 32;

    for (i = 0; i < 4; i++) indices[i] = RageReadU16(face + i * 2);
    if (type == RAGE_MODEL_FT4) {
        POLY_FT4 temp = {0};
        RageCopyFt4UvWithMode(&temp, face + 8,
                              (uint32_t)g_ScratchRenderMode);
        uv[0][0]=temp.u0; uv[0][1]=temp.v0;
        uv[1][0]=temp.u1; uv[1][1]=temp.v1;
        uv[2][0]=temp.u2; uv[2][1]=temp.v2;
        uv[3][0]=temp.u3; uv[3][1]=temp.v3;
        clut=temp.clut; tpage=temp.tpage;
    } else if (type == RAGE_MODEL_GT4) {
        POLY_GT4 temp = {0};
        RageCopyGt4UvWithMode(&temp, face + 16,
                              (uint32_t)g_ScratchRenderMode);
        uv[0][0]=temp.u0; uv[0][1]=temp.v0;
        uv[1][0]=temp.u1; uv[1][1]=temp.v1;
        uv[2][0]=temp.u2; uv[2][1]=temp.v2;
        uv[3][0]=temp.u3; uv[3][1]=temp.v3;
        clut=temp.clut; tpage=temp.tpage;
    }
    if (type == RAGE_MODEL_G4 || type == RAGE_MODEL_GT4) {
        CVECTOR base = type == RAGE_MODEL_G4
            ? *(const CVECTOR *)(face + 16)
            : (CVECTOR){SCRATCH_GT4_R, SCRATCH_GT4_G, SCRATCH_GT4_B,
                        SCRATCH_GT4_CODE};
        for (i = 0; i < 4; i++)
            NormalColorCol((SVECTOR *)&normals[RageReadU16(face + 8 + i * 2)],
                           &base, &lit[i]);
    } else {
        for (i = 0; i < 4; i++) {
            lit[i].r = type == RAGE_MODEL_F4 ? face[8] : 0x80;
            lit[i].g = type == RAGE_MODEL_F4 ? face[9] : 0x80;
            lit[i].b = type == RAGE_MODEL_F4 ? face[10] : 0x80;
        }
    }
    for (i = 0; i < 4; i++) {
        VECTOR transformed;
        int flag;
        RotTrans((SVECTOR *)&vertices[indices[i]], &transformed, &flag);
        source[i].x=transformed.vx; source[i].y=transformed.vy;
        source[i].z=transformed.vz;
        source[i].u=uv[i][0]; source[i].v=uv[i][1];
        source[i].r=lit[i].r; source[i].g=lit[i].g; source[i].b=lit[i].b;
        anyBehind |= source[i].z < nearZ;
        anyInFront |= source[i].z >= nearZ;
    }
    if (!anyBehind || !anyInFront) return cursor;
    g_RageNearFacesCrossing++;

    for (triangle = 0; triangle < 2; triangle++) {
        RageNearVertex input[3], clipped[4];
        int count, fan;
        for (i = 0; i < 3; i++) input[i] = source[triangles[triangle][i]];
        count = RageClipTriangleNear(input, clipped, nearZ);
        for (fan = 1; fan + 1 < count; fan++) {
            RageNearVertex *v[3] = {&clipped[0], &clipped[fan],
                                      &clipped[fan + 1]};
            int sxy[3] = {RageProjectNearVertex(v[0]),
                          RageProjectNearVertex(v[1]),
                          RageProjectNearVertex(v[2])};
            int depth = (int)(((v[0]->z + v[1]->z + v[2]->z) / 3) >> 7)
                        + bias;
            if (depth <= 0) depth = 1;
            if (depth >= 448)
                continue;
            if (type == RAGE_MODEL_F4) {
                POLY_F3 *p;
                if (!RagePrimitiveSpaceAvailable(cursor, sizeof(*p))) return cursor;
                p=(POLY_F3 *)cursor; SetPolyF3(p);
                p->r0=v[0]->r; p->g0=v[0]->g; p->b0=v[0]->b;
                RageStoreSxy(&p->x0,&p->y0,sxy[0]);
                RageStoreSxy(&p->x1,&p->y1,sxy[1]);
                RageStoreSxy(&p->x2,&p->y2,sxy[2]);
                AddPrim(&ot[depth],p); cursor += sizeof(*p);
            } else if (type == RAGE_MODEL_FT4) {
                POLY_FT3 *p;
                if (!RagePrimitiveSpaceAvailable(cursor, sizeof(*p))) return cursor;
                p=(POLY_FT3 *)cursor; SetPolyFT3(p); SetShadeTex(p,1);
                p->r0=p->g0=p->b0=0x80; p->clut=clut; p->tpage=tpage;
                p->u0=v[0]->u; p->v0=v[0]->v;
                p->u1=v[1]->u; p->v1=v[1]->v;
                p->u2=v[2]->u; p->v2=v[2]->v;
                RageStoreSxy(&p->x0,&p->y0,sxy[0]);
                RageStoreSxy(&p->x1,&p->y1,sxy[1]);
                RageStoreSxy(&p->x2,&p->y2,sxy[2]);
                AddPrim(&ot[depth],p); cursor += sizeof(*p);
            } else if (type == RAGE_MODEL_G4) {
                POLY_G3 *p;
                if (!RagePrimitiveSpaceAvailable(cursor, sizeof(*p))) return cursor;
                p=(POLY_G3 *)cursor; SetPolyG3(p);
                p->r0=v[0]->r;p->g0=v[0]->g;p->b0=v[0]->b;
                p->r1=v[1]->r;p->g1=v[1]->g;p->b1=v[1]->b;
                p->r2=v[2]->r;p->g2=v[2]->g;p->b2=v[2]->b;
                RageStoreSxy(&p->x0,&p->y0,sxy[0]);
                RageStoreSxy(&p->x1,&p->y1,sxy[1]);
                RageStoreSxy(&p->x2,&p->y2,sxy[2]);
                AddPrim(&ot[depth],p); cursor += sizeof(*p);
            } else {
                POLY_GT3 *p;
                if (!RagePrimitiveSpaceAvailable(cursor, sizeof(*p))) return cursor;
                p=(POLY_GT3 *)cursor; SetPolyGT3(p);
                p->r0=v[0]->r;p->g0=v[0]->g;p->b0=v[0]->b;
                p->r1=v[1]->r;p->g1=v[1]->g;p->b1=v[1]->b;
                p->r2=v[2]->r;p->g2=v[2]->g;p->b2=v[2]->b;
                {
                    int vertex;
                    for (vertex = 0; vertex < 3; vertex++) {
                        unsigned value = v[vertex]->r;
                        if ((unsigned)v[vertex]->g > value) value = v[vertex]->g;
                        if ((unsigned)v[vertex]->b > value) value = v[vertex]->b;
                        if (value > g_RageGt4ColorMaximum)
                            g_RageGt4ColorMaximum = value;
                    }
                }
                p->clut=clut;p->tpage=tpage;
                p->u0=v[0]->u;p->v0=v[0]->v;
                p->u1=v[1]->u;p->v1=v[1]->v;
                p->u2=v[2]->u;p->v2=v[2]->v;
                RageStoreSxy(&p->x0,&p->y0,sxy[0]);
                RageStoreSxy(&p->x1,&p->y1,sxy[1]);
                RageStoreSxy(&p->x2,&p->y2,sxy[2]);
                AddPrim(&ot[depth],p); cursor += sizeof(*p);
                g_RageNearGt4TrianglesEmitted++;
            }
            g_RageNearTrianglesEmitted++;
        }
    }
    return cursor;
}

static void RageSubmitModelFaces(
    int type, int count, const uint8_t *faces, const SVECTOR *vertices,
    const SVECTOR *normals) {
    static const uint8_t strides[4] = {16, 24, 24, 32};
    uint8_t *cursor = SCRATCH_PRIM_CURSOR_AS(uint8_t);
    /* Retail seeds t5 with scratch OT + 0x200 bytes before dispatching any
     * model face.  On PS1 that is 128 four-byte OT entries. */
    OT_TYPE *ot = SCRATCH_OT_BASE_AS(OT_TYPE) + 128;
    int i;

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
        } else if (type == RAGE_MODEL_FT4) {
            if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_FT4))) break;
            POLY_FT4 *poly = (POLY_FT4 *)cursor;
            SetPolyFT4(poly);
            SetShadeTex(poly, 1);
            poly->r0 = poly->g0 = poly->b0 = 0x80;
            RageCopyFt4UvWithMode(poly, faces + 8,
                                  (uint32_t)g_ScratchRenderMode);
            RageStoreSxy(&poly->x0, &poly->y0, sxy[0]);
            RageStoreSxy(&poly->x1, &poly->y1, sxy[1]);
            RageStoreSxy(&poly->x2, &poly->y2, sxy[2]);
            RageStoreSxy(&poly->x3, &poly->y3, sxy[3]);
            AddPrim(&ot[depth], poly);
            cursor += sizeof(*poly);
        } else if (type == RAGE_MODEL_G4) {
            if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_G4))) break;
            POLY_G4 *poly = (POLY_G4 *)cursor;
            CVECTOR base = *(const CVECTOR *)(faces + 16);
            CVECTOR colors[4];
            SetPolyG4(poly);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 8)], &base,
                           &colors[0]);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 10)], &base,
                           &colors[1]);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 12)], &base,
                           &colors[2]);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 14)], &base,
                           &colors[3]);
            poly->r0=colors[0].r; poly->g0=colors[0].g; poly->b0=colors[0].b;
            poly->r1=colors[1].r; poly->g1=colors[1].g; poly->b1=colors[1].b;
            poly->r2=colors[2].r; poly->g2=colors[2].g; poly->b2=colors[2].b;
            poly->r3=colors[3].r; poly->g3=colors[3].g; poly->b3=colors[3].b;
            RageStoreSxy(&poly->x0,&poly->y0,sxy[0]); RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
            RageStoreSxy(&poly->x2,&poly->y2,sxy[2]); RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
            AddPrim(&ot[depth], poly); cursor += sizeof(*poly);
        } else {
            if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_GT4))) break;
            POLY_GT4 *poly = (POLY_GT4 *)cursor;
            CVECTOR base = {SCRATCH_GT4_R, SCRATCH_GT4_G, SCRATCH_GT4_B,
                            SCRATCH_GT4_CODE};
            CVECTOR colors[4];
            SetPolyGT4(poly);
            RageCopyGt4UvWithMode(poly, faces + 16,
                                  (uint32_t)g_ScratchRenderMode);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 8)], &base,
                           &colors[0]);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 10)], &base,
                           &colors[1]);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 12)], &base,
                           &colors[2]);
            NormalColorCol((SVECTOR *)&normals[RageReadU16(faces + 14)], &base,
                           &colors[3]);
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
        }
    }
    SCRATCH_PRIM_CURSOR_AS(uint8_t) = cursor;
}

void SubmitModel(void *ctx, int index) {
    uint8_t *stream;
    uint32_t opcode;
    void **models = (void **)SCRATCH_MODEL_MODELS;
    (void)ctx;
    if (models == NULL || index < 0 || models[index] == NULL) return;
    stream = (uint8_t *)models[index];
    g_RageSubmittedModelIndex = index;
    while ((opcode = RageReadU32(stream)) != 0) {
        int type = opcode & 0xffff;
        int count = (int)(opcode >> 16);
        stream += 4;
        RageSubmitModelFaces(type, count, stream,
                             (const SVECTOR *)SCRATCH_MODEL_TABLE1,
                             (const SVECTOR *)SCRATCH_MODEL_NORMALS);
        if ((unsigned)type >= 4) break;
        stream += count * (const uint8_t[]){16,24,24,32}[type];
    }
}

/* Course and terrain decoders are implemented next. Keeping these explicit
 * non-rendering bodies here makes the remaining fidelity gap visible and
 * avoids hiding it among generic platform adapters. */
static void RageSubmitCourseModel(int index, int fogged) {
    static const uint8_t strides[4] = {16, 28, 32, 32};
    NativeCourseModel *models = (NativeCourseModel *)SCRATCH_COURSE_BANK;
    uint8_t *stream;
    uint8_t *cursor = SCRATCH_PRIM_CURSOR_AS(uint8_t);
    /* The course dispatcher applies the same fixed +0x200-byte OT base. */
    OT_TYPE *ot = SCRATCH_OT_BASE_AS(OT_TYPE) + 128;
    const SVECTOR *vertices;
    uint32_t opcode;
    RageInitializeCourseTrace();
    if (models == NULL || index < 0 || index >= g_CourseModelCount ||
        models[index].geometry == NULL || models[index].model == NULL) return;
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
            int sxy[4], depth, fog, rawDepth;
            int projected;
            int trace = RageCourseTraceMatches(g_SceneTimer, type, stream);
            int bias = (int8_t)stream[type == 0 ? 13 : 25];
            uint8_t color[3] = {stream[8], stream[9], stream[10]};
            projected = RageProjectCourseFace(stream, vertices, sxy, &depth,
                                               &fog, &rawDepth);
            if (trace) {
                int clip = projected ? NormalClip(sxy[0], sxy[1], sxy[2]) : 0;
                fprintf(stderr,
                        "course-face timer=%d model=%d type=%d face=%d "
                        "fogged=%d projected=%d reject=%d mirror=%d "
                        "idx=%u,%u,%u,%u clut=%04x tpage=%04x "
                        "raw_depth=%d depth=%d bias=%d clip=%d "
                        "sxy=%d,%d/%d,%d/%d,%d/%d,%d "
                        "uv=%u,%u/%u,%u/%u,%u/%u,%u\n",
                        g_SceneTimer, index, type, i, fogged, projected,
                        projected ? 0 : g_RageProjectionReject,
                        SCRATCH_MIRROR, RageReadU16(stream + 0),
                        RageReadU16(stream + 2), RageReadU16(stream + 4),
                        RageReadU16(stream + 6), RageReadU16(stream + 14),
                        RageReadU16(stream + 18) & 0x9ff,
                        projected ? rawDepth : -1,
                        projected ? depth : -1, bias, clip,
                        (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                        (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                        (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                        (int16_t)sxy[3], (int16_t)(sxy[3] >> 16),
                        stream[12], stream[13], stream[16], stream[17],
                        stream[20], stream[21], stream[22], stream[23]);
            }
            if (!projected)
                continue;
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
                uint8_t *next = RageEmitCourseFt4(
                    cursor, ot, depth, sxy, uv, RageReadU16(stream + 14),
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
                    (rawDepth >> SCRATCH_FACE_OT_SHIFT);
                int vLevel = stream[27] -
                    (rawDepth >> SCRATCH_FACE_OT_SHIFT);
                int uSteps, vSteps;
                int sy, sx;
                memcpy(uvRecord, stream + 12, sizeof(uvRecord));
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
                if (uLevel < 0) uLevel = 0;
                if (vLevel < 0) vLevel = 0;
                if (uLevel > 6 || vLevel > 6) continue;
                uSteps = 1 << uLevel;
                vSteps = 1 << vLevel;
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
    SCRATCH_PRIM_CURSOR_AS(uint8_t) = cursor;
    return;
course_buffer_full:
    fprintf(stderr, "rage course: primitive buffer exhausted\n");
    SCRATCH_PRIM_CURSOR_AS(uint8_t) = cursor;
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
    void **cellTable = (void **)SCRATCH_CELL_TABLE;
    const SVECTOR *vertices = (const SVECTOR *)SCRATCH_CELL_FACES;
    uint8_t *cursor = SCRATCH_PRIM_CURSOR_AS(uint8_t);
    /* func_80028E9C seeds its terrain OT register from scratch+4 + 0x200. */
    OT_TYPE *ot = SCRATCH_OT_BASE_AS(OT_TYPE) + 128;
    int cell;
    int decodedFaces = 0;
    int emittedFaces = 0;
    (void)ctx;
    RageInitializeTerrainTrace();
    if (visible == NULL || cellTable == NULL || vertices == NULL) return;

    /* The hand-written retail dispatcher mirrors the active GTE view by
     * negating RT11, RT12 and RT13 when scratch+0x68 is set.  It intentionally
     * leaves that matrix installed: DrawCourseObjects and DrawCars share it
     * for the remainder of the rear-view pass. */
    if (SCRATCH_MIRROR) {
        MATRIX mirrorMatrix;
        memcpy(&mirrorMatrix, SCRATCH_VIEW_MATRIX_GTE, sizeof(mirrorMatrix));
        mirrorMatrix.m[0][0] = -mirrorMatrix.m[0][0];
        mirrorMatrix.m[0][1] = -mirrorMatrix.m[0][1];
        mirrorMatrix.m[0][2] = -mirrorMatrix.m[0][2];
        SetRotMatrix(&mirrorMatrix);
    }

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
        translation.vx = SCRATCH_MIRROR ? -visible[0] : visible[0];
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
            dispatch = mode < 2 ? mode * 2 + (g_ScratchEnvMode4 != 0)
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
                int bias;
                if (farCell && (stream[20] & 2) != 0) continue;
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
                                "terrain-face timer=%d cell=%d face=%d "
                                "mode=%d mirror=%d reject=%d depth=%d raw=%d "
                                "bias=%d lod=%u,%u shift=%d "
                                "rgb=%02x%02x%02x indices=%u,%u,%u,%u "
                                "translation=%d,%d,%d "
                                "sxy=%d,%d/%d,%d/%d,%d/%d,%d\n",
                                g_SceneTimer, cellIndex, faceIndex, dispatch,
                                SCRATCH_MIRROR,
                                projected ? 0 : g_RageProjectionReject, depth,
                                rawDepth, (int8_t)stream[21], stream[22], stream[23],
                                SCRATCH_FACE_OT_SHIFT,
                                color[0], color[1], color[2],
                                RageReadU16(stream + 0), RageReadU16(stream + 2),
                                RageReadU16(stream + 4), RageReadU16(stream + 6),
                                translation.vx, translation.vy, translation.vz,
                                (int16_t)sxy[0], (int16_t)(sxy[0] >> 16),
                                (int16_t)sxy[1], (int16_t)(sxy[1] >> 16),
                                (int16_t)sxy[2], (int16_t)(sxy[2] >> 16),
                                (int16_t)sxy[3], (int16_t)(sxy[3] >> 16));
                    }
                    if (!projected) continue;
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
                }
                bias = (int8_t)stream[21];
                uLevel = stream[22] - (rawDepth >> SCRATCH_FACE_OT_SHIFT);
                vLevel = stream[23] - (rawDepth >> SCRATCH_FACE_OT_SHIFT);
                if (uLevel < 0) uLevel = 0;
                if (vLevel < 0) vLevel = 0;
                if (uLevel > 6 || vLevel > 6) continue;
                uSteps = 1 << uLevel;
                vSteps = 1 << vLevel;
                if (uSteps == 1 && vSteps == 1) {
                    uint8_t *next;
                    depth += bias;
                    next = RageEmitTerrainFt4(cursor, ot, depth, fog, dispatch,
                                              sxy, baseUv, clut, tpage, color,
                                              textureWindow);
                    if (next == NULL) goto terrain_buffer_full;
                    cursor = next;
                    emittedFaces++;
                } else {
                    int sy, sx;
                    uint32_t lineCommand = RageReadU32(stream + 24);
                    if ((((uint32_t)g_RageProjectionFlag | lineCommand) &
                         0x80000000u) == 0) {
                        uint8_t *next = RageEmitTerrainSubdivisionLines(
                            cursor, ot, depth, sxy, lineCommand);
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
                                            SCRATCH_MIRROR, g_RageTerrainClip0,
                                            g_RageTerrainClip1, childVisible,
                                            childVisible ? 0 : g_RageProjectionReject);
                                }
                                if (!childVisible) continue;
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
                                            "face=%d child=%d,%d/%d,%d "
                                            "visible=%d mirror=%d depth=%d bias=%d ot=%d "
                                            "rgb=%02x%02x%02x "
                                            "xyz=%d,%d,%d/%d,%d,%d/"
                                            "%d,%d,%d/%d,%d,%d "
                                            "sxy=%d,%d/%d,%d/%d,%d/%d,%d "
                                            "uv=%u,%u/%u,%u/%u,%u/%u,%u\n",
                                            g_SceneTimer, cellIndex, faceIndex,
                                            sy, sx, uSteps, vSteps, 1,
                                            SCRATCH_MIRROR, depth, bias,
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
                                                      tpage,color,textureWindow);
                            if (next == NULL) goto terrain_buffer_full;
                            cursor = next;
                            emittedFaces++;
                        }
                    }
                }
            }
        }
    }
    SCRATCH_PRIM_CURSOR_AS(uint8_t) = cursor;
    return;
terrain_buffer_full:
    fprintf(stderr, "rage terrain: primitive buffer exhausted decoded=%d emitted=%d\n",
            decodedFaces, emittedFaces);
    SCRATCH_PRIM_CURSOR_AS(uint8_t) = cursor;
}
