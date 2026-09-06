#include "../../launcher/native/legacy_index_cli.h"
#include <assert.h>

static int Write(const char *json, const char *output) {
    const char *input="mod_legacy_writer_input.tmp";
    FILE *file=fopen(input,"wbx");assert(file);
    size_t size=strlen(json);
    assert(fwrite(json,1,size,file)==size);assert(fclose(file)==0);
    assert(freopen(input,"rb",stdin));
    int result=LegacyIndexWriteCommand(output);
#ifdef _WIN32
    assert(freopen("NUL","rb",stdin));
#else
    assert(freopen("/dev/null","rb",stdin));
#endif
    assert(remove(input)==0);
    return result;
}

int main(void) {
    const char *output="mod_legacy_writer_output.tmp";
    const char *bad[]={"null","{}","[[135,\"a.json\"]]",
        "[[0,\"a.json\"],[1,\"../bad.json\"]]",
        "[[0,\"a.json\\n1 b.json\"]]","[[0,\"a.json\\u0000\"]]"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
        assert(Write(bad[i],output)==1);
        FILE *file=fopen(output,"rb");assert(!file);
    }
    assert(Write("[[134,\"nested/a.json\"],[0,\"b.json\"],[134,\"c.json\"]]",output)==0);
    assert(Write("[]",output)==1);
    const char *expected="134 nested/a.json\n0 b.json\n134 c.json\n";
    char bytes[128]={0};FILE *file=fopen(output,"rb");assert(file);
    assert(fread(bytes,1,sizeof(bytes)-1,file)==strlen(expected));
    assert(fclose(file)==0);assert(!strcmp(bytes,expected));
    assert(LegacyIndexCommand(output)==0);
    assert(remove(output)==0);
    return 0;
}
