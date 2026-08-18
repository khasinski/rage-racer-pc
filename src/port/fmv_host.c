#include <psyz/video.h>
#include <libgpu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL_timer.h>

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
#include "game/fmv.h"
#include "game/fmv_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "psyq/cd.h"
#include "runtime_config.h"
#include "platform_paths.h"
#include "timing_control.h"

extern int g_FrameCounter;

static unsigned char *s_pixels;
static int s_width;
static int s_height;
static int s_clock;
static unsigned int s_frame;
static unsigned int s_sectorSpan;
static unsigned int s_cadenceNumerator;
static unsigned int s_cadenceDenominator;
static Uint64 s_introStartNs;
static int s_wallClockIntro;
static int s_xaPlaying;

/* RAGE.STR plays at 30 frames per second. The format carries no frame rate,
 * so it has to be derived from the disc: one sector in eight is XA audio, and
 * XA level B stereo consumes 18.75 sectors per second, which pins the stream
 * to 150 sectors per second (double speed). Frames span five sectors, so
 * 150/5 = 30. */
#define RAGE_FMV_FPS 30

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

enum {
    RAGE_CDL_SETLOC = 0x02, RAGE_CDL_READN = 0x06, RAGE_CDL_PAUSE = 0x09,
    RAGE_CDL_SETFILTER = 0x0d, RAGE_CDL_SETMODE = 0x0e,
    RAGE_CDL_MODE_DA = 0x01, RAGE_CDL_MODE_RT = 0x40
};

int RageHostReadStreamSector(unsigned int sector, unsigned char *raw);
int RageHostStreamAbsoluteSector(unsigned int sector);

/* The rest of the MDEC front end comes from game/render.h and psyq/cd.h;
 * only the VLC stage has no declaration there. */
int DecDCTvlc(u_long *bs, u_long *buf);

static const unsigned int s_sectorSpans[11] = {
    0x2F10,
    0x062D, 0x062D, 0x062D, 0x062D, 0x062D,
    0x062D, 0x062D, 0x062D, 0x062D,
    0x3B40,
};

static void RageReleaseFmvBuffers(void) {
    free(s_sectors);
    s_sectors = NULL;
    free(s_bitstream);
    s_bitstream = NULL;
    free(s_codes);
    s_codes = NULL;
    s_sectorCount = 0;
    s_sectorCursor = 0;
}

static int RageHostExtractFmv(unsigned int first, unsigned int count) {
    unsigned int index;
    RageReleaseFmvBuffers();
    if (count == 0) return 0;
    s_sectors = malloc((size_t)count * RAGE_STR_SECTOR_SIZE);
    s_bitstream = malloc(RAGE_BS_MAX);
    s_codes = malloc((size_t)RAGE_CODES_MAX * 2 + 64);
    if (s_sectors == NULL || s_bitstream == NULL || s_codes == NULL) {
        RageReleaseFmvBuffers();
        return 0;
    }
    for (index = 0; index < count; index++) {
        if (!RageHostReadStreamSector(
                first + index,
                s_sectors + (size_t)index * RAGE_STR_SECTOR_SIZE)) {
            RageReleaseFmvBuffers();
            return 0;
        }
    }
    s_sectorCount = count;
    s_sectorCursor = 0;
    return 1;
}

static unsigned int RageReadLe16(const unsigned char *data) {
    return (unsigned int)data[0] | ((unsigned int)data[1] << 8);
}

static unsigned int RageReadLe32(const unsigned char *data) {
    return RageReadLe16(data) | (RageReadLe16(data + 2) << 16);
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
        if (RageReadLe32(body) != RAGE_STR_MAGIC) continue;
        chunk = RageReadLe16(body + 4);
        if (size == 0 && chunk != 0) continue; /* resynchronise on a frame */
        if (chunk == 0) {
            size = 0;
            seen = 0;
            chunks = RageReadLe16(body + 6);
            width = (int)RageReadLe16(body + 16);
            height = (int)RageReadLe16(body + 18);
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
    DecDCTin((volatile u32 *)s_codes, 3);

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
                    memcpy(s_pixels + (((size_t)(y + row) * width) +
                                       (size_t)(x + column)) * 3,
                           src + ((size_t)row * 16 + column) * 3, 3);
                }
            }
        }
    }
    return 1;
}

static void RageStartXaAudio(unsigned int firstSector) {
    unsigned char raw[2352], filter[2] = {0, 0};
    unsigned char mode = RAGE_CDL_MODE_RT | CdlModeSpeed;
    CdlLOC location;
    int absolute;
    unsigned int index;
    for (index = 0; index < 16; index++) {
        if (!RageHostReadStreamSector(firstSector + index, raw)) return;
        if ((raw[0x12] & 0x0e) == 0x04) {
            filter[0] = raw[0x10];
            filter[1] = raw[0x11];
            break;
        }
    }
    absolute = RageHostStreamAbsoluteSector(firstSector);
    if (index == 16 || absolute < 0) return;
    CdIntToPos(absolute, &location);
    CdControl(RAGE_CDL_SETFILTER, filter, NULL);
    CdControl(RAGE_CDL_SETMODE, &mode, NULL);
    CdControl(RAGE_CDL_SETLOC, (unsigned char *)&location, NULL);
    CdControl(RAGE_CDL_READN, NULL, NULL);
}

