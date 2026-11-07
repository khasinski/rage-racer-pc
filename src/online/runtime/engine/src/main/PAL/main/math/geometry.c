#include "game/geometry.h"
#include "game/integer.h"

s32 TriangleArea(u32 first, u32 second, u32 third) {
    const s32 ax = WrapSigned16(first), ay = WrapSigned16(first >> 16);
    const s32 bx = WrapSigned16(second), by = WrapSigned16(second >> 16);
    const s32 cx = WrapSigned16(third), cy = WrapSigned16(third >> 16);
    return WrapSigned32((int64_t)ax * (by - cy) +
                        (int64_t)bx * (cy - ay) +
                        (int64_t)cx * (ay - by));
}
