#ifndef RAGE_MOD_SELECTION_CLI_H
#define RAGE_MOD_SELECTION_CLI_H
#include "render/mod_selection.h"

typedef struct SelectionStorage {
    RageModDependency dependencies[RAGE_MOD_DEPENDENCY_LIMIT];
    char manifestId[RAGE_MOD_MANIFEST_ID_CAPACITY];
    char requirements[RAGE_MOD_MANIFEST_MAX_REQUIREMENTS][RAGE_MOD_MANIFEST_ID_CAPACITY];
} SelectionStorage;

/* Arguments are structured IPC, not shell code:
 * --mod PACKAGE VERSION REGION MANIFEST_PATH, followed by zero or more
 * --requires PACKAGE VERSION. Empty dependency version means any version.
 * Empty manifest path denotes a raw-only package. TOML is reread from disk. */
static int SelectionCommand(int argc, char **argv) {
    RageModSelectionEntry entries[RAGE_MOD_SELECTION_LIMIT] = {0};
    SelectionStorage *storage = calloc(RAGE_MOD_SELECTION_LIMIT,sizeof(*storage));
    RageModSelectionOrder order;
    size_t count = 0;
    int ok = 0;
    if (!storage) return 1;
    for (int arg = 2; arg < argc;) {
        if (!strcmp(argv[arg],"--mod") && arg + 4 < argc) {
            if (count == RAGE_MOD_SELECTION_LIMIT) goto done;
            RageModSelectionEntry *e = &entries[count];
            SelectionStorage *s = &storage[count++];
            e->packageId = argv[arg+1]; e->version = argv[arg+2];
            e->region = argv[arg+3]; e->dependencies = s->dependencies;
            const char *path = argv[arg+4];
            if (!*e->packageId || (strcmp(e->region,"PAL") &&
                strcmp(e->region,"NTSC-U") && strcmp(e->region,"NTSC-J"))) goto done;
            if (*path) {
                FILE *file = fopen(path,"rb");
                char *bytes = malloc(2*1024*1024+1);
                if (!file || !bytes) { if (file) fclose(file); free(bytes); goto done; }
                size_t size = fread(bytes,1,2*1024*1024+1,file);
                int readOk = !ferror(file);
                if (fclose(file)) readOk = 0;
                if (!readOk || size > 2*1024*1024 || !ModManifestParse(bytes,size,&manifest)) {
                    free(bytes); goto done;
                }
                free(bytes);
                strcpy(s->manifestId,manifest.id); e->manifestId = s->manifestId;
                for (size_t r = 0; r < manifest.requirementCount; ++r) {
                    strcpy(s->requirements[r],manifest.requirements[r]);
                    s->dependencies[e->dependencyCount++] = (RageModDependency){
                        RAGE_MOD_MANIFEST_ID,s->requirements[r],NULL};
                }
            }
            arg += 5;
        } else if (!strcmp(argv[arg],"--requires") && count && arg + 2 < argc) {
            RageModSelectionEntry *e = &entries[count-1];
            if (e->dependencyCount == RAGE_MOD_DEPENDENCY_LIMIT) goto done;
            storage[count-1].dependencies[e->dependencyCount++] = (RageModDependency){
                RAGE_MOD_PACKAGE_ID,argv[arg+1],*argv[arg+2] ? argv[arg+2] : NULL};
            arg += 3;
        } else goto done;
    }
    if (!ModSelectionBuildOrder(entries,count,&order)) {
        if (order.modIndex < count && order.dependencyIndex < entries[order.modIndex].dependencyCount) {
            const RageModDependency *d = &entries[order.modIndex].dependencies[order.dependencyIndex];
            fprintf(stderr,"%s requires %s%s%s: %s\n",entries[order.modIndex].packageId,
                d->id,d->version ? " version " : "",d->version ? d->version : "",order.error);
        } else fprintf(stderr,"%s\n",order.error);
        goto done;
    }
    putchar('[');
    for (size_t i = 0; i < order.count; ++i) printf("%s%zu",i ? "," : "",order.indices[i]);
    puts("]"); ok = 1;
done:
    free(storage);
    if (!ok) fputs("Invalid active mod selection\n",stderr);
    return ok ? 0 : 1;
}
#endif
