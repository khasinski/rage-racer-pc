#include "car_export.h"
#include "native_asset_importer.h"
#include "native_mesh_writer.h"
#include "game/asset.h"
#include "game/track_internal.h"
#include "game/car.h"
#include "render/mod_file_snapshot.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static void Store32(unsigned char *p,uint32_t value){
    for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(value>>(i*8));
}
/* Reuse the live game's validated installation/import path, without a window
 * or disc mount. The worker owns its source bytes and writes one static part. */
int ExportRawCar(const char *source,int bank,int rival,int part,const char *output){
    void *bytes=NULL;unsigned char *wire=NULL;size_t size=0;int ok=0;
    SDL_PathInfo info;RenderMeshInstance instance={0};
    const RageRuntimeCachedMesh *cached;RageRuntimeMeshLayout layout;
    uint32_t first,count;
    if(!source||!output||part<0||part>255||bank%2||
       (rival?(bank<88||bank>134):(bank<10||bank>72))||
       !SDL_GetPathInfo(source,&info)||info.type!=SDL_PATHTYPE_FILE||
       info.size>128u*1024u*1024u)return 0;
    bytes=SDL_LoadFile(source,&size);if(!bytes)goto done;
    if(rival){if(!InstallTrackRuntimeAssetPack(bytes,size,bank,0))goto done;}
    else{if(!InstallCarModelAsset(bytes,size,0,-1))goto done;g_CarModelSlot=0;}
    if(!NativeAssetImporterInit())goto done;
    instance.assetKey=(uint32_t)bank;
    instance.assetSet=rival?RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1:RAGE_RENDER_ASSET_MODEL_BANK;
    cached=NativeAssetImporterFind(&instance);
    if(!cached||!RuntimeMeshRange(&cached->mesh,(uint32_t)part,&first,&count)||
       !count||!RuntimeMeshLayout(1,count,count,&layout)||
       layout.totalSize>128u*1024u*1024u)goto done;
    wire=SDL_calloc(1,layout.totalSize);if(!wire||
       !RuntimeMeshEncodeHeader(wire,layout.totalSize,1,count,count))goto done;
    Store32(wire+layout.offsetsOffset+4,count);
    for(uint32_t i=0;i<count;i++){
        uint32_t index;RageRuntimeVertex vertex;
        if(!RuntimeMeshIndex(&cached->mesh,first+i,&index)||
           !RuntimeMeshVertex(&cached->mesh,index,&vertex)||
           !RuntimeVertexEncode(wire+layout.verticesOffset+i*40,40,&vertex))goto done;
        if ((vertex.material & RAGE_RUNTIME_MATERIAL_INDEX_MASK) != 65535u) {
            if ((vertex.material & RAGE_RUNTIME_MATERIAL_INDEX_MASK) >= 64u) goto done;
            vertex.material |= RAGE_RUNTIME_MATERIAL_CAR_BANK;
            if (!RuntimeVertexEncode(wire+layout.verticesOffset+i*40,40,&vertex)) goto done;
        }
        Store32(wire+layout.indicesOffset+i*4,i);
    }
    RageRuntimeMesh check;if(!RuntimeMeshOpen(&check,wire,layout.totalSize)||
       !ModFileWriteExclusive(output,wire,layout.totalSize))goto done;
    printf("{\"materials\":[");
    RageImportedTextureKey key;uint32_t m=0;
    while(NativeAssetImporterMaterialKey(&instance,m,&key)){
        printf("%s{\"source\":%u,\"page\":%u,\"clut\":%u}",m?",":"",m,key.tpage,key.clut);m++;
    }
    puts("]}");ok=1;
done:
    NativeAssetImporterShutdown();SDL_free(wire);SDL_free(bytes);
    if(!ok)fprintf(stderr,"Invalid raw car bank or part; output unchanged\n");
    return ok;
}
