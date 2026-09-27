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
int DiscPathIsChd(const char *path) { return PathEndsWith(path, ".chd"); }
int DiscPathIsBin(const char *path) { return PathEndsWith(path, ".bin"); }

int DiscPathIsSupportedImage(const char *path) {
    return DiscPathIsCue(path) || DiscPathIsBin(path) || DiscPathIsChd(path);
}

int DiscReadSavedPath(const char *configPath, char *path, size_t pathSize) {
    char *lineEnd;
    FILE *file;

    if (path == NULL || pathSize == 0) {
        return 0;
    }
    path[0] = '\0';
    if (pathSize < 2 || pathSize > INT_MAX || configPath == NULL) return 0;
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

int DiscWriteSavedPath(const char *configPath, const char *path) {
    char *temporaryPath;
    size_t temporarySize;
    FILE *file;
    int ok;

    if (configPath == NULL || configPath[0] == '\0' || path == NULL ||
        path[0] == '\0' || strpbrk(path, "\r\n") != NULL ||
        strlen(configPath) > SIZE_MAX - sizeof(".tmp")) {
        return 0;
    }
    temporarySize = strlen(configPath) + sizeof(".tmp");
    temporaryPath = malloc(temporarySize);
    if (temporaryPath == NULL) return 0;
    snprintf(temporaryPath, temporarySize, "%s.tmp", configPath);
    file = fopen(temporaryPath, "w");
    if (file == NULL) {
        free(temporaryPath);
        return 0;
    }
    ok = fputs(path, file) >= 0 && fputc('\n', file) != EOF;
    if (fclose(file) != 0) ok = 0;
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temporaryPath, configPath,
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
        ok = rename(temporaryPath, configPath) == 0;
#endif
    }
    if (!ok) remove(temporaryPath);
    free(temporaryPath);
    return ok;
}

int DiscPathsReferToSameFile(const char *first, const char *second) {
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION firstInfo;
    BY_HANDLE_FILE_INFORMATION secondInfo;
    HANDLE firstHandle;
    HANDLE secondHandle;
    int same;

    if (first == NULL || second == NULL) return 0;
    firstHandle = CreateFileA(first, FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE |
                                  FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    secondHandle = CreateFileA(second, FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE |
                                   FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (firstHandle == INVALID_HANDLE_VALUE ||
        secondHandle == INVALID_HANDLE_VALUE ||
        !GetFileInformationByHandle(firstHandle, &firstInfo) ||
        !GetFileInformationByHandle(secondHandle, &secondInfo)) {
        if (firstHandle != INVALID_HANDLE_VALUE) CloseHandle(firstHandle);
        if (secondHandle != INVALID_HANDLE_VALUE) CloseHandle(secondHandle);
        return 0;
    }
    same = firstInfo.dwVolumeSerialNumber == secondInfo.dwVolumeSerialNumber &&
           firstInfo.nFileIndexHigh == secondInfo.nFileIndexHigh &&
           firstInfo.nFileIndexLow == secondInfo.nFileIndexLow;
    CloseHandle(firstHandle);
    CloseHandle(secondHandle);
    return same;
#else
    struct stat firstInfo;
    struct stat secondInfo;

    if (first == NULL || second == NULL || stat(first, &firstInfo) != 0 ||
        stat(second, &secondInfo) != 0) {
        return 0;
    }
    return firstInfo.st_dev == secondInfo.st_dev &&
           firstInfo.st_ino == secondInfo.st_ino;
#endif
}

static int BuildCandidate(char *path, size_t pathSize, const char *directory,
                          char separator, const char *name) {
    int written = snprintf(path, pathSize, "%s%c%s", directory, separator,
                           name);

    return written >= 0 && (size_t)written < pathSize;
}

int DiscDiscoverImage(const char *directory, char *path, size_t pathSize,
                      DiscImageValidator validate, void *context) {
    if (path == NULL || pathSize == 0) {
        return 0;
    }
    path[0] = '\0';
    if (directory == NULL || validate == NULL) return 0;
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
