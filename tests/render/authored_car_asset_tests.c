#include "render/rmesh.h"
#include <math.h>
#include <stdio.h>
#include "port/modern/authored_car_data.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static int ValidateInBounds(const void *bytes,size_t size,const float low[3],const float high[3]) {
    RageRuntimeMesh mesh;
    RageRuntimeVertex v;
    uint32_t i,material=0;
    CHECK(RuntimeMeshOpen(&mesh,bytes,size));
    CHECK(mesh.meshCount==1 && mesh.indexCount>228*3 && mesh.indexCount<20000*3);
    for(i=0;i<mesh.indexCount;i++) {
        uint32_t index,axis;
        CHECK(RuntimeMeshIndex(&mesh,i,&index) && RuntimeMeshVertex(&mesh,index,&v));
        for(axis=0;axis<3;axis++) CHECK(v.position[axis]>=low[axis] && v.position[axis]<=high[axis]);
        CHECK(v.color[3]==255);
        CHECK(isfinite(v.uv[0]) && isfinite(v.uv[1]));
        CHECK(v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2]>0.9f);
        if(i%3==0) material=v.material;
        else CHECK(v.material==material);
    }
    return 0;
}

static int Validate(const void *bytes,size_t size,int model) {
    /* Retail body bounds plus four model units for narrow panel bevels. */
    float low[3]={-126,-23,-81}, high[3]={126,143,360};
    if(model==1) {
        low[0]=-147; low[1]=-43; low[2]=-73;
        high[0]=147; high[1]=142; high[2]=436;
    } else if(model==2) {
        low[0]=-140; low[1]=-23; low[2]=-95;
        high[0]=140; high[1]=118; high[2]=413;
    } else if(model==3) {
        low[0]=-148; low[1]=-35; low[2]=-146;
        high[0]=148; high[1]=144; high[2]=501;
    } else if(model==4) {
        low[0]=-164; low[1]=-33; low[2]=-150;
        high[0]=164; high[1]=137; high[2]=529;
    } else if(model==5) {
        low[0]=-144; low[1]=-25; low[2]=-149;
        high[0]=144; high[1]=121; high[2]=485;
    } else if(model==6) {
        low[0]=-149; low[1]=-36; low[2]=-161;
        high[0]=149; high[1]=164; high[2]=529;
    } else if(model==7) {
        low[0]=-146; low[1]=-38; low[2]=-136;
        high[0]=146; high[1]=113; high[2]=469;
    } else if(model==8) {
        low[0]=-161; low[1]=-42; low[2]=-123;
        high[0]=161; high[1]=109; high[2]=516;
    } else if(model==9) {
        low[0]=-162; low[1]=-46; low[2]=-137;
        high[0]=162; high[1]=103; high[2]=525;
    } else if(model==10) {
        low[0]=-157; low[1]=-31; low[2]=-95;
        high[0]=157; high[1]=140; high[2]=485;
    } else if(model==11) {
        low[0]=-162; low[1]=-48; low[2]=-158;
        high[0]=162; high[1]=165; high[2]=426;
    } else if(model==12) {
        low[0]=-154; low[1]=-32; low[2]=-165;
        high[0]=154; high[1]=114; high[2]=552;
    } else if(model==13) {
        low[0]=-126; low[1]=-31; low[2]=-104;
        high[0]=126; high[1]=132; high[2]=478;
    }
    return ValidateInBounds(bytes,size,low,high);
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

static int ValidateRegistry(void) {
    size_t i,j,k;
    int standard=0, alternate=0;
    for(i=0;i<RAGE_AUTHORED_CAR_COUNT;i++) {
        const AuthoredCarReplacement *car=&s_authoredCars[i];
        RageRuntimeMesh mesh;
        CHECK(RuntimeMeshOpen(&mesh,car->bytes,car->byteCount));
        for(j=0;j<car->materialCount;j++) {
            const AuthoredCarMaterial *m=&car->materials[j];
            CHECK(m->source<64 && m->cacheSlot<64);
            for(k=0;k<j;k++) CHECK(car->materials[k].source!=m->source);
            if(car->assetKey==112 && m->source==13) {
                CHECK(m->cacheSlot==13 && m->page==12 && m->clut==0x78c7);
                standard=1;
            }
            if(car->assetKey==94 && m->source==13) {
                CHECK(m->cacheSlot==12 && m->page==12 && m->clut==0x78c7);
                alternate=1;
            }
        }
        for(j=0;j<mesh.vertexCount;j++) {
            RageRuntimeVertex v;
            uint32_t slot;
            CHECK(RuntimeMeshVertex(&mesh,(uint32_t)j,&v));
            slot=v.material & RAGE_RUNTIME_MATERIAL_INDEX_MASK;
            if(slot==65535) continue;
            for(k=0;k<car->materialCount;k++) if(car->materials[k].source==slot) break;
            CHECK(k<car->materialCount);
        }
        for(j=0;j<i;j++) CHECK(car->assetKey!=s_authoredCars[j].assetKey ||
            car->assetSet!=s_authoredCars[j].assetSet || car->submesh!=s_authoredCars[j].submesh);
    }
    CHECK(standard && alternate);
    return 0;
}

static int ValidatePegaseHoodDecal(void) {
    RageRuntimeMesh mesh;
    RageRuntimeVertex v;
    uint32_t i, count=0, corners=0;
    CHECK(RuntimeMeshOpen(&mesh,s_pegase_body,sizeof(s_pegase_body)));
    for(i=0;i<mesh.vertexCount;i++) {
        CHECK(RuntimeMeshVertex(&mesh,i,&v));
        if(v.material!=552271874u) continue;
        count++;
        CHECK(fabsf(fabsf(v.position[0])-44.0f)<0.001f);
        if(fabsf(v.position[2]-224.0f)<0.001f) {
            CHECK(fabsf(v.position[1]-56.0f)<0.001f);
            CHECK(fabsf(v.uv[0]-(v.position[0]>0?0.494140625f:0.251953125f))<0.00001f);
            CHECK(fabsf(v.uv[1]-0.189453125f)<0.00001f);
            corners|=v.position[0]>0?1u:2u;
        } else {
            CHECK(fabsf(v.position[2]-309.0f)<0.001f && fabsf(v.position[1]-51.0f)<0.001f);
            CHECK(fabsf(v.uv[0]-(v.position[0]>0?0.498046875f:0.251953125f))<0.00001f);
            CHECK(fabsf(v.uv[1]-0.435546875f)<0.00001f);
            corners|=v.position[0]>0?4u:8u;
        }
    }
    CHECK(count==6 && corners==15);
    return 0;
}

int main(void) {
    CHECK(ValidateInBounds(s_erriso_grade1,sizeof(s_erriso_grade1),
        (const float[3]){-126,-25,-111},(const float[3]){126,143,363})==0);
    CHECK(ValidateInBounds(s_erriso_grade2,sizeof(s_erriso_grade2),
        (const float[3]){-139,-23,-118},(const float[3]){139,143,372})==0);
    CHECK(ValidateInBounds(s_erriso_grade3,sizeof(s_erriso_grade3),
        (const float[3]){-139,-30,-97},(const float[3]){139,135,388})==0);
    CHECK(ValidateInBounds(s_abeille_grade1,sizeof(s_abeille_grade1),
        (const float[3]){-145,-21,-73},(const float[3]){145,156,435})==0);
    CHECK(ValidateInBounds(s_abeille_grade2,sizeof(s_abeille_grade2),
        (const float[3]){-145,-27,-73},(const float[3]){145,142,435})==0);
    CHECK(ValidateInBounds(s_pegase_grade1,sizeof(s_pegase_grade1),
        (const float[3]){-140,-23,-99},(const float[3]){140,118,413})==0);
    CHECK(ValidateInBounds(s_esperanza_grade1,sizeof(s_esperanza_grade1),
        (const float[3]){-148,-35,-146},(const float[3]){148,144,501})==0);
    CHECK(ValidateInBounds(s_esperanza_grade2,sizeof(s_esperanza_grade2),
        (const float[3]){-148,-35,-146},(const float[3]){148,144,501})==0);
    CHECK(ValidateInBounds(s_esperanza_grade3,sizeof(s_esperanza_grade3),
        (const float[3]){-167,-36,-139},(const float[3]){167,144,512})==0);
    CHECK(ValidateInBounds(s_esperanza_grade4,sizeof(s_esperanza_grade4),
        (const float[3]){-166,-41,-152},(const float[3]){166,144,512})==0);
    CHECK(ValidateInBounds(s_acceron_grade1,sizeof(s_acceron_grade1),
        (const float[3]){-164,-33,-165},(const float[3]){164,137,529})==0);
    CHECK(ValidateInBounds(s_acceron_grade2,sizeof(s_acceron_grade2),
        (const float[3]){-164,-33,-153},(const float[3]){164,137,526})==0);
    CHECK(ValidateInBounds(s_acceron_grade3,sizeof(s_acceron_grade3),
        (const float[3]){-168,-52,-172},(const float[3]){168,135,529})==0);
    CHECK(ValidateInBounds(s_bayonet_grade1,sizeof(s_bayonet_grade1),
        (const float[3]){-158,-33,-149},(const float[3]){158,121,485})==0);
    CHECK(ValidateInBounds(s_bayonet_grade2,sizeof(s_bayonet_grade2),
        (const float[3]){-151,-43,-148},(const float[3]){151,139,485})==0);
    CHECK(ValidateInBounds(s_hijack_grade1,sizeof(s_hijack_grade1),
        (const float[3]){-149,-40,-168},(const float[3]){149,161,528})==0);
    CHECK(ValidateInBounds(s_fatalita_grade1,sizeof(s_fatalita_grade1),
        (const float[3]){-146,-38,-137},(const float[3]){146,113,469})==0);
    CHECK(ValidateInBounds(s_fatalita_grade2,sizeof(s_fatalita_grade2),
        (const float[3]){-156,-41,-137},(const float[3]){156,119,486})==0);
    CHECK(ValidateInBounds(s_istante_grade1,sizeof(s_istante_grade1),
        (const float[3]){-174,-42,-123},(const float[3]){174,111,516})==0);
    CHECK(Validate(s_errisoBody,sizeof(s_errisoBody),0)==0);
    CHECK(Validate(s_errisoRivalBody,sizeof(s_errisoRivalBody),0)==0);
    CHECK(Validate(s_abeille_body,sizeof(s_abeille_body),1)==0);
    CHECK(Validate(s_abeille_rival,sizeof(s_abeille_rival),1)==0);
    CHECK(Validate(s_pegase_body,sizeof(s_pegase_body),2)==0);
    CHECK(Validate(s_pegase_rival,sizeof(s_pegase_rival),2)==0);
    CHECK(Validate(s_esperanza_body,sizeof(s_esperanza_body),3)==0);
    CHECK(Validate(s_esperanza_rival_duplicate,sizeof(s_esperanza_rival_duplicate),3)==0);
    CHECK(Validate(s_esperanza_rival_slot1,sizeof(s_esperanza_rival_slot1),3)==0);
    CHECK(Validate(s_esperanza_rival_slot2,sizeof(s_esperanza_rival_slot2),3)==0);
    CHECK(Validate(s_esperanza_rival,sizeof(s_esperanza_rival),3)==0);
    CHECK(Validate(s_esperanza_rival_late,sizeof(s_esperanza_rival_late),3)==0);
    CHECK(Validate(s_acceron_body,sizeof(s_acceron_body),4)==0);
    CHECK(Validate(s_acceron_rival,sizeof(s_acceron_rival),4)==0);
    CHECK(Validate(s_bayonet_body,sizeof(s_bayonet_body),5)==0);
    CHECK(Validate(s_bayonet_rival,sizeof(s_bayonet_rival),5)==0);
    CHECK(Validate(s_hijack_body,sizeof(s_hijack_body),6)==0);
    CHECK(Validate(s_hijack_rival,sizeof(s_hijack_rival),6)==0);
    CHECK(Validate(s_hijack_rival_alternate,sizeof(s_hijack_rival_alternate),6)==0);
    CHECK(Validate(s_fatalita_body,sizeof(s_fatalita_body),7)==0);
    CHECK(Validate(s_fatalita_rival,sizeof(s_fatalita_rival),7)==0);
    CHECK(Validate(s_istante_body,sizeof(s_istante_body),8)==0);
    CHECK(Validate(s_istante_rival,sizeof(s_istante_rival),8)==0);
    CHECK(Validate(s_ghepardo_body,sizeof(s_ghepardo_body),9)==0);
    CHECK(Validate(s_ghepardo_rival,sizeof(s_ghepardo_rival),9)==0);
    CHECK(Validate(s_vainqure_body,sizeof(s_vainqure_body),10)==0);
    CHECK(Validate(s_vainqure_rival,sizeof(s_vainqure_rival),10)==0);
    CHECK(Validate(s_bulshade_body,sizeof(s_bulshade_body),11)==0);
    CHECK(Validate(s_bulshade_rival,sizeof(s_bulshade_rival),11)==0);
    CHECK(Validate(s_bulshade_rival_alternate,sizeof(s_bulshade_rival_alternate),11)==0);
    CHECK(Validate(s_squaldon_body,sizeof(s_squaldon_body),12)==0);
    CHECK(Validate(s_squaldon_rival,sizeof(s_squaldon_rival),12)==0);
    CHECK(Validate(s_compacta_rival_early,sizeof(s_compacta_rival_early),13)==0);
    CHECK(Validate(s_compactb_rival_early,sizeof(s_compactb_rival_early),13)==0);
    CHECK(Validate(s_compactc_rival_early,sizeof(s_compactc_rival_early),13)==0);
    CHECK(Validate(s_compacta_rival_middle,sizeof(s_compacta_rival_middle),3)==0);
    CHECK(Validate(s_compacta_rival_late,sizeof(s_compacta_rival_late),3)==0);
    CHECK(Validate(s_compactb_rival_middle,sizeof(s_compactb_rival_middle),3)==0);
    CHECK(Validate(s_compactb_rival_late,sizeof(s_compactb_rival_late),3)==0);
    CHECK(Validate(s_compactc_rival_middle,sizeof(s_compactc_rival_middle),3)==0);
    CHECK(Validate(s_compactc_rival_late,sizeof(s_compactc_rival_late),3)==0);
    CHECK(ValidateAbeillePanelColorSeam()==0);
    CHECK(ValidateRegistry()==0);
    CHECK(ValidatePegaseHoodDecal()==0);
    puts("authored car geometry, scale, normals, seams and material registries valid");
    return 0;
}
