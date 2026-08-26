#include "platform_paths.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define RageMkdir(path) _mkdir(path)
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <unistd.h>
#define RageMkdir(path) mkdir(path, 0755)
#else
#include <unistd.h>
#include <sys/stat.h>
#define RageMkdir(path) mkdir(path, 0755)
#endif

static int RagePathExists(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    fclose(file);
    return 1;
}

static int RageDirectoryExists(const char *path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat status;
    return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
#endif
}

static int RageJoinPath(char *out, size_t outSize, const char *directory,
                        const char *name) {
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif
    size_t length = strlen(directory);
    int written = snprintf(out, outSize, "%s%s%s", directory,
                           length > 0 && directory[length - 1] != '/' &&
                                   directory[length - 1] != '\\'
                               ? (char[2]){separator, '\0'}
                               : "",
                           name);
    return written >= 0 && (size_t)written < outSize;
}

static int RageExecutableDirectory(const char *argv0, char *out,
                                   size_t outSize) {
    char executable[4096];
    size_t length = 0;
    char *slash;
#ifdef _WIN32
    DWORD result = GetModuleFileNameA(NULL, executable, sizeof(executable));
    if (result == 0 || result >= sizeof(executable)) return 0;
    length = result;
#elif defined(__APPLE__)
    uint32_t size = sizeof(executable);
    if (_NSGetExecutablePath(executable, &size) != 0) return 0;
    length = strlen(executable);
#else
    ssize_t result = readlink("/proc/self/exe", executable,
                              sizeof(executable) - 1);
    if (result > 0) length = (size_t)result;
#endif
    if (length == 0 && argv0 != NULL && strchr(argv0, '/') != NULL) {
        if (strlen(argv0) >= sizeof(executable)) return 0;
        strcpy(executable, argv0);
        length = strlen(executable);
    }
    if (length == 0) return 0;
    executable[length] = '\0';
    slash = strrchr(executable, '/');
#ifdef _WIN32
    {
        char *backslash = strrchr(executable, '\\');
        if (backslash != NULL && (slash == NULL || backslash > slash))
            slash = backslash;
    }
#endif
    if (slash == NULL) return 0;
    *slash = '\0';
    if (strlen(executable) + 1 > outSize) return 0;
    strcpy(out, executable);
    return 1;
}

int RagePlatformExecutableDirectory(const char *argv0, char *out,
                                    size_t outSize) {
    return RageExecutableDirectory(argv0, out, outSize);
}

int RagePlatformExistingPortableStateDirectory(
    const char *executableDirectory, char *out, size_t outSize) {
    char candidate[4096];
    char card[4096];
    size_t length;
    const char bundleSuffix[] = "/Contents/MacOS";

    if (executableDirectory == NULL || executableDirectory[0] == '\0')
        return 0;
    length = strlen(executableDirectory);
    if (length >= sizeof(candidate)) return 0;
    memcpy(candidate, executableDirectory, length + 1);

    /* A macOS archive keeps bu00 beside Rage Racer.app, not inside its
     * signed Contents directory. Recognize the bundle shape by path rather
     * than by host so this helper remains straightforward to test. */
    if (length > sizeof(bundleSuffix) - 1 &&
        strcmp(candidate + length - (sizeof(bundleSuffix) - 1),
               bundleSuffix) == 0) {
        char *app = candidate + length - (sizeof(bundleSuffix) - 1);
        char *parent;
        *app = '\0';
        parent = strrchr(candidate, '/');
        if (parent != NULL) {
            *parent = '\0';
            if (RageJoinPath(card, sizeof(card), candidate, "bu00") &&
                RageDirectoryExists(card)) {
                if (strlen(candidate) + 1 > outSize) return 0;
                strcpy(out, candidate);
                return 1;
            }
        }
        memcpy(candidate, executableDirectory, length + 1);
    }

    if (!RageJoinPath(card, sizeof(card), candidate, "bu00") ||
        !RageDirectoryExists(card) || strlen(candidate) + 1 > outSize)
        return 0;
    strcpy(out, candidate);
    return 1;
}

