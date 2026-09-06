#ifndef RAGE_MOD_PACKAGE_CLI_H
#define RAGE_MOD_PACKAGE_CLI_H
#include "render/mod_package.h"
#include "render/mod_file_snapshot.h"
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
static int PackageMetadata(const char *path, const char *target) {
    char bytes[RAGE_MOD_PACKAGE_BYTES + 1];
    RageModPackage package;
    FILE *file = path ? fopen(path,"rb") : stdin;
    if (!file) { perror(path); return 1; }
#ifdef _WIN32
    if (!path && _setmode(_fileno(stdin),_O_BINARY) == -1) return 1;
#endif
    size_t size = fread(bytes,1,sizeof(bytes),file);
    int ok = !ferror(file);
    if (path && fclose(file)) ok = 0;
    if (!ok || !ModPackageParseJSON(bytes,size,&package)) {
        fputs("Invalid mod metadata\n",stderr); return 1;
    }
    if (target) {
        if (ModFileWriteExclusive(target, bytes, size)) return 0;
        fputs("Cannot write mod metadata\n", stderr);
        return 1;
    }
    /* Return the validated JSON without losing extension fields or changing
     * Unicode spelling. The launcher only decodes this IPC response. */
    return fwrite(bytes,1,size,stdout) == size && !ferror(stdout) ? 0 : 1;
}
#endif
