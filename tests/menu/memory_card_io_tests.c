#include "game/memcard.h"
#include "game/menu.h"
#include "game/save_internal.h"

#include <stdio.h>
#include <string.h>

enum { MOCK_FILE_SIZE = 0x1400, MOCK_FILE_COUNT = 3 };

s32 GameMenuLoadPhase;
char g_SaveFilePath[MEMORY_CARD_SAVE_SLOT_COUNT * MC_SAVE_PATH_SIZE];
char g_SaveTitleSjis[MEMORY_CARD_SAVE_SLOT_COUNT * MC_SAVE_TITLE_SIZE];
char g_FmtCardWildcard[] = "bu%d%d:*";
u8 g_TeamNameChars[16];
u8 g_TeamNameLength;
s32 g_SaveElapsedTicks;
DirEntry g_McDirEntries[MEMORY_CARD_MAX_FILES];
s32 g_McCardFileCount;
s32 g_McFreeBlocks;
char g_FmtPlayTime[] = "%5d:%02d:%02d";

static u8 s_files[MOCK_FILE_COUNT][MOCK_FILE_SIZE];
static long s_positions[MOCK_FILE_COUNT];
static s32 s_exists[MOCK_FILE_COUNT];
static s32 s_openResults[16];
static s32 s_openResultCount;
static s32 s_openCalls;
static s32 s_closeCalls;
static s32 s_readCalls;
static s32 s_writeCalls;
static s32 s_failReadCall;
static s32 s_failWriteCall;
static s32 s_loadOk;
static s32 s_directoryFiles;
static s32 s_directoryNextCalls;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void ResetMock(void) {
    memset(s_files, 0, sizeof(s_files));
    memset(s_positions, 0, sizeof(s_positions));
    memset(s_exists, 0, sizeof(s_exists));
    memset(s_openResults, 0, sizeof(s_openResults));
    memset(g_SaveFilePath, 0, sizeof(g_SaveFilePath));
    memset(g_McDirEntries, 0, sizeof(g_McDirEntries));
    memset(g_TeamNameChars, 0, sizeof(g_TeamNameChars));
    strcpy(&g_SaveFilePath[0 * MC_SAVE_PATH_SIZE], "slot-0");
    strcpy(&g_SaveFilePath[1 * MC_SAVE_PATH_SIZE], "slot-1");
    strcpy(&g_SaveFilePath[2 * MC_SAVE_PATH_SIZE], "slot-2");
    s_openResultCount = 0;
    s_openCalls = 0;
    s_closeCalls = 0;
    s_readCalls = 0;
    s_writeCalls = 0;
    s_failReadCall = -1;
    s_failWriteCall = -1;
    s_loadOk = 1;
    s_directoryFiles = 0;
    s_directoryNextCalls = 0;
    GameMenuLoadPhase = 0;
}

static void SealHeader(GameSaveHeaderRow *header) {
    header->fields.checksum = 0;
    header->fields.checksum = CalculateSaveHeaderChecksum(header);
}

static void PutHeader(s32 file, s32 offset, u8 marker, s32 valid) {
    GameSaveHeaderRow header;

    memset(&header, 0, sizeof(header));
    header.fields.name[0] = marker;
    header.fields.nameLength = 7;
    header.fields.saveCounter = 123456 + marker;
    SealHeader(&header);
    if (!valid) header.fields.checksum++;
    memcpy(&s_files[file][offset], &header, sizeof(header));
}

long BiosFileOpen(void *path, long mode) {
    const char *name = path;
    s32 file;

    (void)mode;
    if (s_openCalls < s_openResultCount) {
        return s_openResults[s_openCalls++];
    }
    s_openCalls++;
    for (file = 0; file < MOCK_FILE_COUNT; file++) {
        if (name == &g_SaveFilePath[file * MC_SAVE_PATH_SIZE] ||
            strcmp(name, &g_SaveFilePath[file * MC_SAVE_PATH_SIZE]) == 0) {
            return s_exists[file] ? 10 + file : -1;
        }
    }
    return -1;
}

long BiosFileSeek(long fd, long offset, long whence) {
    s32 file = (s32)fd - 10;

    if (file < 0 || file >= MOCK_FILE_COUNT || whence != 0 || offset < 0 ||
        offset > MOCK_FILE_SIZE) return -1;
    s_positions[file] = offset;
    return offset;
}

