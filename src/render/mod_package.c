#include "mod_package.h"
#include "yyjson.h"
#include <string.h>

static int Space(unsigned c) {
    return (c >= 9 && c <= 13) || c == 32 || c == 0xa0 || c == 0x1680 ||
        (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 ||
        c == 0x202f || c == 0x205f || c == 0x3000 || c == 0xfeff;
}
/* yyjson has already validated Unicode and decoded escapes. */
static int Text(yyjson_val *value, char *out, size_t limit, int multiline, int nonblank) {
    if (!yyjson_is_str(value)) return 0;
    const unsigned char *s = (const unsigned char *)yyjson_get_str(value);
    size_t size = yyjson_get_len(value), units = 0;
    int content = 0;
    for (size_t i = 0; i < size;) {
        unsigned c = s[i++];
        if (c >= 0xc0) {
            unsigned more = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
            c &= more == 1 ? 31u : more == 2 ? 15u : 7u;
            while (more--) c = (c << 6) | (s[i++] & 63u);
        }
        units += c >= 0x10000 ? 2 : 1;
        if (units > limit || c == 127 || (c < 32 &&
            !(multiline && (c == 9 || c == 10 || c == 13)))) return 0;
        if (!Space(c)) content = 1;
    }
    if (nonblank && !content) return 0;
    memcpy(out,s,size); out[size] = 0;
    return 1;
}
static int Identifier(yyjson_val *value, char out[129]) {
    if (!yyjson_is_str(value)) return 0;
    const char *s = yyjson_get_str(value);
    size_t n = yyjson_get_len(value);
    if (!n || n > 128) return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || (i && (c == '.' || c == '_' || c == '-')))) return 0;
    }
    memcpy(out,s,n); out[n] = 0; return 1;
}
static int Requirements(yyjson_val *value, RageModPackage *out) {
    if (yyjson_is_null(value)) return 1; /* Legacy null means omitted. */
    if (!yyjson_is_arr(value) || yyjson_arr_size(value) > RAGE_MOD_PACKAGE_DEPENDENCIES) return 0;
    size_t i, count; yyjson_val *dep;
    yyjson_arr_foreach(value,i,count,dep) {
        if (!yyjson_is_obj(dep)) return 0;
        RageModPackageDependency *d = &out->requires[out->requirementCount];
        size_t j, n; yyjson_val *key, *v; unsigned seen = 0;
        yyjson_obj_foreach(dep,j,n,key,v) {
            if (yyjson_equals_str(key,"packageId")) {
                if ((seen & 1) || !Identifier(v,d->packageId)) return 0;
                seen |= 1;
            } else if (yyjson_equals_str(key,"version")) {
                if ((seen & 2) || !Text(v,d->version,80,0,1)) return 0;
                seen |= 2; d->hasVersion = 1;
            } else return 0;
        }
        if (!(seen & 1)) return 0;
        for (size_t j = 0; j < out->requirementCount; ++j)
            if (!strcmp(d->packageId,out->requires[j].packageId)) return 0;
        ++out->requirementCount;
    }
    return 1;
}
int ModPackageParseJSON(const char *bytes, size_t size, RageModPackage *out) {
    static const char *fields[] = {"format","name","region","packageId","requires","author","version","description"};
    yyjson_doc *doc = NULL;
    int ok = 0; unsigned seen = 0;
    if (!out) return 0;
    memset(out,0,sizeof(*out));
    if (!bytes || !size || size > RAGE_MOD_PACKAGE_BYTES) return 0;
    doc = yyjson_read(bytes,size,0);
    if (!doc) return 0;
    yyjson_val *root = yyjson_doc_get_root(doc), *key, *value;
    if (!yyjson_is_obj(root)) goto done;
    size_t i, count;
    yyjson_obj_foreach(root,i,count,key,value) {
        unsigned field;
        for (field = 0; field < 8; ++field)
            if (yyjson_equals_str(key,fields[field])) break;
        if (field == 8) continue;
        if (seen & (1u << field)) goto done;
        seen |= 1u << field;
        switch (field) {
        case 0: if (!yyjson_is_num(value) || yyjson_get_num(value) != 1) goto done; break;
        case 1: if (!Text(value,out->name,200,0,1)) goto done; break;
        case 2:
            if (!Text(value,out->region,7,0,1) || (strcmp(out->region,"PAL") &&
                strcmp(out->region,"NTSC-U") && strcmp(out->region,"NTSC-J"))) goto done;
            break;
        case 3: if (!Identifier(value,out->packageId)) goto done; out->hasPackageId=1; break;
        case 4: if (!Requirements(value,out)) goto done; break;
        case 5: if (!Text(value,out->author,120,0,0)) goto done; out->hasAuthor=1; break;
        case 6: if (!Text(value,out->version,80,0,0)) goto done; out->hasVersion=1; break;
        case 7: if (!Text(value,out->description,1200,1,0)) goto done; out->hasDescription=1; break;
        }
    }
    if ((seen & 7) != 7) goto done;
    if (out->hasPackageId) for (size_t i = 0; i < out->requirementCount; ++i)
        if (!strcmp(out->packageId,out->requires[i].packageId)) goto done;
    ok = 1;
done:
    yyjson_doc_free(doc);
    if (!ok) memset(out,0,sizeof(*out));
    return ok;
}
