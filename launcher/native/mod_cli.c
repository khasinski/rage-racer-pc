#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif

#include "port/platform_paths.h"
#include "render/mod_manifest.h"
#include "render/mod_package.h"
#include "render/rmesh.h"
#include "content/track_pack.h"
#include "content/sha256.h"
#include "content/active_build.h"
#include "port/car_catalog.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "preview_png.h"
#include "texture_preview.h"
#include "car_spec.h"
#include "car_materials.h"
static RageModManifest manifest;
#include "manifest_edit.h"
#include "mod_selection_cli.h"
#include "mod_provider_cli.h"
#include "mod_snapshot.h"
#include "mod_profile_cli.h"
#include "mod_file_policy_cli.h"
#include "mod_inventory.h"
#include "legacy_index_cli.h"
#include "mod_package_cli.h"
static int ValidatePng(const char *path) {
    unsigned char header[24];
    FILE *file=fopen(path,"rb");
    long size;
    unsigned w,h;
    SDL_Surface *surface;
    if(!file)return 1;
    if(fread(header,1,sizeof(header),file)!=sizeof(header)||
       fseek(file,0,SEEK_END)||(size=ftell(file))<24||size>32*1024*1024){fclose(file);return 1;}
    fclose(file);
    if(memcmp(header,"\x89PNG\r\n\x1a\n",8)||memcmp(header+12,"IHDR",4))return 1;
    w=(unsigned)header[16]<<24|(unsigned)header[17]<<16|(unsigned)header[18]<<8|header[19];
    h=(unsigned)header[20]<<24|(unsigned)header[21]<<16|(unsigned)header[22]<<8|header[23];
    if(!w||!h||w>4096||h>4096)return 1;
    surface=SDL_LoadPNG(path);
    if(!surface){fprintf(stderr,"Cannot decode PNG: %s\n",SDL_GetError());return 1;}
    printf("{\"width\":%d,\"height\":%d}\n",surface->w,surface->h);
    SDL_DestroySurface(surface);return 0;
}
static void String(const char *s) {
    const unsigned char *p=(const unsigned char *)s;putchar('"');
    for(;*p;p++){if(*p=='"'||*p=='\\')printf("\\%c",*p);else if(*p<32||*p>=127)printf("\\u%04x",*p);else putchar(*p);}putchar('"');
}
int main(int argc,char **argv) {
    FILE *f;long size;char *bytes;size_t i;
    if(argc==5&&strcmp(argv[1],"--active-build")==0){
        ActiveBuild build;char error[160];int result=ActiveBuildOpen(argv[2],argv[3],argv[4],&build,error,sizeof(error));
        if(result<0){fprintf(stderr,"%s\n",error);return 1;}
        if(result==0){puts("{\"active\":false}");return 0;}
        printf("{\"active\":true,\"directory\":");String(build.directory);
        printf(",\"profile\":");String(build.profile);puts("}");return 0;
    }
    if(argc==3&&strcmp(argv[1],"--hash-file")==0){
        char hex[65];if(!Sha256File(argv[2],SIZE_MAX,hex))return 1;
        printf("{\"sha256\":\"%s\"}\n",hex);return 0;
    }
    if(argc==5 && strcmp(argv[1],"--compose-car-profile")==0) {
        RageCarCatalog base,overlay;char error[256];
        if(!CarCatalogReadFile(argv[2],&base,error,sizeof(error))||
           !CarCatalogReadFile(argv[3],&overlay,error,sizeof(error))||
           !CarCatalogMerge(&base,&overlay,error,sizeof(error))||
           !CarCatalogWriteFile(argv[4],&base,error,sizeof(error))) {
            fprintf(stderr,"%s\n",error);return 1;
        }
        puts("{\"valid\":true}");return 0;
    }
    if(argc==3 && strcmp(argv[1],"--car-profile")==0) {
        RageCarCatalog catalog;char error[256];
        if(!CarCatalogReadFile(argv[2],&catalog,error,sizeof(error))){fprintf(stderr,"%s\n",error);return 1;}
        printf("{\"cars\":[");
        for(i=0;i<catalog.count;i++) {
            const RageCarCatalogEntry *e=&catalog.entries[i];
            size_t count,k,n;const CarField *fields=CarCatalogFields(&count);
            printf("%s{\"variant\":%d,\"fields\":%llu",i?",":"",CarCatalogVariant(e->modelIndex,e->grade),(unsigned long long)e->fields);
            for(k=0;k<count;k++) {
                const CarField *field=&fields[k];const unsigned char *p=(const unsigned char *)e+field->offset;
                if(!(e->fields&field->bit))continue;
                putchar(',');String(field->name);putchar(':');
                if(field->type==FIELD_TEXT){String((const char *)p);continue;}
                if(field->count>1)putchar('[');
                for(n=0;n<field->count;n++) {
                    int value;
                    if(field->type==FIELD_SHORT){int16_t v;memcpy(&v,p+n*2,2);value=v;}
                    else if(field->type==FIELD_USHORT){uint16_t v;memcpy(&v,p+n*2,2);value=v;}
                    else memcpy(&value,p+n*4,4);
                    if(n)putchar(',');
                    if(field->type==FIELD_BOOL)printf("%s",value?"true":"false");else printf("%d",value);
                }
                if(field->count>1)putchar(']');
            }
            printf("}");
        }
        puts("]}");return 0;
    }
    if(argc==3 && (strcmp(argv[1],"--track-pack")==0 || strcmp(argv[1],"--track-scene")==0)) {
        TrackBlock blocks[TRACK_PACK_BLOCK_COUNT];
        f=fopen(argv[2],"rb");if(!f)return 1;
        if(fseek(f,0,SEEK_END)||(size=ftell(f))<44||size>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return 1;}
        bytes=malloc((size_t)size);if(!bytes){fclose(f);return 1;}
        i=fread(bytes,1,(size_t)size,f);fclose(f);
        if(i!=(size_t)size||!TrackPackOpen(bytes,(size_t)size,blocks)){
            free(bytes);fprintf(stderr,"Invalid track pack container\n");return 1;
        }
        printf("{\"blocks\":[");
        for(i=0;i<TRACK_PACK_BLOCK_COUNT;i++) {
            printf("%s{\"role\":",i?",":"");String(TrackBlockName((unsigned)i));
            printf(",\"offset\":%zu,\"size\":%zu}",(size_t)(blocks[i].data-(unsigned char *)bytes),blocks[i].size);
        }
        if(strcmp(argv[1],"--track-scene")==0) {
            Road road;
            if(!RoadOpen(blocks[4].data,blocks[4].size,&road)) {
                free(bytes);fprintf(stderr,"Invalid road data\n");return 1;
            }
            printf("],\"road\":{\"length\":%u,\"points\":[",road.length);
            for(i=0;i<road.count;i++) {
                RoadPoint p;RoadRead(&road,(uint32_t)i,&p);
                printf("%s{\"x\":%d,\"y\":%d,\"z\":%d,\"angle\":%d,\"pitch\":%d,\"crossSlope\":%d,\"leftWidth\":%d,\"rightWidth\":%d,\"arc\":%u,\"length\":%u}",
                       i?",":"",p.x,p.y,p.z,p.angle,p.pitch,p.crossSlope,p.leftWidth,p.rightWidth,p.arc,p.length);
            }
            printf("],\"arcs\":[");
            for(i=0;i<road.arcCount;i++) {
                const unsigned char *p=road.arcs+i*12;
                printf("%s{\"x\":%d,\"z\":%d,\"reserved\":%d}",i?",":"",(int32_t)SpecU32(p),(int32_t)SpecU32(p+4),(int32_t)SpecU32(p+8));
            }
            printf("]}");
        } else printf("]");
        puts("}");free(bytes);return 0;
    }
    if(argc==2 && strcmp(argv[1],"--mod-directory")==0) {
        char directory[4096];
        if(!PlatformUserConfigPath("mods",directory,sizeof(directory))) return 1;
        printf("{\"directory\":");String(directory);puts("}");return 0;
    }
    if(argc==3 && strcmp(argv[1],"--write-profile-stdin")==0) return ProfileCommand(argv[2]);
    if(argc==3 && strcmp(argv[1],"--write-legacy-index-stdin")==0) return LegacyIndexWriteCommand(argv[2]);
    if(argc==3 && strcmp(argv[1],"--legacy-index")==0) return LegacyIndexCommand(argv[2]);
    if(argc==3 && strcmp(argv[1],"--inventory")==0) return InventoryCommand(argv[2]);
    if(argc==2 && strcmp(argv[1],"--check-files-stdin")==0) return FilePolicyCommand(0);
    if(argc==2 && strcmp(argv[1],"--check-directories-stdin")==0) return FilePolicyCommand(2);
    if(argc==2 && strcmp(argv[1],"--file-dispositions-stdin")==0) return FilePolicyCommand(1);
    if(argc==2 && strcmp(argv[1],"--copy-snapshot-stdin")==0) return SnapshotCommand();
    if(argc==3 && strcmp(argv[1],"--metadata")==0) return PackageMetadata(argv[2],NULL);
    if(argc==2 && strcmp(argv[1],"--metadata-stdin")==0) return PackageMetadata(NULL,NULL);
    if(argc==3 && strcmp(argv[1],"--write-metadata-stdin")==0) return PackageMetadata(NULL,argv[2]);
    if(argc>1 && strcmp(argv[1],"--check-selection")==0) return SelectionCommand(argc,argv);
    if(argc==2 && strcmp(argv[1],"--check-selection-stdin")==0) return NativeArgumentStream(SelectionCommand);
    if(argc>1 && strcmp(argv[1],"--resolve-providers")==0) return ProviderCommand(argc,argv);
    if(argc==2 && strcmp(argv[1],"--resolve-providers-stdin")==0) return ProviderStdinCommand();
    if(argc==6 && strcmp(argv[1],"--set-material")==0)
        return ManifestEditMaterial(argv[2],argv[3],argv[4],argv[5]);
    if(argc==3 && strcmp(argv[1],"--png")==0)return ValidatePng(argv[2]);
    if(argc>1 && strcmp(argv[1],"--car-transmission")==0)return CarTransmissionCommand(argc,argv);
    if(argc>1 && strcmp(argv[1],"--car-materials")==0)return CarMaterialsCommand(argc,argv);
    if(argc>1 && strcmp(argv[1],"--car-layout")==0)return CarLayoutCommand(argc,argv);
    if(argc>1 && strcmp(argv[1],"--rival-layout")==0)return RivalLayoutCommand(argc,argv);
    if(argc>1 && strcmp(argv[1],"--car-spec")==0)return CarSpecCommand(argc,argv);
    if(argc>1 && strcmp(argv[1],"--texture")==0)return TexturePreview(argc,argv);
    if(argc==3 && (strcmp(argv[1],"--mesh")==0 || strcmp(argv[1],"--preview")==0)) {
        RageRuntimeMesh mesh;
        f=fopen(argv[2],"rb");if(!f)return 1;
        if(fseek(f,0,SEEK_END)||(size=ftell(f))<=0||size>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return 1;}
        bytes=malloc((size_t)size);if(!bytes){fclose(f);return 1;}
        i=fread(bytes,1,(size_t)size,f);fclose(f);
        if(i!=(size_t)size||!RuntimeMeshOpen(&mesh,bytes,(size_t)size)||mesh.meshCount!=1){free(bytes);fprintf(stderr,"Invalid single-part RRMESH\n");return 1;}
        if(strcmp(argv[1],"--preview")==0) {
            if(mesh.vertexCount>100000 || mesh.indexCount>300000){free(bytes);fprintf(stderr,"Model exceeds preview limit (100,000 vertices / 100,000 triangles)\n");return 1;}
            printf("{\"vertices\":[");
            for(i=0;i<mesh.vertexCount;i++) {
                RageRuntimeVertex v;
                RuntimeMeshVertex(&mesh,(uint32_t)i,&v);
                printf("%s%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%u,%.9g,%.9g",i?",":"",
                       v.position[0],v.position[1],v.position[2],v.normal[0],v.normal[1],v.normal[2],v.material,v.uv[0],v.uv[1]);
            }
            printf("],\"colors\":[");
            for(i=0;i<mesh.vertexCount;i++) {
                RageRuntimeVertex v;
                RuntimeMeshVertex(&mesh,(uint32_t)i,&v);
                printf("%s%u,%u,%u,%u",i?",":"",v.color[0],v.color[1],v.color[2],v.color[3]);
            }
            printf("],\"indices\":[");
            for(i=0;i<mesh.indexCount;i++) {uint32_t index;RuntimeMeshIndex(&mesh,(uint32_t)i,&index);printf("%s%u",i?",":"",index);}
            puts("]}");
        }else {
            unsigned char used[65536]={0};
            int comma=0;
            for(i=0;i<mesh.indexCount;i++){
                uint32_t index;RageRuntimeVertex v;
                if(!RuntimeMeshIndex(&mesh,(uint32_t)i,&index)||!RuntimeMeshVertex(&mesh,index,&v)){
                    free(bytes);fprintf(stderr,"Invalid mesh vertex reference\n");return 1;
                }
                used[v.material&0xffff]=1;
            }
            printf("{\"vertices\":%u,\"indices\":%u,\"materials\":[",mesh.vertexCount,mesh.indexCount);
            for(i=0;i<sizeof(used);i++)if(used[i]){printf("%s%u",comma?",":"",(unsigned)i);comma=1;}
            puts("]}");
        }
        free(bytes);return 0;
    }
    if(argc!=2){fprintf(stderr,"usage: rage-mod-cli mod.toml | --mesh model.rmesh\n");return 1;}
    f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 1;}
    if(fseek(f,0,SEEK_END)||(size=ftell(f))<=0||size>2*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return 1;}
    bytes=malloc((size_t)size);if(!bytes){fclose(f);return 1;}
    if(fread(bytes,1,(size_t)size,f)!=(size_t)size){free(bytes);fclose(f);return 1;}fclose(f);
    if(!ModManifestParse(bytes,(size_t)size,&manifest)){fprintf(stderr,"Invalid mod manifest at line %zu\n",manifest.errorLine);free(bytes);return 1;}free(bytes);
    printf("{\"id\":");String(manifest.id);
    printf(",\"schemaVersion\":%u,\"requires\":[",manifest.schemaVersion);
    for(i=0;i<manifest.requirementCount;i++){if(i)putchar(',');String(manifest.requirements[i]);}
    printf("],\"textures\":{");
    for(i=0;i<manifest.textureCount;i++){if(i)putchar(',');String(manifest.textures[i].key);putchar(':');String(manifest.textures[i].path);}
    printf("},\"materials\":{");for(i=0;i<manifest.materialCount;i++){if(i)putchar(',');String(manifest.materials[i].key);putchar(':');String(manifest.materials[i].properties);}
    printf("},\"meshes\":{");for(i=0;i<manifest.meshCount;i++){if(i)putchar(',');String(manifest.meshes[i].key);putchar(':');String(manifest.meshes[i].path);}
    printf("},\"backingFiles\":[");
    for(i=0;i<manifest.textureCount;i++){if(i)putchar(',');String(manifest.textures[i].path);}
    for(i=0;i<manifest.meshCount;i++){if(i||manifest.textureCount)putchar(',');String(manifest.meshes[i].path);}
    printf("],\"resourceClaims\":[");
    char claim[RAGE_MOD_MANIFEST_KEY_CAPACITY + 16];
    int comma=0;
    for(i=0;i<manifest.textureCount;i++){
        if(comma)putchar(',');
        comma=1;
        snprintf(claim,sizeof(claim),"texture:%s",manifest.textures[i].key);String(claim);
    }
    for(i=0;i<manifest.materialCount;i++){
        if(comma)putchar(',');
        comma=1;
        snprintf(claim,sizeof(claim),"material:%s",manifest.materials[i].key);String(claim);
    }
    for(i=0;i<manifest.meshCount;i++){
        if(comma)putchar(',');
        comma=1;
        snprintf(claim,sizeof(claim),"mesh:%s",manifest.meshes[i].key);String(claim);
    }
    puts("]}");return 0;
}