long BiosFileRead(long fd, void *buffer, long length) {
    s32 file = (s32)fd - 10;

    s_readCalls++;
    if (s_readCalls == s_failReadCall || file < 0 ||
        file >= MOCK_FILE_COUNT || s_positions[file] + length > MOCK_FILE_SIZE) {
        return -1;
    }
    memcpy(buffer, &s_files[file][s_positions[file]], (size_t)length);
    s_positions[file] += length;
    return length;
}

long BiosFileWrite(long fd, void *buffer, long length) {
    s32 file = (s32)fd - 10;

    (void)buffer;
    s_writeCalls++;
    if (s_writeCalls == s_failWriteCall || file < 0 ||
        file >= MOCK_FILE_COUNT) return -1;
    return length;
}

long BiosFileClose(long fd) {
    (void)fd;
    s_closeCalls++;
    return 0;
}

void *BiosFirstFile(char *path, void *entry) {
    (void)path;
    return s_directoryFiles > 0 ? entry : NULL;
}

void *BiosNextFile(void *entry) {
    s_directoryNextCalls++;
    return s_directoryNextCalls < s_directoryFiles ? entry : NULL;
}

void BuildSaveIconBlock(GameSaveIconBlock *block, const char *title, s32 iconTile,
                        s32 imageX, s32 imageY) {
    (void)title; (void)iconTile; (void)imageX; (void)imageY;
    memset(block, 0x11, sizeof(*block));
}

void WriteSaveHeaderRow(GameSaveHeaderRow *row) { SealHeader(row); }
void StoreSaveStateBlock(GameSaveBlock *block) { memset(block, 0x22, sizeof(*block)); }
s32 LoadSaveStateBlock(const GameSaveBlock *block) { (void)block; return s_loadOk; }
void ClearSaveHeaderRows(GameSaveHeaderRow *rows) {
    memset(rows, 0, 3 * sizeof(*rows));
}

static int TestVerifiedHeaders(void) {
    GameSaveHeaderRow header;

    ResetMock();
    s_exists[0] = 1;
    PutHeader(0, 0x1280, 1, 1);
    CHECK(ReadVerifiedSaveHeader(10, &header) == 1);
    CHECK(header.fields.name[0] == 1);
    CHECK(GameMenuLoadPhase == 0x140);

    PutHeader(0, 0x1280, 2, 0);
    PutHeader(0, 0x200, 3, 1);
    CHECK(ReadVerifiedSaveHeader(10, &header) == 1);
    CHECK(header.fields.name[0] == 3);
    CHECK(GameMenuLoadPhase == 0x170);

    PutHeader(0, 0x200, 4, 0);
    CHECK(ReadVerifiedSaveHeader(10, &header) == 0);
    CHECK(GameMenuLoadPhase == 0x180);
    return 0;
}

static int TestHeaderScan(void) {
    GameSaveHeaderRow headers[MEMORY_CARD_SAVE_SLOT_COUNT];

    ResetMock();
    s_exists[0] = 1;
    s_exists[1] = 1;
    PutHeader(0, 0x1280, 10, 1);
    PutHeader(1, 0x1280, 11, 0);
    PutHeader(1, 0x200, 12, 0);
    CHECK(ScanMemoryCardSaveHeaders(headers) == ((1 << 0) | (0x10000 << 1)));
    CHECK(headers[0].fields.name[0] == 10);
    CHECK(s_closeCalls == 2);
    CHECK(GameMenuLoadPhase == 0x190);
    return 0;
}

static int TestLoadAndFailuresClose(void) {
    GameSaveHeaderRow header;
    GameSaveHeaderRow storedHeader;
    GameSaveBlock block;
    s32 i;

    ResetMock();
    s_exists[1] = 1;
    PutHeader(1, 0x1280, 21, 1);
    memcpy(&storedHeader, &s_files[1][0x1280], sizeof(storedHeader));
    storedHeader.fields.nameLength = 0xFF;
    SealHeader(&storedHeader);
    memcpy(&s_files[1][0x1280], &storedHeader, sizeof(storedHeader));
    memset(&block, 0, sizeof(block));
    memcpy(&s_files[1][0x280], &block, sizeof(block));
    s_openResults[0] = -1;
    s_openResults[1] = 11;
    s_openResultCount = 2;
    CHECK(LoadMemoryCardSaveSlot(1, &header) == 1);
    CHECK(GameMenuLoadPhase == 0x3901);
    CHECK(g_TeamNameLength == SAVE_TEAM_NAME_CAPACITY);
    for (i = 0; i < SAVE_TEAM_NAME_CAPACITY; i++) {
        CHECK(g_TeamNameChars[i] == header.fields.name[i]);
    }
    CHECK(g_SaveElapsedTicks == header.fields.saveCounter);
    CHECK(s_closeCalls == 1);

    ResetMock();
    s_exists[0] = 1;
    PutHeader(0, 0x1280, 22, 1);
    s_failReadCall = 2;
    CHECK(LoadMemoryCardSaveSlot(0, &header) == 0);
    CHECK(s_closeCalls == 1);
    return 0;
}

