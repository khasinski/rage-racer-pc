#include "content/sha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void Check(const char *text,const char *expected){
    Sha256 hash;char hex[65];size_t i;
    Sha256Init(&hash);Sha256Update(&hash,text,strlen(text));Sha256Finish(&hash,hex);
    assert(!strcmp(hex,expected));
    Sha256Init(&hash);
    for(i=0;i<strlen(text);i++)Sha256Update(&hash,text+i,1);
    Sha256Finish(&hash,hex);assert(!strcmp(hex,expected));
}
int main(void){
    Sha256 hash;char hex[65],unchanged[65];unsigned i;
    Check("","e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    Check("abc","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    Check("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq","248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    Sha256Init(&hash);for(i=0;i<1000000;i++)Sha256Update(&hash,"a",1);
    Sha256Finish(&hash,hex);assert(!strcmp(hex,"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
    {
        const char *path="sha256_fixture.tmp";FILE *file=fopen(path,"wbx");
        assert(file);assert(fwrite("abc",1,3,file)==3);assert(!fclose(file));
        assert(Sha256File(path,3,hex));assert(!strcmp(hex,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        memcpy(unchanged,hex,sizeof(hex));assert(!Sha256File(path,2,hex));assert(!memcmp(hex,unchanged,sizeof(hex)));
        assert(!remove(path));assert(!Sha256File(path,3,hex));assert(!memcmp(hex,unchanged,sizeof(hex)));
    }
    return 0;
}
