#include <libetc.h>
#include <libgte.h>

#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libapi.h>
#include <psyz.h>
#include <psyz/cd.h>

#include "psyq/cd_types.h"
#include "game/scratchpad.h"

extern CdlLOC *CdIntToPos(int sector, CdlLOC *position);

/* Host storage for values which lived in the PS1 scratchpad or were aliases. */
unsigned char g_AudioRuntimeState[4096];
int g_CourseSelectScrollValue;
int g_McConfirmChoice_v;
unsigned char g_RageScratchpad[0x400];
GameScratchpadRenderState g_RageScratchpadState;

void UpdateBootLogoScene(void);
void EnterFrontend(void);
void EnterTitleScreen(void);
void UpdateFrontend(void);
void UpdateFmv(void);
void UpdateTitleScreen(void);
void UpdateMainMenuOpen(void);
void UpdateMainMenuInput(void);
void UpdateMainMenuExit(void);
void EnterCourseSelectScreen(void);
void UpdateCourseSelectScreen(void);
void UpdateRankingScreen(void);
void EnterCarSelectScreen(void);
void UpdateCarSelectScreen(void);
void UpdateCustomizeScreen(void);
void UpdateDesignModeScreen(void);
void UpdateTeamLogoScreen(void);
void UpdateLogoSampleScreen(void);
void UpdateTeamNameScreen(void);
void UpdatePaintColorScreen(void);
void UpdateCarShopScreen(void);
void UpdateEngineerShopScreen(void);
void ShopScreenNoOp(void);
int DrawCourseSelectScreen(int step);
int DrawRankingScreen(int step);
int DrawCarSelectScreen(int step);
int DrawCustomizeScreen(int step);
int DrawDesignModeScreen(int step);
int DrawTeamLogoScreen(int step);
int DrawLogoSampleScreen(int step);
int DrawTeamNameScreen(int step);
int DrawPaintColorScreen(int step);
int DrawCarShopScreen(int step);
unsigned DrawEngineerShopScreen(int step);
void InitMenuMode(void);
void ReturnFromClassFmv(void);
void UpdateMenuMode(void);
void EnterRoundScreen(void);
void UpdateRoundScreen(void);
void EnterRaceScene(void);
void UpdateRaceScene(void);
void EnterLostRaceScreen(void);
void UpdateLostRaceScreen(void);
void EnterRaceEndScreen(void);
void UpdateRaceEndScreen(void);
void UpdateReplayScene(void);
void EnterPrizeScreen(void);
void UpdatePrizeMoneyScreen(void);
void EnterRecordEntry(void);
void UpdateRecordEntry(void);
void EnterAttractScene(void);
void UpdateOptionScene(void);
void EnterMemoryCardMenu(void);
void EnterMemoryCardMenuFromLoad(void);
void UpdateMemoryCardMenu(void);
void EnterBgmSelectScreen(void);
void UpdateBgmSelectScene(void);
void EnterAttractDemo(void);
void UpdateAttractDemoScene(void);
void EnterPrologue(void);
void TickPrologueStep(void);
void ReturnFromEndingFmv(void);
void UpdateEndingStill(void);
void UpdatePrologueLoadStep0(void);
void UpdatePrologueLoadStep1(void);
void UpdatePrologueLoadStep2(void);
void UpdatePrologue(void);
void (*g_PrologueSteps[])(void) = {
    UpdatePrologueLoadStep0,
    UpdatePrologueLoadStep1,
    UpdatePrologueLoadStep2,
    UpdatePrologue,
};
void (*g_SceneHandlers[40])(void) = {
    [1] = UpdateBootLogoScene,
    [2] = EnterFrontend,
    [3] = EnterTitleScreen,
    [4] = UpdateFrontend,
    [5] = UpdateFmv,
    [6] = InitMenuMode,
    [7] = ReturnFromClassFmv,
    [8] = UpdateMenuMode,
    [9] = EnterRoundScreen,
    [10] = UpdateRoundScreen,
    [11] = EnterRaceScene,
    [12] = UpdateRaceScene,
    [13] = EnterLostRaceScreen,
    [14] = UpdateLostRaceScreen,
    [15] = EnterRaceEndScreen,
    [16] = UpdateRaceEndScreen,
    [17] = UpdateReplayScene,
    [18] = EnterPrizeScreen,
    [19] = UpdatePrizeMoneyScreen,
    [20] = EnterRecordEntry,
    [21] = UpdateRecordEntry,
    [22] = EnterAttractScene,
    [23] = UpdateOptionScene,
    [24] = EnterMemoryCardMenu,
    [25] = EnterMemoryCardMenuFromLoad,
    [26] = UpdateMemoryCardMenu,
    [27] = EnterBgmSelectScreen,
    [28] = UpdateBgmSelectScene,
    [29] = EnterAttractDemo,
    [30] = UpdateAttractDemoScene,
    [31] = EnterPrologue,
    [32] = TickPrologueStep,
    [33] = ReturnFromEndingFmv,
    [34] = UpdateEndingStill,
};
void (*g_FrontendDrawHandlers[4])(void) = {
    UpdateTitleScreen,
    UpdateMainMenuOpen,
    UpdateMainMenuInput,
    UpdateMainMenuExit,
};

