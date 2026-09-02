#include <psyz/video.h>
#include <psyz/cd.h>
#include <libgpu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disc_stream_table.h"
#include "fmv_audio.h"
#include "fmv_stream_index.h"
#include "host_clock.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
/* PsyQ exposes its own RECT and LoadImage symbols. Keep the Win32 header's
 * namespace from colliding with those compatibility declarations. */
#define RECT WIN32_RECT
#include <windows.h>
#undef RECT
#undef LoadImage
#include <fcntl.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "game/asset.h"
#include "game/cd.h"
#include "game/fmv.h"
#include "game/fmv_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "psyq/cd.h"
#include "psyq/press.h"
#include "runtime_config.h"
#include "timing_control.h"

extern int g_FrameCounter;

static unsigned char *s_pixels;
static int s_width;
static int s_height;
static unsigned int s_frame;
static unsigned int s_tickSectors;
static unsigned long long s_startNs;
static int s_wallClock;

/*
 * A movie in RAGE.STR carries no frame rate, and there is no single one to
 * carry: the opening movie runs at twenty-five frames a second and the other
 * ten at fifteen. What they share is the disc. At double speed the drive
 * delivers 150 sectors a second, a frame appears once the sectors carrying it
 * have been read, and the XA soundtrack is interleaved into those same sectors,
 * so playing the stream at the rate the drive would deliver it is what keeps
 * picture and sound together, whatever rate a movie was authored at.
 *
 * The port used to pace the opening movie at thirty frames a second, which ran
 * it a fifth too fast and left its soundtrack twelve seconds behind by the end.
 */
#define RAGE_STR_SECTORS_PER_SECOND 150

/* Movie sectors, held as read off the disc, plus the decoder's scratch. */
#define RAGE_STR_SECTOR_SIZE 2352
#define RAGE_STR_PAYLOAD_OFFSET 32
#define RAGE_STR_PAYLOAD_SIZE 0x7E0
#define RAGE_STR_MAGIC 0x80010160u
#define RAGE_BS_MAX 0x20000
#define RAGE_CODES_MAX 0x20000

static unsigned char *s_sectors;
static unsigned int s_sectorCount;
static unsigned int s_sectorCursor;
static unsigned char *s_bitstream;
static u_long *s_codes;
static u_long s_macroblock[192];

int HostReadStreamSector(unsigned int sector, unsigned char *raw);
/* How many sectors of RAGE.STR a movie occupies, which is where the next one
 * begins.  The mounted disc decides it; the movies do not sit in the same
 * places on a PAL disc and an American one. */
unsigned int HostStreamSectorSpan(int stream);

/* The rest of the MDEC front end comes from game/render.h and psyq/cd.h;
 * only the VLC stage has no declaration there. */
int DecDCTvlc(u_long *bs, u_long *buf);

static void ReleaseFmvBuffers(void) {
    free(s_sectors);
    s_sectors = NULL;
    free(s_bitstream);
    s_bitstream = NULL;
    free(s_codes);
    s_codes = NULL;
    s_sectorCount = 0;
    s_sectorCursor = 0;
}

static void ReleaseFmvPixels(void) {
    free(s_pixels);
    s_pixels = NULL;
    s_width = 0;
    s_height = 0;
}

static int HostExtractFmv(unsigned int first, unsigned int count) {
    unsigned int index;
    ReleaseFmvBuffers();
    if (count == 0) return 0;
    s_sectors = malloc((size_t)count * RAGE_STR_SECTOR_SIZE);
    s_bitstream = malloc(RAGE_BS_MAX);
    s_codes = malloc((size_t)RAGE_CODES_MAX * 2 + 64);
    if (s_sectors == NULL || s_bitstream == NULL || s_codes == NULL) {
        ReleaseFmvBuffers();
        return 0;
    }
    for (index = 0; index < count; index++) {
        if (!HostReadStreamSector(
                first + index,
                s_sectors + (size_t)index * RAGE_STR_SECTOR_SIZE)) {
            ReleaseFmvBuffers();
            return 0;
        }
    }
    s_sectorCount = count;
    s_sectorCursor = 0;
    return 1;
}

static unsigned int ReadLe16(const unsigned char *data) {
    return (unsigned int)data[0] | ((unsigned int)data[1] << 8);
}

static unsigned int ReadLe32(const unsigned char *data) {
    return ReadLe16(data) | (ReadLe16(data + 2) << 16);
}

