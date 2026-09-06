#include "render/mod_package.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    RageModPackage out, zero = {0};
    const char *valid = "{\"format\":1,\"name\":\"Za\\u017c\\u00f3\\u0142\\u0107 \\ud83d\\ude97\","
        "\"region\":\"NTSC-J\",\"packageId\":\"author.mod\",\"requires\":[{\"packageId\":\"base\",\"version\":\"1.0\"}]}";
    assert(ModPackageParseJSON(valid,strlen(valid),&out));
    assert(!strcmp(out.name,"Za\xc5\xbc\xc3\xb3\xc5\x82\xc4\x87 \xf0\x9f\x9a\x97"));
    assert(out.hasPackageId && !strcmp(out.region,"NTSC-J"));
    assert(out.requirementCount == 1 && out.requires[0].hasVersion);
    assert(!strcmp(out.requires[0].version,"1.0"));
    const char *bad[] = {
        "{}", "[]", "null", "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\",}",
        "{\"format\":2,\"name\":\"x\",\"region\":\"PAL\"}",
        "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\",\"na\\u006de\":\"other\"}",
        "{\"format\":1,\"name\":\"\\u2003\",\"region\":\"PAL\"}",
        "{\"format\":1,\"name\":\"\\ud800\",\"region\":\"PAL\"}",
        "{\"format\":1,\"name\":\"\\u0000\",\"region\":\"PAL\"}",
        "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\",\"requires\":[{}]}",
        "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\",\"requires\":[{\"packageId\":\"a\",\"other\":1}]}",
        "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\",\"requires\":[{\"packageId\":\"a\"},{\"packageId\":\"a\"}]}",
        "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\",\"requires\":[{\"packageId\":\"a\",\"version\":\" \"}]}",
        "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\",\"requires\":[{\"packageId\":\"a\"}],\"packageId\":\"a\"}",
        "{\"format\":1,\"name\":\"x\",\"region\":\"PAL\"} garbage"
    };
    for (size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        memset(&out,0xa5,sizeof(out));
        assert(!ModPackageParseJSON(bad[i],strlen(bad[i]),&out));
        assert(!memcmp(&out,&zero,sizeof(out)));
    }
    assert(!ModPackageParseJSON(NULL,0,&out));
    assert(!ModPackageParseJSON(valid,strlen(valid),NULL));
    assert(!ModPackageParseJSON(valid,RAGE_MOD_PACKAGE_BYTES+1,&out));
    puts("compiled package metadata validation and owned Unicode fields passed");
    return 0;
}
