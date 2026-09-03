#ifndef RAGE_PORT_PSYQ_KERNEL_H
#define RAGE_PORT_PSYQ_KERNEL_H

#include <libapi.h>
#include <libetc.h>
#include <kernel.h>

typedef void (*KernelCallback)(void);
typedef struct DirEntry {
    char name[20];
    int attributes;
    int size;
    struct DirEntry *next;
    char system[8];
} DirEntry;

long BiosFileOpen(void *path, long mode);
long BiosFileSeek(long fd, long offset, long whence);
long BiosFileRead(long fd, void *buffer, long length);
long BiosFileWrite(long fd, void *buffer, long length);
long BiosFileClose(long fd);
long BiosFormatDevice(void *device);
void *BiosFirstFile(char *path, void *entry);
void *BiosNextFile(void *entry);

#endif
