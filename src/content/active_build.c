#include "active_build.h"
#include "sha256.h"
#include "port/car_catalog.h"
#include "yyjson.h"
#include "render/mod_file_policy.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int Reject(char *error,size_t size,const char *reason){
    if(error&&size)snprintf(error,size,"%s",reason);
    return -1;
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
/* A sorted inventory bounds both parsing and duplicate detection. The pointer
 * commits its bytes; each entry commits a runtime-supported payload file. */
static int Payload(const char *directory,const char *expected){
    enum { LIMIT=8*1024*1024 };
    char path[1100],hex[65];char *bytes=NULL;FILE *file=NULL;yyjson_doc *doc=NULL;
    int ok=0,failed;size_t size,i,count;const char *previous=NULL;yyjson_val *entry;
    if(!Hex(expected,64)||!Join(path,sizeof(path),directory,"files.json"))return 0;
    file=fopen(path,"rb");if(!file)return 0;
    bytes=malloc(LIMIT+1u);if(!bytes){fclose(file);return 0;}
    size=fread(bytes,1,LIMIT+1u,file);failed=ferror(file);if(fclose(file))failed=1;
    if(failed||size>LIMIT)goto done;
    Sha256 hash;Sha256Init(&hash);Sha256Update(&hash,bytes,size);Sha256Finish(&hash,hex);
    if(strcmp(hex,expected))goto done;
    doc=yyjson_read(bytes,size,0);if(!doc)goto done;
    yyjson_val *root=yyjson_doc_get_root(doc);
    if(!yyjson_is_arr(root)||yyjson_arr_size(root)>10000)goto done;
    yyjson_arr_foreach(root,i,count,entry){
        if(!yyjson_is_arr(entry)||yyjson_arr_size(entry)!=2)goto done;
        const char *name=String(yyjson_arr_get(entry,0)),*digest=String(yyjson_arr_get(entry,1));
        if(!name||!Hex(digest,64)||ModFileClassify(name)==RAGE_MOD_FILE_INVALID||
           !strcmp(name,"mod.toml")||(previous&&strcmp(previous,name)>=0)||
           !Join(path,sizeof(path),directory,name)||
           !Sha256File(path,128u*1024u*1024u,hex)||strcmp(hex,digest))goto done;
        previous=name;
    }
    ok=1;
done:
    yyjson_doc_free(doc);free(bytes);return ok;
}
int ActiveBuildRead(const char *root,const char *pointerPath,const char *region,
                    const char *source,ActiveBuild *build,char *error,size_t errorSize){
    char path[1100],bytes[8193],hex[65],suffix[128];size_t size;int failed;
    FILE *file;yyjson_doc *doc;yyjson_val *pointer,*identity;
    const char *id,*storedRegion,*fingerprint,*profileHash,*manifestHash;
    ActiveBuild next;
    RageCarCatalog catalog;char profileError[160];
    if(!root||!build||!pointerPath)return Reject(error,errorSize,"invalid active build path");
    file=fopen(pointerPath,"rb");
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
    yyjson_val *format=yyjson_obj_get(pointer,"format");
    if(!Unique(pointer)||!Unique(identity)||!yyjson_is_uint(format)||
       (yyjson_get_uint(format)!=1&&yyjson_get_uint(format)!=2)||!Uuid(id)||
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
    if(yyjson_get_uint(format)==2&&!Payload(next.directory,String(yyjson_obj_get(pointer,"payloadHash")))){
        yyjson_doc_free(doc);return Reject(error,errorSize,"active build payload is missing or damaged");
    }
    yyjson_doc_free(doc);*build=next;return 1;
}
int ActiveBuildOpen(const char *root,const char *region,const char *source,
                    ActiveBuild *build,char *error,size_t errorSize){
    char path[1100];
    if(!root||!Join(path,sizeof(path),root,"active.json"))return Reject(error,errorSize,"active build path is too long");
    return ActiveBuildRead(root,path,region,source,build,error,errorSize);
}
