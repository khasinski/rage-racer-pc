#include "render/rmesh_replace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static void W(uint8_t *p,uint32_t v) {
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
int main(void) {
    uint8_t baseBytes[180]={0}, replacementBytes[176]={0};
    RageRuntimeMesh base, replacement, result;
    RageRuntimeVertex v;
    uint32_t map[1]={7},first,count,index,i;
    size_t size=123;
    void *bytes;
    memcpy(baseBytes,"RRMESH1",7);W(baseBytes+8,1);W(baseBytes+12,2);
    W(baseBytes+16,3);W(baseBytes+20,6);W(baseBytes+28,3);W(baseBytes+32,6);
    memcpy(replacementBytes,"RRMESH1",7);W(replacementBytes+8,1);W(replacementBytes+12,1);
    W(replacementBytes+16,3);W(replacementBytes+20,6);W(replacementBytes+28,6);
    for(i=0;i<3;i++) {
        float p=(float)i;
        memcpy(baseBytes+36+i*40,&p,4);
        memcpy(replacementBytes+32+i*40,&p,4);
        W(replacementBytes+32+i*40+36,0x20ef0000u);
    }
    for(i=0;i<6;i++) {W(baseBytes+156+i*4,i%3);W(replacementBytes+152+i*4,2-i%3);}
    CHECK(RuntimeMeshOpen(&base,baseBytes,sizeof(baseBytes)));
    CHECK(RuntimeMeshOpen(&replacement,replacementBytes,sizeof(replacementBytes)));
    bytes=RuntimeMeshReplace(&base,0,&replacement,map,1,&size);
    CHECK(bytes && RuntimeMeshOpen(&result,bytes,size));
    CHECK(result.meshCount==2 && result.vertexCount==6 && result.indexCount==9);
    CHECK(RuntimeMeshRange(&result,0,&first,&count) && first==0 && count==6);
    CHECK(RuntimeMeshRange(&result,1,&first,&count) && first==6 && count==3);
    for(i=0;i<3;i++) {
        CHECK(RuntimeMeshIndex(&result,6+i,&index) && index==i);
        CHECK(RuntimeMeshIndex(&result,i,&index) && index==5-i);
        CHECK(RuntimeMeshVertex(&result,3+i,&v) && v.material==0x20ef0007u);
    }
    CHECK(!memcmp(result.bytes+result.verticesOffset,baseBytes+36,120));
    free(bytes);
    CHECK(!RuntimeMeshReplace(&base,2,&replacement,map,1,&size) && size==0);
    CHECK(!RuntimeMeshReplace(&base,0,&replacement,map,0,&size));
    map[0]=UINT32_MAX;
    CHECK(!RuntimeMeshReplace(&base,0,&replacement,map,1,&size));
    W(replacementBytes+32+36,0x20efffffu);
    W(replacementBytes+72+36,UINT32_MAX);
    map[0]=7;
    bytes=RuntimeMeshReplace(&base,0,&replacement,map,1,&size);
    CHECK(bytes && RuntimeMeshOpen(&result,bytes,size));
    CHECK(RuntimeMeshVertex(&result,3,&v) && v.material==0x20efffffu);
    CHECK(RuntimeMeshVertex(&result,4,&v) && v.material==UINT32_MAX);
    free(bytes);
    replacementBytes[0]=0;
    CHECK(!RuntimeMeshReplace(&base,0,&replacement,map,1,&size) && size==0);
    puts("submesh replacement, wheel preservation, material remap tests passed");
    return 0;
}
