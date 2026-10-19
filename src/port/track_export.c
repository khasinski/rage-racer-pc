#include "track_export.h"
#include "native_asset_importer.h"
#include "native_mesh_writer.h"
#include "game/asset.h"
#include "game/track_internal.h"
#include "course_coordinate.h"
#include "render/mod_file_snapshot.h"
#include <SDL3/SDL.h>
#include <stdio.h>

int ExportTrack(const char *source,int bank,const char *directory) {
    void *bytes=NULL;size_t size=0;int ok=0;
    const RageRuntimeCachedMesh *meshes[2]={0};
    RenderMeshInstance instances[2]={{0},{0}};
    const char *names[]={"course","terrain"};char files[2][2048]={{0}};int written[2]={0};
    SDL_PathInfo info;
    if(!source||!directory||bank<88||bank>198||bank%2||
       !SDL_GetPathInfo(source,&info)||info.type!=SDL_PATHTYPE_FILE||info.size>128u*1024u*1024u)return 0;
    bytes=SDL_LoadFile(source,&size);
    if(!bytes||size>128u*1024u*1024u||!InstallTrackRuntimeAssetPack(bytes,size,bank,0)||!NativeAssetImporterInit())goto done;
    for(unsigned i=0;i<2;i++){
        instances[i].assetKey=(uint32_t)bank;
        instances[i].assetSet=i?RAGE_RENDER_ASSET_TERRAIN:RAGE_RENDER_ASSET_COURSE;
        meshes[i]=NativeAssetImporterFind(&instances[i]);if(!meshes[i])goto done;
        int n=snprintf(files[i],sizeof(files[i]),"%s/%s.rmesh",directory,names[i]);
        if(n<0||(size_t)n>=sizeof(files[i]))goto done;
        if(!ModFileWriteExclusive(files[i],meshes[i]->mesh.bytes,meshes[i]->mesh.size))goto done;
        written[i]=1;
    }
    putchar('{');
    for(unsigned i=0;i<2;i++){
        printf("%s\"%s\":{\"meshes\":%u,\"materials\":[",i?",":"",names[i],meshes[i]->mesh.meshCount);
        RageImportedTextureKey key;uint32_t m=0;
        while(NativeAssetImporterMaterialKey(&instances[i],m,&key)){
            printf("%s{\"source\":%u,\"page\":%u,\"clut\":%u,\"windowWidthU\":%u,\"windowWidthV\":%u,\"windowOffsetU\":%u,\"windowOffsetV\":%u}",m?",":"",m,key.tpage,key.clut,key.windowWidthU,key.windowWidthV,key.windowOffsetU,key.windowOffsetV);m++;
        }
        printf("],\"instances\":[");
        int comma=0;
        if(i){
            for(unsigned z=0;z<32;z++)for(unsigned x=0;x<32;x++){
                unsigned mesh=g_TerrainCellGrid[(31-z)*32+x]&TERRAIN_CELL_INDEX_MASK;
                if(mesh==TERRAIN_MISSING_CELL_INDEX)continue;
                printf("%s{\"mesh\":%u,\"cell\":%u,\"position\":[%u,0,%d],\"yaw\":0}",comma?",":"",mesh,(31-z)*32+x,x*2048+1024,-(int)(z*2048+1024));comma=1;
            }
        }else{
            for(int at=0;at<g_CourseObjectCount;at++){
                const CourseObject *object=&g_CourseObjects[at];if(object->modelId==-1)continue;
                printf("%s{\"mesh\":%d,\"object\":%d,\"position\":[%d,%d,%d],\"yaw\":%d}",comma?",":"",object->modelId,at,CourseCoordinateNearReference(object->x,32768),-CourseCoordinateNearReference(object->y,0),-CourseCoordinateNearReference(object->z,32768),object->rotationY);comma=1;
            }
        }
        printf("]}");
    }
    puts("}");ok=!ferror(stdout);
done:
    NativeAssetImporterShutdown();SDL_free(bytes);
    if(!ok)for(unsigned i=0;i<2;i++)if(written[i])SDL_RemovePath(files[i]);
    return ok;
}
