#ifdef _WIN32
#include <io.h>
#define close _close
#else
#include <unistd.h>
#endif

#include <libapi.h>
#include <psyz.h>

long BiosFileOpen(void *path, long mode) {
    if (path == NULL) return -1;
    return psyz_open(path, (int)mode);
}

long BiosFileSeek(long fd, long offset, long whence) {
    return lseek((int)fd, offset, (int)whence);
}

long BiosFileRead(long fd, void *buffer, long length) {
    if (length < 0) return -1;
    return read((int)fd, buffer, (size_t)length);
}

long BiosFileWrite(long fd, void *buffer, long length) {
    if (length < 0) return -1;
    return write((int)fd, buffer, (size_t)length);
}

long BiosFileClose(long fd) { return close((int)fd); }

void *BiosFirstFile(char *path, void *entry) {
    return firstfile(path, entry);
}

void *BiosNextFile(void *entry) { return nextfile(entry); }

long BiosFormatDevice(void *device) { return format(device); }
