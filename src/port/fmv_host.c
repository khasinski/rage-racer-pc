#include <psyz/video.h>
#include <libgpu.h>

#include <stdio.h>
#include <stdlib.h>

#include "game/asset.h"
#include "game/fmv.h"
#include "game/fmv_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

extern int g_FrameCounter;

static FILE *g_RageFmvPipe;
static unsigned char *g_RageFmvPixels;
static int g_RageFmvWidth;
static int g_RageFmvHeight;
static int g_RageFmvClock;
static unsigned int g_RageFmvFrame;
static unsigned int g_RageFmvSectorSpan;

/* RAGE.STR is read in double-speed Stream2 mode (150 sectors/second).  The
 * retail offsets delimit each multiplexed movie; its last ISO extent is
 * 41445 sectors.  Frame availability therefore differs by stream according
 * to compressed size instead of following the 15 fps tag on the extracted
 * convenience MP4 files. */
static const unsigned int g_RageFmvSectorSpans[11] = {
    0x2F10,
    0x062D, 0x062D, 0x062D, 0x062D, 0x062D,
    0x062D, 0x062D, 0x062D, 0x062D,
    0x3B40,
};

static int RageHostReadFmvFrame(void) {
    size_t bytes = (size_t)g_RageFmvWidth * (size_t)g_RageFmvHeight * 3;
    if (g_RageFmvPipe == NULL || fread(g_RageFmvPixels, 1, bytes, g_RageFmvPipe) != bytes) {
        return 0;
    }
    if (!Psyz_VideoUploadRgb24Frame(
            g_RageFmvPixels, g_RageFmvWidth, g_RageFmvHeight)) {
        return 0;
    }
    g_RageFmvFrame++;
    if (getenv("RAGE_PORT_FMV_TRACE") != NULL) {
        fprintf(stderr, "fmv frame=%u vblank=%d scene_timer=%d\n",
                g_RageFmvFrame - 1, g_FrameCounter, g_SceneTimer);
    }
    if (g_StreamSectorCount != 0 && g_RageFmvFrame >= g_StreamSectorCount) {
        g_FmvStreamEnded = 1;
    }
    return 1;
}

void StartFmvPlayback(FmvWorkBuffers *buffers) {
    char command[256];
    RECT clearRect;
    long streamIndex = g_StreamLoc - g_StreamCdEntries;
    const char *path;

    (void)buffers;
    if (streamIndex < 0 || streamIndex > 10) streamIndex = 0;
    g_RageFmvWidth = 320;
    g_RageFmvHeight = streamIndex == 10 ? 240 : 192;
    g_RageFmvSectorSpan = g_RageFmvSectorSpans[streamIndex];
    path = "assets/PAL/fmv";
    snprintf(command, sizeof(command),
             "ffmpeg -v error -i %s/fmv%02ld.mp4 -f rawvideo -pix_fmt rgb24 - 2>/dev/null",
             path, streamIndex);
    g_RageFmvPixels = malloc(
        (size_t)g_RageFmvWidth * (size_t)g_RageFmvHeight * 3);
    g_RageFmvPipe = g_RageFmvPixels ? popen(command, "r") : NULL;
    clearRect.x = 0;
    clearRect.y = 0;
    clearRect.w = g_RageFmvWidth * 3 / 2;
    clearRect.h = 480;
    ClearImage(&clearRect, 0, 0, 0);
    g_RageFmvClock = 0;
    g_RageFmvFrame = 0;
    g_SceneTimer = 0;
    g_FmvStreamEnded = 0;
    g_FmvState = FMV_PLAYBACK_DECODE;
    g_FrameContexts[0].environment.draw.isbg = 0;
    g_FrameContexts[1].environment.draw.isbg = 0;
    SetDispMask(1);
    if (!RageHostReadFmvFrame()) g_FmvState = FMV_PLAYBACK_FINISH;
}

void DecodeFmvFrame(void) {
    g_SceneTimer++;
    if (g_FmvStreamEnded) {
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    if (g_PadPressed & PAD_START) {
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    /* One PAL VBlank advances the double-speed CD by three sectors.  Keep the
     * accumulator in sector*frame units so no host wall clock is involved. */
    g_RageFmvClock += 3 * (int)g_StreamSectorCount;
    if ((unsigned int)g_RageFmvClock >= g_RageFmvSectorSpan) {
        g_RageFmvClock -= (int)g_RageFmvSectorSpan;
        if (!RageHostReadFmvFrame()) {
            g_FmvStreamEnded = 1;
            g_FmvState = FMV_PLAYBACK_FINISH;
        }
    }
}

void EndFmv(void) {
    if (g_RageFmvPipe != NULL) {
        pclose(g_RageFmvPipe);
        g_RageFmvPipe = NULL;
    }
    free(g_RageFmvPixels);
    g_RageFmvPixels = NULL;
    g_SceneId = g_StreamReturnScene;
    g_StreamReturnScene = g_FmvStreamEnded;
}
