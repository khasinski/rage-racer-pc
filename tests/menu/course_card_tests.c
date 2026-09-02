#include "common.h"
#include "game/menu.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 g_CourseCardFace;
s32 g_CourseCardPendingGrade;
s32 g_CourseCardSpin;
s32 g_CourseCardSpinTarget;
SVec g_CourseCardVerts[4];
GameRenderState g_RenderState;

static GameOrderingTableEntry s_orderingTable[2];
static s32 s_rotationAngle;
static s32 s_matrixCalls;
static s32 s_drawCalls;
static s32 s_drawDepth;
static s32 s_wrongOrderingTable;
static s16 s_drawX[4];
static u16 s_drawY[4];

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)matrix;
    s_rotationAngle = angle;
}

short *ApplyMatrixSV(void *matrix, void *source, short *destination) {
    const SVec *vertex = source;

    (void)matrix;
    destination[0] = vertex->vx;
    destination[1] = vertex->vy;
    destination[2] = vertex->vz;
    destination[3] = 0;
    s_matrixCalls++;
    return destination;
}

void GameDrawTexturedQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1,
                          u16 x2, u16 y2, u16 x3, u16 y3, u8 u0, u8 v0,
                          u8 u1, u8 v1, u8 u2, u8 v2, u8 u3, u8 v3, u8 r,
                          u8 g, u8 b, u16 clutIndex, s32 shadeTex,
                          s32 semiTrans, u16 tpage) {
    (void)u0;
    (void)v0;
    (void)u1;
    (void)v1;
    (void)u2;
    (void)v2;
    (void)u3;
    (void)v3;
    (void)r;
    (void)g;
    (void)b;
    (void)shadeTex;
    (void)semiTrans;
    (void)tpage;
    if (ot != &s_orderingTable[1]) {
        s_wrongOrderingTable = 1;
    }
    s_drawX[0] = x0;
    s_drawX[1] = x1;
    s_drawX[2] = (s16)x2;
    s_drawX[3] = (s16)x3;
    s_drawY[0] = (u16)y0;
    s_drawY[1] = y1;
    s_drawY[2] = y2;
    s_drawY[3] = y3;
    s_drawDepth = clutIndex;
    s_drawCalls++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    RENDER_OT_BASE = s_orderingTable;
    g_CourseCardFace = 1;
    g_CourseCardPendingGrade = -1;
    g_CourseCardSpin = 0;
    g_CourseCardSpinTarget = 12000;
    g_CourseCardVerts[0] = (SVec){-10, -20, 0, 0};
    g_CourseCardVerts[1] = (SVec){10, -20, 0, 0};
    g_CourseCardVerts[2] = (SVec){-10, 20, 0, 0};
    g_CourseCardVerts[3] = (SVec){10, 20, 0, 0};
    s_rotationAngle = -1;
    s_matrixCalls = 0;
    s_drawCalls = 0;
    s_drawDepth = -1;
    s_wrongOrderingTable = 0;
}

int main(void) {
    Reset();
    UpdateAndDrawCourseCard();
    CHECK(g_CourseCardSpin == 1001);
    CHECK(s_rotationAngle == 11 && s_matrixCalls == 4);
    CHECK(s_drawCalls == 1 && s_drawDepth == 0x1F8);
    CHECK(s_wrongOrderingTable == 0);
    CHECK(s_drawX[0] == 0xE4 - 10 && s_drawY[0] == 0x58 - 20);
    CHECK(s_drawX[3] == 0xE4 + 10 && s_drawY[3] == 0x58 + 20);

    Reset();
    g_CourseCardSpin = 2000000;
    g_CourseCardSpinTarget = 2000000;
    g_CourseCardPendingGrade = 2;
    UpdateAndDrawCourseCard();
    CHECK(g_CourseCardPendingGrade == 2 && g_CourseCardFace == 1);
    CHECK(s_rotationAngle == 2000);

    Reset();
    g_CourseCardSpin = 12000;
    g_CourseCardSpinTarget = 0;
    g_CourseCardPendingGrade = 2;
    UpdateAndDrawCourseCard();
    CHECK(g_CourseCardSpin == 10999);
    CHECK(g_CourseCardPendingGrade == -1 && g_CourseCardFace == 2);
    CHECK(s_rotationAngle == 11 && s_drawDepth == 0x20B);

    Reset();
    g_CourseCardFace = 0;
    UpdateAndDrawCourseCard();
    CHECK(s_matrixCalls == 0 && s_drawCalls == 0);

    puts("course card easing, face swap, and rendering are preserved");
    return 0;
}
