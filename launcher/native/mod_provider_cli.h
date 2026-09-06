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

#include "argument_stream.h"
static int ProviderStdinCommand(void) {
    return NativeArgumentStream(ProviderCommand);
}
#endif
