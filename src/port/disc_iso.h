#ifndef RAGE_DISC_ISO_H
#define RAGE_DISC_ISO_H

#include <stddef.h>

enum {
    DISC_RAW_SECTOR_SIZE = 2352,
    DISC_ISO_SECTOR_SIZE = 2048,
    DISC_MODE2_USER_OFFSET = 24,
};

typedef int (*DiscRawSectorReader)(void *context, unsigned int sector,
                                   unsigned char *raw);

typedef struct DiscIsoReader {
    DiscRawSectorReader read;
    void *context;
    int userOffset;
} DiscIsoReader;

typedef struct DiscIsoFile {
    unsigned int lba;
    unsigned int size;
} DiscIsoFile;

/* Return non-zero from a visitor to stop iteration successfully. */
typedef int (*DiscIsoFileVisitor)(void *context, const unsigned char *name,
                                  unsigned int nameLength,
                                  const DiscIsoFile *file);

int DiscIsoOpen(DiscIsoReader *reader, DiscRawSectorReader read,
                void *context);
int DiscIsoReadUserSector(DiscIsoReader *reader, unsigned int sector,
                          unsigned char *user);
int DiscIsoVisitRoot(DiscIsoReader *reader, DiscIsoFileVisitor visitor,
                     void *context);
int DiscIsoFindFile(DiscIsoReader *reader, const char *name,
                    DiscIsoFile *file);
unsigned char *DiscIsoReadWholeFile(DiscIsoReader *reader,
                                    const DiscIsoFile *file);

#endif