static s32 RageDrawShopScreenNoOp(s32 step) {
    (void)step;
    ShopScreenNoOp();
    return 0;
}

/* Retail main.exe tables at 0x80082EB8 and 0x80082EF0.  They contain code
 * addresses, so copying their 32-bit words into native storage is invalid. */
void (*g_MenuScreenUpdate[14])(void) = {
    EnterCourseSelectScreen,
    UpdateCourseSelectScreen,
    UpdateRankingScreen,
    EnterCarSelectScreen,
    UpdateCarSelectScreen,
    UpdateCustomizeScreen,
    UpdateDesignModeScreen,
    UpdateTeamLogoScreen,
    UpdateLogoSampleScreen,
    UpdateTeamNameScreen,
    UpdatePaintColorScreen,
    UpdateCarShopScreen,
    UpdateEngineerShopScreen,
    ShopScreenNoOp,
};
s32 (*g_MenuScreenDraw[14])(s32) = {
    RageDrawShopScreenNoOp,
    DrawCourseSelectScreen,
    DrawRankingScreen,
    RageDrawShopScreenNoOp,
    DrawCarSelectScreen,
    DrawCustomizeScreen,
    DrawDesignModeScreen,
    DrawTeamLogoScreen,
    DrawLogoSampleScreen,
    DrawTeamNameScreen,
    DrawPaintColorScreen,
    DrawCarShopScreen,
    (s32 (*)(s32))DrawEngineerShopScreen,
    RageDrawShopScreenNoOp,
};

int RageMapPs1Scratchpad(void) {
    memset(g_RageScratchpad, 0, sizeof(g_RageScratchpad));
    memset(&g_RageScratchpadState, 0, sizeof(g_RageScratchpadState));
    return 1;
}

int RageHostInitDisc(void) {
    const char *cue = getenv("RAGE_PORT_DISC_CUE");
    if (cue == NULL || cue[0] == '\0')
        cue = "disc/PAL/Rage Racer (Europe).cue";
    if (access(cue, R_OK) != 0) return 1;
    return Psyz_CdSetDiskPath(cue) == 0;
}

typedef struct RageHostCdEntry {
    uint32_t byte_offset;
    uint32_t size;
} RageHostCdEntry;

int RageHostLoadArchiveIndex(void *entries_ptr, int count) {
    RageHostCdEntry *entries = entries_ptr;
    uint32_t words[270];
    FILE *file = fopen("assets/PAL/RAGE.BIN", "rb");
    int index;
    if (!file || count > 135 || fread(words, 1, sizeof(words), file) != sizeof(words)) {
        if (file) fclose(file);
        return 0;
    }
    fclose(file);
    for (index = 0; index < count; index++) {
        entries[index].byte_offset = words[index * 2] * 2048u;
        entries[index].size = words[index * 2 + 1];
    }
    return 1;
}

int RageHostLoadAsset(unsigned int byte_offset, unsigned int size, void *destination) {
    FILE *file = fopen("assets/PAL/RAGE.BIN", "rb");
    size_t loaded;
    if (!file || fseek(file, (long)byte_offset, SEEK_SET) != 0) {
        if (file) fclose(file);
        return 0;
    }
    loaded = fread(destination, 1, size, file);
    fclose(file);
    return loaded == size ? (int)(size & ~3u) : 0;
}

void ApplyMatrixLV(void *matrix, const int32_t *input, int32_t *output) {
    const int16_t *m = matrix;
    int row;
    for (row = 0; row < 3; row++) {
        int64_t value = (int64_t)m[row * 3] * input[0]
                      + (int64_t)m[row * 3 + 1] * input[1]
                      + (int64_t)m[row * 3 + 2] * input[2];
        output[row] = (int32_t)(value >> 12);
    }
}

void MatrixApplyVectorComponents(MATRIX *matrix, int32_t x, int32_t y, int32_t z,
                                 int32_t *out_x, int32_t *out_y, int32_t *out_z) {
    int32_t input[3] = {x, y, z};
    int32_t output[3];
    ApplyMatrixLV(matrix, input, output);
    *out_x = output[0];
    *out_y = output[1];
    *out_z = output[2];
}

