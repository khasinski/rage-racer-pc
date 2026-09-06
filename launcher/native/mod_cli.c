#include "render/mod_manifest.h"
#include "render/rmesh.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "preview_png.h"
#include "texture_preview.h"
#include "car_spec.h"
#include "car_materials.h"
static RageModManifest manifest;
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
    printf("{\"id\":");String(manifest.id);printf(",\"textures\":{");
    for(i=0;i<manifest.textureCount;i++){if(i)putchar(',');String(manifest.textures[i].key);putchar(':');String(manifest.textures[i].path);}
    printf("},\"materials\":{");for(i=0;i<manifest.materialCount;i++){if(i)putchar(',');String(manifest.materials[i].key);putchar(':');String(manifest.materials[i].properties);}
    printf("},\"meshes\":{");for(i=0;i<manifest.meshCount;i++){if(i)putchar(',');String(manifest.meshes[i].key);putchar(':');String(manifest.meshes[i].path);}puts("}}");return 0;
}
