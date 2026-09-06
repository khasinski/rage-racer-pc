#include "texture_patch.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <miniz.h>
#ifdef _WIN32
#include <direct.h>
#define MakeDir(p) _mkdir(p)
#define RemoveDir(p) _rmdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define MakeDir(p) mkdir(p,0700)
#define RemoveDir(p) rmdir(p)
#endif
static void Be32(FILE *f,uint32_t n) {
    unsigned char b[4]={(unsigned char)(n>>24),(unsigned char)(n>>16),(unsigned char)(n>>8),(unsigned char)n};
    assert(fwrite(b,1,4,f)==4);
}
static void Chunk(FILE *f,const char *tag,const unsigned char *data,size_t size) {
    Be32(f,(uint32_t)size);assert(fwrite(tag,1,4,f)==4);
    assert(fwrite(data,1,size,f)==size);
    mz_ulong crc=mz_crc32(0,(const unsigned char *)tag,4);
    Be32(f,(uint32_t)mz_crc32(crc,data,size));
}
static void Write(const char *path,const char *text) {
    FILE *f=fopen(path,"wb");assert(f);
    assert(fwrite(text,1,strlen(text),f)==strlen(text));assert(!fclose(f));
}
int main(void) {
    const char *root="texture_patch_fixture",*dir="texture_patch_fixture/textures";
    assert(!MakeDir(root));assert(!MakeDir(dir));
    FILE *f=fopen("texture_patch_fixture/textures/a.png","wbx");assert(f);
    assert(fwrite("\x89PNG\r\n\x1a\n",1,8,f)==8);
    const unsigned char ihdr[13]={0,0,0,1,0,0,0,1,8,6,0,0,0};
    /* zlib with one stored DEFLATE block: filter=0, one opaque red pixel. */
    const unsigned char idat[]={0x78,0x01,0x01,5,0,0xfa,0xff,0,255,0,0,255,5,0,1,255};
    Chunk(f,"IHDR",ihdr,sizeof(ihdr));Chunk(f,"IDAT",idat,sizeof(idat));
    Chunk(f,"IEND",ihdr,0);assert(!fclose(f));
    unsigned char original[528],bytes[528],expected[528];
    memset(original,0xa5,sizeof(original));
    for(int depth=4;depth<=16;depth*=2) {
        memset(original+1,0,512);original[3]=31; /* Unaligned palette, red slot 1. */
        original[520]=(unsigned char)(depth==4?0xa0:0);
        original[521]=(unsigned char)(depth==16?0:0xa5);
        memcpy(bytes,original,sizeof(bytes));memcpy(expected,original,sizeof(expected));
        expected[520]=(unsigned char)(depth==16?31:depth==4?0xa1:1);
        char json[256];
        snprintf(json,sizeof(json),"{\"asset\":0,\"pixels_offset\":520,\"pixel_bytes\":2,\"depth\":%d,\"width\":1,\"height\":1,\"offset\":1,\"colours\":%d}",depth,depth==4?16:256);
        Write("texture_patch_fixture/textures/a.json",json);
        Write("texture_patch_fixture/textures/index.txt","# comment\r\n0 a.json\r\n");
        assert(TexturePatchAsset(root,0,bytes,sizeof(bytes))==1);
        assert(!memcmp(bytes,expected,sizeof(bytes))); /* Including padding and sentinels. */
        assert(TexturePatchAsset(root,0,bytes,sizeof(bytes))==0); /* Untouched repack. */
        memcpy(bytes,original,sizeof(bytes));
        assert(TexturePatchAsset(root,1,bytes,sizeof(bytes))==0);
        assert(!memcmp(bytes,original,sizeof(bytes)));
        Write("texture_patch_fixture/textures/index.txt","0 ../a.json\n0 a.json trailing\n");
        assert(TexturePatchAsset(root,0,bytes,sizeof(bytes))==0);
        assert(!memcmp(bytes,original,sizeof(bytes)));
        f=fopen("texture_patch_fixture/textures/index.txt","wb");assert(f);
        const char nulEntry[]="0 a.json\0hidden\n";
        assert(fwrite(nulEntry,1,sizeof(nulEntry)-1,f)==sizeof(nulEntry)-1);
        assert(!fclose(f));
        assert(TexturePatchAsset(root,0,bytes,sizeof(bytes))==0);
        assert(!memcmp(bytes,original,sizeof(bytes)));
        Write("texture_patch_fixture/textures/index.txt","0 a.json\n");
        assert(TexturePatchAsset(root,0,bytes,521)==0); /* Pixel range exceeds asset. */
        assert(!memcmp(bytes,original,sizeof(bytes)));
    }
    assert(!remove("texture_patch_fixture/textures/index.txt"));
    assert(TexturePatchAsset(root,0,bytes,sizeof(bytes))==-1);
    assert(!remove("texture_patch_fixture/textures/a.json"));
    assert(!remove("texture_patch_fixture/textures/a.png"));
    assert(!RemoveDir(dir));assert(!RemoveDir(root));
    puts("compiled texture patch: 4/8/16-bit edits, unchanged repack and rejection passed");
    return 0;
}
