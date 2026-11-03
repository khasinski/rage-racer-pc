#include "port/hd_pack.h"
#include "content/sha256.h"
#include <SDL3/SDL.h>
#include "yyjson.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void clean(const char *root);
static SDL_EnumerationResult removeEntry(void *context,const char *directory,const char *name){
    (void)context;char path[2000];snprintf(path,sizeof(path),"%s/%s",directory,name);
    SDL_PathInfo info;if(SDL_GetPathInfo(path,&info)&&info.type==SDL_PATHTYPE_DIRECTORY)clean(path);else SDL_RemovePath(path);return SDL_ENUM_CONTINUE;
}
static void clean(const char *root){SDL_EnumerateDirectory(root,removeEntry,NULL);SDL_RemovePath(root);}
int main(int argc,char **argv){
    if(argc!=4){fprintf(stderr,"Usage: rage-hd-publish PACK_DIRECTORY RELEASE_DIRECTORY PUBLIC_BASE_URL\n");return 2;}
    char metadata[1800],archive[1800],stage[1800],store[1800],catalog[1800],temp[1800],url[1200],error[256]={0};
    snprintf(metadata,sizeof(metadata),"%s/pack.json",argv[1]);size_t length;
    char *bytes=SDL_LoadFile(metadata,&length);yyjson_doc *doc=bytes?yyjson_read(bytes,length,0):NULL;SDL_free(bytes);if(!doc)return 1;
    yyjson_val *root=yyjson_doc_get_root(doc);const char *version=yyjson_get_str(yyjson_obj_get(root,"version")),*region=yyjson_get_str(yyjson_obj_get(root,"region"));
    if(!version||!*version||strlen(version)>64||!region||(strcmp(region,"PAL")&&strcmp(region,"NTSC-U")&&strcmp(region,"NTSC-J")))return 1;
    for(const char *p=version;*p;p++)if(!strchr("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._-",*p))return 1;
    if(strstr(version,"..")||!*argv[3]||strlen(argv[3])>800||strpbrk(argv[3],"\r\n\t ?#@\\")||argv[3][strlen(argv[3])-1]=='/')return 1;
    if(strncmp(argv[3],"https://",8)&&strncmp(argv[3],"http://127.0.0.1:",17))return 1;
    snprintf(archive,sizeof(archive),"%s/hd-%s-v%s.rrhd",argv[2],region,version);
    snprintf(stage,sizeof(stage),"%s/publish-%llu.partial",argv[2],(unsigned long long)SDL_GetPerformanceCounter());
    snprintf(store,sizeof(store),"%s/check-%llu.partial",argv[2],(unsigned long long)SDL_GetPerformanceCounter());
    snprintf(catalog,sizeof(catalog),"%s/catalog-%s.json",argv[2],region);snprintf(temp,sizeof(temp),"%s/catalog-%s.partial",argv[2],region);
    SDL_PathInfo info;if(SDL_GetPathInfo(archive,&info)){fprintf(stderr,"Immutable release already exists. Choose a new version.\n");return 1;}
    int ok=SDL_CreateDirectory(argv[2])&&HdPackCreate(argv[1],stage,error,sizeof(error))&&
        HdPackInstall(stage,store,region,NULL,NULL,error,sizeof(error));
    HdPackState state;if(ok)ok=HdPackReadState(store,region,&state)&&HdPackVerify(store,&state,NULL,NULL,error,sizeof(error));
    if(ok)ok=SDL_GetPathInfo(stage,&info);
    yyjson_mut_doc *out=yyjson_mut_doc_new(NULL);yyjson_mut_val *object=yyjson_mut_obj(out);yyjson_mut_doc_set_root(out,object);
    if(ok){
        snprintf(url,sizeof(url),"%s/hd-%s-v%s.rrhd",argv[3],region,version);
        yyjson_mut_obj_add_str(out,object,"version",version);yyjson_mut_obj_add_str(out,object,"region",region);
        yyjson_mut_obj_add_str(out,object,"format","rrhd-webp-v1");yyjson_mut_obj_add_str(out,object,"url",url);
        yyjson_mut_obj_add_str(out,object,"sha256",state.hash);yyjson_mut_obj_add_uint(out,object,"size",info.size);
        bytes=yyjson_mut_write(out,YYJSON_WRITE_PRETTY,&length);
        ok=bytes&&SDL_SaveFile(temp,bytes,length)&&SDL_RenamePath(stage,archive)&&SDL_RenamePath(temp,catalog);
        if(ok)printf("%s\n%s\n",archive,bytes);free(bytes);
    }
    if(!ok)fprintf(stderr,"Publish failed: %s\n",error);
    SDL_RemovePath(stage);SDL_RemovePath(temp);clean(store);yyjson_mut_doc_free(out);yyjson_doc_free(doc);return ok?0:1;
}
