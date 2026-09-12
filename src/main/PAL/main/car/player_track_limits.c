#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/render.h"

enum {
    CAR_HULL_COORDINATE_SCALE = 4,
};

void MeasurePlayerTrackLimits(const Matrix *toTrack,
                              CarTrackLimits *limits) {
    Matrix transform = *toTrack;
    SVec corner;
    Vec4 reach;
    s32 index;

    limits->rightInset = -1;
    limits->leftInset = -1;
    limits->rightContact = CAR_TRACK_CONTACT_NONE;
    limits->leftContact = CAR_TRACK_CONTACT_NONE;
    for (index = 0; index < CAR_HULL_CORNER_COUNT; index++) {
        corner.vx = WrapSigned16(
            (int64_t)g_CarCornerOffsets[index].x *
            CAR_HULL_COORDINATE_SCALE);
        corner.vy = 0;
        corner.vz = WrapSigned16(
            (int64_t)g_CarCornerOffsets[index].z *
            CAR_HULL_COORDINATE_SCALE);
        ApplyMatrix(&transform, &corner, &reach);
        /* Contact values are one-based, so zero means no reaching corner. */
        if (limits->rightInset < reach.x) {
            limits->rightContact =
                index + CAR_TRACK_CONTACT_FRONT_LEFT;
            limits->rightInset = reach.x;
        } else if (reach.x < limits->leftInset) {
            limits->leftContact =
                index + CAR_TRACK_CONTACT_FRONT_LEFT;
            limits->leftInset = reach.x;
        }
    }
}
