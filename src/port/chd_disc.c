#include "chd_disc.h"

#include <libchdr/chd.h>
#include <psyz/cd.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { RAGE_CHD_MAX_TRACKS = 99, RAGE_CHD_RAW_SECTOR = 2352 };

typedef struct RageChdTrack {
    int sector;
    int endSector;
    uint32_t frameOffset;
    int audio;
} RageChdTrack;

static chd_file *s_chd;
static unsigned char *s_hunk;
static uint32_t s_hunkBytes;
static uint32_t s_unitBytes;
static int s_cachedHunk = -1;
static RageChdTrack s_tracks[RAGE_CHD_MAX_TRACKS];
static int s_trackCount;

static int RageChdReadCallback(unsigned int sector, void *buffer, void *user) {
    (void)user;
    return RageChdReadRawSector(sector, buffer) ? RAGE_CHD_RAW_SECTOR : -1;
}

void RageChdClose(void) {
    free(s_hunk);
    s_hunk = NULL;
    if (s_chd != NULL) chd_close(s_chd);
    s_chd = NULL;
    s_hunkBytes = 0;
    s_unitBytes = 0;
    s_cachedHunk = -1;
    s_trackCount = 0;
    memset(s_tracks, 0, sizeof(s_tracks));
}

static int RageChdLoadToc(PsyzCdTrackInfo *psyzTracks, int *leadOut) {
    int plba = -150;
    uint32_t frameOffset = 0;
    int index;

    for (index = 0; index < RAGE_CHD_MAX_TRACKS; index++) {
        char metadata[256] = {0};
        char type[64] = {0};
        char subtype[32] = {0};
        char pgtype[32] = {0};
        char pgsub[32] = {0};
        uint32_t metadataSize = 0;
        int number = 0, frames = 0, pregap = 0, postgap = 0;
        int parsed;
        chd_error error = chd_get_metadata(
            s_chd, CDROM_TRACK_METADATA2_TAG, (uint32_t)index, metadata,
            sizeof(metadata) - 1, &metadataSize, NULL, NULL);
        if (error == CHDERR_NONE) {
            parsed = sscanf(metadata,
                            "TRACK:%d TYPE:%63s SUBTYPE:%31s FRAMES:%d "
                            "PREGAP:%d PGTYPE:%31s PGSUB:%31s POSTGAP:%d",
                            &number, type, subtype, &frames, &pregap, pgtype,
                            pgsub, &postgap);
            if (parsed != 8) return 0;
        } else {
            error = chd_get_metadata(
                s_chd, CDROM_TRACK_METADATA_TAG, (uint32_t)index, metadata,
                sizeof(metadata) - 1, &metadataSize, NULL, NULL);
            if (error == CHDERR_METADATA_NOT_FOUND) break;
            if (error != CHDERR_NONE) return 0;
            parsed = sscanf(metadata,
                            "TRACK:%d TYPE:%63s SUBTYPE:%31s FRAMES:%d",
                            &number, type, subtype, &frames);
            if (parsed != 4) return 0;
        }
        if (number != index + 1 || frames <= 0 ||
            (strcmp(type, "AUDIO") != 0 &&
             strcmp(type, "MODE2_RAW") != 0 &&
             strcmp(type, "MODE1_RAW") != 0)) {
            return 0;
        }
        {
            int logicalPregap = number == 1 ? 150
                                : pgtype[0] == 'V' ? 0 : pregap;
            int storedPregap = pgtype[0] == 'V' ? pregap : 0;
            int playableFrames = frames - storedPregap;
            RageChdTrack *track = &s_tracks[index];
            if (playableFrames <= 0) return 0;
            plba += logicalPregap + storedPregap;
            frameOffset += (uint32_t)storedPregap;
            track->sector = plba;
            track->endSector = plba + playableFrames;
            track->frameOffset = frameOffset;
            track->audio = strcmp(type, "AUDIO") == 0;
            psyzTracks[index].sector = track->sector;
            psyzTracks[index].end_sector = track->endSector;
            psyzTracks[index].is_audio = track->audio;
            psyzTracks[index].audio_big_endian = track->audio;
            frameOffset += (uint32_t)playableFrames + (uint32_t)postgap;
            frameOffset += (((uint32_t)frames + 3U) & ~3U) -
                           (uint32_t)frames;
            plba += playableFrames + postgap;
        }
        (void)subtype;
        (void)pgsub;
    }
    if (index == 0) return 0;
    s_trackCount = index;
    *leadOut = plba;
    return 1;
}

int RageChdOpen(const char *path) {
    const chd_header *header;
    PsyzCdTrackInfo tracks[RAGE_CHD_MAX_TRACKS];
    int leadOut;
    chd_error error;

    RageChdClose();
    error = chd_open(path, CHD_OPEN_READ, NULL, &s_chd);
    if (error != CHDERR_NONE) {
        fprintf(stderr, "rage-port: cannot open CHD %s: %s\n", path,
                chd_error_string(error));
        return 0;
    }
    header = chd_get_header(s_chd);
    if (header == NULL || header->unitbytes < RAGE_CHD_RAW_SECTOR ||
        header->hunkbytes < header->unitbytes ||
        header->hunkbytes % header->unitbytes != 0) {
        fprintf(stderr, "rage-port: unsupported CHD sector layout\n");
        RageChdClose();
        return 0;
    }
    s_hunkBytes = header->hunkbytes;
    s_unitBytes = header->unitbytes;
    s_hunk = malloc(s_hunkBytes);
    if (s_hunk == NULL || !RageChdLoadToc(tracks, &leadOut) ||
        Psyz_CdSetSectorBackend(tracks, s_trackCount, leadOut,
                                RageChdReadCallback, NULL) != 0) {
        fprintf(stderr, "rage-port: invalid or unsupported CD CHD metadata\n");
        RageChdClose();
        return 0;
    }
    fprintf(stderr, "rage-port: CHD disc opened (%d tracks)\n", s_trackCount);
    return 1;
}

int RageChdReadRawSector(unsigned int sector, unsigned char *raw) {
    const RageChdTrack *track = NULL;
    uint32_t storedFrame;
    uint32_t framesPerHunk;
    uint32_t hunkNumber;
    uint32_t hunkOffset;
    int index;

    if (s_chd == NULL || raw == NULL) return 0;
    for (index = 0; index < s_trackCount; index++) {
        if ((int)sector >= s_tracks[index].sector &&
            (int)sector < s_tracks[index].endSector) {
            track = &s_tracks[index];
            break;
        }
    }
    if (track == NULL) return 0;
    storedFrame = track->frameOffset + sector - (uint32_t)track->sector;
    framesPerHunk = s_hunkBytes / s_unitBytes;
    hunkNumber = storedFrame / framesPerHunk;
    hunkOffset = storedFrame % framesPerHunk;
    if ((int)hunkNumber != s_cachedHunk) {
        if (chd_read(s_chd, hunkNumber, s_hunk) != CHDERR_NONE) return 0;
        s_cachedHunk = (int)hunkNumber;
    }
    memcpy(raw, s_hunk + hunkOffset * s_unitBytes, RAGE_CHD_RAW_SECTOR);
    return 1;
}
