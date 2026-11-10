#include "terrain_seams.h"
#include "render_instance_transform.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum { MAX_POINTS=131072, MAX_CELLS=2048 };
typedef struct SeamPoint {
    int32_t q[3]; /* Exact source GTE steps in world space. */
    uint32_t asset,parent,owner[2];
    int8_t low[3],high[3];
} SeamPoint;
typedef struct SeamCell {
    const RageRuntimeMesh *source;
    uint32_t asset,entity,mesh;
    RenderTransform transform;
} SeamCell;
struct RenderTerrainSeams {
    SeamPoint *points;
    uint32_t *slots;
    uint32_t count,capacity,slotCount;
    SeamCell cells[MAX_CELLS];
    uint32_t cellSlots[MAX_CELLS*2];
    uint32_t cellCount;
};
static uint32_t Hash(const int32_t q[3],uint32_t asset){
    uint32_t h=(uint32_t)q[0]*0x8da6b343u^(uint32_t)q[1]*0xd8163841u^
        (uint32_t)q[2]*0xcb1ab31fu^asset*0x9e3779b9u;
    h^=h>>16;h*=0x7feb352du;return h^(h>>15);
}
static int Quantize(Vec3 p,int32_t q[3]){
    float values[3]={p.x,p.y,p.z};
    for(int axis=0;axis<3;axis++){
        float v=values[axis]*4.0f;
        if(!isfinite(v)||fabsf(v)>10000000.0f)return 0;
        float rounded=roundf(v);
        if(fabsf(v-rounded)>0.001f)return 0; /* Preserve arbitrary mod transforms. */
        q[axis]=(int32_t)rounded;
    }
    return 1;
}
static uint32_t Find(const RenderTerrainSeams *c,const int32_t q[3],uint32_t asset){
    if(!c||!c->slotCount)return UINT32_MAX;
    uint32_t slot=Hash(q,asset)&(c->slotCount-1);
    while(c->slots[slot]){
        uint32_t i=c->slots[slot]-1;
        if(c->points[i].asset==asset&&!memcmp(c->points[i].q,q,3*sizeof(*q)))return i;
        slot=(slot+1)&(c->slotCount-1);
    }
    return UINT32_MAX;
}
static int Grow(RenderTerrainSeams *c){
    if(c->count<c->capacity)return 1;
    if(c->capacity==MAX_POINTS)return 0;
    uint32_t capacity=c->capacity?c->capacity*2:4096,slotCount=capacity*2;
    uint32_t *slots=calloc(slotCount,sizeof(*slots));
    if(!slots)return 0;
    SeamPoint *points=realloc(c->points,(size_t)capacity*sizeof(*points));
    if(!points){free(slots);return 0;}
    c->points=points;
    for(uint32_t i=0;i<c->count;i++){
        uint32_t slot=Hash(points[i].q,points[i].asset)&(slotCount-1);
        while(slots[slot])slot=(slot+1)&(slotCount-1);
        slots[slot]=i+1;
    }
    free(c->slots);c->slots=slots;c->slotCount=slotCount;c->capacity=capacity;return 1;
}
static uint32_t Root(RenderTerrainSeams *c,uint32_t at){
    uint32_t root=at;
    while(c->points[root].parent!=root)root=c->points[root].parent;
    while(c->points[at].parent!=at){uint32_t next=c->points[at].parent;c->points[at].parent=root;at=next;}
    return root;
}
static int Less(const SeamPoint *a,const SeamPoint *b){
    for(int axis=0;axis<3;axis++)if(a->q[axis]!=b->q[axis])return a->q[axis]<b->q[axis];
    return 0;
}
static int Neighbours(uint32_t a,uint32_t b){
    if(!a||!b||a==b)return 0;
    int ax=(int)(a&31u),az=(int)((a>>5)&31u);
    int bx=(int)(b&31u),bz=(int)((b>>5)&31u);
    return abs(ax-bx)<=1&&abs(az-bz)<=1;
}
static void Insert(RenderTerrainSeams *c,Vec3 p,uint32_t asset,uint32_t owner){
    int32_t q[3];if(!Quantize(p,q))return;
    uint32_t at=Find(c,q,asset);
    if(at==UINT32_MAX){
        if(!Grow(c))return;
        at=c->count++;SeamPoint *point=&c->points[at];
        memcpy(point->q,q,sizeof(q));point->asset=asset;point->parent=at;
        point->owner[0]=owner;point->owner[1]=0;
        memset(point->low,0,sizeof(point->low));memset(point->high,0,sizeof(point->high));
        uint32_t slot=Hash(q,asset)&(c->slotCount-1);
        while(c->slots[slot])slot=(slot+1)&(c->slotCount-1);
        c->slots[slot]=at+1;
    }else{
        SeamPoint *point=&c->points[at];
        if(point->owner[0]==owner||point->owner[1]==owner)return;
        if(!point->owner[1])point->owner[1]=owner;
    }
    for(int z=-1;z<=1;z++)for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){
        if(!x&&!y&&!z)continue;
        int32_t near[3]={q[0]+x,q[1]+y,q[2]+z};uint32_t other=Find(c,near,asset);
        if(other==UINT32_MAX||!(Neighbours(owner,c->points[other].owner[0])||Neighbours(owner,c->points[other].owner[1])))continue;
        uint32_t a=Root(c,at),b=Root(c,other);if(a==b)continue;
        /* Keep each group within ONE source step: no transitive collapse of
         * a chain of legitimate small features across several cells. */
        int close=1;int32_t low[3],high[3];
        for(int axis=0;axis<3;axis++){
            int32_t al=c->points[a].q[axis]+c->points[a].low[axis],bl=c->points[b].q[axis]+c->points[b].low[axis];
            int32_t ah=c->points[a].q[axis]+c->points[a].high[axis],bh=c->points[b].q[axis]+c->points[b].high[axis];
            low[axis]=al<bl?al:bl;high[axis]=ah>bh?ah:bh;if(high[axis]-low[axis]>1)close=0;
        }
        if(close){
            if(!Less(&c->points[a],&c->points[b])){uint32_t swap=a;a=b;b=swap;}
            c->points[b].parent=a;
            for(int axis=0;axis<3;axis++){c->points[a].low[axis]=(int8_t)(low[axis]-c->points[a].q[axis]);c->points[a].high[axis]=(int8_t)(high[axis]-c->points[a].q[axis]);}
        }
    }
}
static int TerrainCell(const RenderMeshInstance *i,int pass){
    return i->assetSet==RAGE_RENDER_ASSET_TERRAIN&&(i->entity&~1023u)==0x20000u&&
        (pass<0||i->pass==(RenderPass)pass);
}
static uint32_t CellSlot(const RenderTerrainSeams *c,uint32_t asset,uint32_t entity){
    uint32_t slot=(asset*0x9e3779b9u^entity*0x8da6b343u)&(MAX_CELLS*2-1);
    while(c->cellSlots[slot]){
        const SeamCell *cell=&c->cells[c->cellSlots[slot]-1];
        if(cell->asset==asset&&cell->entity==entity)break;
        slot=(slot+1)&(MAX_CELLS*2-1);
    }
    return slot;
}
void RenderTerrainSeamsRelease(RenderTerrainSeams *c){if(c){free(c->points);free(c->slots);free(c);}}
void RenderTerrainSeamsPrepare(RenderTerrainSeams **cache,const RenderWorld *world,
    int pass,RageRenderMeshLookup lookup,void *context){
    if(!cache||!world||!lookup||world->instanceCount>world->instanceCapacity||
        (world->instanceCount&&!world->instances))return;
    unsigned cells=0;for(uint32_t i=0;i<world->instanceCount;i++)if(TerrainCell(&world->instances[i],pass))cells++;
    if(!*cache&&cells<2)return;
    if(!*cache)*cache=calloc(1,sizeof(**cache));
    RenderTerrainSeams *c=*cache;if(!c)return;
    /* If a source/transform is replaced in-place, stale correspondence must
     * not weld the replacement against vertices that no longer exist. */
    for(uint32_t i=0;i<world->instanceCount;i++){
        const RenderMeshInstance *item=&world->instances[i];if(!TerrainCell(item,pass))continue;
        const RageRuntimeMesh *mesh=lookup(context,item);if(!mesh)continue;
        uint32_t slot=CellSlot(c,item->assetKey,item->entity);
        if(c->cellSlots[slot]){
            const SeamCell *cell=&c->cells[c->cellSlots[slot]-1];
            if(cell->source!=mesh||cell->mesh!=item->mesh||memcmp(&cell->transform,&item->transform,sizeof(item->transform))){
                c->count=0;c->cellCount=0;
                if(c->slots)memset(c->slots,0,(size_t)c->slotCount*sizeof(*c->slots));
                memset(c->cellSlots,0,sizeof(c->cellSlots));
            }
        }
    }
    for(uint32_t i=0;i<world->instanceCount;i++){
        const RenderMeshInstance *item=&world->instances[i];if(!TerrainCell(item,pass))continue;
        uint32_t slot=CellSlot(c,item->assetKey,item->entity);
        if(c->cellSlots[slot]||c->cellCount==MAX_CELLS)continue;
        const RageRuntimeMesh *mesh=lookup(context,item);uint32_t first,count;
        if(!mesh||!RuntimeMeshRange(mesh,item->mesh,&first,&count))continue;
        RenderInstanceTransform transform=RenderPrepareInstanceTransform(&item->transform);
        for(uint32_t j=0;j<count;j++){
            uint32_t index;RageRuntimeVertex vertex;
            if(!RuntimeMeshIndex(mesh,first+j,&index)||!RuntimeMeshVertex(mesh,index,&vertex))continue;
            Vec3 p=RenderTransformInstancePoint(&transform,(Vec3){vertex.position[0],vertex.position[1],vertex.position[2]});
            p.x=RenderSnapTerrainCellBoundary(p.x);p.z=RenderSnapTerrainCellBoundary(p.z);
            Insert(c,p,item->assetKey,item->entity);
        }
        SeamCell *cell=&c->cells[c->cellCount++];cell->source=mesh;cell->asset=item->assetKey;
        cell->entity=item->entity;cell->mesh=item->mesh;cell->transform=item->transform;
        c->cellSlots[slot]=c->cellCount;
    }
}
Vec3 RenderTerrainSeamsPosition(RenderTerrainSeams *c,const RenderMeshInstance *instance,Vec3 p){
    int32_t q[3];if(!c||!instance||!TerrainCell(instance,-1)||!Quantize(p,q))return p;
    uint32_t at=Find(c,q,instance->assetKey);if(at==UINT32_MAX)return p;
    const SeamPoint *root=&c->points[Root(c,at)];
    return (Vec3){(float)root->q[0]*0.25f,(float)root->q[1]*0.25f,(float)root->q[2]*0.25f};
}