void MatrixApplyZRotation(MATRIX *matrix, int32_t degrees) {
    MATRIX rotation = {0};
    int angle;
    if (degrees == 0) return;
    angle = degrees / 360;
    rotation.m[0][0] = (short)rcos(angle);
    rotation.m[0][1] = (short)-rsin(angle);
    rotation.m[1][0] = (short)rsin(angle);
    rotation.m[1][1] = (short)rcos(angle);
    rotation.m[2][2] = 0x1000;
    MulMatrix(matrix, &rotation);
}

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output) {
    int row, column, inner;
    MATRIX result = *output;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            int64_t value = 0;
            for (inner = 0; inner < 3; inner++)
                value += (int64_t)left->m[row][inner] * right->m[inner][column];
            result.m[row][column] = (short)(value >> 12);
        }
    }
    *output = result;
    return output;
}

void TransformCollisionVector(const int16_t *input, int32_t *output) {
    SVECTOR vector;
    VECTOR transformed;

    vector.vx = input[0];
    vector.vy = input[1];
    vector.vz = input[2];
    vector.pad = 0;
    ApplyRotMatrix(&vector, &transformed);
    output[0] = transformed.vx;
    output[1] = transformed.vy;
    output[2] = transformed.vz;
}

/* The following adapters keep optional PS1 services non-fatal on the host.
 * Their real filesystem/audio implementations are introduced before those
 * subsystems are enabled in the native startup path. */
#define VOID_ADAPTER(name) void name(void) {}
#define ZERO_ADAPTER(name) long name(void) { return 0; }
#define FAIL_ADAPTER(name) long name(void) { return -1; }

void BiosBuInit(void) { _bu_init(); }
VOID_ADAPTER(BiosExit)
VOID_ADAPTER(BiosSetMemSize)
VOID_ADAPTER(KernelCallbackSlot3)
VOID_ADAPTER(SetDMAInterruptState)
void StartPad(void) { (void)StartPAD(); }
VOID_ADAPTER(StCdInterrupt)
VOID_ADAPTER(StUnSetRing)
long BiosFileOpen(void *path, long mode) {
    if (path == NULL) return -1;
    return psyz_open(path, (int)mode);
}

long BiosFileSeek(long fd, long offset, long whence) {
    return lseek((int)fd, offset, (int)whence);
}

long BiosFileRead(long fd, void *buffer, long length) {
    return read((int)fd, buffer, (size_t)length);
}

long BiosFileWrite(long fd, void *buffer, long length) {
    return write((int)fd, buffer, (size_t)length);
}

long BiosFileClose(long fd) { return close((int)fd); }

void *BiosFirstFile(char *path, void *entry) {
    return firstfile(path, entry);
}

void *BiosNextFile(void *entry) { return nextfile(entry); }

long BiosFormatDevice(void *device) { return format(device); }
ZERO_ADAPTER(CdPosToInt_Local)
ZERO_ADAPTER(CdReadBreak)
void InitPad(void *buf0, int len0, void *buf1, int len1) {
    (void)InitPAD((char *)buf0, len0, (char *)buf1, len1);
}
ZERO_ADAPTER(MdecUnpackStatus)
ZERO_ADAPTER(SpuGetKeyStatus)
ZERO_ADAPTER(SpuVmDamperStep)
ZERO_ADAPTER(SsSeqCloseWrapper)
ZERO_ADAPTER(SsSetSpuInputAttr)
ZERO_ADAPTER(SsSetVoiceCount)
ZERO_ADAPTER(SsStartSoundTickMode1)
ZERO_ADAPTER(SsStopSoundTick)
ZERO_ADAPTER(SsUtChangePitch)
ZERO_ADAPTER(SsUtKeyOffV)
ZERO_ADAPTER(SsUtPitchBend)
ZERO_ADAPTER(StGetBackloc)
ZERO_ADAPTER(ssinit)

CdlFILE *DsSearchFile(CdlFILE *file, char *name) {
    const char *marker;
    int track;
    int sector;

    if (file == NULL || name == NULL) return NULL;
    marker = name;
    while ((marker = strstr(marker, "DA")) != NULL) {
        if (marker[2] >= '0' && marker[2] <= '9' &&
            marker[3] >= '0' && marker[3] <= '9') break;
        marker += 2;
    }
    if (marker == NULL) return NULL;
    track = (marker[2] - '0') * 10 + marker[3] - '0';
    sector = Psyz_CdGetTrackSector(track);
    if (sector < 0) return NULL;
    CdIntToPos(sector, &file->pos);
    file->size = 0;
    strncpy(file->name, marker, sizeof(file->name) - 1);
    file->name[sizeof(file->name) - 1] = '\0';
    return file;
}
