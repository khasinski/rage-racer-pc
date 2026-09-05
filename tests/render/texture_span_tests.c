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
            ++samples;
        }
    }
    printf("Prepared spans: %lu exact UV/RGB comparisons passed\n",samples);
    return 0;
}
