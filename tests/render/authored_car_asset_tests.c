#include "render/rmesh.h"
#include "render/authored_car_surface.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

static void TestCross(const float a[3], const float b[3], float out[3]) {
    out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];
}
static float TestDot(const float a[3], const float b[3]) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
static int SegmentCrossesTriangle(const float a[3], const float b[3],
                                 const RageRuntimeVertex tri[3]) {
    float d[3],e1[3],e2[3],p[3],q[3],s[3],det,u,v,t;
    unsigned k;
    for(k=0;k<3;k++) {
        d[k]=b[k]-a[k];e1[k]=tri[1].position[k]-tri[0].position[k];
        e2[k]=tri[2].position[k]-tri[0].position[k];s[k]=a[k]-tri[0].position[k];
    }
    TestCross(d,e2,p);det=TestDot(e1,p);
    if(fabsf(det)<.00001f) return 0;
    u=TestDot(s,p)/det;TestCross(s,e1,q);v=TestDot(d,q)/det;t=TestDot(e2,q)/det;
    /* Shared edges and T junctions at the lip are intentional. Use model-space
     * distances as well as barycentric coordinates for very thin clipped faces. */
    if (!(u>.0001f && v>.0001f && u+v<.9999f && t>.0001f && t<.9999f)) return 0;
    {
        float n[3], nlen, da, db, hit[3];
        TestCross(e1,e2,n); nlen=sqrtf(TestDot(n,n));
        if(nlen<.000001f) return 0;
        da=TestDot(s,n)/nlen; db=da+TestDot(d,n)/nlen;
        if(da*db>=0 || fabsf(da)<=.001f || fabsf(db)<=.001f) return 0;
        for(k=0;k<3;k++) hit[k]=a[k]+t*d[k];
        for(k=0;k<3;k++) {
            float edge[3], rel[3], cross[3]; unsigned axis;
            for(axis=0;axis<3;axis++) {
                edge[axis]=tri[(k+1)%3].position[axis]-tri[k].position[axis];
                rel[axis]=hit[axis]-tri[k].position[axis];
            }
            TestCross(edge,rel,cross);
            if(TestDot(cross,cross)<=.000001f*TestDot(edge,edge)) return 0;
        }
    }
    return 1;
}
static int ValidateFenderIntersections(const void *bytes, size_t size) {
    RageRuntimeMesh mesh;
    uint32_t i,j,k,index;
    RageRuntimeVertex *vertices;
    const uint32_t liner=RAGE_RUNTIME_MATERIAL_METADATA | RAGE_RUNTIME_MATERIAL_INDEX_MASK;
    CHECK(RuntimeMeshOpen(&mesh,bytes,size));
    vertices=malloc(mesh.indexCount*sizeof(*vertices)); CHECK(vertices);
    for(i=0;i<mesh.indexCount;i++) {
        CHECK(RuntimeMeshIndex(&mesh,i,&index));
        CHECK(RuntimeMeshVertex(&mesh,index,&vertices[i]));
    }
    for(i=0;i<mesh.indexCount;i+=3) {
        const RageRuntimeVertex *panel=&vertices[i];
        if(panel[0].material==liner) continue;
        for(j=0;j<mesh.indexCount;j+=3) {
            const RageRuntimeVertex *wall=&vertices[j];
            int separate=0;
            if(wall[0].material!=liner) continue;
            for(k=0;k<3;k++) {
                float pmin=fminf(panel[0].position[k],fminf(panel[1].position[k],panel[2].position[k]));
                float pmax=fmaxf(panel[0].position[k],fmaxf(panel[1].position[k],panel[2].position[k]));
                float wmin=fminf(wall[0].position[k],fminf(wall[1].position[k],wall[2].position[k]));
                float wmax=fmaxf(wall[0].position[k],fmaxf(wall[1].position[k],wall[2].position[k]));
                if(pmax<wmin || wmax<pmin) separate=1;
            }
            if(separate) continue;
            for(k=0;k<3;k++) {
                CHECK(!SegmentCrossesTriangle(panel[k].position,panel[(k+1)%3].position,wall));
                CHECK(!SegmentCrossesTriangle(wall[k].position,wall[(k+1)%3].position,panel));
            }
        }
    }
    free(vertices);
    return 0;
}

static int ValidateEsperanzaWellLiners(void) {
    RageRuntimeMesh mesh;
    unsigned hood = 0, liners = 0;
    int side, axle, sample;
    uint32_t i;
    static const float samples[][2] = {{0,0},{30,0},{45,0},{20,30},{20,-30}};
    CHECK(RuntimeMeshOpen(&mesh, s_esperanza_rounded, sizeof(s_esperanza_rounded)));
    for (i = 0; i < mesh.vertexCount; i++) {
        RageRuntimeVertex v;
        CHECK(RuntimeMeshVertex(&mesh,i,&v));
        if (v.material == (RAGE_RUNTIME_MATERIAL_METADATA | RAGE_RUNTIME_MATERIAL_INDEX_MASK)) {
            CHECK(v.color[0] <= 7 && v.color[1] <= 7 && v.color[2] <= 7);
            liners++;
        }
        if (fabsf(v.position[0]-50)<.001f && fabsf(v.position[1]-59)<.001f &&
            fabsf(v.position[2]-399)<.001f) {
            CHECK(fabsf(v.normal[0]-.03248f)<.002f);
            CHECK(fabsf(v.normal[1]-.98228f)<.002f);
            CHECK(fabsf(v.normal[2]-.18459f)<.002f);
            hood++;
        }
    }
    CHECK(liners > 300 && hood > 0);
    /* Rays from each wheel opening must hit the outward-facing inner closure,
     * rather than passing through the car to the opposite body panel. */
    for (side = -1; side <= 1; side += 2) for (axle = 0; axle < 2; axle++)
        for (sample = 0; sample < 5; sample++) {
            int hit = 0;
            for (i = 0; i < mesh.indexCount; i += 3) {
                RageRuntimeVertex v[3];
                uint32_t j,index;
                float ay,az,by,bz,py,pz,det,u,w;
                for (j = 0; j < 3; j++) {
                    CHECK(RuntimeMeshIndex(&mesh,i+j,&index));
                    CHECK(RuntimeMeshVertex(&mesh,index,&v[j]));
                }
                if (v[0].material != (RAGE_RUNTIME_MATERIAL_METADATA | RAGE_RUNTIME_MATERIAL_INDEX_MASK)) continue;
                if (fabsf(v[0].position[0]-side*74)>.001f ||
                    fabsf(v[1].position[0]-side*74)>.001f ||
                    fabsf(v[2].position[0]-side*74)>.001f) continue;
                ay=v[1].position[1]-v[0].position[1];
                az=v[1].position[2]-v[0].position[2];
                by=v[2].position[1]-v[0].position[1];
                bz=v[2].position[2]-v[0].position[2];
                det=ay*bz-az*by;
                CHECK(det*side > 0 && v[0].normal[0]*side>.99f);
                py=samples[sample][0]-v[0].position[1];
                pz=samples[sample][1]+axle*368-v[0].position[2];
                u=(py*bz-pz*by)/det;w=(ay*pz-az*py)/det;
                if(u>=-.0001f && w>=-.0001f && u+w<=1.0001f) hit=1;
            }
            CHECK(hit);
        }
    return 0;
}

