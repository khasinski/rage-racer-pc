/* Independent sector/XA oracle for all retail FMVs. No game loop,
 * SDL, decoder, renderer or Python dependency. Times simulation ticks, not the
 * unthrottled smoke process's wall clock or physical audio device latency. */
#include "disc_cue.h"
#include "disc_raw_file.h"
#include "disc_stream_table.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned Le16(const unsigned char *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}
static unsigned Le32(const unsigned char *p) {
    return Le16(p) | (Le16(p + 2) << 16);
}
static int Number(const char **cursor, const char *prefix, unsigned *value) {
    size_t length = strlen(prefix);
    if (strncmp(*cursor, prefix, length) != 0) return 0;
    const char *text = *cursor + length;
    if (*text < '0' || *text > '9') return 0;
    errno = 0;
    char *end;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno != 0 || parsed > UINT_MAX) return 0;
    *cursor = end;
    *value = (unsigned)parsed;
    return 1;
}
static int Check(DiscRawFile *raw, FILE *log, unsigned stream, int strictAudio) {
    DiscIdentity identity;
    DiscIsoReader iso;
    DiscIsoFile str;
    unsigned ends[10000], audio = 0, firstAudio = 0, found = 0;
    unsigned chunks = 0, seen = 0;
    if (!DiscIdentify(DiscRawFileReadSector, raw, &identity) ||
        !identity.tableValid || !DiscIsoOpen(&iso, DiscRawFileReadSector, raw) ||
        !DiscIsoFindFile(&iso, "RAGE.STR", &str)) return 0;
    unsigned shown = identity.table.frames[stream];
    if (shown < 2 || shown > sizeof(ends) / sizeof(ends[0])) return 0;
    unsigned begin = identity.table.offset[stream];
    unsigned span = identity.table.span[stream];
    if (span == 0 || begin > UINT_MAX - span) return 0;
    for (unsigned i = 0; i < span && found < shown; ++i) {
        unsigned absolute;
        unsigned char sector[DISC_RAW_SECTOR_SIZE];
        if (!DiscIsoResolveSector(&str, begin + i, &absolute) ||
            !DiscRawFileReadSector(raw, absolute, sector)) return 0;
        const unsigned char *header = sector + DISC_MODE2_USER_OFFSET;
        if (Le32(header) != 0x80010160u) {
            if (sector[18] & 4) {
                /* Retail XA: stereo 37800 Hz, 4-bit. Do not silently apply
                 * its 2016-sample duration to a different encoding. */
                if (sector[19] != 1) return 0;
                ++audio;
            }
            continue;
        }
        unsigned chunk = Le16(header + 4), declared = Le16(header + 6);
        if (chunk == 0) {
            if (seen != 0) return 0;
            chunks = declared;
        }
        if (chunks == 0 || chunk != seen || declared != chunks) return 0;
        if (++seen == chunks) {
            ends[found++] = i + 1;
            if (found == 1) firstAudio = audio;
            seen = chunks = 0;
        }
    }
    if (found != shown || audio <= firstAudio) return 0;
    unsigned expectedHz = strcmp(identity.region, "PAL") == 0 ? 50 : 60;
    if (expectedHz == 60 && strcmp(identity.region, "NTSC-U") != 0 &&
        strcmp(identity.region, "NTSC-J") != 0) return 0;
    unsigned baseHz = 0, count = 0, firstTick = 0, lastTick = 0;
    char line[4096];
    while (fgets(line, sizeof(line), log)) {
        const char *base = strstr(line, "base_hz=");
        if (base != NULL && !Number(&base, "base_hz=", &baseHz)) return 0;
        unsigned frame, tick, timer, position;
        if (strncmp(line, "fmv frame=", 10) != 0) continue;
        const char *cursor = line;
        if (!Number(&cursor, "fmv frame=", &frame) ||
            !Number(&cursor, " vblank=", &tick) ||
            !Number(&cursor, " scene_timer=", &timer) ||
            !Number(&cursor, " sector=", &position)) return 0;
        while (*cursor == '\r' || *cursor == '\n') ++cursor;
        if (*cursor != '\0') return 0;
        if (frame == 0 && count != 0) break; /* title-screen replay */
        if (count >= shown || frame != count || position != ends[count]) return 0;
        if (count == 0) firstTick = tick;
        else if (tick <= lastTick) return 0;
        lastTick = tick;
        ++count;
    }
    if (ferror(log) || count != shown || baseHz != expectedHz ||
        lastTick <= firstTick) return 0;
    double elapsed = (double)(lastTick - firstTick) / baseHz;
    double soundtrack = (double)(audio - firstAudio) * 2016.0 / 37800.0;
    double sectorsPerSecond = (double)(ends[shown - 1] - ends[0]) / elapsed;
    printf("%s stream=%u frames=%u ticks_seconds=%.6f xa_seconds=%.6f sectors_per_second=%.6f\n",
           identity.boot, stream, shown, elapsed, soundtrack, sectorsPerSecond);
    /* XA coverage is not always the movie duration: the PAL ending has fewer
     * audio sectors than a continuously filled soundtrack would require.
     * Representative movies opt into the stricter timing assertion below. */
    if (strictAudio) {
        double ratio = elapsed / soundtrack;
        if (ratio < 0.98 || ratio > 1.02) {
            fprintf(stderr, "FMV picture/XA duration mismatch: picture=%.6f xa=%.6f\n",
                    elapsed, soundtrack);
            return 0;
        }
    }
    return sectorsPerSecond >= 147.0 && sectorsPerSecond <= 153.0;
}
int main(int argc, char **argv) {
    unsigned stream = 0;
    int strictAudio = argc == 5 && strcmp(argv[4], "--strict-audio") == 0;
    if ((argc != 4 && !strictAudio) ||
        !((strlen(argv[2]) == 1 && argv[2][0] >= '0' && argv[2][0] <= '9') ||
          strcmp(argv[2], "10") == 0)) {
        fprintf(stderr, "usage: rage-fmv-pacing-check BIN_OR_CUE STREAM_0_TO_10 GAME_LOG [--strict-audio]\n");
        return 2;
    }
    stream = strcmp(argv[2], "10") == 0 ? 10u : (unsigned)(argv[2][0] - '0');
    DiscRawFile raw = {0};
    char image[4096];
    const char *path = argv[1];
    size_t length = strlen(path);
    if (length >= 4 && path[length - 4] == '.' &&
        tolower((unsigned char)path[length - 3]) == 'c' &&
        tolower((unsigned char)path[length - 2]) == 'u' &&
        tolower((unsigned char)path[length - 1]) == 'e') {
        if (!DiscCueResolveDataTrack(path, image, sizeof(image), &raw.trackOffset))
            return 1;
        path = image;
    }
    raw.file = fopen(path, "rb");
    FILE *log = fopen(argv[3], "r");
    int ok = raw.file != NULL && log != NULL &&
             Check(&raw, log, stream, strictAudio);
    if (raw.file != NULL && fclose(raw.file) != 0) ok = 0;
    if (log != NULL && fclose(log) != 0) ok = 0;
    if (!ok) fprintf(stderr, "FMV sector/XA pacing verification failed\n");
    return ok ? 0 : 1;
}
