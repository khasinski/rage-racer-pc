#include "game/hull_rotation.h"
#include "game/integer.h"
#include "game/random.h"
#include "psyq/gte.h"
#include <stdio.h>

int main(void) {
    u32 seed = 123;
    for (int i = 0; i < 4096; i++) {
        SVec rotation = {.vx = WrapSigned16(RandomNext(&seed) - 16384),
                        .vy = WrapSigned16(i),
                        .vz = WrapSigned16(RandomNext(&seed) - 16384)};
        Matrix matrix;
        RotMatrix(&rotation, &matrix);
        const HullAxes axes = BuildHullAxes(rotation.vx, rotation.vy, rotation.vz);
        if (axes.xx != matrix.m[0][0] || axes.xz != matrix.m[0][2] ||
            axes.zx != matrix.m[2][0] || axes.zz != matrix.m[2][2]) {
            fprintf(stderr, "rotation mismatch %d\n", i); return 1;
        }
        const CarHullPoint point = {.x = WrapSigned16(RandomNext(&seed) * 2 - 32768),
                                    .z = WrapSigned16(RandomNext(&seed) * 2 - 32768)};
        SVec input = {.vx = point.x, .vz = point.z};
        Vec4 expected;
        ApplyMatrix(&matrix, &input, &expected);
        const LVec actual = RotateHullPoint(&axes, &point);
        if (actual.x != expected.x || actual.z != expected.z) {
            fprintf(stderr, "point mismatch %d\n", i); return 1;
        }
    }
    return 0;
}
