#include "game/car.h"
#include "game/car_internal.h"
#include "game/diagnostics.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

static int ShouldTraceTrackLimits(void) {
    if (!DiagnosticsEnabled("car.track_trace")) {
        return 0;
    }
    return g_SceneTimer == DiagnosticsIntValue(
        "car.track_trace_timer", g_SceneTimer);
}

static void TraceTrackLimit(const Matrix *toTrack, const SVec *corner,
                            const Vec4 *reach) {
    Trace("car-limit", "timer=%d matrix=%d,%d,%d,%d,%d,%d,%d,%d,%d "
          "vector=%d,%d,%d output=%d,%d,%d", g_SceneTimer,
          toTrack->m[0][0], toTrack->m[0][1], toTrack->m[0][2],
          toTrack->m[1][0], toTrack->m[1][1], toTrack->m[1][2],
          toTrack->m[2][0], toTrack->m[2][1], toTrack->m[2][2],
          corner->vx, corner->vy, corner->vz, reach->x, reach->y,
          reach->z);
}

void MeasurePlayerTrackLimits(const Matrix *toTrack,
                              CarTrackLimits *limits) {
    int traceLimits = ShouldTraceTrackLimits();
    Matrix transform = *toTrack;
    SVec corner;
    Vec4 reach;
    s32 index;

    limits->rightInset = -1;
    limits->leftInset = -1;
    limits->rightKnockbackMode = 0;
    limits->leftKnockbackMode = 0;
    for (index = 0; index < 4; index++) {
        corner.vx = WrapSigned16(
            (int64_t)g_CarCornerOffsets[index].x * 4);
        corner.vy = 0;
        corner.vz = WrapSigned16(
            (int64_t)g_CarCornerOffsets[index].z * 4);
        ApplyMatrix(&transform, &corner, &reach);
        if (traceLimits) {
            TraceTrackLimit(toTrack, &corner, &reach);
        }
        /* The modes are one-based, so zero means no reaching corner. */
        if (limits->rightInset < reach.x) {
            limits->rightKnockbackMode = index + 1;
            limits->rightInset = reach.x;
        } else if (reach.x < limits->leftInset) {
            limits->leftKnockbackMode = index + 1;
            limits->leftInset = reach.x;
        }
    }
}
