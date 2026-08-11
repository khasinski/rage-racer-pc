#include <libgpu.h>
#include <libgte.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/asset.h"

extern int g_CourseModelCount;
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
static int g_RageProjectionReject;

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

static void RageStoreSxy(short *x, short *y, int packed) {
    *x = (short)(packed & 0xffff);
    *y = (short)((uint32_t)packed >> 16);
}

static int RageProjectQuad(
    const SVECTOR *v0, const SVECTOR *v1, const SVECTOR *v2,
    const SVECTOR *v3, int sxy[4], int *depth, int *fog, int *rawDepth,
    int rejectLargeSpan) {
    int p;
    int flag;
    long otz;

    otz = RotAverage4((SVECTOR *)v0, (SVECTOR *)v1, (SVECTOR *)v2,
                      (SVECTOR *)v3, &sxy[0], &sxy[1], &sxy[2], &sxy[3],
                      &p, &flag);
    g_RageProjectionReject = 0;
    if (fog != NULL) *fog = p;
    if (rawDepth != NULL) *rawDepth = (int)otz;
    {
        int x[4], y[4], i;
        int allLeft = 1, allRight = 1, allAbove = 1, allBelow = 1;
        int minX = 32767, maxX = -32768, minY = 32767, maxY = -32768;
        for (i = 0; i < 4; i++) {
            x[i] = (int16_t)(sxy[i] & 0xffff);
            y[i] = (int16_t)((uint32_t)sxy[i] >> 16);
            if (x[i] < minX) minX = x[i];
            if (x[i] > maxX) maxX = x[i];
            if (y[i] < minY) minY = y[i];
            if (y[i] > maxY) maxY = y[i];
            allLeft &= x[i] < g_RageScratchpadState.x0;
            allRight &= x[i] > g_RageScratchpadState.x1;
            allAbove &= y[i] < g_RageScratchpadState.y0;
            allBelow &= y[i] > g_RageScratchpadState.y1;
        }
        if (allLeft || allRight || allAbove || allBelow) {
            g_RageProjectionReject = 1;
            return 0;
        }
        /* Retail subdivides quads which cross the near plane. Until that
         * path is decoded, never submit their wrapped GTE coordinates as a
         * single screen-sized polygon. */
        if (rejectLargeSpan && (maxX - minX > 640 || maxY - minY > 512))
            { g_RageProjectionReject = 1; return 0; }
    }
    {
        int clip0 = NormalClip(sxy[0], sxy[1], sxy[2]);
        if (g_RageInsideModelProjection && g_RageSubmittedModelIndex == 0 &&
            g_RageSubmittedModelType == RAGE_MODEL_GT4) {
            if (clip0 > 0) g_RageGt4ClipPositive++;
            if (clip0 < 0) g_RageGt4ClipNegative++;
        }
        /* The retail face loop executes one NCLIP after RTPT, before it
         * projects the fourth vertex.  Testing the second half of the quad
         * as an alternative admits twisted/back-facing records and produces
         * the large stray polygons visible in the prologue. */
        if ((!SCRATCH_MIRROR && clip0 <= 0) ||
            (SCRATCH_MIRROR && clip0 >= 0)) {
            g_RageProjectionReject = 2;
            return 0;
        }
    }
    *depth = ((int)otz >> SCRATCH_OT_SHIFT);
    if (*depth <= 0 || *depth >= 448) {
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

static int RageBilerpSxy(
    const int sxy[4], int u, int v, int uSteps, int vSteps) {
    int64_t w0 = (int64_t)(uSteps - u) * (vSteps - v);
    int64_t w1 = (int64_t)u * (vSteps - v);
    int64_t w2 = (int64_t)(uSteps - u) * v;
    int64_t w3 = (int64_t)u * v;
    int64_t divisor = (int64_t)uSteps * vSteps;
    int x = ((int16_t)sxy[0] * w0 + (int16_t)sxy[1] * w1 +
             (int16_t)sxy[2] * w2 + (int16_t)sxy[3] * w3) / divisor;
    int y = ((int16_t)(sxy[0] >> 16) * w0 +
             (int16_t)(sxy[1] >> 16) * w1 +
             (int16_t)(sxy[2] >> 16) * w2 +
             (int16_t)(sxy[3] >> 16) * w3) / divisor;
    return (int)((uint16_t)x | ((uint32_t)(uint16_t)y << 16));
}

static int RageScreenQuadVisible(const int sxy[4]) {
    int i;
    int allLeft = 1, allRight = 1, allAbove = 1, allBelow = 1;
    int clip = NormalClip(sxy[0], sxy[1], sxy[2]);
    if ((!SCRATCH_MIRROR && clip <= 0) || (SCRATCH_MIRROR && clip >= 0))
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
    int w0 = (uSteps - u) * (vSteps - v);
    int w1 = u * (vSteps - v);
    int w2 = (uSteps - u) * v;
    int w3 = u * v;
    return (uint8_t)((c0*w0 + c1*w1 + c2*w2 + c3*w3) /
                     (uSteps*vSteps));
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
    OT_TYPE *ot = SCRATCH_OT_BASE_AS(OT_TYPE);
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
    NativeCourseModel *models = (NativeCourseModel *)SCRATCH_COURSE_BANK;
    uint8_t *stream;
    uint8_t *cursor = SCRATCH_PRIM_CURSOR_AS(uint8_t);
    OT_TYPE *ot = SCRATCH_OT_BASE_AS(OT_TYPE);
    const SVECTOR *vertices;
    uint32_t opcode;
    (void)fogged;
    if (models == NULL || index < 0 || index >= g_CourseModelCount ||
        models[index].geometry == NULL || models[index].model == NULL) return;
    vertices = (const SVECTOR *)models[index].geometry;
    stream = (uint8_t *)models[index].model;
    while ((opcode = RageReadU32(stream)) != 0) {
        int type = opcode & 0xffff;
        int count = (int)(opcode >> 16);
        int stride = type == 0 ? 16 : (type == 1 ? 28 : 32);
        int i;
        stream += 4;
        if ((unsigned)type >= 4 || count <= 0) break;
        for (i = 0; i < count; i++, stream += stride) {
            int sxy[4], depth;
            int bias = (int8_t)stream[stride - 3];
            if (!RageProjectModelFace(stream, vertices, sxy, &depth, NULL)) continue;
            depth += bias;
            if (depth <= 0 || depth >= 448) continue;
            if (type == 0) {
                if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_F4))) break;
                POLY_F4 *poly = (POLY_F4 *)cursor;
                SetPolyF4(poly);
                poly->r0=stream[8]; poly->g0=stream[9]; poly->b0=stream[10];
                RageStoreSxy(&poly->x0,&poly->y0,sxy[0]); RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
                RageStoreSxy(&poly->x2,&poly->y2,sxy[2]); RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
                AddPrim(&ot[depth],poly); cursor += sizeof(*poly);
            } else {
                if (!RagePrimitiveSpaceAvailable(cursor, sizeof(POLY_FT4))) break;
                POLY_FT4 *poly = (POLY_FT4 *)cursor;
                SetPolyFT4(poly);
                poly->r0=stream[8]; poly->g0=stream[9]; poly->b0=stream[10];
                RageCopyFt4Uv(poly, stream + 12);
                RageStoreSxy(&poly->x0,&poly->y0,sxy[0]); RageStoreSxy(&poly->x1,&poly->y1,sxy[1]);
                RageStoreSxy(&poly->x2,&poly->y2,sxy[2]); RageStoreSxy(&poly->x3,&poly->y3,sxy[3]);
                AddPrim(&ot[depth],poly); cursor += sizeof(*poly);
            }
        }
    }
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
    OT_TYPE *ot = SCRATCH_OT_BASE_AS(OT_TYPE);
    int cell;
    int decodedFaces = 0;
    int emittedFaces = 0;
    (void)ctx;
    if (visible == NULL || cellTable == NULL || vertices == NULL) return;

    for (cell = 0; cell < count; cell++, visible += 4) {
        uint8_t *stream;
        uint32_t opcode;
        VECTOR translation;
        int cellIndex = visible[3];
        int farCell = visible[2] >= 0xa000;
        if (cellIndex < 0 || cellIndex >= GAME_TERRAIN_CELL_LIMIT ||
            cellTable[cellIndex] == NULL) continue;
        translation.vx = visible[0];
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
                if (!RageProjectQuad(v0, v1, v2, v3, sxy, &depth, &fog,
                                     &rawDepth, 0))
                    continue;
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
                    if (!RageProjectQuad(v0,v1,v2,v3,sxy,&depth,&fog,
                                         &rawDepth,1)) continue;
                    depth += bias;
                    if (depth <= 0 || depth >= 448) continue;
                    next = RageEmitTerrainFt4(cursor, ot, depth, fog, dispatch,
                                              sxy, baseUv, clut, tpage, color,
                                              textureWindow);
                    if (next == NULL) goto terrain_buffer_full;
                    cursor = next;
                    emittedFaces++;
                } else {
                    int sy, sx;
                    for (sy = 0; sy < vSteps; sy++) {
                        for (sx = 0; sx < uSteps; sx++) {
                            int subSxy[4];
                            int subDepth = depth + bias;
                            uint8_t uv[8];
                            uint8_t *next;
                            subSxy[0] = RageBilerpSxy(sxy,sx,sy,uSteps,vSteps);
                            subSxy[1] = RageBilerpSxy(sxy,sx+1,sy,uSteps,vSteps);
                            subSxy[2] = RageBilerpSxy(sxy,sx,sy+1,uSteps,vSteps);
                            subSxy[3] = RageBilerpSxy(sxy,sx+1,sy+1,uSteps,vSteps);
#define RAGE_SUB_UV(out, corner, uu, vv) do { \
    (out)[(corner)*2] = RageBilerpByte(baseUv[0],baseUv[2],baseUv[4],baseUv[6],(uu),(vv),uSteps,vSteps); \
    (out)[(corner)*2+1] = RageBilerpByte(baseUv[1],baseUv[3],baseUv[5],baseUv[7],(uu),(vv),uSteps,vSteps); \
} while (0)
                            RAGE_SUB_UV(uv,0,sx,sy); RAGE_SUB_UV(uv,1,sx+1,sy);
                            RAGE_SUB_UV(uv,2,sx,sy+1); RAGE_SUB_UV(uv,3,sx+1,sy+1);
#undef RAGE_SUB_UV
                            if (!RageScreenQuadVisible(subSxy)) continue;
                            if (subDepth <= 0 || subDepth >= 448) continue;
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