int RagePlatformUserConfigDirectory(char *out, size_t outSize) {
    const char *base;
    int written;
#ifdef _WIN32
    base = getenv("APPDATA");
    if (base == NULL || base[0] == '\0') return 0;
    written = snprintf(out, outSize, "%s\\Rage Racer", base);
#elif defined(__APPLE__)
    base = getenv("HOME");
    if (base == NULL || base[0] == '\0') return 0;
    written = snprintf(out, outSize,
                       "%s/Library/Application Support/Rage Racer", base);
#else
    base = getenv("XDG_CONFIG_HOME");
    if (base != NULL && base[0] != '\0')
        written = snprintf(out, outSize, "%s/rage-racer", base);
    else {
        base = getenv("HOME");
        if (base == NULL || base[0] == '\0') return 0;
        written = snprintf(out, outSize, "%s/.config/rage-racer", base);
    }
#endif
    return written >= 0 && (size_t)written < outSize;
}

int RagePlatformUserStateDirectory(char *out, size_t outSize) {
#ifdef _WIN32
    return RagePlatformUserConfigDirectory(out, outSize);
#elif defined(__APPLE__)
    return RagePlatformUserConfigDirectory(out, outSize);
#else
    const char *base;
    int written;
    base = getenv("XDG_STATE_HOME");
    if (base != NULL && base[0] != '\0')
        written = snprintf(out, outSize, "%s/rage-racer", base);
    else {
        base = getenv("HOME");
        if (base == NULL || base[0] == '\0') return 0;
        written = snprintf(out, outSize, "%s/.local/state/rage-racer", base);
    }
    return written >= 0 && (size_t)written < outSize;
#endif
}

int RagePlatformUserConfigPath(const char *name, char *out, size_t outSize) {
    char directory[4096];
    return RagePlatformUserConfigDirectory(directory, sizeof(directory)) &&
           RageJoinPath(out, outSize, directory, name);
}

int RagePlatformTemporaryDirectory(char *out, size_t outSize) {
#ifdef _WIN32
    DWORD length = GetTempPathA((DWORD)outSize, out);
    return length > 0 && length < outSize;
#else
    const char *temporary = getenv("TMPDIR");
    int written;
    if (temporary == NULL || temporary[0] == '\0') temporary = "/tmp";
    written = snprintf(out, outSize, "%s", temporary);
    return written >= 0 && (size_t)written < outSize;
#endif
}

int RagePlatformEnsureDirectory(const char *path) {
    char buffer[4096];
    char *cursor;
    size_t length = strlen(path);
    if (length == 0 || length >= sizeof(buffer)) return 0;
    memcpy(buffer, path, length + 1);
    for (cursor = buffer + 1; *cursor != '\0'; cursor++) {
        if (*cursor != '/' && *cursor != '\\') continue;
#ifdef _WIN32
        if (cursor == buffer + 2 && buffer[1] == ':') continue;
#endif
        {
            char separator = *cursor;
            *cursor = '\0';
            if (RageMkdir(buffer) != 0 && errno != EEXIST) return 0;
            *cursor = separator;
        }
    }
    return RageMkdir(buffer) == 0 || errno == EEXIST;
}

int RagePlatformFindConfigFile(const char *argv0, const char *name,
                               char *path, size_t pathSize) {
    char directory[4096];
    if (RagePlatformUserConfigDirectory(directory, sizeof(directory)) &&
        RageJoinPath(path, pathSize, directory, name) && RagePathExists(path))
        return 1;
    if (RageExecutableDirectory(argv0, directory, sizeof(directory))) {
        if (RageJoinPath(path, pathSize, directory, name) &&
            RagePathExists(path)) return 1;
#ifdef __APPLE__
        {
            char beside[4096];
            /* Beside the .app, where the archive keeps the editable copies.
             * Anything inside Contents/Resources is sealed by the code
             * signature, so editing it there stops the app from launching. */
            if (RageJoinPath(beside, sizeof(beside), directory, "../../..") &&
                RageJoinPath(path, pathSize, beside, name) &&
                RagePathExists(path)) return 1;
        }
        {
            char resources[4096];
            if (RageJoinPath(resources, sizeof(resources), directory,
                             "../Resources") &&
                RageJoinPath(path, pathSize, resources, name) &&
                RagePathExists(path)) return 1;
        }
#endif
    }
    if (snprintf(path, pathSize, "%s", name) < (int)pathSize &&
        RagePathExists(path)) return 1;
    path[0] = '\0';
    return 0;
}
