#include "classic_motion.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static RageSceneSnapshot a, b;
static RageClassicPacketSource sa[RAGE_CAPTURE_MAX_PACKETS], sb[RAGE_CAPTURE_MAX_PACKETS];

static void Quad(RageSceneSnapshot *s, RageClassicPacketSource *sources,
                 int packet, int draw, int x) {
    RageCaptureFace *f = &s->faces[packet];
    f->drawIndex = (int16_t)draw;
    f->pos[0][0] = (int16_t)(packet * 10);
    f->pos[1][0] = f->pos[0][0] + 8;
    f->pos[2][1] = f->pos[3][1] = 8;
    sources[packet] = (RageClassicPacketSource){packet, 0, 28};
    RageCapturePacket *p = &s->packets[packet];
    p->flags = RAGE_CAPTURE_PACKET_3D;
    p->size = 5;
    p->words[0] = 0x28808080;
    for (int v = 0; v < 4; ++v)
        p->words[v+1] = (uint16_t)(x + (v&1)*8) | ((uint32_t)(20 + (v>>1)*8)<<16);
    if (s->packetCount <= packet) s->packetCount = s->faceCount = packet + 1;
}

static void Reset(void) {
    ClassicMotionReset();
    memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b));
    memset(sa,0,sizeof(sa)); memset(sb,0,sizeof(sb));
    a.frameCounter = 100; b.frameCounter = 101;
    a.sceneId = b.sceneId = 12;
    a.sceneTimer = 500; b.sceneTimer = 501;
    a.displayHeight = b.displayHeight = 240;
    a.drawCount = b.drawCount = 1;
    for (int i = 0; i < 3; ++i) a.viewMatrix.m[i][i] = b.viewMatrix.m[i][i] = 4096;
    Quad(&a,sa,0,0,10); Quad(&b,sb,0,0,30);
}
static void Prepare(void) { ClassicMotionPrepare(&a,sa,&b,sb); }

int main(void) {
    float x[4]={10,18,10,18}, y[4]={20,20,28,28};
    RageCapturePacket sky = {.flags = RAGE_CAPTURE_PACKET_SKY};
    CHECK(CapturePacketIsMainSky(&sky));
    sky.table = 1; CHECK(!CapturePacketIsMainSky(&sky));
    sky.table = 0; sky.flags = RAGE_CAPTURE_PACKET_3D;
    CHECK(!CapturePacketIsMainSky(&sky));
    CHECK(!CapturePacketIsMainSky(NULL));
    Reset(); Prepare();
    CHECK(ClassicMotionMatchCount()==1);
    CHECK(ClassicMotionCoordinates(0,0.5f,x,y));
    CHECK(x[0]==20 && x[1]==28 && y[0]==20);
    CHECK(a.packets[0].words[1] == (10u | (20u<<16))); /* source immutable */
    CHECK(!ClassicMotionCoordinates(0,NAN,x,y));
    CHECK(!ClassicMotionCoordinates(-1,0.5f,x,y));
    CHECK(!ClassicMotionCoordinates(1,0.5f,x,y));

    Reset(); b.sceneId++; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.sceneTimer=a.sceneTimer; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.frameCounter++; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.viewPosition[0]=10000; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.viewMatrix.m[0][0]=-4096; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.packetOverflow=1; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); sb[0].bytes*=2; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); sb[0].offset=28; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.packets[0].words[1]+=300; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.packets[0].flags=0; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.draws[0].bankId=1; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.draws[0].table=1; Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); b.draws[0].mirror=1; Prepare(); CHECK(ClassicMotionMatchCount()==0);

    /* Camera moves while the object stays fixed: GPU translations use 4x
     * world units. A different same-model draw occupies the old view origin. */
    Reset(); b.viewPosition[0]=2000; b.drawCount=2;
    b.draws[0].gte.rot.t[0]=-8000;
    b.draws[1].gte.rot.t[0]=0;
    Prepare(); CHECK(ClassicMotionMatchCount()==1);
    Reset(); a.drawCount=b.drawCount=2;
    /* Coincident identical model draws cannot be paired safely. */
    Prepare(); CHECK(ClassicMotionMatchCount()==0);

    /* An OT reorder must not change correspondence. */
    Reset(); Quad(&a,sa,1,0,40); Quad(&b,sb,1,0,60);
    RageCapturePacket temp=b.packets[0]; b.packets[0]=b.packets[1];b.packets[1]=temp;
    RageClassicPacketSource ts=sb[0];sb[0]=sb[1];sb[1]=ts;
    Prepare(); CHECK(ClassicMotionMatchCount()==2);
    x[0]=10; CHECK(ClassicMotionCoordinates(0,0.5f,x,y)); CHECK(x[0]==20);

    /* Duplicate child identity on either side must not morph two polygons
     * into one. */
    Reset(); b.packetCount=2;b.packets[1]=b.packets[0];sb[1]=sb[0];
    Prepare(); CHECK(ClassicMotionMatchCount()==0);
    Reset(); a.packetCount=2;a.packets[1]=a.packets[0];sa[1]=sa[0];
    Prepare(); CHECK(ClassicMotionMatchCount()==0);

    /* Adjacent road polygons share an edge. A subdivision change on one
     * must not leave it behind while its neighbour moves away. */
    Reset(); Quad(&a,sa,1,0,18); Quad(&b,sb,1,0,38);
    Prepare(); CHECK(ClassicMotionMatchCount()==2);
    Reset(); Quad(&a,sa,1,0,18); Quad(&b,sb,1,0,38);
    sb[1].bytes *= 2;
    Prepare(); CHECK(ClassicMotionMatchCount()==0);

    /* A changed projection on a common vertex cannot split a welded edge,
     * even when both individual packet identities match successfully. */
    Reset(); Quad(&a,sa,1,0,18); Quad(&b,sb,1,0,39);
    Prepare(); CHECK(ClassicMotionMatchCount()==0);

    /* Hold propagation crosses an intervening matched child. */
    Reset();
    Quad(&a,sa,1,0,18); Quad(&b,sb,1,0,38);
    Quad(&a,sa,2,0,26); Quad(&b,sb,2,0,46);
    sb[2].bytes *= 2;
    Prepare(); CHECK(ClassicMotionMatchCount()==0);

    /* Terrain and course packets belong to the same projected road, even
     * when they came from separate source draws. */
    Reset(); a.drawCount=b.drawCount=2;
    a.draws[1].gte.rot.t[0]=b.draws[1].gte.rot.t[0]=8000;
    Quad(&a,sa,1,1,18); Quad(&b,sb,1,1,38);
    a.faces[0].kind=b.faces[0].kind=RAGE_CAPTURE_KIND_COURSE;
    a.faces[1].kind=b.faces[1].kind=RAGE_CAPTURE_KIND_COURSE;
    sb[1].bytes *= 2;
    Prepare(); CHECK(ClassicMotionMatchCount()==0);

    /* Overlapping independent cars must not hold each other. */
    Reset(); a.drawCount=b.drawCount=2;
    a.draws[1].gte.rot.t[0]=b.draws[1].gte.rot.t[0]=8000;
    Quad(&a,sa,1,1,10); Quad(&b,sb,1,1,30);
    sb[1].bytes *= 2;
    Prepare(); CHECK(ClassicMotionMatchCount()==1);

    Reset(); ClassicMotionPrepare(&a,NULL,&b,sb); CHECK(ClassicMotionMatchCount()==0);
    ClassicMotionReset();CHECK(!ClassicMotionCoordinates(0,0.5f,x,y));
    return 0;
}
