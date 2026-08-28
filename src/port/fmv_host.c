#include <psyz/video.h>
#include <psyz/cd.h>
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
#include "game/cd.h"
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
static unsigned int s_frame;
static unsigned int s_sectorSpan;
static unsigned int s_tickSectors;
static Uint64 s_startNs;
static int s_wallClock;
static int s_xaPlaying;
static int s_xaTailAllowed;

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

enum {
    RAGE_CDL_SETLOC = 0x02, RAGE_CDL_READN = 0x06, RAGE_CDL_PAUSE = 0x09,
    RAGE_CDL_SETFILTER = 0x0d, RAGE_CDL_SETMODE = 0x0e,
    RAGE_CDL_MODE_DA = 0x01, RAGE_CDL_MODE_RT = 0x40
};

int HostReadStreamSector(unsigned int sector, unsigned char *raw);
int HostStreamAbsoluteSector(unsigned int sector);

/* The rest of the MDEC front end comes from game/render.h and psyq/cd.h;
 * only the VLC stage has no declaration there. */
int DecDCTvlc(u_long *bs, u_long *buf);

static const unsigned int s_sectorSpans[11] = {
    0x2F10,
    0x062D, 0x062D, 0x062D, 0x062D, 0x062D,
    0x062D, 0x062D, 0x062D, 0x062D,
    0x3B40,
};

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

/* Whether a movie's XA soundtrack is streaming. Asset loads pause CD audio,
 * because the console's drive cannot read data and play CD-DA at once; the
 * soundtrack interleaved into a movie is not CD-DA and must survive them. */
static void FinishXaAudio(void) {
    unsigned char mode = RAGE_CDL_MODE_DA | CdlModeSpeed;
    Psyz_CdSetXaEndSector(-1);
    CdControl(RAGE_CDL_SETMODE, &mode, NULL);
    s_xaPlaying = 0;
    s_xaTailAllowed = 0;
}

static void StopXaAudio(void) {
    if (s_xaPlaying) CdControl(RAGE_CDL_PAUSE, NULL, NULL);
    FinishXaAudio();
}

void HostFmvAudioTick(void) {
    if (s_xaPlaying && !Psyz_CdAudioPlaying()) {
        if (RuntimeConfigEnabled("diagnostics.fmv_trace",
                                     "RAGE_PORT_FMV_TRACE")) {
            fprintf(stderr, "fmv xa end\n");
        }
        FinishXaAudio();
    }
}

int FmvXaStreaming(void) {
    HostFmvAudioTick();
    return s_xaPlaying;
}

static int StartXaAudio(unsigned int firstSector,
                            unsigned int sectorCount) {
    unsigned char raw[2352], filter[2] = {0, 0};
    unsigned char mode = RAGE_CDL_MODE_RT | CdlModeSpeed;
    CdlLOC location;
    int absolute;
    unsigned int index;
    for (index = 0; index < 16; index++) {
        if (!HostReadStreamSector(firstSector + index, raw)) return 0;
        if ((raw[0x12] & 0x0e) == 0x04) {
            filter[0] = raw[0x10];
            filter[1] = raw[0x11];
            break;
        }
    }
    absolute = HostStreamAbsoluteSector(firstSector);
    if (index == 16 || absolute < 0) return 0;
    CdIntToPos(absolute, &location);
    if (RuntimeConfigEnabled("diagnostics.fmv_trace",
                                 "RAGE_PORT_FMV_TRACE")) {
        fprintf(stderr, "fmv xa start: sector=%d filter=%u/%u\n", absolute,
                filter[0], filter[1]);
    }
    CdControl(RAGE_CDL_SETFILTER, filter, NULL);
    CdControl(RAGE_CDL_SETMODE, &mode, NULL);
    CdControl(RAGE_CDL_SETLOC, (unsigned char *)&location, NULL);
    Psyz_CdSetXaEndSector(absolute + (int)sectorCount);
    CdControl(RAGE_CDL_READN, NULL, NULL);
    return Psyz_CdAudioPlaying();
}

static int HostDecodeFmvFrame(void) {
    if (s_pixels == NULL || !RageDecodeFmvFrame()) return 0;
    s_frame++;
    if (RuntimeConfigEnabled("diagnostics.fmv_trace",
                                 "RAGE_PORT_FMV_TRACE")) {
        fprintf(stderr, "fmv frame=%u vblank=%d scene_timer=%d sector=%u\n",
                s_frame - 1, g_FrameCounter, g_SceneTimer, s_sectorCursor);
    }
    /* Every movie has a few trailing frames the game never shows; the stream
     * table names the last one it does. */
    if (g_StreamSectorCount != 0 && s_frame >= g_StreamSectorCount) {
        g_FmvStreamEnded = 1;
        s_xaTailAllowed = 1;
    }
    return 1;
}

static int HostUploadFmvFrame(void) {
    return s_pixels != NULL &&
           Psyz_VideoUploadRgb24Frame(s_pixels, s_width, s_height);
}

void StartFmvPlayback(FmvWorkBuffers *buffers) {
    RECT clearRect;
    long streamIndex = g_StreamLoc - g_StreamCdEntries;
    unsigned int firstSector;

    (void)buffers;
    if (streamIndex < 0 || streamIndex > 10) streamIndex = 0;
    {
        /* Every movie but the opening one sits behind hours of play, so this
         * puts any of them where the opening one is asked for. */
        const char *forced = RuntimeConfigGet("diagnostics.fmv_stream");
        if (forced != NULL && forced[0] != '\0') {
            long chosen = strtol(forced, NULL, 10);
            if (chosen >= 0 && chosen <= 10) {
                streamIndex = chosen;
                g_StreamSectorCount = g_StreamCdEntries[streamIndex].size;
                fprintf(stderr, "rage-port: FMV stream forced to %ld\n", chosen);
            } else {
                fprintf(stderr,
                        "rage-port: diagnostics.fmv_stream %s is not 0 to 10\n",
                        forced);
            }
        }
    }
    s_width = 320;
    s_height = streamIndex == 10 ? 240 : 192;
    s_sectorSpan = s_sectorSpans[streamIndex];
    firstSector = g_StreamCdEntries[streamIndex].position.sectorOffset;
    if (!HostExtractFmv(firstSector, s_sectorSpan)) {
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
    s_xaTailAllowed = 0;
    s_xaPlaying = StartXaAudio(firstSector, s_sectorSpan);
    if (!HostDecodeFmvFrame() || !HostUploadFmvFrame()) {
        fprintf(stderr, "rage-port: could not decode FMV %ld\n", streamIndex);
        g_FmvState = FMV_PLAYBACK_FINISH;
    }
    s_startNs = SDL_GetTicksNS();
}

/* How much of the stream the drive would have delivered by now. */
static unsigned int FmvArrivedSectors(void) {
    if (s_wallClock) {
        Uint64 elapsed = SDL_GetTicksNS() - s_startNs;
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
    if (s_xaPlaying && !s_xaTailAllowed) {
        StopXaAudio();
    } else if (s_xaPlaying &&
               RuntimeConfigEnabled("diagnostics.fmv_trace",
                                        "RAGE_PORT_FMV_TRACE")) {
        fprintf(stderr, "fmv video end: xa tail continues\n");
    }
    ReleaseFmvBuffers();
    free(s_pixels);
    s_pixels = NULL;
    g_SceneId = g_StreamReturnScene;
    g_StreamReturnScene = g_FmvStreamEnded;
}
