#include "render/rmesh.h"
#include <math.h>
#include <stdio.h>
#include "erriso_body.inc"
#include "erriso_rival.inc"
#include "abeille_body.inc"
#include "abeille_rival.inc"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static int Validate(const void *bytes,size_t size,int rival,int abeille) {
    RageRuntimeMesh mesh;
    RageRuntimeVertex v;
    uint32_t i,material=0;
    /* Retail body bounds plus four model units for narrow panel bevels. */
    float low[3]={-126,-23,-81}, high[3]={126,143,360};
    if(abeille) {
        low[0]=-147; low[1]=-43; low[2]=-73;
        high[0]=147; high[1]=142; high[2]=436;
    }
    CHECK(RuntimeMeshOpen(&mesh,bytes,size));
    CHECK(mesh.meshCount==1 && mesh.indexCount>228*3 && mesh.indexCount<20000*3);
    for(i=0;i<mesh.indexCount;i++) {
        uint32_t index,axis,slot;
        CHECK(RuntimeMeshIndex(&mesh,i,&index) && RuntimeMeshVertex(&mesh,index,&v));
        for(axis=0;axis<3;axis++) CHECK(v.position[axis]>=low[axis] && v.position[axis]<=high[axis]);
        CHECK(v.color[3]==255);
        CHECK(v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2]>0.9f);
        slot=v.material & RAGE_RUNTIME_MATERIAL_INDEX_MASK;
        CHECK(slot==65535 || (rival ? (abeille ?
            (slot==0 || slot==11 || slot==12 || slot==17) :
            (slot==0 || slot==13 || slot==14 || slot==19)) : slot<(abeille?12u:10u)));
        if(i%3==0) material=v.material;
        else CHECK(v.material==material);
    }
    return 0;
}

static int ValidateAbeillePanelColorSeam(void) {
    RageRuntimeMesh mesh;
    RageRuntimeVertex v;
    uint32_t i;
    int dark=0, light=0;
    /* A bumper/body seam shares a position but deliberately has distinct
     * native multipliers. Blender's default CORNER-to-POINT OBJ conversion
     * averages them unless the export mesh splits this vertex. */
    CHECK(RuntimeMeshOpen(&mesh,s_abeille_body,sizeof(s_abeille_body)));
    for(i=0;i<mesh.vertexCount;i++) {
        CHECK(RuntimeMeshVertex(&mesh,i,&v));
        if(fabsf(v.position[0]-140.078f)<0.01f &&
           fabsf(v.position[1]-21.167f)<0.01f &&
           fabsf(v.position[2]+41.287f)<0.01f) {
            if(v.color[0]==12 && v.color[1]==12 && v.color[2]==12) dark=1;
            if(v.color[0]==255 && v.color[1]==255 && v.color[2]==255) light=1;
        }
    }
    CHECK(dark && light);
    return 0;
}

int main(void) {
    CHECK(Validate(s_errisoBody,sizeof(s_errisoBody),0,0)==0);
    CHECK(Validate(s_errisoRivalBody,sizeof(s_errisoRivalBody),1,0)==0);
    CHECK(Validate(s_abeille_body,sizeof(s_abeille_body),0,1)==0);
    CHECK(Validate(s_abeille_rival,sizeof(s_abeille_rival),1,1)==0);
    CHECK(ValidateAbeillePanelColorSeam()==0);
    puts("authored Erriso/Abeille player and rival geometry, scale, normals and materials valid");
    return 0;
}