/* Reassembles the next frame from its STR chunks and decodes it with the
 * software MDEC. Returns 0 once the movie runs out of frames. */
static int RageDecodeFmvFrame(void) {
    size_t size = 0;
    unsigned int chunks = 0;
    unsigned int seen = 0;
    int width = 0;
    int height = 0;
    int x;
    int y;

    while (s_sectorCursor < s_sectorCount) {
        const unsigned char *body =
            s_sectors + (size_t)s_sectorCursor * RAGE_STR_SECTOR_SIZE + 24;
        unsigned int chunk;
        s_sectorCursor++;
        if (ReadLe32(body) != RAGE_STR_MAGIC) continue;
        chunk = ReadLe16(body + 4);
        if (size == 0 && chunk != 0) continue; /* resynchronise on a frame */
        if (chunk == 0) {
            size = 0;
            seen = 0;
            chunks = ReadLe16(body + 6);
            width = (int)ReadLe16(body + 16);
            height = (int)ReadLe16(body + 18);
        }
        if (size + RAGE_STR_PAYLOAD_SIZE > RAGE_BS_MAX) return 0;
        memcpy(s_bitstream + size, body + RAGE_STR_PAYLOAD_OFFSET,
               RAGE_STR_PAYLOAD_SIZE);
        size += RAGE_STR_PAYLOAD_SIZE;
        if (++seen >= chunks && chunks != 0) break;
    }
    if (size == 0 || chunks == 0 || seen < chunks) return 0;
    if (width <= 0 || height <= 0 || width > s_width || height > s_height)
        return 0;

    DecDCTReset(0);
    if (DecDCTvlc((u_long *)s_bitstream, s_codes) < 0) return 0;
    DecDCTin(s_codes, 3);

    /* The decoder hands back macroblocks in the order the hardware did:
     * down each column of the frame, then on to the next column. */
    for (x = 0; x < width; x += 16) {
        for (y = 0; y < height; y += 16) {
            const unsigned char *src = (const unsigned char *)s_macroblock;
            int row;
            DecDCTout(s_macroblock, 192);
            for (row = 0; row < 16; row++) {
                int column;
                if (y + row >= height) break;
                for (column = 0; column < 16; column++) {
                    if (x + column >= width) break;
                    memcpy(s_pixels + (((size_t)(y + row) * s_width) +
                                       (size_t)(x + column)) * 3,
                           src + ((size_t)row * 16 + column) * 3, 3);
                }
            }
        }
    }
    return 1;
}

static int HostDecodeFmvFrame(void) {
    if (s_pixels == NULL || !RageDecodeFmvFrame()) return 0;
    s_frame++;
    if (RuntimeConfigEnabled("diagnostics.fmv_trace")) {
        fprintf(stderr, "fmv frame=%u vblank=%d scene_timer=%d sector=%u\n",
                s_frame - 1, g_FrameCounter, g_SceneTimer, s_sectorCursor);
    }
    /* Every movie has a few trailing frames the game never shows; the stream
     * table names the last one it does. */
    if (g_StreamSectorCount != 0 && s_frame >= g_StreamSectorCount) {
        g_FmvStreamEnded = 1;
        HostFmvAudioAllowTail();
    }
    return 1;
}

static int HostUploadFmvFrame(void) {
    return s_pixels != NULL &&
           Psyz_VideoUploadRgb24Frame(s_pixels, s_width, s_height);
}

static long ResolveFmvStreamIndex(long streamIndex) {
    const char *forced;
    long chosen;

    if (streamIndex < 0 || streamIndex >= RAGE_DISC_STREAM_COUNT) {
        streamIndex = 0;
    }

    /* Every movie but the opening one sits behind hours of play, so this puts
     * any of them where the opening one is asked for. */
    forced = RuntimeConfigGet("diagnostics.fmv_stream");
    if (forced == NULL || forced[0] == '\0') {
        return streamIndex;
    }

    chosen = strtol(forced, NULL, 10);
    if (chosen < 0 || chosen >= RAGE_DISC_STREAM_COUNT) {
        fprintf(stderr, "rage-port: diagnostics.fmv_stream %s is not 0 to 10\n",
                forced);
        return streamIndex;
    }

    g_StreamSectorCount = g_StreamCdEntries[chosen].size;
    fprintf(stderr, "rage-port: FMV stream forced to %ld\n", chosen);
    return chosen;
}

