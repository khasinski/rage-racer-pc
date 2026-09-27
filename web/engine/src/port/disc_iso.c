#include "disc_iso.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
int _strnicmp(const char *lhs, const char *rhs, unsigned long long count);
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

static unsigned int ReadLe16(const unsigned char *data) {
    return (unsigned int)data[0] | ((unsigned int)data[1] << 8);
}

static unsigned int ReadLe32(const unsigned char *data) {
    return ReadLe16(data) | (ReadLe16(data + 2) << 16);
}

int DiscIsoReadUserSector(DiscIsoReader *reader, unsigned int sector,
                          unsigned char *user) {
    unsigned char raw[DISC_RAW_SECTOR_SIZE];

    if (reader == NULL || reader->read == NULL || user == NULL ||
        (reader->userOffset != 16 &&
         reader->userOffset != DISC_MODE2_USER_OFFSET) ||
        !reader->read(reader->context, sector, raw)) {
        return 0;
    }
    memcpy(user, raw + reader->userOffset, DISC_ISO_SECTOR_SIZE);
    return 1;
}

int DiscIsoOpen(DiscIsoReader *reader, DiscRawSectorReader read,
                void *context) {
    unsigned char volume[DISC_ISO_SECTOR_SIZE];
    int offset;

    if (reader == NULL) return 0;
    reader->read = NULL;
    reader->context = NULL;
    reader->userOffset = 0;
    if (read == NULL) return 0;
    reader->read = read;
    reader->context = context;
    for (offset = 16; offset <= DISC_MODE2_USER_OFFSET; offset += 8) {
        reader->userOffset = offset;
        if (DiscIsoReadUserSector(reader, 16, volume) &&
            volume[0] == 1 && memcmp(volume + 1, "CD001", 5) == 0) {
            return 1;
        }
    }
    reader->read = NULL;
    reader->context = NULL;
    reader->userOffset = 0;
    return 0;
}

int DiscIsoVisitRoot(DiscIsoReader *reader, DiscIsoFileVisitor visitor,
                     void *context) {
    unsigned char volume[DISC_ISO_SECTOR_SIZE];
    unsigned char user[DISC_ISO_SECTOR_SIZE];
    unsigned int root;
    unsigned int size;
    unsigned int sectorCount;
    unsigned int sector;

    if (visitor == NULL ||
        !DiscIsoReadUserSector(reader, 16, volume) || volume[156] < 34) {
        return 0;
    }
    root = ReadLe32(volume + 158);
    size = ReadLe32(volume + 166);
    sectorCount = size / DISC_ISO_SECTOR_SIZE +
                  (size % DISC_ISO_SECTOR_SIZE != 0);
    for (sector = 0; sector < sectorCount; sector++) {
        unsigned int cursor = 0;
        unsigned int sectorSize = size - sector * DISC_ISO_SECTOR_SIZE;

        if (root > UINT_MAX - sector ||
            !DiscIsoReadUserSector(reader, root + sector, user)) {
            return 0;
        }
        if (sectorSize > DISC_ISO_SECTOR_SIZE) {
            sectorSize = DISC_ISO_SECTOR_SIZE;
        }
        while (cursor < sectorSize) {
            const unsigned char *record = user + cursor;
            unsigned int length = record[0];
            DiscIsoFile file;

            if (length == 0) break;
            if (length < 34 || length > sectorSize - cursor ||
                record[32] > length - 33) {
                return 0;
            }
            file.lba = ReadLe32(record + 2);
            file.size = ReadLe32(record + 10);
            if (visitor(context, record + 33, record[32], &file)) return 1;
            cursor += length;
        }
    }
    return 1;
}

typedef struct FindFileContext {
    const char *wanted;
    DiscIsoFile *file;
    int found;
} FindFileContext;

static int FindFileVisitor(void *context, const unsigned char *name,
                           unsigned int nameLength,
                           const DiscIsoFile *file) {
    FindFileContext *find = context;
    size_t wantedLength = strlen(find->wanted);

    if (nameLength < wantedLength ||
        strncasecmp((const char *)name, find->wanted, wantedLength) != 0 ||
        (nameLength != wantedLength && name[wantedLength] != ';') ||
        file->size == 0) {
        return 0;
    }
    *find->file = *file;
    find->found = 1;
    return 1;
}

int DiscIsoFindFile(DiscIsoReader *reader, const char *name,
                    DiscIsoFile *file) {
    FindFileContext context;

    if (file == NULL) return 0;
    memset(file, 0, sizeof(*file));
    if (name == NULL || name[0] == '\0') return 0;
    context.wanted = name;
    context.file = file;
    context.found = 0;
    return DiscIsoVisitRoot(reader, FindFileVisitor, &context) && context.found;
}

int DiscIsoResolveSector(const DiscIsoFile *file, unsigned int relativeSector,
                         unsigned int *absoluteSector) {
    unsigned int sectorCount;

    if (absoluteSector == NULL) return 0;
    *absoluteSector = 0;
    if (file == NULL || file->size == 0) return 0;
    sectorCount = file->size / DISC_ISO_SECTOR_SIZE +
                  (file->size % DISC_ISO_SECTOR_SIZE != 0);
    if (relativeSector >= sectorCount ||
        file->lba > UINT_MAX - relativeSector) {
        return 0;
    }
    *absoluteSector = file->lba + relativeSector;
    return 1;
}

unsigned char *DiscIsoReadWholeFile(DiscIsoReader *reader,
                                    const DiscIsoFile *file) {
    unsigned int sectors;
    unsigned char *data;
    unsigned int index;

    if (file == NULL || file->size == 0) return NULL;
    sectors = file->size / DISC_ISO_SECTOR_SIZE +
              (file->size % DISC_ISO_SECTOR_SIZE != 0);
#if SIZE_MAX <= UINT_MAX
    if (sectors > SIZE_MAX / DISC_ISO_SECTOR_SIZE) return NULL;
#endif
    data = malloc((size_t)sectors * DISC_ISO_SECTOR_SIZE);
    if (data == NULL) return NULL;
    for (index = 0; index < sectors; index++) {
        unsigned int absoluteSector;

        if (!DiscIsoResolveSector(file, index, &absoluteSector) ||
            !DiscIsoReadUserSector(
                reader, absoluteSector,
                data + (size_t)index * DISC_ISO_SECTOR_SIZE)) {
            free(data);
            return NULL;
        }
    }
    return data;
}
