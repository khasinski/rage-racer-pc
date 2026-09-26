#include "game/angle.h"
#include <limits.h>
#include <stdio.h>

int main(void) {
    for (s32 angle = -8192; angle <= 8192; angle++) {
        const s32 sine = SinAngle(angle), cosine = CosAngle(angle);
        if (sine != -SinAngle(-angle) || cosine != CosAngle(-angle) ||
            sine != SinAngle(angle + 4096) || cosine != SinAngle(angle + 1024) ||
            sine < -4096 || sine > 4096 || cosine < -4096 || cosine > 4096) {
            fprintf(stderr, "trig invariant at %d\n", angle); return 1;
        }
#ifdef TRIG_REFERENCE
        if (sine != rsin(angle) || cosine != rcos(angle)) {
            fprintf(stderr, "retail trig mismatch at %d\n", angle); return 1;
        }
#endif
    }
    if (SinAngle(0) != 0 || SinAngle(1024) != 4096 || SinAngle(2048) != 0 ||
        SinAngle(3072) != -4096 || CosAngle(0) != 4096 ||
        SinAngle(INT_MIN) != 0 || CosAngle(INT_MIN) != 4096 ||
        SinAngle(INT_MAX) != -6 || CosAngle(INT_MAX) != 4096) return 1;
    return 0;
}
