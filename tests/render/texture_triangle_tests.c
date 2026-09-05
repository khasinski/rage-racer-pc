#include <stdio.h>
#include <stdint.h>
#include "texture_triangle.h"
typedef TextureTriangleVertex Vertex;
static struct { int x, y; } draw_offset;
/* Frozen pre-optimization implementation, including per-scanline sorting. */
static bool ReferenceTriangleSpan(const Vertex input[3], int y,
                                   RasterTextureSpan* span) {
    Vertex v[3] = {input[0], input[1], input[2]};
    for (int i = 1; i < 3; i++) {
        Vertex key = v[i];
        int j = i;
        while (j > 0 && v[j - 1].y + draw_offset.y >
                            key.y + draw_offset.y) {
            v[j] = v[j - 1];
            j--;
        }
        v[j] = key;
    }
    int y0 = v[0].y + draw_offset.y;
    int y1 = v[1].y + draw_offset.y;
    int y2 = v[2].y + draw_offset.y;
    if (y2 == y0 || y < y0 || y > y2) return false;

    double t1;
    if (y < y1) {
        if (y1 == y0) return false;
        t1 = (double)(y - y0) / (double)(y1 - y0);
        span->x_left = v[0].x + draw_offset.x + (v[1].x - v[0].x) * t1;
        span->u_left = v[0].u + (v[1].u - v[0].u) * t1;
        span->v_left = v[0].v + (v[1].v - v[0].v) * t1;
        span->r_left = (int)(v[0].r + (v[1].r - v[0].r) * t1);
        span->g_left = (int)(v[0].g + (v[1].g - v[0].g) * t1);
        span->b_left = (int)(v[0].b + (v[1].b - v[0].b) * t1);
    } else {
        if (y2 == y1) return false;
        t1 = (double)(y - y1) / (double)(y2 - y1);
        span->x_left = v[1].x + draw_offset.x + (v[2].x - v[1].x) * t1;
        span->u_left = v[1].u + (v[2].u - v[1].u) * t1;
        span->v_left = v[1].v + (v[2].v - v[1].v) * t1;
        span->r_left = (int)(v[1].r + (v[2].r - v[1].r) * t1);
        span->g_left = (int)(v[1].g + (v[2].g - v[1].g) * t1);
        span->b_left = (int)(v[1].b + (v[2].b - v[1].b) * t1);
    }
    double t2 = (double)(y - y0) / (double)(y2 - y0);
    span->x_right = v[0].x + draw_offset.x + (v[2].x - v[0].x) * t2;
    span->u_right = v[0].u + (v[2].u - v[0].u) * t2;
    span->v_right = v[0].v + (v[2].v - v[0].v) * t2;
    span->r_right = (int)(v[0].r + (v[2].r - v[0].r) * t2);
    span->g_right = (int)(v[0].g + (v[2].g - v[0].g) * t2);
    span->b_right = (int)(v[0].b + (v[2].b - v[0].b) * t2);
    if (span->x_left > span->x_right) {
        double swap = span->x_left;
        span->x_left = span->x_right;
        span->x_right = swap;
        swap = span->u_left;
        span->u_left = span->u_right;
        span->u_right = swap;
        swap = span->v_left;
        span->v_left = span->v_right;
        span->v_right = swap;
        swap = span->r_left;
        span->r_left = span->r_right;
        span->r_right = swap;
        swap = span->g_left;
        span->g_left = span->g_right;
        span->g_right = swap;
        swap = span->b_left;
        span->b_left = span->b_right;
        span->b_right = swap;
    }
    span->x_start = (int)ceil(span->x_left);
    span->x_end = (int)floor(span->x_right);
    return span->x_start <= span->x_end;
}
static uint32_t state = 0x859714u;
static uint32_t next_value(void) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
int main(void) {
    unsigned long count = 0;
    for (int iteration = 0; iteration < 20000; ++iteration) {
        Vertex v[3];
        draw_offset.x = (int)(next_value() % 2048) - 1024;
        draw_offset.y = (int)(next_value() % 2048) - 1024;
        for (int i = 0; i < 3; ++i)
            v[i] = (Vertex){
                (int)(next_value() % 65536) - 32768,
                (int)(next_value() % 256) - 128,
                next_value() % 256, next_value() % 256,
                next_value() % 256, next_value() % 256, next_value() % 256};
        if (iteration % 3 == 0) v[1].y = v[0].y;
        if (iteration % 7 == 0) v[2].y = v[1].y;
        PreparedTextureTriangle p = TextureTrianglePrepare(
            v, draw_offset.x, draw_offset.y);
        for (int y = -130 + draw_offset.y; y <= 130 + draw_offset.y; ++y) {
            RasterTextureSpan a = {0}, b = {0};
            bool expected = ReferenceTriangleSpan(v, y, &a);
            bool actual = TextureTriangleSpanAt(&p, y, &b);
            if (expected != actual) {
                fprintf(stderr, "Coverage mismatch %d %d\n", iteration, y);
                return 1;
            }
            if (expected) {
#define SAME(field) if (a.field != b.field) { \
    fprintf(stderr, "Span mismatch %d %d: " #field "\n", iteration, y); return 1; }
                SAME(x_start); SAME(x_end); SAME(x_left); SAME(x_right);
                SAME(u_left); SAME(u_right); SAME(v_left); SAME(v_right);
                SAME(r_left); SAME(r_right); SAME(g_left); SAME(g_right);
                SAME(b_left); SAME(b_right);
#undef SAME
            }
            ++count;
        }
    }
    printf("Texture triangle: %lu reference scanlines matched\n", count);
    return 0;
}
