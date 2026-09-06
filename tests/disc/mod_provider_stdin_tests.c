#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define DUP _dup
#define DUP2 _dup2
#define FILENO _fileno
#define CLOSE _close
#else
#include <unistd.h>
#define DUP dup
#define DUP2 dup2
#define FILENO fileno
#define CLOSE close
#endif
#include "../../launcher/native/mod_provider_cli.h"

static int Request(const char *bytes, size_t size) {
    FILE *input = tmpfile();
    assert(input);
    assert(fwrite(bytes,1,size,input) == size);
    assert(!fflush(input)); rewind(input);
    int saved = DUP(FILENO(stdin)); assert(saved >= 0);
    assert(DUP2(FILENO(input),FILENO(stdin)) >= 0);
    clearerr(stdin);
    int result = ProviderStdinCommand();
    assert(DUP2(saved,FILENO(stdin)) >= 0);
    assert(CLOSE(saved) == 0); clearerr(stdin);
    assert(fclose(input) == 0);
    return result;
}
int main(void) {
    const char valid[] = "--resource\0texture:car.a\0--candidate\0a\0--candidate\0b\0"
        "--choice\0b\0--previous\0b\0--previous\0a\0";
    assert(Request(valid,sizeof(valid)-1) == 0);
    assert(Request(valid,sizeof(valid)-2) != 0); /* Missing final token terminator. */
    const char missing[] = "--resource\0texture:car.a\0--candidate\0a\0--candidate\0b\0";
    assert(Request(missing,sizeof(missing)-1) != 0);
    /* Ctrl-Z is data, not an end-of-file marker in Windows binary stdin. */
    const char binary[] = "--resource\0opaque:\x1a\0--candidate\0\xc5\xbc\0";
    assert(Request(binary,sizeof(binary)-1) == 0);
    assert(Request("",0) == 0);
    size_t size = 8u * 1024u * 1024u + 1;
    char *oversize = calloc(size,1); assert(oversize);
    assert(Request(oversize,size) != 0); free(oversize);
    puts("native provider stdin framing, Windows binary mode and byte limit passed");
    return 0;
}
