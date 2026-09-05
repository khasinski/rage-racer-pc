#include "render/rmesh.h"
#include <math.h>
#include <stdio.h>
#include "erriso_body.inc"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    RageRuntimeMesh mesh;
    RageRuntimeVertex v;
    uint32_t i,material=0;
    /* Retail body bounds plus four model units for narrow panel bevels. */
    float low[3]={-126,-23,-81}, high[3]={126,143,360};
    CHECK(RuntimeMeshOpen(&mesh,s_errisoBody,sizeof(s_errisoBody)));
    CHECK(mesh.meshCount==1 && mesh.indexCount>228*3 && mesh.indexCount<20000*3);
    for(i=0;i<mesh.indexCount;i++) {
        uint32_t index,axis,slot;
        CHECK(RuntimeMeshIndex(&mesh,i,&index) && RuntimeMeshVertex(&mesh,index,&v));
        for(axis=0;axis<3;axis++) CHECK(v.position[axis]>=low[axis] && v.position[axis]<=high[axis]);
        CHECK(v.color[3]==255);
        CHECK(v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2]>0.9f);
        slot=v.material & RAGE_RUNTIME_MATERIAL_INDEX_MASK;
        CHECK(slot<10 || slot==65535);
        if(i%3==0) material=v.material;
        else CHECK(v.material==material);
    }
    puts("authored Erriso geometry, scale, normals and materials valid");
    return 0;
}
