#include <stdio.h>
#include <stdlib.h>
#include "vram_read_cache.h"
static VramReadCache cache;
static uint16_t page[448 * 256], output[448 * 256];
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    for (unsigned i=0;i<448*256;++i) page[i]=(uint16_t)(i*17);
    CHECK(!VramReadCacheRead(&cache,576,256,448,256,output));
    VramReadCacheWrite(&cache,576,256,448,256,page);
    for(int row=0;row<256;++row) {
        CHECK(VramReadCacheRead(&cache,576,256+row,448,1,output));
        CHECK(memcmp(output,page+row*448,448*sizeof(uint16_t))==0);
    }
    VramReadCacheInvalidate(&cache,0,0,576,512);
    CHECK(VramReadCacheRead(&cache,576,256,448,256,output));
    VramReadCacheInvalidate(&cache,700,300,1,1);
    CHECK(!VramReadCacheRead(&cache,576,256,448,256,output));
    CHECK(VramReadCacheRead(&cache,576,301,448,1,output));
    VramReadCacheWrite(&cache,700,300,1,1,page);
    CHECK(VramReadCacheRead(&cache,576,256,448,256,output));
    CHECK(output[44*448+124]==page[0]);
    VramReadCacheReset(&cache);
    CHECK(!VramReadCacheRead(&cache,576,256,448,1,output));
    CHECK(!VramReadCacheRect(-1,0,1,1));
    CHECK(!VramReadCacheRect(1023,0,2,1));
    CHECK(!VramReadCacheRect(0,511,1,2));
    CHECK(!VramReadCacheRect(0,0,0,1));
    VramReadCacheWrite(&cache,0,0,448,256,page);
    VramReadCacheInvalidate(&cache,-1,0,1,1);
    CHECK(!VramReadCacheRead(&cache,0,0,1,1,output));
    puts("VRAM read cache: row swaps, partial invalidation, reset and boundaries passed");
    return 0;
}
