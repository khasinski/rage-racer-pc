#ifndef RAGE_MOD_PACKAGE_CLI_H
#define RAGE_MOD_PACKAGE_CLI_H
#include "render/mod_package.h"
#include "render/mod_file_snapshot.h"
#include <stdio.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#endif
static FILE *PackageOpenMetadata(const char *path) {
#ifdef _WIN32
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (count <= 0) return NULL;
    wchar_t *wide = malloc((size_t)count * sizeof(*wide));
    if (!wide) return NULL;
    FILE *file = NULL;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count))
        file = _wfopen(wide, L"rb");
    free(wide);
    return file;
#else
    return fopen(path, "rb");
#endif
}
static int PackageMetadata(const char *path, const char *target) {
    char bytes[RAGE_MOD_PACKAGE_BYTES + 1];
    RageModPackage package;
    FILE *file = path ? PackageOpenMetadata(path) : stdin;
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
