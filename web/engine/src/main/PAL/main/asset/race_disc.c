#include "game/race_data.h"
#include "disc_cue.h"
#include "disc_discovery.h"
#include "disc_iso.h"
#include "disc_identity.h"
#include "disc_raw_file.h"
#include <stdlib.h>
#include <string.h>

static RaceData *AdoptArchive(u8 *data, size_t size) {
    if (data == NULL) return NULL;
    RaceData *archive = malloc(sizeof(*archive));
    if (archive != NULL && ReadRaceData(data, size, archive)) return archive;
    free(archive);
    free(data);
    return NULL;
}

RaceData *LoadRaceArchive(const char *path) {
    if (path == NULL) return NULL;
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;
    u8 *data = NULL;
    size_t size = 0;
    if (fseek(file, 0, SEEK_END) == 0) {
        const long length = ftell(file);
        if (length > 0 && (unsigned long)length <= UINT32_MAX &&
            fseek(file, 0, SEEK_SET) == 0) {
            size = (size_t)length;
            data = malloc(size);
            if (data != NULL && fread(data, 1, size, file) != size) {
                free(data);
                data = NULL;
            }
        }
    }
    fclose(file);
    return AdoptArchive(data, size);
}

RaceData *LoadRaceDisc(const char *path) {
    const char *image = path;
    char resolved[4096];
    DiscRawFile disc = {0};
    if (DiscPathIsCue(path)) {
        if (!DiscCueResolveDataTrack(path, resolved, sizeof(resolved), &disc.trackOffset)) return NULL;
        image = resolved;
    } else if (!DiscPathIsBin(path)) {
        return NULL;
    }
    disc.file = fopen(image, "rb");
    if (disc.file == NULL) return NULL;
    DiscIsoReader reader;
    RaceData *archive = DiscIsoOpen(&reader, DiscRawFileReadSector, &disc) ?
                        LoadRaceIso(&reader) : NULL;
    fclose(disc.file);
    return archive;
}

RaceData *LoadRaceIso(DiscIsoReader *reader) {
    if (!reader || !reader->read) return NULL;
    DiscIsoFile entry;
    u8 *data = NULL;
    size_t size = 0;
    char boot[16] = {0};
    uint64_t executable = 0;
    if (DiscIsoFindFile(reader, "RAGE.BIN", &entry)) {
        DiscReadBootName(reader, boot, sizeof(boot));
        data = DiscIsoReadWholeFile(reader, &entry);
        size = entry.size;
        if (boot[0] && DiscIsoFindFile(reader, boot, &entry)) {
            u8 *code = DiscIsoReadWholeFile(reader, &entry);
            if (code != NULL) {
                executable = ArchiveFingerprint(code, entry.size);
                free(code);
            }
        }
    }
    RaceData *archive = AdoptArchive(data, size);
    if (archive != NULL) {
        memcpy(archive->boot, boot, sizeof(boot));
        archive->executable = executable;
    }
    return archive;
}

void FreeRaceData(RaceData *archive) {
    if (archive == NULL) return;
    free((void *)archive->data);
    free(archive);
}
