#include "mod_file_policy.h"
#include "asset_path.h"
#include <string.h>

static int Ends(const char *path, size_t size, const char *suffix) {
    size_t n = strlen(suffix);
    return size > n && !memcmp(path+size-n,suffix,n);
}
RageModFileKind ModFileClassify(const char *path) {
    if (!path) return RAGE_MOD_FILE_INVALID;
    size_t size = strlen(path);
    if (!AssetPathIsRelativeFile(path,size)) return RAGE_MOD_FILE_INVALID;
    if (!strcmp(path,"mod.toml") || !strcmp(path,"manifest.json") ||
        !strcmp(path,"rage-mod.json")) return RAGE_MOD_FILE_METADATA;
    if (size == 17 && !memcmp(path,"raw/asset_",10) && !strcmp(path+13,".bin")) {
        unsigned index = 0;
        for (size_t i=10;i<13;++i) {
            if (path[i] < '0' || path[i] > '9') return RAGE_MOD_FILE_INVALID;
            index = index*10 + (unsigned)(path[i]-'0');
        }
        return index < 135 ? RAGE_MOD_FILE_RAW : RAGE_MOD_FILE_INVALID;
    }
    size_t prefix;
    RageModFileKind kind;
    if (!strncmp(path,"textures/",9)) { prefix=9; kind=RAGE_MOD_FILE_TEXTURE; }
    else if (!strncmp(path,"meshes/",7)) { prefix=7; kind=RAGE_MOD_FILE_MESH; }
    else return RAGE_MOD_FILE_INVALID;
    for (size_t i=prefix;i<size;++i) {
        unsigned char c=(unsigned char)path[i];
        if (!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||
            c=='_'||c=='.'||c=='/'||c=='-')) return RAGE_MOD_FILE_INVALID;
    }
    const char *relative=path+prefix; size_t n=size-prefix;
    if (kind == RAGE_MOD_FILE_MESH) return Ends(relative,n,".rmesh") ? kind : RAGE_MOD_FILE_INVALID;
    return Ends(relative,n,".png") || Ends(relative,n,".json") || Ends(relative,n,".txt")
        ? kind : RAGE_MOD_FILE_INVALID;
}
