#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include "texture_span.h"
typedef uint16_t u16;
typedef uint8_t u8;
#define CLAMP(x,lo,hi) ((x)<(lo)?(lo):(x)>(hi)?(hi):(x))
static void ReferenceSpanSample(const RasterTextureSpan* span, int x,
                                 u16* u, u16* v, u8* r, u8* g, u8* b) {
    double width = span->x_right - span->x_left;
    double start_t = width > 0.0 ?
        (span->x_start - span->x_left) / width : 0.0;
    double start_u = span->u_left +
        (span->u_right - span->u_left) * start_t;
    double start_v = span->v_left +
        (span->v_right - span->v_left) * start_t;
    int fixed_u = (int)(start_u * 65536.0);
    int fixed_v = (int)(start_v * 65536.0);
    int step_u = width > 0.0 ?
        (int)((span->u_right - span->u_left) / width * 65536.0) : 0;
    int step_v = width > 0.0 ?
        (int)((span->v_right - span->v_left) / width * 65536.0) : 0;
    int fixed_r = (int)((span->r_left +
        (span->r_right - span->r_left) * start_t) * 65536.0);
    int fixed_g = (int)((span->g_left +
        (span->g_right - span->g_left) * start_t) * 65536.0);
    int fixed_b = (int)((span->b_left +
        (span->b_right - span->b_left) * start_t) * 65536.0);
    int step_r = width > 0.0 ?
        (int)((span->r_right - span->r_left) / width * 65536.0) : 0;
    int step_g = width > 0.0 ?
        (int)((span->g_right - span->g_left) / width * 65536.0) : 0;
    int step_b = width > 0.0 ?
        (int)((span->b_right - span->b_left) / width * 65536.0) : 0;
    fixed_u += (x - span->x_start) * step_u;
    fixed_v += (x - span->x_start) * step_v;
    fixed_r += (x - span->x_start) * step_r;
    fixed_g += (x - span->x_start) * step_g;
    fixed_b += (x - span->x_start) * step_b;
    *u = (u16)((fixed_u >> 16) & 0xff);
    *v = (u16)((fixed_v >> 16) & 0xff);
    *r = (u8)CLAMP(fixed_r >> 16, 0, 255);
    *g = (u8)CLAMP(fixed_g >> 16, 0, 255);
    *b = (u8)CLAMP(fixed_b >> 16, 0, 255);
}
static uint32_t state = 0x89113;
static unsigned next_value(void) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
int main(void) {
    for (int iteration = 0; iteration < 1000000; ++iteration) {
        PreparedTextureSpan p = {.x_start = -17,
            .fixed_u = (int)(next_value() % 67108864) - 33554432,
            .step_u = iteration % 2 ? 65536 + (int)(next_value() % 4097) - 2048 :
                (int)(next_value() % 262145) - 131072};
        int x = p.x_start + next_value() % 129;
        int remaining = 1 + next_value() % 512;
        u16 first, v;
        TextureSpanSampleUV(&p, x, &first, &v);
        int expected = 1;
        while (expected < remaining && first + expected < 256) {
            u16 u;
            TextureSpanSampleUV(&p, x + expected, &u, &v);
            if (u != first + expected) break;
            expected++;
        }
        int actual = TextureSpanConsecutiveRun(&p, x, first, remaining);
        if (actual != expected) {
            fprintf(stderr, "Consecutive run mismatch iteration=%d expected=%d actual=%d\n",
                iteration, expected, actual);
            return 1;
        }
        p.step_v = 1;
        if (TextureSpanConsecutiveRun(&p, x, first, remaining) != 1) return 1;
        p.step_v = 0; p.step_b = 1;
        if (TextureSpanConsecutiveRun(&p, x, first, remaining) != 1) return 1;
    }
    puts("Consecutive spans: 1000000 independent texel-walk comparisons passed");
    for (int first = 0; first < 256; ++first) {
        for (int remaining = 1; remaining <= 512; ++remaining) {
            PreparedTextureSpan p = {.x_start = -13, .fixed_u = first * 65536 + 32767,
                .fixed_v = 77 * 65536, .fixed_r = 43 * 65536,
                .fixed_g = 89 * 65536, .fixed_b = 255 * 65536, .step_u = 65536};
            int run = TextureSpanUnitRun(&p, first, remaining);
            if (run < 1 || run > remaining) return 1;
            for (int i = 0; i < run; ++i) {
                u16 u,v; u8 r,g,b;
                TextureSpanSample(&p,p.x_start+i,&u,&v,&r,&g,&b);
                if (u != first+i || v != 77 || r != 43 || g != 89 || b != 255) return 1;
            }
            if (run < remaining) {
                u16 u,v;
                TextureSpanSampleUV(&p,p.x_start+run,&u,&v);
                if (u != 0) return 1;
            }
            p.step_u = -65536;
            if (TextureSpanUnitRun(&p, first, remaining) != 1) return 1;
            p.step_u = 65535;
            if (TextureSpanUnitRun(&p, first, remaining) != 1) return 1;
            p.step_u = 65536; p.step_v = 1;
            if (TextureSpanUnitRun(&p, first, remaining) != 1) return 1;
            p.step_v = 0; p.step_r = 1;
            if (TextureSpanUnitRun(&p, first, remaining) != 1) return 1;
        }
    }
    unsigned long samples = 0;
    for (int iteration = 0; iteration < 10000; ++iteration) {
        RasterTextureSpan s = {0};
        s.x_left = (int)(next_value() % 1024) - 512 + (next_value() % 16) / 16.0;
        s.x_right = s.x_left + next_value() % 1024;
        s.x_start = (int)ceil(s.x_left);
        s.x_end = (int)floor(s.x_right);
        s.u_left = next_value() % 512; s.u_right = next_value() % 512;
        s.v_left = next_value() % 512; s.v_right = next_value() % 512;
        s.r_left = next_value() % 256; s.r_right = next_value() % 256;
        s.g_left = next_value() % 256; s.g_right = next_value() % 256;
        s.b_left = next_value() % 256; s.b_right = next_value() % 256;
        if (iteration % 100 == 0)
            s.x_left = s.x_right = s.x_start = s.x_end = 0;
        PreparedTextureSpan p = TextureSpanPrepare(&s);
        for (int x = s.x_start; x <= s.x_end; ++x) {
            u16 au,av,bu,bv; u8 ar,ag,ab,br,bg,bb;
            ReferenceSpanSample(&s,x,&au,&av,&ar,&ag,&ab);
            TextureSpanSample(&p,x,&bu,&bv,&br,&bg,&bb);
            if (au!=bu || av!=bv || ar!=br || ag!=bg || ab!=bb) {
                fprintf(stderr,"Span mismatch iteration=%d x=%d\n",iteration,x);
                return 1;
            }
            bu = bv = 0; br = bg = bb = 0;
            TextureSpanSampleUV(&p,x,&bu,&bv);
            TextureSpanSampleColor(&p,x,&br,&bg,&bb);
            if (au!=bu || av!=bv || ar!=br || ag!=bg || ab!=bb) {
                fprintf(stderr,"Split span mismatch iteration=%d x=%d\n",iteration,x);
                return 1;
            }
            ++samples;
        }
    }
    printf("Prepared spans: %lu exact UV/RGB comparisons passed\n",samples);
    return 0;
}
