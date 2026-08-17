#include <psyz/video.h>
#include <libgpu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
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

extern int g_FrameCounter;

static FILE *s_pipe;
static unsigned char *s_pixels;
static int s_width;
static int s_height;
static int s_clock;
static unsigned int s_frame;
static unsigned int s_sectorSpan;
static unsigned int s_cadenceNumerator;
static unsigned int s_cadenceDenominator;
static char s_streamPath[1024];
static int s_xaPlaying;
#ifdef _WIN32
static HANDLE s_ffmpegProcess;
#else
static pid_t s_ffmpegProcess = -1;
#endif

enum {
    RAGE_CDL_SETLOC = 0x02, RAGE_CDL_READN = 0x06, RAGE_CDL_PAUSE = 0x09,
    RAGE_CDL_SETFILTER = 0x0d, RAGE_CDL_SETMODE = 0x0e,
    RAGE_CDL_MODE_RT = 0x40
};

int RageHostReadStreamSector(unsigned int sector, unsigned char *raw);
int RageHostStreamAbsoluteSector(unsigned int sector);

static const unsigned int s_sectorSpans[11] = {
    0x2F10,
    0x062D, 0x062D, 0x062D, 0x062D, 0x062D,
    0x062D, 0x062D, 0x062D, 0x062D,
    0x3B40,
};

static int RageHostExtractFmv(unsigned int first, unsigned int count) {
    unsigned char raw[2352];
    FILE *file;
    unsigned int index;
#ifdef _WIN32
    char temporary[768];
    if (!RagePlatformTemporaryDirectory(temporary, sizeof(temporary))) return 0;
    if (GetTempFileNameA(temporary, "RGR", 0, s_streamPath) == 0) return 0;
    file = fopen(s_streamPath, "wb");
#else
    char temporary[768];
    if (!RagePlatformTemporaryDirectory(temporary, sizeof(temporary))) return 0;
    if (snprintf(s_streamPath, sizeof(s_streamPath),
                 "%s/rage-racer-XXXXXX", temporary) >=
        (int)sizeof(s_streamPath)) return 0;
    {
        int descriptor = mkstemp(s_streamPath);
        if (descriptor < 0) return 0;
        file = fdopen(descriptor, "wb");
        if (file == NULL) close(descriptor);
    }
#endif
    if (file == NULL) {
        remove(s_streamPath);
        s_streamPath[0] = '\0';
        return 0;
    }
    for (index = 0; index < count; index++) {
        if (!RageHostReadStreamSector(first + index, raw) ||
            fwrite(raw, 1, sizeof(raw), file) != sizeof(raw)) {
            fclose(file);
            remove(s_streamPath);
            s_streamPath[0] = '\0';
            return 0;
        }
    }
    if (fclose(file) != 0) {
        remove(s_streamPath);
        s_streamPath[0] = '\0';
        return 0;
    }
    return 1;
}