static int TestWriteAndDirectoryCount(void) {
    GameSaveHeaderRow header;
    GameSaveIconBlock icon;
    GameSaveBlock block;

    ResetMock();
    CHECK(WriteMemoryCardSaveSlot(-1, &header) == 0);
    CHECK(WriteMemoryCardSaveSlot(MEMORY_CARD_SAVE_SLOT_COUNT, &header) == 0);
    CHECK(LoadMemoryCardSaveSlot(-1, &header) == 0);
    CHECK(LoadMemoryCardSaveSlot(MEMORY_CARD_SAVE_SLOT_COUNT, &header) == 0);
    CHECK(s_openCalls == 0);

    ResetMock();
    s_openResults[0] = -1;
    s_openResults[1] = 10;
    s_openResults[2] = 10;
    s_openResultCount = 3;
    CHECK(WriteMemoryCardSaveFile(g_SaveFilePath, g_SaveTitleSjis, &icon,
                                  &header, &block) == 1);
    CHECK(s_writeCalls == 4);
    CHECK(s_closeCalls == 2);
    CHECK(GameMenuLoadPhase == 0x1570);

    ResetMock();
    s_openResults[0] = 10;
    s_openResultCount = 1;
    s_failWriteCall = 2;
    CHECK(WriteMemoryCardSaveFile(g_SaveFilePath, g_SaveTitleSjis, &icon,
                                  &header, &block) == 0);
    CHECK(s_closeCalls == 1);

    s_directoryFiles = 4;
    CHECK(CountMemoryCardFiles(0, 0) == 4);
    s_directoryFiles = 0;
    CHECK(CountMemoryCardFiles(0, 0) == 0);

    ResetMock();
    s_directoryFiles = MEMORY_CARD_MAX_FILES + 5;
    CHECK(CountMemoryCardFiles(0, 0) == MEMORY_CARD_MAX_FILES);
    CHECK(s_directoryNextCalls == MEMORY_CARD_MAX_FILES - 1);
    return 0;
}

static int TestCardStatus(void) {
    GameSaveHeaderRow headers[MEMORY_CARD_SAVE_SLOT_COUNT];
    char elapsed[16];
    char *visibleElapsed;

    ResetMock();
    g_McDirEntries[0].size = 0x2000;
    g_McDirEntries[1].size = 0x1000;
    CHECK(CalculateMemoryCardFreeBlocks(0) == 15);
    CHECK(CalculateMemoryCardFreeBlocks(-1) == 15);
    CHECK(CalculateMemoryCardFreeBlocks(2) == 14);
    CHECK(CalculateMemoryCardFreeBlocks(MEMORY_CARD_MAX_FILES + 5) == 14);
    g_McDirEntries[0].size = 0x7FFFFFFF;
    g_McDirEntries[1].size = 0x7FFFFFFF;
    CHECK(CalculateMemoryCardFreeBlocks(2) == 0);
    g_McDirEntries[0].size = 0x2000;
    g_McDirEntries[1].size = 0x1000;

    visibleElapsed = FormatSaveElapsedTime(elapsed, 3723 * 60);
    CHECK(strcmp(elapsed, "    1:02:03") == 0);
    CHECK(visibleElapsed == elapsed + 2);
    CHECK(strcmp(visibleElapsed, "  1:02:03") == 0);

    s_directoryFiles = 2;
    s_exists[0] = 1;
    PutHeader(0, 0x1280, 31, 1);
    memset(headers, 0xCC, sizeof(headers));
    CHECK(RefreshMemoryCardSaveStatus(headers) == 1);
    CHECK(g_McCardFileCount == 2);
    CHECK(g_McFreeBlocks == 14);
    CHECK(headers[0].fields.name[0] == 31);
    CHECK(GameMenuLoadPhase == 0x200);
    return 0;
}

int main(void) {
    if (TestVerifiedHeaders() != 0) return 1;
    if (TestHeaderScan() != 0) return 1;
    if (TestLoadAndFailuresClose() != 0) return 1;
    if (TestWriteAndDirectoryCount() != 0) return 1;
    if (TestCardStatus() != 0) return 1;
    puts("memory_card_io: verified retries, checksums, cleanup and directory scan");
    return 0;
}
