#include <stdint.h>
#include <stdio.h>
#include "triangle_coverage.h"
static long long RasterEdge(RasterPoint a, RasterPoint b, int x, int y) {
    return (long long)(b.x - a.x) * (y - a.y) -
           (long long)(b.y - a.y) * (x - a.x);
}

static bool ModernTriangleContains(const RasterPoint p[3], int x, int y) {
    long long e0 = RasterEdge(p[0], p[1], x, y);
    long long e1 = RasterEdge(p[1], p[2], x, y);
    long long e2 = RasterEdge(p[2], p[0], x, y);
    long long area = RasterEdge(p[0], p[1], p[2].x, p[2].y);
    if (area == 0) return false;
    /* A sample exactly at a polygon vertex touches two directed edges. Metal
     * can reject that endpoint even when the individual edge predicates below
     * are inclusive; treating it as uncovered is safe for the opaque
     * compatibility path and lets the PS1 span decide the final texel. */
    if ((e0 == 0) + (e1 == 0) + (e2 == 0) >= 2) return false;
    for (int i = 0; i < 3; i++) {
        long long edge = i == 0 ? e0 : i == 1 ? e1 : e2;
        RasterPoint a = p[i];
        RasterPoint b = p[(i + 1) % 3];
        if (area > 0) {
            if (edge < 0) return false;
            if (edge == 0 && !((b.y < a.y) ||
                               (b.y == a.y && b.x > a.x))) return false;
        } else {
            if (edge > 0) return false;
            if (edge == 0 && !((b.y > a.y) ||
                               (b.y == a.y && b.x < a.x))) return false;
        }
    }
    return true;
}
static uint32_t state=0x919131;
static unsigned random_value(void) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
static unsigned long count;
static int check(const RasterPoint p[3],const PreparedTriangleCoverage *c,int x,int y) {
    ++count;
    if (ModernTriangleContains(p,x,y)==TriangleCoverageContains(c,x,y)) return 0;
    fprintf(stderr,"Coverage mismatch at %d,%d\n",x,y);
    return 1;
}
int main(void) {
    for(int a=0;a<25;++a) for(int b=0;b<25;++b) for(int c=0;c<25;++c) {
        RasterPoint p[3]={{a%5-2,a/5-2},{b%5-2,b/5-2},{c%5-2,c/5-2}};
        PreparedTriangleCoverage prepared=TriangleCoveragePrepare(p);
        for(int y=-3;y<=3;++y) for(int x=-3;x<=3;++x)
            if(check(p,&prepared,x,y)) return 1;
    }
    for(int i=0;i<20000;++i) {
        RasterPoint p[3];
        for(int v=0;v<3;++v)
            p[v]=(RasterPoint){(int)(random_value()%65536)-32768,
                               (int)(random_value()%65536)-32768};
        PreparedTriangleCoverage prepared=TriangleCoveragePrepare(p);
        for(int v=0;v<3;++v) {
            if(check(p,&prepared,p[v].x,p[v].y)) return 1;
            int n=(v+1)%3;
            if(check(p,&prepared,(p[v].x+p[n].x)/2,(p[v].y+p[n].y)/2)) return 1;
        }
        for(int s=0;s<64;++s)
            if(check(p,&prepared,random_value()%1024,random_value()%512)) return 1;
    }
    printf("Triangle coverage: %lu exact comparisons passed\n",count);
    return 0;
}
