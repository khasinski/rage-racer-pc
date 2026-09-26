#include "game/geometry.h"
#include "game/random.h"
#include <psyz/gte.h>
#include <stdio.h>

int main(void) {
    u32 seed = 123;
    for (int i = 0; i < 4096; i++) {
        u32 points[3];
        for (int j = 0; j < 3; j++) {
            points[j] = (u32)RandomNext(&seed) | ((u32)RandomNext(&seed) << 16);
            if (i & 1) points[j] ^= 0x80008000u;
        }
        const s32 expected = (s32)NormalClip(points[0], points[1], points[2]);
        u32 data[32], controls[32];
        for (unsigned j = 0; j < 32; j++) {
            data[j] = Psyz_GteDataRead(j);
            controls[j] = Psyz_GteCtrlRead(j);
        }
        if (TriangleArea(points[0], points[1], points[2]) != expected) {
            fprintf(stderr, "area mismatch case %d\n", i); return 1;
        }
        for (unsigned j = 0; j < 32; j++) {
            if (data[j] != Psyz_GteDataRead(j) || controls[j] != Psyz_GteCtrlRead(j)) {
                fprintf(stderr, "GTE changed case %d register %u\n", i, j); return 1;
            }
        }
    }
    if (TriangleArea(0, 1, 1u << 16) != 1 ||
        TriangleArea(0, 1u << 16, 1) != -1 ||
        TriangleArea(0, 1, 2) != 0) return 1;
    return 0;
}
