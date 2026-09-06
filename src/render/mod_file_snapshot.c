#include "mod_file_snapshot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef _WIN32
static wchar_t *SnapshotWidePath(const char *path) {
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (count <= 0) return NULL;
    wchar_t *wide = malloc((size_t)count * sizeof(*wide));
    if (wide && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count)) {
        free(wide); wide = NULL;
    }
    return wide;
}
#endif

/* Validate the opened object, not a separate pathname stat. Parent directory
 * traversal is not pinned here; the caller still owns its staging boundary. */
static FILE *SnapshotOpenSource(const char *path) {
#ifdef _WIN32
    wchar_t *wide = SnapshotWidePath(path);
    if (!wide) return NULL;
    HANDLE handle = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    free(wide);
    if (handle == INVALID_HANDLE_VALUE) return NULL;
    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileType(handle) != FILE_TYPE_DISK || !GetFileInformationByHandle(handle, &info) ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))) {
        CloseHandle(handle); return NULL;
    }
    int descriptor = _open_osfhandle((intptr_t)handle, _O_RDONLY | _O_BINARY);
    if (descriptor < 0) { CloseHandle(handle); return NULL; }
    FILE *file = _fdopen(descriptor, "rb");
    if (!file) _close(descriptor);
#else
    int descriptor = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (descriptor < 0) return NULL;
    struct stat info;
    if (fstat(descriptor, &info) || !S_ISREG(info.st_mode)) {
        close(descriptor); return NULL;
    }
    FILE *file = fdopen(descriptor, "rb");
    if (!file) close(descriptor);
#endif
    return file;
}

static FILE *SnapshotOpenTarget(const char *path) {
#ifdef _WIN32
    wchar_t *wide = SnapshotWidePath(path);
    if (!wide) return NULL;
    FILE *file = _wfopen(wide, L"w+bx");
    free(wide);
    return file;
#else
    return fopen(path, "w+bx");
#endif
}

static void SnapshotRemoveTarget(const char *path) {
#ifdef _WIN32
    wchar_t *wide = SnapshotWidePath(path);
    if (wide) { _wremove(wide); free(wide); }
#else
    remove(path);
#endif
}

int ModFileWriteExclusive(const char *target, const void *bytes, size_t size) {
    if (!target || (!bytes && size)) return 0;
    FILE *output = SnapshotOpenTarget(target);
    if (!output) return 0;
    int ok = !size || fwrite(bytes, 1, size, output) == size;
    if (fclose(output)) ok = 0;
    if (!ok) SnapshotRemoveTarget(target);
    return ok;
}

/* Copy through one open source handle, then compare the copied bytes against
 * a second read of that handle. Differing reads reject the copy; writers that
 * restore bytes between reads are not detected. Private staging
 * is validated after copying, never against the external source directory. */
int ModFileSnapshotCopy(const char *source, const char *target, size_t *total) {
    unsigned char buffer[65536], copy[65536];
    if (!source || !target || !total || *total > 1024u*1024u*1024u) return 0;
    FILE *input = SnapshotOpenSource(source), *output = NULL;
    size_t size = 0, n;
    int ok = 0;
    if (!input) return 0;
    output = SnapshotOpenTarget(target);
    if (!output) { fclose(input); return 0; }
    while ((n = fread(buffer,1,sizeof(buffer),input)) != 0) {
        if (size > 128u*1024u*1024u-n ||
            *total > 1024u*1024u*1024u-size-n ||
            fwrite(buffer,1,n,output) != n) goto done;
        size += n;
    }
    if (ferror(input) || fflush(output) || fseek(input,0,SEEK_SET) || fseek(output,0,SEEK_SET)) goto done;
    for (;;) {
        n = fread(buffer,1,sizeof(buffer),input);
        size_t copied = fread(copy,1,sizeof(copy),output);
        if (n != copied || memcmp(buffer,copy,n)) goto done;
        if (!n) break;
    }
    ok = !ferror(input) && !ferror(output);
done:
    if (fclose(input)) ok = 0;
    if (fclose(output)) ok = 0;
    if (!ok) SnapshotRemoveTarget(target);
    else *total += size;
    return ok;
}
