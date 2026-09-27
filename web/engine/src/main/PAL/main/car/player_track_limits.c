#include "game/car_track_internal.h"

enum {
    CAR_HULL_COORDINATE_SCALE = 4,
};

void MeasureCarTrackLimits(const Matrix *toTrack,
                            const CarHullPoint corners[CAR_HULL_CORNER_COUNT],
                            CarTrackLimits *limits) {
    SVec corner;
    s32 index;

    limits->rightInset = -1;
    limits->leftInset = -1;
    limits->rightContact = CAR_TRACK_CONTACT_NONE;
    limits->leftContact = CAR_TRACK_CONTACT_NONE;
    for (index = 0; index < CAR_HULL_CORNER_COUNT; index++) {
        corner.vx = WrapSigned16(
            (int64_t)corners[index].x *
            CAR_HULL_COORDINATE_SCALE);
        corner.vy = 0;
        corner.vz = WrapSigned16(
            (int64_t)corners[index].z *
            CAR_HULL_COORDINATE_SCALE);
        const s32 reachX = ((int64_t)toTrack->m[0][0] * corner.vx +
                            (int64_t)toTrack->m[0][2] * corner.vz) >> 12;
        /* Contact values are one-based, so zero means no reaching corner. */
        if (limits->rightInset < reachX) {
            limits->rightContact =
                index + CAR_TRACK_CONTACT_FRONT_LEFT;
            limits->rightInset = reachX;
        } else if (reachX < limits->leftInset) {
            limits->leftContact =
                index + CAR_TRACK_CONTACT_FRONT_LEFT;
            limits->leftInset = reachX;
        }
    }
}
