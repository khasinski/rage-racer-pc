#ifndef RAGE_LEGACY_INDEX_CLI_H
#define RAGE_LEGACY_INDEX_CLI_H
#include "render/legacy_texture_index.h"
#include "render/mod_file_snapshot.h"
#include "yyjson.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
static int LegacyIndexWriteCommand(const char *output) {
    enum { LIMIT=2*1024*1024 };
    char *input=malloc(LIMIT+1u),*text=malloc(LIMIT+1u);
    yyjson_doc *doc=NULL;int ok=0;
    if(!input||!text)goto done;
#ifdef _WIN32
    if(_setmode(_fileno(stdin),_O_BINARY)==-1)goto done;
#endif
    size_t size=fread(input,1,LIMIT+1u,stdin),used=0;
    if(size>LIMIT||ferror(stdin))goto done;
    doc=yyjson_read(input,size,0);if(!doc)goto done;
    yyjson_val *root=yyjson_doc_get_root(doc),*pair;
    if(!yyjson_is_arr(root)||yyjson_arr_size(root)>10000)goto done;
    size_t i,count;
    yyjson_arr_foreach(root,i,count,pair){
        if(!yyjson_is_arr(pair)||yyjson_arr_size(pair)!=2)goto done;
        yyjson_val *asset=yyjson_arr_get(pair,0),*path=yyjson_arr_get(pair,1);
        if(!yyjson_is_uint(asset)||yyjson_get_uint(asset)>=135||!yyjson_is_str(path))goto done;
        const char *name=yyjson_get_str(path);
        if(strlen(name)!=yyjson_get_len(path))goto done;
        char line[512],parsedPath[256];int owner;
        int n=snprintf(line,sizeof(line),"%u %s",(unsigned)yyjson_get_uint(asset),name);
        if(n<0||(size_t)n>=sizeof(line)||
           LegacyTextureIndexLine(line,(size_t)n,&owner,parsedPath)!=1||
           strcmp(name,parsedPath)||used+(size_t)n+1>LIMIT)goto done;
        memcpy(text+used,line,(size_t)n);used+=(size_t)n;text[used++]='\n';
    }
    ok=ModFileWriteExclusive(output,text,used);
done:
    yyjson_doc_free(doc);free(input);free(text);
    if(!ok)fputs("Invalid legacy texture index or output failure\n",stderr);
    return ok?0:1;
}
static int LegacyIndexCommand(const char *path) {
    FILE *file=fopen(path,"rb");
    if(!file)return 1;
    char *bytes=malloc(2*1024*1024+1u);
    if(!bytes){fclose(file);return 1;}
    size_t size=fread(bytes,1,2*1024*1024+1u,file);
    int ok=!ferror(file)&&size<=2*1024*1024;
    if(fclose(file))ok=0;
    if(!ok)goto done;
    size_t start=0;int comma=0;
    putchar('[');
    for(size_t i=0;i<=size;++i)if(i==size||bytes[i]=='\n') {
        int owner;char stem[256];
        int parsed=LegacyTextureIndexLine(bytes+start,i-start,&owner,stem);
        start=i+1;
        if(parsed<0){ok=0;goto done;}
        if(parsed) {
            printf("%s{\"asset\":%d,\"json\":\"textures/%s\",\"png\":\"textures/%.*s.png\",\"resourceClaim\":\"legacy-textures:asset-%d\"}",
                   comma?",":"",owner,stem,(int)strlen(stem)-5,stem,owner);
            comma=1;
        }
    }
    puts("]");
done:
    free(bytes);
    if(!ok)fputs("Invalid legacy texture index entry\n",stderr);
    return ok?0:1;
}
#endif