static FILE *RageOpenFfmpeg(const char *path) {
#ifdef _WIN32
    SECURITY_ATTRIBUTES security = {sizeof(security), NULL, TRUE};
    PROCESS_INFORMATION process;
    STARTUPINFOA startup;
    HANDLE readPipe, writePipe, nullOutput;
    char command[1400];
    int descriptor;
    if (!CreatePipe(&readPipe, &writePipe, &security, 0)) return NULL;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    nullOutput = CreateFileA("NUL", GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = writePipe;
    startup.hStdError = nullOutput;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    snprintf(command, sizeof(command),
             "ffmpeg -v error -i \"%s\" -f rawvideo -pix_fmt rgb24 -", path);
    if (!CreateProcessA(NULL, command, NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL, &startup, &process)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        if (nullOutput != INVALID_HANDLE_VALUE) CloseHandle(nullOutput);
        return NULL;
    }
    CloseHandle(writePipe);
    if (nullOutput != INVALID_HANDLE_VALUE) CloseHandle(nullOutput);
    CloseHandle(process.hThread);
    s_ffmpegProcess = process.hProcess;
    descriptor = _open_osfhandle((intptr_t)readPipe, _O_RDONLY | _O_BINARY);
    if (descriptor < 0) {
        CloseHandle(readPipe);
        TerminateProcess(s_ffmpegProcess, 1);
        WaitForSingleObject(s_ffmpegProcess, INFINITE);
        CloseHandle(s_ffmpegProcess);
        s_ffmpegProcess = NULL;
        return NULL;
    }
    return _fdopen(descriptor, "rb");
#else
    int output[2];
    pid_t child;
    if (pipe(output) != 0) return NULL;
    child = fork();
    if (child < 0) {
        close(output[0]);
        close(output[1]);
        return NULL;
    }
    if (child == 0) {
        int nullOutput = open("/dev/null", O_WRONLY);
        dup2(output[1], STDOUT_FILENO);
        if (nullOutput >= 0) dup2(nullOutput, STDERR_FILENO);
        close(output[0]);
        close(output[1]);
        if (nullOutput >= 0) close(nullOutput);
        execlp("ffmpeg", "ffmpeg", "-v", "error", "-i", path,
               "-f", "rawvideo", "-pix_fmt", "rgb24", "-", (char *)NULL);
        _exit(127);
    }
    close(output[1]);
    s_ffmpegProcess = child;
    s_pipe = fdopen(output[0], "rb");
    if (s_pipe == NULL) close(output[0]);
    return s_pipe;
#endif
}

static int RageCloseFfmpeg(void) {
    int success = 1;
    if (s_pipe != NULL) {
        fclose(s_pipe);
        s_pipe = NULL;
    }
#ifdef _WIN32
    if (s_ffmpegProcess != NULL) {
        DWORD code = 1;
        WaitForSingleObject(s_ffmpegProcess, INFINITE);
        GetExitCodeProcess(s_ffmpegProcess, &code);
        CloseHandle(s_ffmpegProcess);
        s_ffmpegProcess = NULL;
        success = code == 0;
    }
#else
    if (s_ffmpegProcess > 0) {
        int status;
        success = waitpid(s_ffmpegProcess, &status, 0) == s_ffmpegProcess &&
                  WIFEXITED(status) && WEXITSTATUS(status) == 0;
        s_ffmpegProcess = -1;
    }
#endif
    return success;
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

static int RageHostReadFmvFrame(void) {
    size_t bytes = (size_t)s_width * (size_t)s_height * 3;
    if (s_pipe == NULL || fread(s_pixels, 1, bytes, s_pipe) != bytes) return 0;
    if (!Psyz_VideoUploadRgb24Frame(s_pixels, s_width, s_height)) return 0;
    s_frame++;
    if (RageRuntimeConfigEnabled("diagnostics.fmv_trace",
                                 "RAGE_PORT_FMV_TRACE")) {
        fprintf(stderr, "fmv frame=%u vblank=%d scene_timer=%d\n",
                s_frame - 1, g_FrameCounter, g_SceneTimer);
    }
    if (g_StreamSectorCount != 0 && s_frame >= g_StreamSectorCount) {
        g_FmvStreamEnded = 1;
    }
    return 1;
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
        s_cadenceNumerator = 53;
        s_cadenceDenominator = 128;
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
    s_pipe = s_pixels != NULL ? RageOpenFfmpeg(s_streamPath) : NULL;
    clearRect.x = 0;
    clearRect.y = 0;
    clearRect.w = s_width * 3 / 2;
    clearRect.h = 480;
    ClearImage(&clearRect, 0, 0, 0);
    s_clock = streamIndex == 0 ? -10 * (int)s_cadenceNumerator : 0;
    s_frame = 0;
    g_SceneTimer = 0;
    g_FmvStreamEnded = 0;
    g_FmvState = FMV_PLAYBACK_DECODE;
    g_FrameContexts[0].environment.draw.isbg = 0;
    g_FrameContexts[1].environment.draw.isbg = 0;
    SetDispMask(1);
    /* The opening movie is accompanied by the separately selected prologue
     * CD-DA track. The remaining STR movies carry their own XA stream. */
    s_xaPlaying = streamIndex != 0;
    if (s_xaPlaying) RageStartXaAudio(firstSector);
    if (!RageHostReadFmvFrame()) {
        fprintf(stderr, "rage-port: FFmpeg could not decode FMV %ld\n", streamIndex);
        g_FmvState = FMV_PLAYBACK_FINISH;
    }
}

void DecodeFmvFrame(void) {
    g_SceneTimer++;
    if (g_FmvStreamEnded || (g_PadPressed & PAD_START)) {
        g_FmvState = FMV_PLAYBACK_FINISH;
        return;
    }
    s_clock += (int)s_cadenceNumerator;
    if (s_clock >= (int)s_cadenceDenominator) {
        s_clock -= (int)s_cadenceDenominator;
        if (!RageHostReadFmvFrame()) {
            g_FmvStreamEnded = 1;
            g_FmvState = FMV_PLAYBACK_FINISH;
        }
    }
}

void EndFmv(void) {
    if (s_xaPlaying) {
        CdControl(RAGE_CDL_PAUSE, NULL, NULL);
        s_xaPlaying = 0;
    }
    if (!RageCloseFfmpeg())
        fprintf(stderr, "rage-port: FFmpeg exited with an error\n");
    free(s_pixels);
    s_pixels = NULL;
    if (s_streamPath[0] != '\0') {
        remove(s_streamPath);
        s_streamPath[0] = '\0';
    }
    g_SceneId = g_StreamReturnScene;
    g_StreamReturnScene = g_FmvStreamEnded;
}