void StartFmvPlayback(void) {
    RECT clearRect;
    long streamIndex = HostFmvStreamIndex(
        g_StreamCdEntries, RAGE_DISC_STREAM_COUNT, g_StreamLoc);
    unsigned int firstSector;
    unsigned int sectorSpan;

    ReleaseFmvPixels();
    streamIndex = ResolveFmvStreamIndex(streamIndex);
    s_width = 320;
    s_height = streamIndex == RAGE_DISC_STREAM_COUNT - 1 ? 240 : 192;
    sectorSpan = HostStreamSectorSpan((int)streamIndex);
    firstSector = g_StreamCdEntries[streamIndex].position.sectorOffset;
    if (!HostExtractFmv(firstSector, sectorSpan)) {
        fprintf(stderr, "rage-port: could not extract FMV %ld from RAGE.STR\n",
                streamIndex);
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    s_pixels = calloc((size_t)s_width * (size_t)s_height, 3);
    if (s_pixels == NULL) {
        fprintf(stderr, "rage-port: could not allocate FMV %ld frame\n",
                streamIndex);
        ReleaseFmvBuffers();
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    clearRect.x = 0;
    clearRect.y = 0;
    clearRect.w = s_width * 3 / 2;
    clearRect.h = 480;
    ClearImage(&clearRect, 0, 0, 0);
    s_tickSectors = 0;
    s_frame = 0;
    /* Tests run the game as fast as the host manages, so there the stream is
     * paced off the vblank count instead of the clock on the wall. */
    s_wallClock = getenv("RAGE_PORT_TEST_MODE") == NULL;
    g_SceneTimer = 0;
    g_FmvStreamEnded = 0;
    g_FmvState = FMV_PLAYBACK_DECODE;
    g_FrameContexts[0].environment.draw.isbg = 0;
    g_FrameContexts[1].environment.draw.isbg = 0;
    SetDispMask(1);
    /* Every movie in RAGE.STR, including the opening movie, carries its own
     * interleaved XA stream.  The prologue CD-DA cue ends before playback and
     * must not be mistaken for the opening movie's soundtrack. */
    HostFmvAudioStart(firstSector, sectorSpan);
    if (!HostDecodeFmvFrame() || !HostUploadFmvFrame()) {
        fprintf(stderr, "rage-port: could not decode FMV %ld\n", streamIndex);
        g_FmvState = FMV_PLAYBACK_FINISH;
    }
    s_startNs = HostNanoseconds();
}

/* How much of the stream the drive would have delivered by now. */
static unsigned int FmvArrivedSectors(void) {
    if (s_wallClock) {
        unsigned long long elapsed = HostNanoseconds() - s_startNs;
        return (unsigned int)((elapsed * RAGE_STR_SECTORS_PER_SECOND) /
                              1000000000u);
    }
    s_tickSectors += RAGE_STR_SECTORS_PER_SECOND;
    return s_tickSectors / (unsigned int)TimingBaseHz();
}

void DecodeFmvFrame(void) {
    g_SceneTimer++;
    if (g_FmvStreamEnded || (g_PadPressed & PAD_START)) {
        if (g_PadPressed & PAD_START) StartCdVolumeFade(1);
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    {
        unsigned int arrived = FmvArrivedSectors();
        int decoded = 0;
        while (s_sectorCursor < arrived && !g_FmvStreamEnded) {
            if (!HostDecodeFmvFrame()) {
                g_FmvStreamEnded = 1;
                g_FmvState = FMV_PLAYBACK_FINISH;
                break;
            }
            decoded = 1;
        }
        /* A slow host can fall several movie frames behind. Decode all of
         * them to hold the cadence, but upload only the newest image. Reusing
         * one GPU transfer buffer several times in the same pending command
         * buffer lets later CPU writes overwrite data that earlier copies have
         * not consumed yet on some Vulkan drivers. */
        if (decoded && !HostUploadFmvFrame()) {
            g_FmvStreamEnded = 1;
            g_FmvState = FMV_PLAYBACK_FINISH;
        }
    }
}

void EndFmv(void) {
    HostFmvAudioEnd();
    ReleaseFmvBuffers();
    ReleaseFmvPixels();
    g_SceneId = g_StreamReturnScene;
    g_StreamReturnScene = g_FmvStreamEnded;
}
