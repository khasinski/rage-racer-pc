#include "../../launcher/native/mod_profile_cli.h"
#include <assert.h>

int main(void) {
    const char *output="mod_profile_contract.tmp";
    const char *valid="{\"textures\":{},\"materials\":{},\"meshes\":{}}";
    const char *invalid[]={"null","{}",
        "{\"textures\":[],\"materials\":{},\"meshes\":{}}",
        "{\"textures\":{\"car.a\":\"../outside.png\"},\"materials\":{},\"meshes\":{}}",
        "{\"textures\":{},\"materials\":{\"car.a\":\"invalid\"},\"meshes\":{}}"};
    for(size_t i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++){
        assert(ProfileWriteJSON(invalid[i],strlen(invalid[i]),output)==1);
        FILE *file=fopen(output,"rb");assert(!file);
    }
    assert(ProfileWriteJSON(NULL,0,output)==1);
    assert(ProfileWriteJSON(valid,strlen(valid),NULL)==1);
    assert(ProfileWriteJSON(valid,2*1024*1024+1u,output)==1);
    assert(ProfileWriteJSON(valid,strlen(valid),output)==0);
    assert(ProfileWriteJSON(valid,strlen(valid),output)==1);
    char bytes[256];FILE *file=fopen(output,"rb");assert(file);
    size_t size=fread(bytes,1,sizeof(bytes),file);assert(fclose(file)==0);
    RageModManifest *parsed=malloc(sizeof(*parsed));assert(parsed);
    assert(ModManifestParse(bytes,size,parsed));
    assert(!strcmp(parsed->id,"launcher-profile"));free(parsed);
    assert(remove(output)==0);
    return 0;
}
