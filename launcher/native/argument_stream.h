#ifndef RAGE_NATIVE_ARGUMENT_STREAM_H
#define RAGE_NATIVE_ARGUMENT_STREAM_H
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

/* NUL-delimited UTF-8 argument tokens avoid platform command-line limits for
 * large texture packs. The batch is bounded before constructing pointers. */
static int NativeArgumentStream(int (*command)(int, char **)) {
    enum { BYTE_LIMIT = 8 * 1024 * 1024, TOKEN_LIMIT = 262144 };
    char *bytes = malloc(BYTE_LIMIT + 1u);
    char **args;
    size_t size, tokens = 0;
    if (!bytes) return 1;
#ifdef _WIN32
    if (_setmode(_fileno(stdin),_O_BINARY) == -1) { free(bytes); return 1; }
#endif
    size = fread(bytes,1,BYTE_LIMIT + 1u,stdin);
    if (ferror(stdin) || size > BYTE_LIMIT || (size && bytes[size-1] != 0)) goto invalid;
    for (size_t i = 0; i < size; ++i) if (!bytes[i]) ++tokens;
    if (tokens > TOKEN_LIMIT) goto invalid;
    args = malloc((tokens + 2) * sizeof(*args));
    if (!args) { free(bytes); return 1; }
    args[0] = args[1] = NULL;
    size_t index = 2, start = 0;
    for (size_t i = 0; i < size; ++i) if (!bytes[i]) {
        args[index++] = bytes + start; start = i + 1;
    }
    int result = command((int)index,args);
    free(args); free(bytes); return result;
invalid:
    free(bytes); fputs("Invalid native argument stream\n",stderr); return 1;
}
#endif
