#include "chd_disc.h"
#include "chd_track_layout.h"

#include <libchdr/chd.h>
#include <psyz/cd.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { RAGE_CHD_MAX_TRACKS = 99, RAGE_CHD_RAW_SECTOR = 2352 };

static chd_file *s_chd;
static unsigned char *s_hunk;
static uint32_t s_hunkBytes;
static uint32_t s_unitBytes;
static uint32_t s_cachedHunk = UINT32_MAX;
static RageChdTrack s_tracks[RAGE_CHD_MAX_TRACKS];
static int s_trackCount;

static int ChdReadCallback(unsigned int sector, void *buffer, void *user) {
    (void)user;
    return ChdReadRawSector(sector, buffer) ? RAGE_CHD_RAW_SECTOR : -1;
}

void ChdClose(void) {
    free(s_hunk);
    s_hunk = NULL;
    if (s_chd != NULL) chd_close(s_chd);
    s_chd = NULL;
    s_hunkBytes = 0;
    s_unitBytes = 0;
    s_cachedHunk = UINT32_MAX;
    s_trackCount = 0;
    memset(s_tracks, 0, sizeof(s_tracks));
}

static int ChdLoadToc(PsyzCdTrackInfo *psyzTracks, int *leadOut) {
    RageChdTrackLayout layout;
    int index;

    ChdTrackLayoutInit(&layout);
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
        if ((strcmp(type, "AUDIO") != 0 &&
             strcmp(type, "MODE2_RAW") != 0 &&
             strcmp(type, "MODE1_RAW") != 0)) {
            return 0;
        }
        if (!ChdTrackLayoutAppend(
                &layout, index, number, frames, pregap, postgap,
                pgtype[0] == 'V', &s_tracks[index])) {
            return 0;
        }
        s_tracks[index].audio = strcmp(type, "AUDIO") == 0;
        psyzTracks[index].sector = s_tracks[index].sector;
        psyzTracks[index].end_sector = s_tracks[index].endSector;
        psyzTracks[index].is_audio = s_tracks[index].audio;
        psyzTracks[index].audio_big_endian = s_tracks[index].audio;
        (void)subtype;
        (void)pgsub;
    }
    if (index == 0) return 0;
    s_trackCount = index;
    *leadOut = layout.nextSector;
    return 1;
}

int ChdOpen(const char *path) {
    const chd_header *header;
    PsyzCdTrackInfo tracks[RAGE_CHD_MAX_TRACKS];
    int leadOut;
    chd_error error;

    ChdClose();
    if (path == NULL || path[0] == '\0') {
        fprintf(stderr, "rage-port: no CHD path was provided\n");
        return 0;
    }
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
        ChdClose();
        return 0;
    }
    s_hunkBytes = header->hunkbytes;
    s_unitBytes = header->unitbytes;
    s_hunk = malloc(s_hunkBytes);
    if (s_hunk == NULL || !ChdLoadToc(tracks, &leadOut) ||
        Psyz_CdSetSectorBackend(tracks, s_trackCount, leadOut,
                                ChdReadCallback, NULL) != 0) {
        fprintf(stderr, "rage-port: invalid or unsupported CD CHD metadata\n");
        ChdClose();
        return 0;
    }
    fprintf(stderr, "rage-port: CHD disc opened (%d tracks)\n", s_trackCount);
    return 1;
}

int ChdReadRawSector(unsigned int sector, unsigned char *raw) {
    const RageChdTrack *track = NULL;
    uint32_t storedFrame;
    uint32_t framesPerHunk;
    uint32_t hunkNumber;
    uint32_t hunkOffset;
    int index;

    if (s_chd == NULL || raw == NULL) return 0;
    for (index = 0; index < s_trackCount; index++) {
        if (sector >= (uint32_t)s_tracks[index].sector &&
            sector < (uint32_t)s_tracks[index].endSector) {
            track = &s_tracks[index];
            break;
        }
    }
    if (track == NULL) return 0;
    storedFrame = track->frameOffset + sector - (uint32_t)track->sector;
    framesPerHunk = s_hunkBytes / s_unitBytes;
    hunkNumber = storedFrame / framesPerHunk;
    hunkOffset = storedFrame % framesPerHunk;
    if (hunkNumber != s_cachedHunk) {
        if (chd_read(s_chd, hunkNumber, s_hunk) != CHDERR_NONE) return 0;
        s_cachedHunk = hunkNumber;
    }
    memcpy(raw, s_hunk + hunkOffset * s_unitBytes, RAGE_CHD_RAW_SECTOR);
    return 1;
}