static int RageHostDecodeFmvFrame(void) {
    if (s_pixels == NULL || !RageDecodeFmvFrame()) return 0;
    s_frame++;
    if (RageRuntimeConfigEnabled("diagnostics.fmv_trace",
                                 "RAGE_PORT_FMV_TRACE")) {
        fprintf(stderr, "fmv frame=%u vblank=%d scene_timer=%d\n",
                s_frame - 1, g_FrameCounter, g_SceneTimer);
    }
    if (!s_wallClockIntro && g_StreamSectorCount != 0 &&
        s_frame >= g_StreamSectorCount) {
        g_FmvStreamEnded = 1;
    }
    return 1;
}

static int RageHostUploadFmvFrame(void) {
    return s_pixels != NULL &&
           Psyz_VideoUploadRgb24Frame(s_pixels, s_width, s_height);
}

void StartFmvPlayback(FmvWorkBuffers *buffers) {
    RECT clearRect;
    long streamIndex = g_StreamLoc - g_StreamCdEntries;
    unsigned int firstSector;

    (void)buffers;
    if (streamIndex < 0 || streamIndex > 10) streamIndex = 0;
    s_width = 320;
    s_height = streamIndex == 10 ? 240 : 192;
    s_sectorSpan = s_sectorSpans[streamIndex];
    firstSector = g_StreamCdEntries[streamIndex].position.sectorOffset;
    if (streamIndex == 0) {
        /* Frames advanced per vblank: 30/50 on PAL, 30/60 on NTSC. */
        s_cadenceNumerator = RageTimingGetStandard() == RAGE_TIMING_PAL ? 3 : 1;
        s_cadenceDenominator = RageTimingGetStandard() == RAGE_TIMING_PAL ? 5 : 2;
    } else {
        s_cadenceNumerator = 3 * g_StreamSectorCount;
        s_cadenceDenominator = s_sectorSpan;
    }
    if (!RageHostExtractFmv(firstSector, s_sectorSpan)) {
        fprintf(stderr, "rage-port: could not extract FMV %ld from RAGE.STR\n",
                streamIndex);
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    s_pixels = malloc((size_t)s_width * (size_t)s_height * 3);
    clearRect.x = 0;
    clearRect.y = 0;
    clearRect.w = s_width * 3 / 2;
    clearRect.h = 480;
    ClearImage(&clearRect, 0, 0, 0);
    s_clock = streamIndex == 0 ? -10 * (int)s_cadenceNumerator : 0;
    s_frame = 0;
    s_wallClockIntro = streamIndex == 0 && getenv("RAGE_PORT_TEST_MODE") == NULL;
    g_SceneTimer = 0;
    g_FmvStreamEnded = 0;
    g_FmvState = FMV_PLAYBACK_DECODE;
    g_FrameContexts[0].environment.draw.isbg = 0;
    g_FrameContexts[1].environment.draw.isbg = 0;
    SetDispMask(1);
    /* Every movie in RAGE.STR, including the opening movie, carries its own
     * interleaved XA stream.  The prologue CD-DA cue ends before playback and
     * must not be mistaken for the opening movie's soundtrack. */
    s_xaPlaying = 1;
    RageStartXaAudio(firstSector);
    if (!RageHostDecodeFmvFrame() || !RageHostUploadFmvFrame()) {
        fprintf(stderr, "rage-port: could not decode FMV %ld\n", streamIndex);
        g_FmvState = FMV_PLAYBACK_FINISH;
    }
    s_introStartNs = SDL_GetTicksNS();
}

void DecodeFmvFrame(void) {
    g_SceneTimer++;
    if (g_FmvStreamEnded || (g_PadPressed & PAD_START)) {
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    if (s_wallClockIntro) {
        int decoded = 0;
        Uint64 elapsed = SDL_GetTicksNS() - s_introStartNs;
        unsigned int target =
            1u + (unsigned int)((elapsed * RAGE_FMV_FPS) / 1000000000u);
        while (s_frame < target) {
            if (!RageHostDecodeFmvFrame()) {
                g_FmvStreamEnded = 1;
                g_FmvState = FMV_PLAYBACK_FINISH;
                break;
            }
            decoded = 1;
        }
        /* A slow host can fall several movie frames behind. Decode all of
         * them to preserve wall-clock cadence, but upload only the newest
         * image. Reusing one GPU transfer buffer several times in the same
         * pending command buffer lets later CPU writes overwrite data that
         * earlier copies have not consumed yet on some Vulkan drivers. */
        if (decoded && !RageHostUploadFmvFrame()) {
            g_FmvStreamEnded = 1;
            g_FmvState = FMV_PLAYBACK_FINISH;
        }
        return;
    }
    s_clock += (int)s_cadenceNumerator;
    if (s_clock >= (int)s_cadenceDenominator) {
        s_clock -= (int)s_cadenceDenominator;
        if (!RageHostDecodeFmvFrame() || !RageHostUploadFmvFrame()) {
            g_FmvStreamEnded = 1;
            g_FmvState = FMV_PLAYBACK_FINISH;
        }
    }
}

void EndFmv(void) {
    if (s_xaPlaying) {
        unsigned char mode = RAGE_CDL_MODE_DA | CdlModeSpeed;
        CdControl(RAGE_CDL_PAUSE, NULL, NULL);
        /* XA playback temporarily changes the drive mode.  CD music requests
         * use CdPlay and rely on the normal CD-DA mode being restored. */
        CdControl(RAGE_CDL_SETMODE, &mode, NULL);
        s_xaPlaying = 0;
    }
    RageReleaseFmvBuffers();
    free(s_pixels);
    s_pixels = NULL;
    g_SceneId = g_StreamReturnScene;
    g_StreamReturnScene = g_FmvStreamEnded;
}
