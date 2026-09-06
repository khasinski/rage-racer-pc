#include "render/mod_file_policy.h"
#include <assert.h>
#include <stdio.h>
#include "render/legacy_texture_index.h"

int main(void) {
    char stem[256];int owner;
    const char *entry=" 134 nested/a.json\r\n";
    assert(LegacyTextureIndexLine(entry,strlen(entry),&owner,stem)==1);
    assert(owner==134&&!strcmp(stem,"nested/a.json"));
    assert(LegacyTextureIndexLine(" # comment",10,&owner,stem)==0);
    const char *badIndex[]={"135 a.json","-1 a.json","1 ../a.json","1 /a.json",
        "1 a.json extra","1 a.png","1 a//b.json","9999999999999 a.json","1 a\\b.json"};
    for(size_t i=0;i<sizeof(badIndex)/sizeof(badIndex[0]);++i)
        assert(LegacyTextureIndexLine(badIndex[i],strlen(badIndex[i]),&owner,stem)==-1);
    const char nulLine[]="1 a.json\0extra";
    assert(LegacyTextureIndexLine(nulLine,sizeof(nulLine)-1,&owner,stem)==-1);
    assert(!ModDirectoryAllowed(NULL));
    const char *badDirs[]={"","other","textures2","textures/..","textures//a",
        "textures/a/","/textures","textures\\a","meshes/a b","raw/a/b/c/d/e/f/g/h"};
    for(size_t i=0;i<sizeof(badDirs)/sizeof(badDirs[0]);++i)assert(!ModDirectoryAllowed(badDirs[i]));
    assert(ModDirectoryAllowed("raw"));assert(ModDirectoryAllowed("textures"));
    assert(ModDirectoryAllowed("meshes/A_1-2"));
    assert(ModDirectoryAllowed("textures/a/b/c/d/e/f/g"));
    assert(ModFileClassify(NULL)==RAGE_MOD_FILE_INVALID);
    char raw[32];
    for (unsigned i=0;i<1000;++i) {
        snprintf(raw,sizeof(raw),"raw/asset_%03u.bin",i);
        assert(ModFileClassify(raw)==(i<135?RAGE_MOD_FILE_RAW:RAGE_MOD_FILE_INVALID));
    }
    const char *bad[]={"","/textures/a.png","textures/../a.png","textures//a.png",
        "textures/./a.png","textures/a.png/","textures\\a.png","textures/C:a.png",
        "textures/a.exe","textures/a.PNG","textures/a b.png","meshes/a.obj",
        "raw/asset_1.bin","raw/asset_0000.bin","raw/nested/asset_000.bin",
        "run.js","mod.toml.exe","textures/a\n.png"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i)
        assert(ModFileClassify(bad[i])==RAGE_MOD_FILE_INVALID);
    assert(ModFileClassify("textures/folder/A_1-2.png")==RAGE_MOD_FILE_TEXTURE);
    assert(ModFileClassify("textures/index.txt")==RAGE_MOD_FILE_TEXTURE);
    assert(ModFileClassify("textures/asset.json")==RAGE_MOD_FILE_TEXTURE);
    assert(ModFileClassify("meshes/body.rmesh")==RAGE_MOD_FILE_MESH);
    assert(ModFileClassify("mod.toml")==RAGE_MOD_FILE_METADATA);
    assert(ModFileClassify("manifest.json")==RAGE_MOD_FILE_METADATA);
    assert(ModFileClassify("rage-mod.json")==RAGE_MOD_FILE_METADATA);
    assert(ModFileDisposition("raw/asset_010.bin",0)==RAGE_MOD_FILE_GLOBAL);
    assert(ModFileDisposition("textures/a.png",0)==RAGE_MOD_FILE_GLOBAL);
    assert(ModFileDisposition("textures/a.png",RAGE_MOD_FILE_SEMANTIC)==RAGE_MOD_FILE_SEMANTIC);
    assert(ModFileDisposition("textures/a.png",RAGE_MOD_FILE_LEGACY)==RAGE_MOD_FILE_LEGACY);
    assert(ModFileDisposition("textures/a.png",6)==6);
    assert(ModFileDisposition("meshes/a.rmesh",RAGE_MOD_FILE_SEMANTIC)==RAGE_MOD_FILE_SEMANTIC);
    assert(ModFileDisposition("meshes/a.rmesh",0)==0);
    assert(ModFileDisposition("rage-mod.json",0)==0);
    assert(ModFileDisposition("mod.toml",0)==0);
    assert(ModFileDisposition("manifest.json",0)==0);
    assert(ModFileDisposition("raw/asset_000.bin",RAGE_MOD_FILE_SEMANTIC)==-1);
    assert(ModFileDisposition("meshes/a.rmesh",RAGE_MOD_FILE_LEGACY)==-1);
    assert(ModFileDisposition("textures/a.png",RAGE_MOD_FILE_GLOBAL)==-1);
    puts("compiled mod path classes, all raw indices and invalid paths passed");
    return 0;
}
