#include "disc_discovery.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int _stricmp(const char *lhs, const char *rhs);
#define strcasecmp _stricmp
#else
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#endif

static int PathEndsWith(const char *path, const char *suffix) {
    size_t length;
    size_t suffixLength;

    if (path == NULL || suffix == NULL) return 0;
    length = strlen(path);
    suffixLength = strlen(suffix);
    return length > suffixLength &&
           strcasecmp(path + length - suffixLength, suffix) == 0;
}

int DiscPathIsCue(const char *path) { return PathEndsWith(path, ".cue"); }
int DiscPathIsBin(const char *path) { return PathEndsWith(path, ".bin"); }
