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
    return 0;
}
