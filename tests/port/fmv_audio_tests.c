#include "fmv_audio.h"

#include "psyq/cd.h"

#include <stdio.h>
#include <string.h>

enum {
    CD_SET_LOCATION = 0x02,
    CD_READ_NORMAL = 0x06,
    CD_PAUSE = 0x09,
    CD_SET_FILTER = 0x0D,
    CD_SET_MODE = 0x0E,
};

static int s_audioPlaying;
static int s_audioSector = -1;
static int s_absoluteSector = 1000;
static int s_failReadAt = -1;
static int s_readCalls;
static long s_commands[16];
static int s_commandCount;
static int s_lastXaEndSector;

int HostReadStreamSector(unsigned int sector, unsigned char *raw) {
    int relative = (int)sector - 50;

    s_readCalls++;
    if (relative == s_failReadAt) {
        return 0;
    }
    memset(raw, 0, 2352);
    if (relative == s_audioSector) {
        raw[0x10] = 3;
        raw[0x11] = 7;
        raw[0x12] = 4;
    }
    return 1;
}

int HostStreamAbsoluteSector(unsigned int sector) {
    (void)sector;
    return s_absoluteSector;
}

int RuntimeConfigEnabled(const char *key) {
    (void)key;
    return 0;
}

void Psyz_CdSetXaEndSector(int sector) {
    s_lastXaEndSector = sector;
}

int Psyz_CdAudioPlaying(void) {
    return s_audioPlaying;
}

long CdControl(long command, void *parameter, u_char *result) {
    (void)parameter;
    (void)result;
    s_commands[s_commandCount++] = command;
    return 1;
}

CdlLOC *CdIntToPos(long sector, CdlLOC *location) {
    memset(location, 0, sizeof(*location));
    location->sector = (u8)sector;
    return location;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    s_audioPlaying = 0;
    s_audioSector = -1;
    s_absoluteSector = 1000;
    s_failReadAt = -1;
    s_readCalls = 0;
    s_commandCount = 0;
    s_lastXaEndSector = -2;
    memset(s_commands, 0, sizeof(s_commands));
}

static int TestStartAndStop(void) {
    Reset();
    s_audioSector = 2;
    s_audioPlaying = 1;

    HostFmvAudioStart(50, 20);

    CHECK(s_readCalls == 3);
    CHECK(s_commandCount == 4);
    CHECK(s_commands[0] == CD_SET_FILTER && s_commands[1] == CD_SET_MODE);
    CHECK(s_commands[2] == CD_SET_LOCATION &&
          s_commands[3] == CD_READ_NORMAL);
    CHECK(s_lastXaEndSector == 1020);
    CHECK(FmvXaStreaming());

    HostFmvAudioEnd();
    CHECK(s_commands[4] == CD_PAUSE && s_commands[5] == CD_SET_MODE);
    CHECK(s_lastXaEndSector == -1);
    CHECK(!FmvXaStreaming());
    return 0;
}

static int TestAllowedTail(void) {
    Reset();
    s_audioSector = 0;
    s_audioPlaying = 1;
    HostFmvAudioStart(50, 4);
    HostFmvAudioAllowTail();
    s_commandCount = 0;

    HostFmvAudioEnd();
    CHECK(s_commandCount == 0 && FmvXaStreaming());

    s_audioPlaying = 0;
    HostFmvAudioTick();
    CHECK(s_commandCount == 1 && s_commands[0] == CD_SET_MODE);
    CHECK(s_lastXaEndSector == -1 && !FmvXaStreaming());
    return 0;
}

static int TestMissingAudio(void) {
    Reset();
    HostFmvAudioStart(50, 2);
    CHECK(s_readCalls == 2);
    CHECK(s_commandCount == 1 && s_commands[0] == CD_SET_MODE);
    CHECK(s_lastXaEndSector == -1 && !FmvXaStreaming());

    Reset();
    HostFmvAudioStart(50, 0);
    CHECK(s_readCalls == 0);
    CHECK(s_commandCount == 1 && s_commands[0] == CD_SET_MODE);

    Reset();
    s_failReadAt = 0;
    HostFmvAudioStart(50, 20);
    CHECK(s_readCalls == 1);
    CHECK(s_commandCount == 1 && s_commands[0] == CD_SET_MODE);
    return 0;
}

int main(void) {
    CHECK(TestStartAndStop() == 0);
    CHECK(TestAllowedTail() == 0);
    CHECK(TestMissingAudio() == 0);
    puts("FMV XA audio starts, tails, stops, and recovers from missing data");
    return 0;
}
