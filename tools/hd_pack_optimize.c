/* Lossless, exact-RGBA authoring tool. Runtime only links the decoder. */
#include "port/hd_pack.h"
#include "port/modern/modern_image.h"
#include "content/sha256.h"
#include "yyjson.h"
#include <webp/encode.h>
#include <webp/decode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Mapping {char old[256],path[100],digest[65];} Mapping;
static Mapping maps[1024];
static int path(char *out,size_t cap,const char *root,const char *file){return SDL_snprintf(out,cap,"%s/%s",root,file)<(int)cap;}
static int run(const char *source,const char *dest,const char *version,const char *archive){
    char file[1800],target[1800],error[256]={0};SDL_PathInfo info;
    RageModManifest *manifest=HdPackReadManifest(source);if(!manifest)return 0;
    if(SDL_GetPathInfo(dest,&info)||!SDL_CreateDirectory(dest))return 0;
    path(target,sizeof(target),dest,"textures");if(!SDL_CreateDirectory(target))return 0;
    unsigned count=0,unique=0,webps=0;uint64_t original=0,packed=0;
    for(size_t i=0;i<manifest->textureCount;i++){
        const char *name=manifest->textures[i].path;unsigned j;
        for(j=0;j<count;j++)if(!strcmp(name,maps[j].old))break;
        if(j<count)continue;if(count==SDL_arraysize(maps))return 0;
        Mapping *m=&maps[count];SDL_strlcpy(m->old,name,sizeof(m->old));
        if(!path(file,sizeof(file),source,name))return 0;
        size_t size=0;void *bytes=SDL_LoadFile(file,&size);if(!bytes)return 0;original+=size;
        SDL_Surface *image=ModernLoadImage(SDL_IOFromConstMem(bytes,size),4096);
        SDL_Surface *rgba=image?SDL_ConvertSurface(image,SDL_PIXELFORMAT_RGBA32):NULL;SDL_DestroySurface(image);if(!rgba)return 0;
        Sha256 sha;Sha256Init(&sha);Uint32 dimensions[2]={SDL_Swap32LE((Uint32)rgba->w),SDL_Swap32LE((Uint32)rgba->h)};
        Sha256Update(&sha,dimensions,sizeof(dimensions));
        for(int y=0;y<rgba->h;y++)Sha256Update(&sha,(Uint8*)rgba->pixels+y*rgba->pitch,(size_t)rgba->w*4);
        Sha256Finish(&sha,m->digest);
        for(j=0;j<count;j++)if(!strcmp(m->digest,maps[j].digest))break;
        if(j<count){SDL_strlcpy(m->path,maps[j].path,sizeof(m->path));}
        else {
            WebPConfig config;WebPPicture picture;WebPMemoryWriter writer;
            if(!WebPConfigInit(&config)||!WebPConfigLosslessPreset(&config,6)||!WebPPictureInit(&picture))return 0;
            config.exact=1;config.thread_level=1;
            picture.use_argb=1;picture.width=rgba->w;picture.height=rgba->h;
            WebPMemoryWriterInit(&writer);picture.writer=WebPMemoryWrite;picture.custom_ptr=&writer;
            int encoded=WebPPictureImportRGBA(&picture,rgba->pixels,rgba->pitch)&&WebPEncode(&config,&picture);
            WebPPictureFree(&picture);if(!encoded)return 0;
            /* Validate the complete output, including RGB below zero alpha. */
            SDL_Surface *decoded=ModernLoadImage(SDL_IOFromConstMem(writer.mem,writer.size),4096);
            if(!decoded||decoded->w!=rgba->w||decoded->h!=rgba->h)return 0;
            for(int y=0;y<rgba->h;y++)if(memcmp((Uint8*)decoded->pixels+y*decoded->pitch,(Uint8*)rgba->pixels+y*rgba->pitch,(size_t)rgba->w*4))return 0;
            SDL_DestroySurface(decoded);
            int useWebp=writer.size<size;
            snprintf(m->path,sizeof(m->path),"textures/%s.%s",m->digest,useWebp?"webp":!strcmp(name+strlen(name)-5,".webp")?"webp":"png");
            path(target,sizeof(target),dest,m->path);
            if(!SDL_SaveFile(target,useWebp?writer.mem:bytes,useWebp?writer.size:size))return 0;
            packed+=useWebp?writer.size:size;unique++;webps+=useWebp;
            WebPMemoryWriterClear(&writer);
        }
        SDL_DestroySurface(rgba);SDL_free(bytes);count++;
        fprintf(stderr,"\rOptimized %u textures (%u unique)",count,unique);fflush(stderr);
    }
    path(file,sizeof(file),source,"mod.toml");size_t length;char *text=SDL_LoadFile(file,&length);if(!text)return 0;
    path(target,sizeof(target),dest,"mod.toml");SDL_IOStream *out=SDL_IOFromFile(target,"wb");if(!out)return 0;
    /* Replace exact quoted paths only; keep material properties and comments. */
    for(size_t at=0;at<length;){unsigned j;
        for(j=0;j<count;j++){size_t n=strlen(maps[j].old);if(text[at]=='"'&&length-at>=n+2&&!memcmp(text+at+1,maps[j].old,n)&&text[at+n+1]=='"')break;}
        if(j<count){if(SDL_IOprintf(out,"\"%s\"",maps[j].path)<=0)return 0;at+=strlen(maps[j].old)+2;}
        else{if(SDL_WriteIO(out,text+at,1)!=1)return 0;at++;}
    }
    SDL_free(text);if(!SDL_CloseIO(out))return 0;
    path(file,sizeof(file),source,"pack.json");text=SDL_LoadFile(file,&length);if(!text)return 0;
    yyjson_doc *doc=yyjson_read(text,length,0);SDL_free(text);if(!doc)return 0;
    yyjson_mut_doc *mutable=yyjson_doc_mut_copy(doc,NULL);yyjson_doc_free(doc);if(!mutable)return 0;
    yyjson_mut_val *root=yyjson_mut_doc_get_root(mutable);
    yyjson_mut_obj_remove_key(root,"version");yyjson_mut_obj_add_strcpy(mutable,root,"version",version);
    yyjson_mut_obj_add_str(mutable,root,"images","exact-rgba-lossless");
    text=yyjson_mut_write(mutable,YYJSON_WRITE_PRETTY,&length);yyjson_mut_doc_free(mutable);
    path(target,sizeof(target),dest,"pack.json");if(!text||!SDL_SaveFile(target,text,length))return 0;free(text);
    if(!HdPackCreate(dest,archive,error,sizeof(error))){fprintf(stderr,"%s\n",error);return 0;}
    printf("\n{\"sourceTextures\":%u,\"uniqueTextures\":%u,\"webpEncoded\":%u,\"sourceBytes\":%llu,\"optimizedBytes\":%llu,\"exactRGBA\":true}\n",count,unique,webps,(unsigned long long)original,(unsigned long long)packed);
    free(manifest);return 1;
}
int main(int argc,char **argv){
    if(argc!=5){fprintf(stderr,"Usage: rage-hd-optimize SOURCE NEW_DIRECTORY VERSION NEW_ARCHIVE.rrhd\n");return 2;}
    return run(argv[1],argv[2],argv[3],argv[4])?0:1;
}
