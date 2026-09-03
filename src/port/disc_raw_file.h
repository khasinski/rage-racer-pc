#ifndef RAGE_DISC_RAW_FILE_H
#define RAGE_DISC_RAW_FILE_H

#include <stdio.h>

typedef struct DiscRawFile {
    FILE *file;
    long trackOffset;
} DiscRawFile;

int DiscRawFileReadSector(void *context, unsigned int sector,
                          unsigned char *raw);

#endif