static int ValidateRoundedEsperanza(void) {
    size_t entry;
    unsigned wheels = 0, glass = 0, paint = 0, arc = 0;
    CHECK(Validate(s_esperanza_rounded, sizeof(s_esperanza_rounded), 3) == 0);
    for (entry = 0; entry < RAGE_AUTHORED_CAR_COUNT; entry++) {
        const AuthoredCarReplacement *car = &s_authoredCars[entry];
        RageRuntimeMesh mesh;
        uint32_t i;
        if (car->assetKey != 28 || car->assetSet != RAGE_RENDER_ASSET_MODEL_BANK) continue;
        CHECK(RuntimeMeshOpen(&mesh, car->bytes, car->byteCount));
        if (car->submesh) wheels++;
        for (i = 0; i < mesh.vertexCount; i++) {
            RageRuntimeVertex v;
            unsigned slot, surface;
            CHECK(RuntimeMeshVertex(&mesh, i, &v));
            slot = v.material & RAGE_RUNTIME_MATERIAL_INDEX_MASK;
            surface = slot / RAGE_CAR_SURFACE_SOURCE_STRIDE;
            if (!car->submesh) {
                float z = fabsf(v.position[2]) < 55 ? v.position[2] : v.position[2] - 368;
                if (surface == RAGE_CAR_SURFACE_GLASS) glass++;
                if (surface == RAGE_CAR_SURFACE_PAINT) paint++;
                if (fabsf(v.position[0]) > 125 && v.position[1] > 8 &&
                    fabsf(hypotf(v.position[1], z) - 51) < 0.01f) arc++;
            } else {
                float radius = hypotf(v.position[1], v.position[2]);
                CHECK(radius <= 43.01f);
                CHECK(fabsf(v.position[0]) <= ((car->submesh & 1) ? 143.01f : 20.01f));
                CHECK(surface == RAGE_CAR_SURFACE_RUBBER || surface == RAGE_CAR_SURFACE_METAL);
                CHECK(v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2] > 0.99f);
                if (radius > 42.9f) CHECK(v.normal[1]*v.position[1]+v.normal[2]*v.position[2] > 0);
            }
        }
    }
    CHECK(wheels == 20 && glass > 20 && paint > 100 && arc > 100);
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
            CHECK(slot < RAGE_CAR_SURFACE_COUNT * RAGE_CAR_SURFACE_SOURCE_STRIDE);
            slot %= RAGE_CAR_SURFACE_SOURCE_STRIDE;
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
    CHECK(RuntimeMeshOpen(&mesh,s_rounded_pegase_body,sizeof(s_rounded_pegase_body)));
    for(i=0;i<mesh.vertexCount;i++) {
        CHECK(RuntimeMeshVertex(&mesh,i,&v));
        if(v.material!=552271874u + RAGE_CAR_SURFACE_DECAL * RAGE_CAR_SURFACE_SOURCE_STRIDE) continue;
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

static int ValidateIntersectionPredicate(void) {
    RageRuntimeVertex tri[3]={0};
    tri[1].position[0]=10;tri[2].position[1]=10;
    CHECK(SegmentCrossesTriangle((float[3]){2,2,-1},(float[3]){2,2,1},tri));
    CHECK(!SegmentCrossesTriangle((float[3]){0,0,-1},(float[3]){0,0,1},tri));
    CHECK(!SegmentCrossesTriangle((float[3]){2,2,1},(float[3]){2,2,2},tri));
    return 0;
}
static int ValidateFleetWheel(const void *bytes,size_t size,float radius,float halfWidth) {
    RageRuntimeMesh mesh; uint32_t i; unsigned rubber=0,metal=0;
    unsigned long long angles=0;
    CHECK(RuntimeMeshOpen(&mesh,bytes,size));
    CHECK(mesh.indexCount>=64*3);
    for(i=0;i<mesh.vertexCount;i++) {
        RageRuntimeVertex v; float r; unsigned surface;
        CHECK(RuntimeMeshVertex(&mesh,i,&v)); r=hypotf(v.position[1],v.position[2]);
        surface=(v.material & RAGE_RUNTIME_MATERIAL_INDEX_MASK)/RAGE_CAR_SURFACE_SOURCE_STRIDE;
        CHECK(surface==RAGE_CAR_SURFACE_RUBBER || surface==RAGE_CAR_SURFACE_METAL);
        CHECK(r<=radius+.01f && fabsf(v.position[0])<=halfWidth+.01f);
        CHECK(TestDot(v.normal,v.normal)>.99f);
        if(surface==RAGE_CAR_SURFACE_RUBBER) {
            rubber++;
            if(r>radius-.01f) {
                int sector=(int)lroundf(atan2f(v.position[1],v.position[2])*64/(2*3.14159265358979323846f));
                angles|=1ull<<(sector&63);
                CHECK(v.normal[1]*v.position[1]+v.normal[2]*v.position[2]>0);
            }
        } else metal++;
    }
    CHECK(rubber && metal && angles==~0ull);
    return 0;
}
static float PointTriangleDistance(const float point[3], const RageRuntimeVertex tri[3]) {
    float a[3],b[3],p[3],n[3],cross[3],best=1e30f;
    unsigned i,j; int inside=1;
    for(j=0;j<3;j++) {
        a[j]=tri[1].position[j]-tri[0].position[j];
        b[j]=tri[2].position[j]-tri[0].position[j];
        p[j]=point[j]-tri[0].position[j];
    }
    TestCross(a,b,n);
    for(i=0;i<3;i++) {
        float length,t,d[3];
        for(j=0;j<3;j++) {
            a[j]=tri[(i+1)%3].position[j]-tri[i].position[j];
            b[j]=point[j]-tri[i].position[j];
        }
        TestCross(a,b,cross);
        if(TestDot(cross,n)<0) inside=0;
        length=TestDot(a,a);
        t=length>0?fmaxf(0,fminf(1,TestDot(a,b)/length)):0;
        for(j=0;j<3;j++) d[j]=b[j]-t*a[j];
        best=fminf(best,sqrtf(TestDot(d,d)));
    }
    if(inside && TestDot(n,n)>1e-12f)
        best=fminf(best,fabsf(TestDot(p,n))/sqrtf(TestDot(n,n)));
    return best;
}
static int ValidateFlareJoin(const void *bytes,size_t size,const float (*samples)[3],unsigned count) {
    RageRuntimeMesh mesh; uint32_t i,k,index; unsigned sample;
    CHECK(RuntimeMeshOpen(&mesh,bytes,size));
    for(sample=0;sample<count;sample++) {
        float nearest=1e30f;
        for(i=0;i<mesh.indexCount;i+=3) {
            RageRuntimeVertex tri[3];
            for(k=0;k<3;k++) {
                CHECK(RuntimeMeshIndex(&mesh,i+k,&index));
                CHECK(RuntimeMeshVertex(&mesh,index,&tri[k]));
            }
            if((tri[0].material&RAGE_RUNTIME_MATERIAL_INDEX_MASK)==65535) continue;
            nearest=fminf(nearest,PointTriangleDistance(samples[sample],tri));
        }
        /* A well can have no intersections yet still protrude through a hole
         * in the paint. These wide fenders previously had gaps of 20 units. */
        CHECK(nearest<2.0f);
    }
    return 0;
}
static int ValidateWellCoverage(const void *bytes,size_t size,
                               const float (*samples)[2],unsigned count,
                               float minPaint,float innerLimit) {
    RageRuntimeMesh mesh; int side; uint32_t i,j,index,sample;
    CHECK(RuntimeMeshOpen(&mesh,bytes,size));
    /* Check coverage along both shoulders and the inboard edge, not just
     * the arch crown. An oversized cut left holes despite zero crossings. */
    for(side=-1;side<=1;side+=2)
    for(sample=0;sample<count;sample++) {
        float topPaint=-1e30f,topLiner=-1e30f;
        for(i=0;i<mesh.indexCount;i+=3) {
            RageRuntimeVertex v[3];float ax,az,bx,bz,px,pz,det,u,w,height;
            for(j=0;j<3;j++) {
                CHECK(RuntimeMeshIndex(&mesh,i+j,&index));
                CHECK(RuntimeMeshVertex(&mesh,index,&v[j]));
            }
            ax=v[1].position[0]-v[0].position[0];az=v[1].position[2]-v[0].position[2];
            bx=v[2].position[0]-v[0].position[0];bz=v[2].position[2]-v[0].position[2];
            det=ax*bz-az*bx;if(fabsf(det)<.00001f) continue;
            px=side*samples[sample][0]-v[0].position[0];
            pz=samples[sample][1]-v[0].position[2];
            u=(px*bz-pz*bx)/det;w=(ax*pz-az*px)/det;
            if(u<-.0001f || w<-.0001f || u+w>1.0001f) continue;
            height=v[0].position[1]+u*(v[1].position[1]-v[0].position[1])+w*(v[2].position[1]-v[0].position[1]);
            if((v[0].material&RAGE_RUNTIME_MATERIAL_INDEX_MASK)==65535) topLiner=fmaxf(topLiner,height);
            else topPaint=fmaxf(topPaint,height);
        }
        CHECK(topPaint>minPaint && topPaint>topLiner);
        if(samples[sample][0]<innerLimit) CHECK(topLiner<0);
        else CHECK(topLiner>0);
    }
    return 0;
}
static int ValidateWellBelowHood(const void *bytes,size_t size) {
    static const float x[]={90,100,110,120,130,140,145};
    static const float z[]={344,354,364,374,384};
    float samples[35][2];unsigned i,j,count=0;
    for(i=0;i<7;i++) for(j=0;j<5;j++) {
        samples[count][0]=x[i];samples[count++][1]=z[j];
    }
    return ValidateWellCoverage(bytes,size,samples,count,30,97);
}
static int ValidateBulshadeWellCoverage(const void *bytes,size_t size) {
    static const float samples[][2]={{115,355},{115,360},{120,350},
        {140,-30},{140,0},{140,30},{150,-30},{150,0},{150,30}};
    return ValidateWellCoverage(bytes,size,samples,9,10,0);
}
static int ValidateRoundedFleet(void) {
    CHECK(ValidateWellBelowHood(s_rounded_vainqure_body,sizeof(s_rounded_vainqure_body))==0);
    CHECK(ValidateWellBelowHood(s_rounded_vainqure_rival,sizeof(s_rounded_vainqure_rival))==0);
    CHECK(ValidateBulshadeWellCoverage(s_rounded_bulshade_body,sizeof(s_rounded_bulshade_body))==0);
    CHECK(ValidateBulshadeWellCoverage(s_rounded_bulshade_rival,sizeof(s_rounded_bulshade_rival))==0);
    CHECK(ValidateBulshadeWellCoverage(s_rounded_bulshade_rival_alternate,sizeof(s_rounded_bulshade_rival_alternate))==0);
    CHECK(ValidateFlareJoin(s_rounded_esperanza_grade3,sizeof(s_rounded_esperanza_grade3),
        (const float[][3]){{-161.90909f,54.97401f,-1.69056f},{162.09091f,54.89298f,3.42944f},
                           {-156,54.86018f,371.91932f},{156,54.86018f,371.91932f}},4)==0);
    CHECK(ValidateFlareJoin(s_rounded_bayonet_grade1,sizeof(s_rounded_bayonet_grade1),
        (const float[][3]){{151,48.91932f,350.41873f},{-151,48.91932f,350.41873f}},2)==0);
    CHECK(ValidateFenderIntersections(s_rounded_erriso_grade1,sizeof(s_rounded_erriso_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_erriso_grade2,sizeof(s_rounded_erriso_grade2))==0);
    CHECK(ValidateFenderIntersections(s_rounded_erriso_grade3,sizeof(s_rounded_erriso_grade3))==0);
    CHECK(ValidateFenderIntersections(s_rounded_abeille_grade1,sizeof(s_rounded_abeille_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_abeille_grade2,sizeof(s_rounded_abeille_grade2))==0);
    CHECK(ValidateFenderIntersections(s_rounded_pegase_grade1,sizeof(s_rounded_pegase_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_grade1,sizeof(s_rounded_esperanza_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_grade2,sizeof(s_rounded_esperanza_grade2))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_grade3,sizeof(s_rounded_esperanza_grade3))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_grade4,sizeof(s_rounded_esperanza_grade4))==0);
    CHECK(ValidateFenderIntersections(s_rounded_acceron_grade1,sizeof(s_rounded_acceron_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_acceron_grade2,sizeof(s_rounded_acceron_grade2))==0);
    CHECK(ValidateFenderIntersections(s_rounded_acceron_grade3,sizeof(s_rounded_acceron_grade3))==0);
    CHECK(ValidateFenderIntersections(s_rounded_bayonet_grade1,sizeof(s_rounded_bayonet_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_bayonet_grade2,sizeof(s_rounded_bayonet_grade2))==0);
    CHECK(ValidateFenderIntersections(s_rounded_hijack_grade1,sizeof(s_rounded_hijack_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_fatalita_grade1,sizeof(s_rounded_fatalita_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_fatalita_grade2,sizeof(s_rounded_fatalita_grade2))==0);
    CHECK(ValidateFenderIntersections(s_rounded_istante_grade1,sizeof(s_rounded_istante_grade1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compacta_rival_middle,sizeof(s_rounded_compacta_rival_middle))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compacta_rival_late,sizeof(s_rounded_compacta_rival_late))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compactb_rival_middle,sizeof(s_rounded_compactb_rival_middle))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compactb_rival_late,sizeof(s_rounded_compactb_rival_late))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compactc_rival_middle,sizeof(s_rounded_compactc_rival_middle))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compactc_rival_late,sizeof(s_rounded_compactc_rival_late))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compacta_rival_early,sizeof(s_rounded_compacta_rival_early))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compactb_rival_early,sizeof(s_rounded_compactb_rival_early))==0);
    CHECK(ValidateFenderIntersections(s_rounded_compactc_rival_early,sizeof(s_rounded_compactc_rival_early))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_rival_slot1,sizeof(s_rounded_esperanza_rival_slot1))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_rival_slot2,sizeof(s_rounded_esperanza_rival_slot2))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_rival_duplicate,sizeof(s_rounded_esperanza_rival_duplicate))==0);
    CHECK(ValidateFenderIntersections(s_rounded_squaldon_body,sizeof(s_rounded_squaldon_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_squaldon_rival,sizeof(s_rounded_squaldon_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_bulshade_body,sizeof(s_rounded_bulshade_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_bulshade_rival,sizeof(s_rounded_bulshade_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_bulshade_rival_alternate,sizeof(s_rounded_bulshade_rival_alternate))==0);
    CHECK(ValidateFenderIntersections(s_rounded_vainqure_body,sizeof(s_rounded_vainqure_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_vainqure_rival,sizeof(s_rounded_vainqure_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_ghepardo_body,sizeof(s_rounded_ghepardo_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_ghepardo_rival,sizeof(s_rounded_ghepardo_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_istante_body,sizeof(s_rounded_istante_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_istante_rival,sizeof(s_rounded_istante_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_fatalita_body,sizeof(s_rounded_fatalita_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_fatalita_rival,sizeof(s_rounded_fatalita_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_hijack_body,sizeof(s_rounded_hijack_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_hijack_rival_alternate,sizeof(s_rounded_hijack_rival_alternate))==0);
    CHECK(ValidateFenderIntersections(s_rounded_hijack_rival,sizeof(s_rounded_hijack_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_bayonet_body,sizeof(s_rounded_bayonet_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_bayonet_rival,sizeof(s_rounded_bayonet_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_acceron_body,sizeof(s_rounded_acceron_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_acceron_rival,sizeof(s_rounded_acceron_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_erriso_body,sizeof(s_rounded_erriso_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_erriso_rival,sizeof(s_rounded_erriso_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_abeille_body,sizeof(s_rounded_abeille_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_abeille_rival,sizeof(s_rounded_abeille_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_pegase_body,sizeof(s_rounded_pegase_body))==0);
    CHECK(ValidateFenderIntersections(s_rounded_pegase_rival,sizeof(s_rounded_pegase_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_rival,sizeof(s_rounded_esperanza_rival))==0);
    CHECK(ValidateFenderIntersections(s_rounded_esperanza_rival_late,sizeof(s_rounded_esperanza_rival_late))==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a12d9f9140027976,sizeof(s_rounded_wheel_a12d9f9140027976),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d490bc12441bdce2,sizeof(s_rounded_wheel_d490bc12441bdce2),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ea1f969498acc831,sizeof(s_rounded_wheel_ea1f969498acc831),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_44f40ca2053bbde4,sizeof(s_rounded_wheel_44f40ca2053bbde4),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_345be5565187b08c,sizeof(s_rounded_wheel_345be5565187b08c),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e5f7a32294424a39,sizeof(s_rounded_wheel_e5f7a32294424a39),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3cac98e4cb8557db,sizeof(s_rounded_wheel_3cac98e4cb8557db),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_37d735e8583b890b,sizeof(s_rounded_wheel_37d735e8583b890b),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_50c58b005e33ca47,sizeof(s_rounded_wheel_50c58b005e33ca47),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5874364080f54dc7,sizeof(s_rounded_wheel_5874364080f54dc7),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_672b0500ccf189e8,sizeof(s_rounded_wheel_672b0500ccf189e8),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2d9d8d0c3c97c1b1,sizeof(s_rounded_wheel_2d9d8d0c3c97c1b1),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8d8c80fe3e3200b6,sizeof(s_rounded_wheel_8d8c80fe3e3200b6),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7305ae6a56cb3945,sizeof(s_rounded_wheel_7305ae6a56cb3945),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4cd439740a51829e,sizeof(s_rounded_wheel_4cd439740a51829e),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_06fde8033a42c238,sizeof(s_rounded_wheel_06fde8033a42c238),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0012b1d415ab309b,sizeof(s_rounded_wheel_0012b1d415ab309b),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_47582d1c187e3fd9,sizeof(s_rounded_wheel_47582d1c187e3fd9),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c700dfee60bb9770,sizeof(s_rounded_wheel_c700dfee60bb9770),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9b263fb874c185f8,sizeof(s_rounded_wheel_9b263fb874c185f8),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8440add740aec291,sizeof(s_rounded_wheel_8440add740aec291),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f10c8418d69fed6f,sizeof(s_rounded_wheel_f10c8418d69fed6f),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3c5ccb18cf183bba,sizeof(s_rounded_wheel_3c5ccb18cf183bba),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f015de10d1d54d49,sizeof(s_rounded_wheel_f015de10d1d54d49),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6ffb7b69d0dd2a0f,sizeof(s_rounded_wheel_6ffb7b69d0dd2a0f),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_39378863271e4c3d,sizeof(s_rounded_wheel_39378863271e4c3d),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9f45447e61d0d175,sizeof(s_rounded_wheel_9f45447e61d0d175),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0cf43bba8e1049b6,sizeof(s_rounded_wheel_0cf43bba8e1049b6),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_24dc3d3f86555bbc,sizeof(s_rounded_wheel_24dc3d3f86555bbc),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c0ba06402c0fe5a0,sizeof(s_rounded_wheel_c0ba06402c0fe5a0),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7afa48f85c2e0181,sizeof(s_rounded_wheel_7afa48f85c2e0181),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_92265aa9a4b21fef,sizeof(s_rounded_wheel_92265aa9a4b21fef),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cc2e51e040c465e8,sizeof(s_rounded_wheel_cc2e51e040c465e8),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5538aff62156e6cf,sizeof(s_rounded_wheel_5538aff62156e6cf),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0a498a71ce773d40,sizeof(s_rounded_wheel_0a498a71ce773d40),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2b8fece1e4ae1d05,sizeof(s_rounded_wheel_2b8fece1e4ae1d05),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_23ce5f9c371c229b,sizeof(s_rounded_wheel_23ce5f9c371c229b),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0f244db62ce41f1a,sizeof(s_rounded_wheel_0f244db62ce41f1a),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4aff1af67f37e9fe,sizeof(s_rounded_wheel_4aff1af67f37e9fe),34.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9824a2260e5044f1,sizeof(s_rounded_wheel_9824a2260e5044f1),34.000000f,135.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_35db04964a702961,sizeof(s_rounded_wheel_35db04964a702961),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a24c50446cf9567c,sizeof(s_rounded_wheel_a24c50446cf9567c),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_957f7c7f3b857aec,sizeof(s_rounded_wheel_957f7c7f3b857aec),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_fa680b26a95d7c09,sizeof(s_rounded_wheel_fa680b26a95d7c09),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bef37859d3d0195b,sizeof(s_rounded_wheel_bef37859d3d0195b),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5c94f97b39a5bc12,sizeof(s_rounded_wheel_5c94f97b39a5bc12),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_fdb43bbd47a4966d,sizeof(s_rounded_wheel_fdb43bbd47a4966d),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2425e6992602a825,sizeof(s_rounded_wheel_2425e6992602a825),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2d198c621a9d7996,sizeof(s_rounded_wheel_2d198c621a9d7996),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7ce755b7327252b4,sizeof(s_rounded_wheel_7ce755b7327252b4),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6b18f130b3b45745,sizeof(s_rounded_wheel_6b18f130b3b45745),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_da515675fb236a43,sizeof(s_rounded_wheel_da515675fb236a43),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d89083287b1bf6d5,sizeof(s_rounded_wheel_d89083287b1bf6d5),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cb3fcc589045c2c3,sizeof(s_rounded_wheel_cb3fcc589045c2c3),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d1323589fb14ffe0,sizeof(s_rounded_wheel_d1323589fb14ffe0),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3b30c0106c939c7e,sizeof(s_rounded_wheel_3b30c0106c939c7e),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a428458341630756,sizeof(s_rounded_wheel_a428458341630756),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0256efd661fdfbf1,sizeof(s_rounded_wheel_0256efd661fdfbf1),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_86b6c7d53c94302f,sizeof(s_rounded_wheel_86b6c7d53c94302f),38.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_90a26069337dd6c5,sizeof(s_rounded_wheel_90a26069337dd6c5),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_06aeb8b6edab5ce3,sizeof(s_rounded_wheel_06aeb8b6edab5ce3),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b6edcadc209a370c,sizeof(s_rounded_wheel_b6edcadc209a370c),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_22b397a2ed610430,sizeof(s_rounded_wheel_22b397a2ed610430),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1b73880a5cc2e3d9,sizeof(s_rounded_wheel_1b73880a5cc2e3d9),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0e90d7a58bd0506c,sizeof(s_rounded_wheel_0e90d7a58bd0506c),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ce9182e78093ab9f,sizeof(s_rounded_wheel_ce9182e78093ab9f),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a4ede34987a11e2c,sizeof(s_rounded_wheel_a4ede34987a11e2c),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7db7698a9f62f32c,sizeof(s_rounded_wheel_7db7698a9f62f32c),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a3b6de1196035103,sizeof(s_rounded_wheel_a3b6de1196035103),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5a7acca6bd789678,sizeof(s_rounded_wheel_5a7acca6bd789678),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_069ac4da0de37709,sizeof(s_rounded_wheel_069ac4da0de37709),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_25047364c42bb32a,sizeof(s_rounded_wheel_25047364c42bb32a),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c7ef7dbc4e5c6be1,sizeof(s_rounded_wheel_c7ef7dbc4e5c6be1),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b97ba9374149c3e7,sizeof(s_rounded_wheel_b97ba9374149c3e7),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8c6660a4fae8760a,sizeof(s_rounded_wheel_8c6660a4fae8760a),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ec955e624db496cf,sizeof(s_rounded_wheel_ec955e624db496cf),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c517af914be4bd46,sizeof(s_rounded_wheel_c517af914be4bd46),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_fabea66a8659cf5d,sizeof(s_rounded_wheel_fabea66a8659cf5d),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2ea1e97316226066,sizeof(s_rounded_wheel_2ea1e97316226066),38.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_82fd5e6d16464578,sizeof(s_rounded_wheel_82fd5e6d16464578),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f71c6c6de9007d5e,sizeof(s_rounded_wheel_f71c6c6de9007d5e),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cad2382c73ca6826,sizeof(s_rounded_wheel_cad2382c73ca6826),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6bca1b0fafc11502,sizeof(s_rounded_wheel_6bca1b0fafc11502),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_eed8c07b9b08820b,sizeof(s_rounded_wheel_eed8c07b9b08820b),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_dc96bfb5b03715ab,sizeof(s_rounded_wheel_dc96bfb5b03715ab),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9fc79a6b9ebb4664,sizeof(s_rounded_wheel_9fc79a6b9ebb4664),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2ce4261edac65c95,sizeof(s_rounded_wheel_2ce4261edac65c95),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7bb3a25c506dc844,sizeof(s_rounded_wheel_7bb3a25c506dc844),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2b6dd13bdbed8764,sizeof(s_rounded_wheel_2b6dd13bdbed8764),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7cfd9eefa6ea383c,sizeof(s_rounded_wheel_7cfd9eefa6ea383c),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2817761eb779d43b,sizeof(s_rounded_wheel_2817761eb779d43b),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_27e7aeca75441db8,sizeof(s_rounded_wheel_27e7aeca75441db8),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c7559bf4160a5018,sizeof(s_rounded_wheel_c7559bf4160a5018),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cfeffc2de4490e1d,sizeof(s_rounded_wheel_cfeffc2de4490e1d),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cd915925b1c2139e,sizeof(s_rounded_wheel_cd915925b1c2139e),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_810abec2f8ce0ffb,sizeof(s_rounded_wheel_810abec2f8ce0ffb),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7ea73f7764fe250e,sizeof(s_rounded_wheel_7ea73f7764fe250e),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2a8a8311de5985ea,sizeof(s_rounded_wheel_2a8a8311de5985ea),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_35f7cad095ba0c1a,sizeof(s_rounded_wheel_35f7cad095ba0c1a),38.000000f,16.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0fe62847bea15757,sizeof(s_rounded_wheel_0fe62847bea15757),38.000000f,134.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ec3cae23a5757f2f,sizeof(s_rounded_wheel_ec3cae23a5757f2f),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9b670fe7158c3a9e,sizeof(s_rounded_wheel_9b670fe7158c3a9e),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_dac11115f7eec33d,sizeof(s_rounded_wheel_dac11115f7eec33d),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9a1245254212fd16,sizeof(s_rounded_wheel_9a1245254212fd16),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6c422d5fee27326e,sizeof(s_rounded_wheel_6c422d5fee27326e),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d6ece84453605d85,sizeof(s_rounded_wheel_d6ece84453605d85),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5ec36bb1b5e0960d,sizeof(s_rounded_wheel_5ec36bb1b5e0960d),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_28875cf3db683a6f,sizeof(s_rounded_wheel_28875cf3db683a6f),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1f5a5d3fc42169e7,sizeof(s_rounded_wheel_1f5a5d3fc42169e7),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bd64421f791a0189,sizeof(s_rounded_wheel_bd64421f791a0189),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_06ccd852bc87bd92,sizeof(s_rounded_wheel_06ccd852bc87bd92),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2b80e0bc00337ed7,sizeof(s_rounded_wheel_2b80e0bc00337ed7),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_82ef6688f5d07fa2,sizeof(s_rounded_wheel_82ef6688f5d07fa2),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_07f302d215ac3d68,sizeof(s_rounded_wheel_07f302d215ac3d68),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b3e4457bc16cce48,sizeof(s_rounded_wheel_b3e4457bc16cce48),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_81981f7f49217128,sizeof(s_rounded_wheel_81981f7f49217128),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1013a101d96b0b9d,sizeof(s_rounded_wheel_1013a101d96b0b9d),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_187a9826228b134a,sizeof(s_rounded_wheel_187a9826228b134a),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8ec23d534fc57979,sizeof(s_rounded_wheel_8ec23d534fc57979),43.000000f,20.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_105b7197a369b1d0,sizeof(s_rounded_wheel_105b7197a369b1d0),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4aafa40185c2f0f0,sizeof(s_rounded_wheel_4aafa40185c2f0f0),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6bbee1f0d8d0c0aa,sizeof(s_rounded_wheel_6bbee1f0d8d0c0aa),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_dbd299e81f03c61e,sizeof(s_rounded_wheel_dbd299e81f03c61e),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cef8f3f6c823d5ef,sizeof(s_rounded_wheel_cef8f3f6c823d5ef),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b873d0086b36db7d,sizeof(s_rounded_wheel_b873d0086b36db7d),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1ef0e80fff8bec60,sizeof(s_rounded_wheel_1ef0e80fff8bec60),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cb44970416cb3455,sizeof(s_rounded_wheel_cb44970416cb3455),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bfce02864db7974e,sizeof(s_rounded_wheel_bfce02864db7974e),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b3a11f87ae9f3a1f,sizeof(s_rounded_wheel_b3a11f87ae9f3a1f),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_88cbd1ab74122c9b,sizeof(s_rounded_wheel_88cbd1ab74122c9b),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_75ed85b4d083314c,sizeof(s_rounded_wheel_75ed85b4d083314c),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_51874012ca473131,sizeof(s_rounded_wheel_51874012ca473131),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_50d9cb6613f34e24,sizeof(s_rounded_wheel_50d9cb6613f34e24),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6df977d26eb13144,sizeof(s_rounded_wheel_6df977d26eb13144),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e88056ffbae6e245,sizeof(s_rounded_wheel_e88056ffbae6e245),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c45a71418b4e551f,sizeof(s_rounded_wheel_c45a71418b4e551f),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_881ebcc61bf0cb40,sizeof(s_rounded_wheel_881ebcc61bf0cb40),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a40c8239022a678d,sizeof(s_rounded_wheel_a40c8239022a678d),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a9be8212f377a11c,sizeof(s_rounded_wheel_a9be8212f377a11c),47.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_448046f4ee23c740,sizeof(s_rounded_wheel_448046f4ee23c740),47.000000f,161.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e89b029dfbfebd2d,sizeof(s_rounded_wheel_e89b029dfbfebd2d),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bae02465c45bad37,sizeof(s_rounded_wheel_bae02465c45bad37),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cd34d5109fa8c258,sizeof(s_rounded_wheel_cd34d5109fa8c258),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_61aaac2b75583d75,sizeof(s_rounded_wheel_61aaac2b75583d75),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_63c2f222fe38123b,sizeof(s_rounded_wheel_63c2f222fe38123b),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b6411a14c5637da7,sizeof(s_rounded_wheel_b6411a14c5637da7),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c828955133cd61c8,sizeof(s_rounded_wheel_c828955133cd61c8),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7820dc3f7e655793,sizeof(s_rounded_wheel_7820dc3f7e655793),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_27e8e5ceb339ed80,sizeof(s_rounded_wheel_27e8e5ceb339ed80),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_295fcdfc0733521b,sizeof(s_rounded_wheel_295fcdfc0733521b),52.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_da536f370d35f3eb,sizeof(s_rounded_wheel_da536f370d35f3eb),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d9017942f6e7dcbe,sizeof(s_rounded_wheel_d9017942f6e7dcbe),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f46ea570705f871e,sizeof(s_rounded_wheel_f46ea570705f871e),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4ada7f40c96535d0,sizeof(s_rounded_wheel_4ada7f40c96535d0),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_eaf50609a3c10817,sizeof(s_rounded_wheel_eaf50609a3c10817),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4a86d3e3fe1e354c,sizeof(s_rounded_wheel_4a86d3e3fe1e354c),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a9bcc660f97b6503,sizeof(s_rounded_wheel_a9bcc660f97b6503),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8395d2cd92c7c391,sizeof(s_rounded_wheel_8395d2cd92c7c391),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c4706ccb4f9b38c9,sizeof(s_rounded_wheel_c4706ccb4f9b38c9),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5e0609f40cd62dea,sizeof(s_rounded_wheel_5e0609f40cd62dea),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ee9e7e4be269e833,sizeof(s_rounded_wheel_ee9e7e4be269e833),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_80fc93183ba732cc,sizeof(s_rounded_wheel_80fc93183ba732cc),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a1f8ece3ec7fc37e,sizeof(s_rounded_wheel_a1f8ece3ec7fc37e),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2a1e8faae082a6c2,sizeof(s_rounded_wheel_2a1e8faae082a6c2),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_dc70c9ae81bf787e,sizeof(s_rounded_wheel_dc70c9ae81bf787e),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cbdf9b0a8eb1704b,sizeof(s_rounded_wheel_cbdf9b0a8eb1704b),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_37f4741fb287976c,sizeof(s_rounded_wheel_37f4741fb287976c),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_dd779985511a13fa,sizeof(s_rounded_wheel_dd779985511a13fa),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7066005eaed2b9bc,sizeof(s_rounded_wheel_7066005eaed2b9bc),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1874888475f325b7,sizeof(s_rounded_wheel_1874888475f325b7),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_80d2668ebc87f5df,sizeof(s_rounded_wheel_80d2668ebc87f5df),43.000000f,19.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e626a4c0e198aa98,sizeof(s_rounded_wheel_e626a4c0e198aa98),51.000000f,153.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e177ed7959d03944,sizeof(s_rounded_wheel_e177ed7959d03944),43.000000f,19.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_97d4f8d1d86aff36,sizeof(s_rounded_wheel_97d4f8d1d86aff36),51.000000f,153.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7277fb5067f66d2d,sizeof(s_rounded_wheel_7277fb5067f66d2d),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5131b1f9b4595d31,sizeof(s_rounded_wheel_5131b1f9b4595d31),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_dcf1f156e8c38007,sizeof(s_rounded_wheel_dcf1f156e8c38007),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5f427c72ce518f55,sizeof(s_rounded_wheel_5f427c72ce518f55),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_27059dc5a35c90c7,sizeof(s_rounded_wheel_27059dc5a35c90c7),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7a55ccbeba4fe965,sizeof(s_rounded_wheel_7a55ccbeba4fe965),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_946d213639cc5354,sizeof(s_rounded_wheel_946d213639cc5354),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_95c473a80fe3a14a,sizeof(s_rounded_wheel_95c473a80fe3a14a),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cb82eda4d9b8a1a3,sizeof(s_rounded_wheel_cb82eda4d9b8a1a3),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_998b7bde211cb3f2,sizeof(s_rounded_wheel_998b7bde211cb3f2),43.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8ffb7ca116ebb099,sizeof(s_rounded_wheel_8ffb7ca116ebb099),51.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5face5b4e28c5afb,sizeof(s_rounded_wheel_5face5b4e28c5afb),51.000000f,146.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a8ceca16e6a8c4eb,sizeof(s_rounded_wheel_a8ceca16e6a8c4eb),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3b9bf1b90e29b5a8,sizeof(s_rounded_wheel_3b9bf1b90e29b5a8),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_96fe77b1bf591c12,sizeof(s_rounded_wheel_96fe77b1bf591c12),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c9add5e3c4635808,sizeof(s_rounded_wheel_c9add5e3c4635808),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_68c375a49411a618,sizeof(s_rounded_wheel_68c375a49411a618),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6bb8692e80a4d3a1,sizeof(s_rounded_wheel_6bb8692e80a4d3a1),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_32591915d4d74130,sizeof(s_rounded_wheel_32591915d4d74130),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3c2109912dadac3b,sizeof(s_rounded_wheel_3c2109912dadac3b),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ccd7e988b66a9248,sizeof(s_rounded_wheel_ccd7e988b66a9248),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3172b79040c553e8,sizeof(s_rounded_wheel_3172b79040c553e8),51.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_504b3a2adfc7d4b0,sizeof(s_rounded_wheel_504b3a2adfc7d4b0),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a3c5559e258ae69f,sizeof(s_rounded_wheel_a3c5559e258ae69f),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_311b855b6059fe5e,sizeof(s_rounded_wheel_311b855b6059fe5e),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_744844db6bc58578,sizeof(s_rounded_wheel_744844db6bc58578),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bf14ef7c1c292429,sizeof(s_rounded_wheel_bf14ef7c1c292429),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c6e684ff86d3e6aa,sizeof(s_rounded_wheel_c6e684ff86d3e6aa),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9831244c7e9967fe,sizeof(s_rounded_wheel_9831244c7e9967fe),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_29547d571d4de081,sizeof(s_rounded_wheel_29547d571d4de081),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5089afb6760b14c9,sizeof(s_rounded_wheel_5089afb6760b14c9),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9739006c0dbf005f,sizeof(s_rounded_wheel_9739006c0dbf005f),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5682f777d1d9d447,sizeof(s_rounded_wheel_5682f777d1d9d447),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ca95eee4161573c0,sizeof(s_rounded_wheel_ca95eee4161573c0),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c1954f5cf89922fc,sizeof(s_rounded_wheel_c1954f5cf89922fc),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_085f7c66effba410,sizeof(s_rounded_wheel_085f7c66effba410),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c91fde7b8f9e4ea0,sizeof(s_rounded_wheel_c91fde7b8f9e4ea0),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0cca98567ecc36a9,sizeof(s_rounded_wheel_0cca98567ecc36a9),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b767a1353bd1cc3c,sizeof(s_rounded_wheel_b767a1353bd1cc3c),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5b4adefc0f89638e,sizeof(s_rounded_wheel_5b4adefc0f89638e),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ba93c2cd591ad405,sizeof(s_rounded_wheel_ba93c2cd591ad405),38.000000f,18.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_37b10877d3008d3a,sizeof(s_rounded_wheel_37b10877d3008d3a),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7567f03aa5d307d5,sizeof(s_rounded_wheel_7567f03aa5d307d5),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9bcab2df1c7681c6,sizeof(s_rounded_wheel_9bcab2df1c7681c6),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7e65af881165e87b,sizeof(s_rounded_wheel_7e65af881165e87b),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cef757310d273d74,sizeof(s_rounded_wheel_cef757310d273d74),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_810cccc8ba5ef74c,sizeof(s_rounded_wheel_810cccc8ba5ef74c),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e0d4a9bf915d77ed,sizeof(s_rounded_wheel_e0d4a9bf915d77ed),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_98b7375ae431dad1,sizeof(s_rounded_wheel_98b7375ae431dad1),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_13998aa4d645243f,sizeof(s_rounded_wheel_13998aa4d645243f),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3646ff8c631ec5ce,sizeof(s_rounded_wheel_3646ff8c631ec5ce),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9750bb5ef5d89b77,sizeof(s_rounded_wheel_9750bb5ef5d89b77),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9313b3b6e7e49181,sizeof(s_rounded_wheel_9313b3b6e7e49181),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c11d987d1262c3f2,sizeof(s_rounded_wheel_c11d987d1262c3f2),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_24dd11dcf86b8a64,sizeof(s_rounded_wheel_24dd11dcf86b8a64),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_07f3bebffb37fce9,sizeof(s_rounded_wheel_07f3bebffb37fce9),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_17f8c10abebc80fe,sizeof(s_rounded_wheel_17f8c10abebc80fe),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_749d02a6ad6d1bd0,sizeof(s_rounded_wheel_749d02a6ad6d1bd0),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_093e92d853cfd1c3,sizeof(s_rounded_wheel_093e92d853cfd1c3),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9629646b48b0a690,sizeof(s_rounded_wheel_9629646b48b0a690),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9540dd4d822081f7,sizeof(s_rounded_wheel_9540dd4d822081f7),45.000000f,22.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3050df533e58ceac,sizeof(s_rounded_wheel_3050df533e58ceac),51.000000f,150.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3364e88951dc0dce,sizeof(s_rounded_wheel_3364e88951dc0dce),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8a247ee8f96cdd64,sizeof(s_rounded_wheel_8a247ee8f96cdd64),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_731e07f7682a3124,sizeof(s_rounded_wheel_731e07f7682a3124),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ec8923dc631023c2,sizeof(s_rounded_wheel_ec8923dc631023c2),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_066082fd8561f3d4,sizeof(s_rounded_wheel_066082fd8561f3d4),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5d1debd06225650a,sizeof(s_rounded_wheel_5d1debd06225650a),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_243806f4e571d608,sizeof(s_rounded_wheel_243806f4e571d608),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9590e349946c3b57,sizeof(s_rounded_wheel_9590e349946c3b57),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2664bbf577969541,sizeof(s_rounded_wheel_2664bbf577969541),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9b61f3641383aca8,sizeof(s_rounded_wheel_9b61f3641383aca8),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_71224dddef390d5f,sizeof(s_rounded_wheel_71224dddef390d5f),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_fbace55ff243fa04,sizeof(s_rounded_wheel_fbace55ff243fa04),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d02b41b2080e1bd5,sizeof(s_rounded_wheel_d02b41b2080e1bd5),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9e3a28b30a92deb6,sizeof(s_rounded_wheel_9e3a28b30a92deb6),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b4620f2c51dfa38b,sizeof(s_rounded_wheel_b4620f2c51dfa38b),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1ffa3e9e863b84e8,sizeof(s_rounded_wheel_1ffa3e9e863b84e8),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_9446d8fa3802a1cb,sizeof(s_rounded_wheel_9446d8fa3802a1cb),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_22c57d84a9b2125b,sizeof(s_rounded_wheel_22c57d84a9b2125b),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ef3212fed2226369,sizeof(s_rounded_wheel_ef3212fed2226369),45.000000f,29.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e87497d74a103139,sizeof(s_rounded_wheel_e87497d74a103139),51.000000f,167.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a488c73815d4d4f7,sizeof(s_rounded_wheel_a488c73815d4d4f7),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e615fdc3ebd766a1,sizeof(s_rounded_wheel_e615fdc3ebd766a1),43.000000f,19.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_251b6c51407be8af,sizeof(s_rounded_wheel_251b6c51407be8af),43.000000f,120.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4c4c101da2c9c0ed,sizeof(s_rounded_wheel_4c4c101da2c9c0ed),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5a616dfae038b035,sizeof(s_rounded_wheel_5a616dfae038b035),43.000000f,136.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_22213475426dfaf7,sizeof(s_rounded_wheel_22213475426dfaf7),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1695711affca59cc,sizeof(s_rounded_wheel_1695711affca59cc),43.000000f,136.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_567a47b8e47c4ba3,sizeof(s_rounded_wheel_567a47b8e47c4ba3),43.000000f,23.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_798a3a4d63db882f,sizeof(s_rounded_wheel_798a3a4d63db882f),43.000000f,136.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c8a1a953a4d9a64c,sizeof(s_rounded_wheel_c8a1a953a4d9a64c),36.000000f,14.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1617c41d1f1d1f36,sizeof(s_rounded_wheel_1617c41d1f1d1f36),59.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_0c78dd98909bce2b,sizeof(s_rounded_wheel_0c78dd98909bce2b),36.000000f,14.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d7bad9b95b87dba0,sizeof(s_rounded_wheel_d7bad9b95b87dba0),59.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_081b9f4cbb51bf10,sizeof(s_rounded_wheel_081b9f4cbb51bf10),43.000000f,153.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d92f65a531381259,sizeof(s_rounded_wheel_d92f65a531381259),43.000000f,153.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_75e5a15f73e93bea,sizeof(s_rounded_wheel_75e5a15f73e93bea),43.000000f,153.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_b220546be180a5a7,sizeof(s_rounded_wheel_b220546be180a5a7),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8078921bd0c91096,sizeof(s_rounded_wheel_8078921bd0c91096),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ba68bac39bbdcd5e,sizeof(s_rounded_wheel_ba68bac39bbdcd5e),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_1fc453ad712d9c93,sizeof(s_rounded_wheel_1fc453ad712d9c93),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_59ca9f678e66dce1,sizeof(s_rounded_wheel_59ca9f678e66dce1),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5e9b87707e9ba105,sizeof(s_rounded_wheel_5e9b87707e9ba105),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_81215908ce754559,sizeof(s_rounded_wheel_81215908ce754559),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_aae7bb98f303f59c,sizeof(s_rounded_wheel_aae7bb98f303f59c),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cceb163ad6182e6a,sizeof(s_rounded_wheel_cceb163ad6182e6a),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bbc1a177cfa9f361,sizeof(s_rounded_wheel_bbc1a177cfa9f361),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a43e4f5c148a20d3,sizeof(s_rounded_wheel_a43e4f5c148a20d3),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ac1fa1b1a1007dbd,sizeof(s_rounded_wheel_ac1fa1b1a1007dbd),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ebf491dc1704d3f7,sizeof(s_rounded_wheel_ebf491dc1704d3f7),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ae0128f051e23d90,sizeof(s_rounded_wheel_ae0128f051e23d90),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_82d8181ceacb19ac,sizeof(s_rounded_wheel_82d8181ceacb19ac),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4fd80ed781523c79,sizeof(s_rounded_wheel_4fd80ed781523c79),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_ff28cae9ec6fbfcf,sizeof(s_rounded_wheel_ff28cae9ec6fbfcf),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bc70c306821e05bf,sizeof(s_rounded_wheel_bc70c306821e05bf),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a20d8c00a859803b,sizeof(s_rounded_wheel_a20d8c00a859803b),41.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_38e9854182bcbb85,sizeof(s_rounded_wheel_38e9854182bcbb85),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_20f7128804b79dba,sizeof(s_rounded_wheel_20f7128804b79dba),51.000000f,158.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f4863f2feacaa5c2,sizeof(s_rounded_wheel_f4863f2feacaa5c2),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a3e0fc3fcd79e89d,sizeof(s_rounded_wheel_a3e0fc3fcd79e89d),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_16732b8469afb8c6,sizeof(s_rounded_wheel_16732b8469afb8c6),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_6632fa10ed39e2c4,sizeof(s_rounded_wheel_6632fa10ed39e2c4),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_fe444f628c5e29c3,sizeof(s_rounded_wheel_fe444f628c5e29c3),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3534eaea674aa7a1,sizeof(s_rounded_wheel_3534eaea674aa7a1),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_bbf291c794bcd5bb,sizeof(s_rounded_wheel_bbf291c794bcd5bb),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3ebc9682e5e5ab6f,sizeof(s_rounded_wheel_3ebc9682e5e5ab6f),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_8f1536c28346612c,sizeof(s_rounded_wheel_8f1536c28346612c),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_10b1bbbb5e6476f3,sizeof(s_rounded_wheel_10b1bbbb5e6476f3),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_2b103f6f3ad3425a,sizeof(s_rounded_wheel_2b103f6f3ad3425a),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f6f33f4a7f5324d7,sizeof(s_rounded_wheel_f6f33f4a7f5324d7),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_72e36308c9dad813,sizeof(s_rounded_wheel_72e36308c9dad813),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_c35d05c36b9024d9,sizeof(s_rounded_wheel_c35d05c36b9024d9),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_60d70ee778a9156c,sizeof(s_rounded_wheel_60d70ee778a9156c),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3ec49629baff265b,sizeof(s_rounded_wheel_3ec49629baff265b),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_a00d7d5c2fb622d2,sizeof(s_rounded_wheel_a00d7d5c2fb622d2),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_450f82a8488221d0,sizeof(s_rounded_wheel_450f82a8488221d0),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f073edff2f961ce9,sizeof(s_rounded_wheel_f073edff2f961ce9),45.000000f,24.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_df55bb02f44fe443,sizeof(s_rounded_wheel_df55bb02f44fe443),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_dd37e744e0dfb246,sizeof(s_rounded_wheel_dd37e744e0dfb246),51.000000f,155.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_56e41509f234d216,sizeof(s_rounded_wheel_56e41509f234d216),43.000000f,140.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_575698a6e6460069,sizeof(s_rounded_wheel_575698a6e6460069),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_574555b4a8426421,sizeof(s_rounded_wheel_574555b4a8426421),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_7a68baa12d346cab,sizeof(s_rounded_wheel_7a68baa12d346cab),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_10a7bf74a04e5c0d,sizeof(s_rounded_wheel_10a7bf74a04e5c0d),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e41c0269a0afa984,sizeof(s_rounded_wheel_e41c0269a0afa984),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3d4bb7a08b3fb6ff,sizeof(s_rounded_wheel_3d4bb7a08b3fb6ff),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_70612cd66ebc5444,sizeof(s_rounded_wheel_70612cd66ebc5444),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4dbde0bffb2e56fa,sizeof(s_rounded_wheel_4dbde0bffb2e56fa),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4797cd1f2fca3ee1,sizeof(s_rounded_wheel_4797cd1f2fca3ee1),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d58cb5b40a29a62e,sizeof(s_rounded_wheel_d58cb5b40a29a62e),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d367f367fa9c4d3f,sizeof(s_rounded_wheel_d367f367fa9c4d3f),43.000000f,143.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_d3da7c5a5b9ba642,sizeof(s_rounded_wheel_d3da7c5a5b9ba642),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_953cced8fcbd8fc7,sizeof(s_rounded_wheel_953cced8fcbd8fc7),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cd5190e009aa34ef,sizeof(s_rounded_wheel_cd5190e009aa34ef),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_48f0c83bd2c78f9e,sizeof(s_rounded_wheel_48f0c83bd2c78f9e),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_5f70aac92d3f49a2,sizeof(s_rounded_wheel_5f70aac92d3f49a2),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_27bc996e5fd0107b,sizeof(s_rounded_wheel_27bc996e5fd0107b),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_cfeff50e00ca4dbb,sizeof(s_rounded_wheel_cfeff50e00ca4dbb),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_f76a7441f960d812,sizeof(s_rounded_wheel_f76a7441f960d812),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_323fd5e4da62d06e,sizeof(s_rounded_wheel_323fd5e4da62d06e),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_4a93424f3effd3c9,sizeof(s_rounded_wheel_4a93424f3effd3c9),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_74219eb71e38374e,sizeof(s_rounded_wheel_74219eb71e38374e),43.000000f,139.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_3b62c8c247e5b548,sizeof(s_rounded_wheel_3b62c8c247e5b548),43.000000f,160.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_515a53decaac6714,sizeof(s_rounded_wheel_515a53decaac6714),34.000000f,15.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_576fffc30212a19e,sizeof(s_rounded_wheel_576fffc30212a19e),34.000000f,121.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_e58cdeec2c3684fb,sizeof(s_rounded_wheel_e58cdeec2c3684fb),38.000000f,141.000000f)==0);
    CHECK(ValidateFleetWheel(s_rounded_wheel_03d47b2360368bf6,sizeof(s_rounded_wheel_03d47b2360368bf6),38.000000f,134.000000f)==0);
    return 0;
}

int main(void) {
    CHECK(ValidateIntersectionPredicate()==0);
    CHECK(ValidateRoundedFleet()==0);
    CHECK(ValidateInBounds(s_rounded_erriso_grade1,sizeof(s_rounded_erriso_grade1),
        (const float[3]){-126,-25,-111},(const float[3]){126,143,363})==0);
    CHECK(ValidateInBounds(s_rounded_erriso_grade2,sizeof(s_rounded_erriso_grade2),
        (const float[3]){-139,-23,-118},(const float[3]){139,143,372})==0);
    CHECK(ValidateInBounds(s_rounded_erriso_grade3,sizeof(s_rounded_erriso_grade3),
        (const float[3]){-139,-30,-97},(const float[3]){139,135,388})==0);
    CHECK(ValidateInBounds(s_rounded_abeille_grade1,sizeof(s_rounded_abeille_grade1),
        (const float[3]){-145,-21,-73},(const float[3]){145,156,435})==0);
    CHECK(ValidateInBounds(s_rounded_abeille_grade2,sizeof(s_rounded_abeille_grade2),
        (const float[3]){-145,-27,-73},(const float[3]){145,142,435})==0);
    CHECK(ValidateInBounds(s_rounded_pegase_grade1,sizeof(s_rounded_pegase_grade1),
        (const float[3]){-140,-23,-99},(const float[3]){140,118,413})==0);
    CHECK(ValidateInBounds(s_rounded_esperanza_grade1,sizeof(s_rounded_esperanza_grade1),
        (const float[3]){-148,-35,-146},(const float[3]){148,144,501})==0);
    CHECK(ValidateInBounds(s_rounded_esperanza_grade2,sizeof(s_rounded_esperanza_grade2),
        (const float[3]){-148,-35,-146},(const float[3]){148,144,501})==0);
    CHECK(ValidateInBounds(s_rounded_esperanza_grade3,sizeof(s_rounded_esperanza_grade3),
        (const float[3]){-167,-36,-139},(const float[3]){167,144,512})==0);
    CHECK(ValidateInBounds(s_rounded_esperanza_grade4,sizeof(s_rounded_esperanza_grade4),
        (const float[3]){-166,-41,-152},(const float[3]){166,144,512})==0);
    CHECK(ValidateInBounds(s_rounded_acceron_grade1,sizeof(s_rounded_acceron_grade1),
        (const float[3]){-164,-33,-165},(const float[3]){164,137,529})==0);
    CHECK(ValidateInBounds(s_rounded_acceron_grade2,sizeof(s_rounded_acceron_grade2),
        (const float[3]){-164,-33,-153},(const float[3]){164,137,526})==0);
    CHECK(ValidateInBounds(s_rounded_acceron_grade3,sizeof(s_rounded_acceron_grade3),
        (const float[3]){-168,-52,-172},(const float[3]){168,135,529})==0);
    CHECK(ValidateInBounds(s_rounded_bayonet_grade1,sizeof(s_rounded_bayonet_grade1),
        (const float[3]){-158,-33,-149},(const float[3]){158,121,485})==0);
    CHECK(ValidateInBounds(s_rounded_bayonet_grade2,sizeof(s_rounded_bayonet_grade2),
        (const float[3]){-151,-43,-148},(const float[3]){151,139,485})==0);
    CHECK(ValidateInBounds(s_rounded_hijack_grade1,sizeof(s_rounded_hijack_grade1),
        (const float[3]){-149,-40,-168},(const float[3]){149,161,528})==0);
    CHECK(ValidateInBounds(s_rounded_fatalita_grade1,sizeof(s_rounded_fatalita_grade1),
        (const float[3]){-146,-38,-137},(const float[3]){146,113,469})==0);
    CHECK(ValidateInBounds(s_rounded_fatalita_grade2,sizeof(s_rounded_fatalita_grade2),
        (const float[3]){-156,-41,-137},(const float[3]){156,119,486})==0);
    CHECK(ValidateInBounds(s_rounded_istante_grade1,sizeof(s_rounded_istante_grade1),
        (const float[3]){-174,-42,-123},(const float[3]){174,111,516})==0);
    CHECK(Validate(s_rounded_erriso_body,sizeof(s_rounded_erriso_body),0)==0);
    CHECK(Validate(s_rounded_erriso_rival,sizeof(s_rounded_erriso_rival),0)==0);
    CHECK(Validate(s_rounded_abeille_body,sizeof(s_rounded_abeille_body),1)==0);
    CHECK(Validate(s_rounded_abeille_rival,sizeof(s_rounded_abeille_rival),1)==0);
    CHECK(Validate(s_rounded_pegase_body,sizeof(s_rounded_pegase_body),2)==0);
    CHECK(Validate(s_rounded_pegase_rival,sizeof(s_rounded_pegase_rival),2)==0);
    CHECK(Validate(s_esperanza_body,sizeof(s_esperanza_body),3)==0);
    CHECK(Validate(s_rounded_esperanza_rival_duplicate,sizeof(s_rounded_esperanza_rival_duplicate),3)==0);
    CHECK(Validate(s_rounded_esperanza_rival_slot1,sizeof(s_rounded_esperanza_rival_slot1),3)==0);
    CHECK(Validate(s_rounded_esperanza_rival_slot2,sizeof(s_rounded_esperanza_rival_slot2),3)==0);
    CHECK(Validate(s_rounded_esperanza_rival,sizeof(s_rounded_esperanza_rival),3)==0);
    CHECK(Validate(s_rounded_esperanza_rival_late,sizeof(s_rounded_esperanza_rival_late),3)==0);
    CHECK(Validate(s_rounded_acceron_body,sizeof(s_rounded_acceron_body),4)==0);
    CHECK(Validate(s_rounded_acceron_rival,sizeof(s_rounded_acceron_rival),4)==0);
    CHECK(Validate(s_rounded_bayonet_body,sizeof(s_rounded_bayonet_body),5)==0);
    CHECK(Validate(s_rounded_bayonet_rival,sizeof(s_rounded_bayonet_rival),5)==0);
    CHECK(Validate(s_rounded_hijack_body,sizeof(s_rounded_hijack_body),6)==0);
    CHECK(Validate(s_rounded_hijack_rival,sizeof(s_rounded_hijack_rival),6)==0);
    CHECK(Validate(s_rounded_hijack_rival_alternate,sizeof(s_rounded_hijack_rival_alternate),6)==0);
    CHECK(Validate(s_rounded_fatalita_body,sizeof(s_rounded_fatalita_body),7)==0);
    CHECK(Validate(s_rounded_fatalita_rival,sizeof(s_rounded_fatalita_rival),7)==0);
    CHECK(Validate(s_rounded_istante_body,sizeof(s_rounded_istante_body),8)==0);
    CHECK(Validate(s_rounded_istante_rival,sizeof(s_rounded_istante_rival),8)==0);
    CHECK(Validate(s_rounded_ghepardo_body,sizeof(s_rounded_ghepardo_body),9)==0);
    CHECK(Validate(s_rounded_ghepardo_rival,sizeof(s_rounded_ghepardo_rival),9)==0);
    CHECK(Validate(s_rounded_vainqure_body,sizeof(s_rounded_vainqure_body),10)==0);
    CHECK(Validate(s_rounded_vainqure_rival,sizeof(s_rounded_vainqure_rival),10)==0);
    CHECK(Validate(s_rounded_bulshade_body,sizeof(s_rounded_bulshade_body),11)==0);
    CHECK(Validate(s_rounded_bulshade_rival,sizeof(s_rounded_bulshade_rival),11)==0);
    CHECK(Validate(s_rounded_bulshade_rival_alternate,sizeof(s_rounded_bulshade_rival_alternate),11)==0);
    CHECK(Validate(s_rounded_squaldon_body,sizeof(s_rounded_squaldon_body),12)==0);
    CHECK(Validate(s_rounded_squaldon_rival,sizeof(s_rounded_squaldon_rival),12)==0);
    CHECK(Validate(s_rounded_compacta_rival_early,sizeof(s_rounded_compacta_rival_early),13)==0);
    CHECK(Validate(s_rounded_compactb_rival_early,sizeof(s_rounded_compactb_rival_early),13)==0);
    CHECK(Validate(s_rounded_compactc_rival_early,sizeof(s_rounded_compactc_rival_early),13)==0);
    CHECK(Validate(s_rounded_compacta_rival_middle,sizeof(s_rounded_compacta_rival_middle),3)==0);
    CHECK(Validate(s_rounded_compacta_rival_late,sizeof(s_rounded_compacta_rival_late),3)==0);
    CHECK(Validate(s_rounded_compactb_rival_middle,sizeof(s_rounded_compactb_rival_middle),3)==0);
    CHECK(Validate(s_rounded_compactb_rival_late,sizeof(s_rounded_compactb_rival_late),3)==0);
    CHECK(Validate(s_rounded_compactc_rival_middle,sizeof(s_rounded_compactc_rival_middle),3)==0);
    CHECK(Validate(s_rounded_compactc_rival_late,sizeof(s_rounded_compactc_rival_late),3)==0);
    CHECK(ValidateAbeillePanelColorSeam()==0);
    CHECK(ValidateRegistry()==0);
    CHECK(ValidateRoundedEsperanza()==0);
    CHECK(ValidateEsperanzaWellLiners()==0);
    CHECK(ValidateFenderIntersections(s_esperanza_rounded,sizeof(s_esperanza_rounded))==0);
    CHECK(ValidatePegaseHoodDecal()==0);
    puts("authored car geometry, scale, normals, seams and material registries valid");
    return 0;
}
