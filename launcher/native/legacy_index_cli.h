#ifndef RAGE_LEGACY_INDEX_CLI_H
#define RAGE_LEGACY_INDEX_CLI_H
#include "render/legacy_texture_index.h"
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
