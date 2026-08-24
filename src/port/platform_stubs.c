#include "rage/compat.h"
#include "rage/render_world_game.h"
#include "modern/modern_renderer.h"
#include <libgte.h>

extern int32_t g_FrameCounter;

void RagePortSmokeBeforeSceneHandler(void) __attribute__((weak));
void RagePortSmokeBeforeSceneHandler(void) {}
void RagePortScenarioBeforeSceneHandler(void) __attribute__((weak));
void RagePortScenarioBeforeSceneHandler(void) {}

void RageCaptureFrameBegin(void);
void RageCaptureFrameEnd(void);
void RageModernFrameWaitTick(int frameLimit);

void RagePortBeforeSceneHandler(void) {
    {
        extern void RageTimingApply(void);
        RageTimingApply();
    }
#ifdef RAGE_SMOKE_TARGET
    RagePortSmokeBeforeSceneHandler();
    /* Scenario input is synthesized after physical/test input sampling so it
     * cannot be cleared by the pad edge update in the same frame. */
    RagePortScenarioBeforeSceneHandler();
#else
    RagePortScenarioBeforeSceneHandler();
    RagePortSmokeBeforeSceneHandler();
#endif
    RageGameRenderWorldBeginFrame((uint64_t)g_FrameCounter);
    RageCaptureFrameBegin();
}

void RagePortAfterSceneHandler(void) {
    RageGameRenderWorldPublishCurrentCamera();
    if (RageModernIsEnabled()) {
        RageGameRenderWorldPublishCourseObjects();
        RageGameRenderWorldPublishTerrainGrid();
    }
    RageCaptureFrameEnd();
}

void RagePortDuringFrameWait(int frameLimit) {
    RageModernFrameWaitTick(frameLimit);
}

long SpuTransferStatus(void *address, long mode) {
    (void)address;
    (void)mode;
    return 0;
}

MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    MATRIX result = *right;
    int row;
    int column;
    int inner;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            long sum = 0;
            for (inner = 0; inner < 3; inner++) {
                sum += (long)left->m[row][inner] * right->m[inner][column];
            }
            result.m[row][column] = (short)(sum >> 12);
        }
    }
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            right->m[row][column] = result.m[row][column];
        }
    }
    return right;
}

short *ApplyMatrixSV(void *matrix, void *input, short *output) {
    MATRIX *m = matrix;
    SVECTOR *v = input;
    int row;

    for (row = 0; row < 3; row++) {
        long sum = (long)m->m[row][0] * v->vx
                 + (long)m->m[row][1] * v->vy
                 + (long)m->m[row][2] * v->vz;
        output[row] = (short)(sum >> 12);
    }
    return output;
}
