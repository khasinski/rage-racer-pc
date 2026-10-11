#include "content/track_pack.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void Write32(unsigned char *p, uint32_t n) {
    p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);
    p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);
}
int main(void) {
    unsigned char storage[89]={0};
    unsigned char *bytes=storage+1; /* deliberately unaligned source */
    TrackBlock blocks[TRACK_PACK_BLOCK_COUNT], before[TRACK_PACK_BLOCK_COUNT];
    unsigned i;
    for(i=0;i<TRACK_PACK_BLOCK_COUNT;i++)Write32(bytes+i*4,44+i*4);
    assert(TrackPackOpen(bytes,88,blocks));
    for(i=0;i<TRACK_PACK_BLOCK_COUNT;i++) {
        assert(blocks[i].data==bytes+44+i*4 && blocks[i].size==4);
        assert(TrackBlockName(i));
    }
    assert(!TrackBlockName(TRACK_PACK_BLOCK_COUNT));
    memcpy(before,blocks,sizeof(blocks));
    for(i=0;i<44;i++)assert(!TrackPackOpen(bytes,i,blocks));
    assert(!TrackPackOpen(NULL,88,blocks));
    assert(!TrackPackOpen(bytes,88,NULL));
    Write32(bytes,43);assert(!TrackPackOpen(bytes,88,blocks));
    Write32(bytes,44);
    Write32(bytes+4,44);assert(!TrackPackOpen(bytes,88,blocks));
    Write32(bytes+4,UINT32_MAX);assert(!TrackPackOpen(bytes,88,blocks));
    Write32(bytes+4,48);
    Write32(bytes+40,88);assert(!TrackPackOpen(bytes,88,blocks));
    assert(!memcmp(before,blocks,sizeof(blocks)));
    {
        unsigned char route[41]={0};
        Road road, prior;
        RoadPoint point;
        unsigned char *r=route+1;
        Write32(r,1);Write32(r+4,UINT32_MAX);Write32(r+8,1234);
        r[24]=1;r[26]=100; /* primary curve at arc 0; segment length 100 */
        Write32(r+28,200);Write32(r+32,300);Write32(r+36,0x12345678);
        assert(RoadOpen(r,40,&road));
        assert(road.count==1 && road.arcCount==1 && road.length==100);
        assert(RoadRead(&road,0,&point));
        assert(point.x==-1 && point.z==1234 && point.arc==1 && point.length==100);
        assert(!RoadRead(&road,1,&point));
        prior=road;
        assert(!RoadOpen(r,27,&road)); /* missing arc */
        r[24]=3;assert(!RoadOpen(r,40,&road)); /* unknown curve */
        r[24]=0xf1;r[25]=0xff;assert(!RoadOpen(r,40,&road)); /* negative arc index */
        r[24]=0x11;r[25]=0;assert(!RoadOpen(r,40,&road)); /* missing second arc */
        r[24]=0;r[26]=0;assert(!RoadOpen(r,40,&road));
        r[26]=0;r[27]=0x80;assert(!RoadOpen(r,40,&road)); /* signed length */
        assert(!memcmp(&road,&prior,sizeof(road)));
        Write32(r,UINT32_MAX);assert(!RoadOpen(r,40,&road));
        assert(!RoadOpen(NULL,40,&road));assert(!RoadOpen(r,40,NULL));
    }
    return 0;
}
