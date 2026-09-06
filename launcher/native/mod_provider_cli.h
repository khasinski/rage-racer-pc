#ifndef RAGE_MOD_PROVIDER_CLI_H
#define RAGE_MOD_PROVIDER_CLI_H
#include "render/mod_provider.h"
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

/* Structured argv IPC: a batch of --resource KEY groups containing
 * --candidate ID, optional --choice ID and --previous ID entries. Output
 * is one selected candidate index per group; nonzero exit rejects the batch. */
static int ProviderCommand(int argc, char **argv) {
    int arg = 2, comma = 0;
    putchar('[');
    while (arg < argc) {
        const char *candidates[RAGE_MOD_PROVIDER_LIMIT], *previous[RAGE_MOD_PROVIDER_LIMIT];
        const char *key, *winner = NULL;
        size_t count = 0, previousCount = 0, selected;
        if (strcmp(argv[arg],"--resource") || arg + 1 >= argc) goto invalid;
        key = argv[arg+1]; arg += 2;
        if (!*key) goto invalid;
        while (arg < argc && strcmp(argv[arg],"--resource")) {
            if (arg + 1 >= argc) goto invalid;
            if (!strcmp(argv[arg],"--candidate") && count < RAGE_MOD_PROVIDER_LIMIT)
                candidates[count++] = argv[arg+1];
            else if (!strcmp(argv[arg],"--previous") && previousCount < RAGE_MOD_PROVIDER_LIMIT)
                previous[previousCount++] = argv[arg+1];
            else if (!strcmp(argv[arg],"--choice") && !winner) winner = argv[arg+1];
            else goto invalid;
            arg += 2;
        }
        RageModProviderResult result = ModProviderResolve(candidates,count,winner,
            previous,previousCount,&selected);
        if (result != RAGE_MOD_PROVIDER_OK) {
            fprintf(stderr,"Mod resource conflict: %s (%s); choose a provider again\n",key,
                result == RAGE_MOD_PROVIDER_STALE ? "provider set changed" :
                result == RAGE_MOD_PROVIDER_UNRESOLVED ? "no choice" : "invalid selection");
            return 1;
        }
        printf("%s%zu",comma ? "," : "",selected); comma = 1;
    }
    puts("]"); return 0;
invalid:
    fputs("Invalid resource conflict selection\n",stderr); return 1;
}

/* NUL-delimited UTF-8 argument tokens avoid platform command-line limits for
 * large texture packs. The batch is bounded before constructing pointers. */
static int ProviderStdinCommand(void) {
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
    int result = ProviderCommand((int)index,args);
    free(args); free(bytes); return result;
invalid:
    free(bytes); fputs("Invalid resource conflict input\n",stderr); return 1;
}
#endif
