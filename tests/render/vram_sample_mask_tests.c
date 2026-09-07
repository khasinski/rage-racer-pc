#include <limits.h>
#include <stdio.h>
#include "vram_sample_mask.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)

int main(void) {
    VramSampleMask mask;
    for (int tile = 0; tile < 512; ++tile) {
        int x = (tile % 32) * 32, y = (tile / 32) * 32;
        VramSampleMaskReset(&mask, false);
        VramSampleMaskMark(&mask, x+31, y+31, 1, 1);
        CHECK(VramSampleMaskIntersects(&mask,x,y,32,32));
        for (int other = 0; other < 512; ++other)
            CHECK(VramSampleMaskIntersects(&mask,(other%32)*32,(other/32)*32,1,1) == (other==tile));
        VramSampleMaskMark(&mask,0,0,1,1);
        CHECK(VramSampleMaskIntersects(&mask,x,y,32,32));
        VramSampleMaskReset(&mask,false);
        CHECK(!VramSampleMaskIntersects(&mask,0,0,1024,512));
    }
    VramSampleMaskMark(&mask,-1,-1,2,2);
    CHECK(VramSampleMaskIntersects(&mask,0,0,1,1));
    CHECK(!VramSampleMaskIntersects(&mask,32,0,1,1));
    VramSampleMaskReset(&mask,false);
    VramSampleMaskMark(&mask,INT_MAX,INT_MAX,INT_MAX,INT_MAX);
    VramSampleMaskMark(&mask,INT_MIN,INT_MIN,INT_MAX,INT_MAX);
    VramSampleMaskMark(&mask,0,0,-1,1);
    CHECK(!VramSampleMaskIntersects(&mask,0,0,1024,512));
    VramSampleMaskMark(&mask,1023,511,INT_MAX,INT_MAX);
    CHECK(VramSampleMaskIntersects(&mask,1023,511,1,1));
    CHECK(!VramSampleMaskIntersects(&mask,1024,512,1,1));
    VramSampleMaskReset(&mask,true);
    CHECK(!VramSampleMaskTextureDirty(&mask,0x8000,0));

    /* Independent shader-address oracle: every possible UV must consult a
     * tracked source word, and indexed modes must include their whole CLUT. */
    unsigned samples=0;
    for (unsigned page=0; page<512; ++page) {
        unsigned depth=(page>>7)&3, shift=depth==0?2:depth==1?1:0;
        for (unsigned u=0; u<256; ++u) {
            unsigned v=(u*73+page*19)&255;
            unsigned x=(page%16)*64+(u>>shift), y=((page/16)%2)*256+v;
            if (x>=1024) continue;
            VramSampleMaskReset(&mask,false);
            VramSampleMaskMark(&mask,x,y,1,1);
            CHECK(VramSampleMaskTextureDirty(&mask,page,0));
            ++samples;
        }
        if (depth>=2) continue;
        for (unsigned clut=0; clut<32768; clut+=97) {
            unsigned count=depth?256:16;
            for (unsigned index=0; index<count; ++index) {
                unsigned x=(clut%64)*16+index, y=clut/64;
                if (x>=1024) continue;
                VramSampleMaskReset(&mask,false);
                VramSampleMaskMark(&mask,x,y,1,1);
                CHECK(VramSampleMaskTextureDirty(&mask,page,clut));
                ++samples;
            }
        }
    }
    VramSampleMaskReset(&mask,false);
    VramSampleMaskMark(&mask,0,511,1,1);
    CHECK(!VramSampleMaskTextureDirty(&mask,0x108,511*64));
    CHECK(VramSampleMaskTextureDirty(&mask,0x008,511*64));
    printf("VRAM sample mask: %u shader-address checks, tile retention and clipping passed\n",samples);
    return 0;
}
