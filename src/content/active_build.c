#include "active_build.h"
#include "sha256.h"
#include "port/car_catalog.h"
#include "yyjson.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int Reject(char *error,size_t size,const char *reason){
    if(error&&size)snprintf(error,size,"%s",reason);return -1;
}
static const char *String(yyjson_val *value){
    const char *s=yyjson_get_str(value);
    return s&&strlen(s)==yyjson_get_len(value)?s:NULL;
}
static int Hex(const char *s,size_t count){
    size_t i;if(!s||strlen(s)!=count)return 0;
    for(i=0;i<count;i++)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return 0;
    return 1;
}
static int Uuid(const char *s){
    size_t i;if(!s||strlen(s)!=36)return 0;
    for(i=0;i<36;i++){
        if(i==8||i==13||i==18||i==23){if(s[i]!='-')return 0;}
        else if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return 0;
    }
    return 1;
}
static int Unique(yyjson_val *object){
    const char *keys[16];size_t i,count,j;yyjson_val *key,*value;
    if(!yyjson_is_obj(object)||yyjson_obj_size(object)>16)return 0;
    yyjson_obj_foreach(object,i,count,key,value){
        (void)value;
        keys[i]=String(key);if(!keys[i])return 0;
        for(j=0;j<i;j++)if(!strcmp(keys[j],keys[i]))return 0;
    }
    return 1;
}
static int Join(char *out,size_t size,const char *root,const char *suffix){
    int n=snprintf(out,size,"%s/%s",root,suffix);return n>=0&&(size_t)n<size;
}
int ActiveBuildOpen(const char *root,const char *region,const char *source,
                    ActiveBuild *build,char *error,size_t errorSize){
    char path[1100],bytes[8193],hex[65],suffix[128];size_t size;int failed;
    FILE *file;yyjson_doc *doc;yyjson_val *pointer,*identity;
    const char *id,*storedRegion,*fingerprint,*profileHash,*manifestHash;
    ActiveBuild next;
    RageCarCatalog catalog;char profileError[160];
    if(!root||!build||!Join(path,sizeof(path),root,"active.json"))return Reject(error,errorSize,"active build path is too long");
    file=fopen(path,"rb");
    if(!file)return errno==ENOENT?0:Reject(error,errorSize,"cannot read active build pointer");
    size=fread(bytes,1,sizeof(bytes),file);failed=ferror(file);if(fclose(file))failed=1;
    if(failed||size>8192)return Reject(error,errorSize,"invalid active build pointer size");
    doc=yyjson_read(bytes,size,0);
    if(!doc)return Reject(error,errorSize,"invalid active build JSON");
    pointer=yyjson_doc_get_root(doc);identity=yyjson_obj_get(pointer,"source");
    id=String(yyjson_obj_get(pointer,"build"));
    storedRegion=String(yyjson_obj_get(identity,"region"));
    fingerprint=String(yyjson_obj_get(identity,"fingerprint"));
    profileHash=String(yyjson_obj_get(pointer,"profileHash"));
    manifestHash=String(yyjson_obj_get(pointer,"manifestHash"));
    if(!Unique(pointer)||!Unique(identity)||!yyjson_is_uint(yyjson_obj_get(pointer,"format"))||yyjson_get_uint(yyjson_obj_get(pointer,"format"))!=1||!Uuid(id)||
       !Hex(profileHash,64)||!Hex(manifestHash,64)||!storedRegion||!fingerprint){
        yyjson_doc_free(doc);return Reject(error,errorSize,"invalid active build schema");
    }
    if(!region||!source||strcmp(region,storedRegion)||strcmp(source,fingerprint)){
        yyjson_doc_free(doc);return Reject(error,errorSize,"active build belongs to a different disc revision");
    }
    snprintf(suffix,sizeof(suffix),"builds/%s",id);
    if(!Join(next.directory,sizeof(next.directory),root,suffix)||
       !Join(next.profile,sizeof(next.profile),next.directory,"cars.toml")||
       !Sha256File(next.profile,1024*1024,hex)||strcmp(hex,profileHash)){
        yyjson_doc_free(doc);return Reject(error,errorSize,"active car profile is missing or damaged");
    }
    if(!Join(path,sizeof(path),next.directory,"mod.toml")||
       !Sha256File(path,2*1024*1024,hex)||strcmp(hex,manifestHash)){
        yyjson_doc_free(doc);return Reject(error,errorSize,"active mod manifest is missing or damaged");
    }
    if(!CarCatalogReadFile(next.profile,&catalog,profileError,sizeof(profileError))){
        yyjson_doc_free(doc);return Reject(error,errorSize,"active car profile failed runtime validation");
    }
    yyjson_doc_free(doc);*build=next;return 1;
}
