/* Compare the cached interpolation against the pre-optimization algorithm,
 * including the integral-UV instability flag used to repair GPU sampling. */
#include <stdint.h>
#include <stdio.h>
#include "texture_sample.h"
typedef TextureSampleVertex Vertex;
typedef uint16_t u16;
static struct { int x, y; } draw_offset;
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
static bool ReferenceTextureSample(const Vertex p[3], int x, int y,
                                u16* u, u16* v) {
    double ax = p[0].x + draw_offset.x;
    double ay = p[0].y + draw_offset.y;
    double bx = p[1].x + draw_offset.x;
    double by = p[1].y + draw_offset.y;
    double cx = p[2].x + draw_offset.x;
    double cy = p[2].y + draw_offset.y;
    double determinant = (bx - ax) * (cy - ay) -
                         (by - ay) * (cx - ax);
    if (determinant == 0.0) {
        *u = p[0].u;
        *v = p[0].v;
        return true;
    }
    double du_dx = ((p[1].u - p[0].u) * (cy - ay) -
                    (p[2].u - p[0].u) * (by - ay)) / determinant;
    double du_dy = ((bx - ax) * (p[2].u - p[0].u) -
                    (cx - ax) * (p[1].u - p[0].u)) / determinant;
    double dv_dx = ((p[1].v - p[0].v) * (cy - ay) -
                    (p[2].v - p[0].v) * (by - ay)) / determinant;
    double dv_dy = ((bx - ax) * (p[2].v - p[0].v) -
                    (cx - ax) * (p[1].v - p[0].v)) / determinant;
    double raw_u = p[0].u + du_dx * (x - ax) + du_dy * (y - ay);
    double raw_v = p[0].v + dv_dx * (x - ax) + dv_dy * (y - ay);
    int sample_u = (int)floor(raw_u);
    int sample_v = (int)floor(raw_v);
    *u = (u16)CLAMP(sample_u, 0, 255);
    *v = (u16)CLAMP(sample_v, 0, 255);
    /* A mathematically integral UV can arrive infinitesimally below the
     * boundary after float interpolation. The fragment shader then floors to
     * the previous texel, while the PS1 fixed accumulator remains exact. */
    return fabs(raw_u - round(raw_u)) < 1e-7 ||
           fabs(raw_v - round(raw_v)) < 1e-7;
}

static uint32_t state = 0x5eed1234;
static unsigned random_value(void) {
    state = state * 1664525u + 1013904223u;
    return state;
}
static int check(const Vertex vertices[3], int x, int y) {
    TextureSampleVertex shifted[3];
    for (int i = 0; i < 3; ++i) {
        shifted[i] = vertices[i];
        shifted[i].x += draw_offset.x;
        shifted[i].y += draw_offset.y;
    }
    TextureSamplePlane plane = TextureSamplePrepare(shifted);
    uint16_t expected_u, expected_v, actual_u, actual_v;
    bool expected = ReferenceTextureSample(vertices, x, y, &expected_u, &expected_v);
    bool actual = TextureSampleAt(&plane, x, y, &actual_u, &actual_v);
    if (expected != actual || expected_u != actual_u || expected_v != actual_v) {
        fprintf(stderr, "UV mismatch at %d,%d: expected %u,%u/%d got %u,%u/%d\n",
                x, y, expected_u, expected_v, expected, actual_u, actual_v, actual);
        return 1;
    }
    return 0;
}
static int check_unit_sprites(void) {
    draw_offset.x = -1024;
    draw_offset.y = 731;
    for (int w = 1; w <= 64; ++w) {
        for (int h = 1; h <= 64; ++h) {
            Vertex p[4] = {
                {-100,17,245,251}, {-100+w,17,245+w,251},
                {-100,17+h,245,251+h}, {-100+w,17+h,245+w,251+h}};
            if (!TextureSampleIsUnitSprite(p)) return 1;
            const Vertex triangles[2][3] = {
                {p[0],p[1],p[2]}, {p[1],p[3],p[2]}};
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                    for (int t = 0; t < 2; ++t) {
                        uint16_t u, v;
                        /* If covered, the original always corrects integral
                         * UVs. If uncovered, it also always corrects. */
                        if (!ReferenceTextureSample(triangles[t],
                                p[0].x + draw_offset.x + x,
                                p[0].y + draw_offset.y + y, &u, &v))
                            return 1;
                    }
            for (int vertex = 0; vertex < 4; ++vertex) {
                for (int field = 0; field < 4; ++field) {
                    Vertex changed[4] = {p[0],p[1],p[2],p[3]};
                    switch (field) {
                    case 0: ++changed[vertex].x; break;
                    case 1: ++changed[vertex].y; break;
                    case 2: ++changed[vertex].u; break;
                    case 3: ++changed[vertex].v; break;
                    }
                    if (TextureSampleIsUnitSprite(changed)) return 1;
                }
            }
        }
    }
    puts("Unit sprites: 8,652,800 reference samples and 65,536 rejection cases passed");
    draw_offset.x = draw_offset.y = 0;
    return 0;
}

int main(void) {
    if (check_unit_sprites()) {
        fputs("Unit sprite correction differs from general path\n", stderr);
        return 1;
    }
    /* Sprites, both windings, degenerate and stretched/negative UV slopes. */
    const Vertex fixtures[][3] = {
        {{0,0,0,0},{256,0,256,0},{0,256,0,256}},
        {{0,256,0,256},{256,0,256,0},{0,0,0,0}},
        {{1,1,255,17},{1,1,2,3},{1,1,4,5}},
        {{-10,3,255,0},{211,7,0,255},{2,87,23,111}},
        {{0,0,0,0},{1,1,255,255},{2,2,128,128}},
    };
    for (unsigned f = 0; f < sizeof(fixtures)/sizeof(fixtures[0]); ++f)
        for (int y = -16; y < 272; ++y)
            for (int x = -16; x < 272; ++x)
                if (check(fixtures[f], x, y)) return 1;
    for (int i = 0; i < 100000; ++i) {
        Vertex vertices[3];
        draw_offset.x = (int)(random_value() % 2048) - 1024;
        draw_offset.y = (int)(random_value() % 2048) - 1024;
        for (int v = 0; v < 3; ++v) {
            vertices[v] = (Vertex){
                (int)(random_value() % 4096) - 2048,
                (int)(random_value() % 4096) - 2048,
                (int)(random_value() % 257), (int)(random_value() % 257)};
        }
        for (int sample = 0; sample < 16; ++sample)
            if (check(vertices, random_value() % 1024, random_value() % 512)) return 1;
    }
    puts("Cached texture sampling: 2,014,720 exact comparisons passed");
    return 0;
}
