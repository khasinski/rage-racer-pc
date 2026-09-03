#include "disc_discovery.h"

#include <limits.h>
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
int DiscPathIsChd(const char *path) { return PathEndsWith(path, ".chd"); }
int DiscPathIsBin(const char *path) { return PathEndsWith(path, ".bin"); }

int DiscPathIsSupportedImage(const char *path) {
    return DiscPathIsCue(path) || DiscPathIsBin(path) || DiscPathIsChd(path);
}

int DiscReadSavedPath(const char *configPath, char *path, size_t pathSize) {
    char *lineEnd;
    FILE *file;

    if (configPath == NULL || path == NULL || pathSize < 2 ||
        pathSize > INT_MAX) {
        return 0;
    }
    file = fopen(configPath, "r");
    if (file == NULL || fgets(path, (int)pathSize, file) == NULL) {
        if (file != NULL) fclose(file);
        return 0;
    }
    lineEnd = strpbrk(path, "\r\n");
    if (lineEnd == NULL && !feof(file)) {
        fclose(file);
        path[0] = '\0';
        return 0;
    }
    fclose(file);
    if (lineEnd != NULL) *lineEnd = '\0';
    return path[0] != '\0';
}

static int BuildCandidate(char *path, size_t pathSize, const char *directory,
                          char separator, const char *name) {
    int written = snprintf(path, pathSize, "%s%c%s", directory, separator,
                           name);

    return written >= 0 && (size_t)written < pathSize;
}

int DiscDiscoverImage(const char *directory, char *path, size_t pathSize,
                      DiscImageValidator validate, void *context) {
    if (directory == NULL || path == NULL || pathSize == 0 ||
        validate == NULL) {
        return 0;
    }
    path[0] = '\0';
#ifdef _WIN32
    {
        size_t patternSize = strlen(directory) + sizeof("\\*.*");
        char *pattern = malloc(patternSize);
        WIN32_FIND_DATAA entry;
        HANDLE search;

        if (pattern == NULL) return 0;
        snprintf(pattern, patternSize, "%s\\*.*", directory);
        search = FindFirstFileA(pattern, &entry);
        free(pattern);
        if (search == INVALID_HANDLE_VALUE) return 0;
        do {
            if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ||
                !DiscPathIsSupportedImage(entry.cFileName) ||
                !BuildCandidate(path, pathSize, directory, '\\',
                                entry.cFileName)) {
                continue;
            }
            if (validate(context, path)) {
                FindClose(search);
                return 1;
            }
        } while (FindNextFileA(search, &entry));
        FindClose(search);
    }
#else
    {
        DIR *handle = opendir(directory);
        struct dirent *entry;

        if (handle == NULL) return 0;
        while ((entry = readdir(handle)) != NULL) {
            if (!DiscPathIsSupportedImage(entry->d_name) ||
                !BuildCandidate(path, pathSize, directory, '/',
                                entry->d_name)) {
                continue;
            }
            if (validate(context, path)) {
                closedir(handle);
                return 1;
            }
        }
        closedir(handle);
    }
#endif
    path[0] = '\0';
    return 0;
}
