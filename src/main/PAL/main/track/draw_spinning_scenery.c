#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/track.h"
#include "rage/render_world_game.h"

void DrawSpinningScenery(s32 timer, s32 animate) {
    s16 yawMatrix[16];
    s16 objectMatrix[16];
    Matrix renderWorldMtx;
    s32 frame = timer;
    s32 update = animate;
    s16 *dst;
    u16 *delta;
    u16 *deltaBase;
    s16 *work = objectMatrix;
    s16 *base;
    s32 offset;
    s32 end;
    s32 start;
    s32 loopIndex;
    s32 limit;
    s32 active;
    s32 activeValue;
    s32 frameMask;
    SpinningSceneryDataAddress dataAddress;
    SpinningSceneryAngleAddress cursorAddress;
    SpinningSceneryAngleAddress endAddress;

    activeValue = g_CourseIndex;
    active = activeValue & 3;
    active = active != 0;
    if (active) {
        start = 1;
        end = 4;
    } else {
        start = 0;
        end = 1;
    }

    loopIndex = start;
    
    if (loopIndex < end) {
        deltaBase = g_SpinningSceneryRate;
        delta = &deltaBase[active];
        work = objectMatrix;
        base = g_SpinningSceneryAngle;
        dst = &base[loopIndex];
        offset = loopIndex * 0x10;

        do {
            if (update != 0) {
                *dst += *delta;
            }
            *dst &= 0xFFF;

            dataAddress.orientationPointer = g_SpinningSceneryYaw;
            dataAddress.bytes += offset;
            BuildRotMatrixY(yawMatrix, dataAddress.orientationPointer->yaw);
            BuildRotMatrixZ(&renderWorldMtx, *dst);
            MulMatrix2((Matrix *)yawMatrix, &renderWorldMtx);
            MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, yawMatrix);
            BuildRotMatrixZ(work, *dst);
            MulMatrix2(yawMatrix, work);
            dataAddress.positionPointer = g_SpinningSceneryPos;
            dataAddress.bytes += offset;
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, dataAddress.positionPointer, work);

            SCRATCH_ENV_MODE4 = 0;
            limit = 1;
            if (g_CourseModelCount >= 0x3F) {
                limit = 0x3E;
            }
            GameRenderWorldSubmitDynamicCourseObject(
                0x100 + loopIndex, limit, dataAddress.positionPointer->x,
                dataAddress.positionPointer->y,
                dataAddress.positionPointer->z, renderWorldMtx.m, 1, 0);
            SubmitCourseModel2(SCRATCHPAD, limit);

            dst++;
            offset += 0x10;
            cursorAddress.pointer = dst;
            endAddress.pointer = base;
            endAddress.value = (end * 2) + endAddress.value;
        } while (cursorAddress.value < endAddress.value);
    }

    frameMask = frame & 0x1FF;
    if ((frameMask == 0) && (update != 0)) {
        g_SpinningSceneryRate[0] = Random15() & 0x1F;
        g_SpinningSceneryRate[1] = Random15() & 0x3F;
    }
}
