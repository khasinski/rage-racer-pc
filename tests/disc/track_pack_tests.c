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
    {
        unsigned char storage[47]={0},before[46];unsigned char *p=storage+1;
        Write32(p,2);p[4]=1;p[24]=0xff;p[25]=0xff;
        p[44]=0xab;p[45]=0xcd;
        Placements objects,prior;Placement value;
        assert(PlacementsOpen(p,46,2,&objects));prior=objects;
        assert(PlacementRead(&objects,1,&value)&&value.model==-1);
        value=(Placement){0,-2048,INT32_MIN,INT32_MAX,-7,15};
        assert(PlacementWrite(p,46,2,0,&value));
        assert(PlacementRead(&objects,0,&value));
        assert(value.x==INT32_MIN&&value.y==INT32_MAX&&value.z==-7&&value.yaw==-2048&&value.flags==15);
        assert(p[44]==0xab&&p[45]==0xcd&&p[24]==0xff&&p[25]==0xff);
        memcpy(before,p,46);value.flags=16;assert(!PlacementWrite(p,46,2,0,&value));
        value.flags=0;value.model=2;assert(!PlacementWrite(p,46,2,0,&value));
        value.model=-2;assert(!PlacementWrite(p,46,2,0,&value));
        value.model=1;assert(!PlacementWrite(p,46,2,2,&value));
        assert(!memcmp(before,p,46));
        assert(!PlacementRead(&objects,2,&value));assert(!PlacementRead(NULL,0,&value));
        p[20]=16;assert(!PlacementsOpen(p,46,2,&objects));
        memcpy(p,before,46);p[4]=2;assert(!PlacementsOpen(p,46,2,&objects));
        memcpy(p,before,46);
        assert(!PlacementsOpen(p,43,2,&objects));assert(!memcmp(&objects,&prior,sizeof(prior)));
        Write32(p,UINT32_MAX);assert(!PlacementsOpen(p,46,2,&objects));
        assert(!PlacementsOpen(NULL,46,2,&objects));
    }
    return 0;
}
