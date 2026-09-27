#include "game/hull_rotation.h"
#include "game/integer.h"

/* Reuse the canonical packed table, not the differently rounded rsin table.
 * No PsyZ runtime or GTE state is needed. */
#include "rcossin_table.inc"

static void HullSinCos(s16 angle, s32 *cosine, s32 *sine) {
    u32 normalized = angle < 0 ? 0u - (u32)(s32)angle : (u32)angle;
    normalized &= 0xFFF;
    const u32 packed = kRcossinQuarter[normalized & 0x3FF];
    const s32 s = WrapSigned16(packed), c = WrapSigned16(packed >> 16);
    switch (normalized >> 10) {
    case 0: *sine = s; *cosine = c; break;
    case 1: *sine = c; *cosine = -s; break;
    case 2: *sine = -s; *cosine = -c; break;
    default: *sine = -c; *cosine = s; break;
    }
    if (angle < 0) *sine = -*sine;
}

HullAxes BuildHullAxes(s16 pitch, s16 yaw, s16 roll) {
    s32 cx, sx, cy, sy, cz, sz;
    HullSinCos(pitch, &cx, &sx);
    HullSinCos(yaw, &cy, &sy);
    HullSinCos(roll, &cz, &sz);
    /* Hull points have y=0: only these four matrix entries are used. */
    return (HullAxes){
        .xx = WrapSigned16(((int64_t)cy * cz) >> 12),
        .xz = (s16)sy,
        .zx = WrapSigned16((-(int64_t)cx * sy * cz + (int64_t)sx * sz * 4096) >> 24),
        .zz = WrapSigned16(((int64_t)cx * cy) >> 12),
    };
}

LVec RotateHullPoint(const HullAxes *axes, const CarHullPoint *point) {
    return (LVec){
        .x = ((int64_t)axes->xx * point->x + (int64_t)axes->xz * point->z) >> 12,
        .z = ((int64_t)axes->zx * point->x + (int64_t)axes->zz * point->z) >> 12,
    };
}
