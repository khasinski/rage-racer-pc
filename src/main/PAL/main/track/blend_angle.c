#include "common.h"

/*
 * Wrap-aware angle blend in the 12-bit (0..0xFFF) angle space: blends angleA
 * toward angleB by `weight` (0..0x400) taking the shorter way around the
 * 0x1000 circle (the 0x801 test unwraps one operand by +0x1000). Returns the
 * blended angle masked to 12 bits.
 */
s32 BlendAngle(s32 angleA, s32 angleB, s32 weight) {
    s32 lhs = angleA & 0xFFF;
    s32 rhs = angleB & 0xFFF;
    s32 inv = 0x400 - weight;
    s32 sum;

    if (rhs < lhs) {
        if (lhs - rhs >= 0x801) {
            rhs += 0x1000;
        }
    } else if (rhs - lhs >= 0x801) {
        lhs += 0x1000;
    }

    /* The original MIPS addu/mult path wraps at 32 bits.  Express that
     * explicitly so large transient interpolation weights cannot be folded
     * under host signed-overflow rules. */
    sum = (s32)((u32)lhs * (u32)inv + (u32)rhs * (u32)weight);
    if (sum < 0) {
        sum += 0x3FF;
    }

    return (sum >> 10) & 0xFFF;
}
