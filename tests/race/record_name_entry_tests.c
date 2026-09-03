#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "game/race.h"
#include "game/records_internal.h"
#include "game/state.h"

s32 g_NameEntryChar;
s32 g_NameEntryCursor;
u8 g_NameEntryCharset[42];
u16 g_PadPressed;
u16 g_PadPressedRepeat;

static s32 s_cues[8];
static s32 s_cueCount;
static s32 s_failures;

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
            s_failures++;                                                                 \
        }                                                                                 \
    } while (0)

void PlaySoundCue(s32 cue) {
    s_cues[s_cueCount++] = cue;
}

static void Reset(u8 *nameCodes) {
    s32 i;

    for (i = 0; i < 42; i++) {
        g_NameEntryCharset[i] = (u8)('A' + i % 26);
    }
    for (i = 0; i < 6; i++) {
        nameCodes[i] = i + 1;
    }
    g_NameEntryChar = nameCodes[0];
    g_NameEntryCursor = 0;
    g_PadPressed = 0;
    g_PadPressedRepeat = 0;
    s_cueCount = 0;
}

static void TestCharacterSelectionWraps(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    g_NameEntryChar = 0;
    g_PadPressedRepeat = PAD_LEFT;
    CHECK(UpdateRecordNameEntry(nameCodes) == 0);
    CHECK(g_NameEntryChar == 41 && nameCodes[0] == 41);
    CHECK(s_cueCount == 1 && s_cues[0] == 1);

    g_NameEntryChar = 41;
    g_PadPressedRepeat = PAD_RIGHT;
    CHECK(UpdateRecordNameEntry(nameCodes) == 0);
    CHECK(g_NameEntryChar == 0 && nameCodes[0] == 0);
    CHECK(s_cueCount == 2 && s_cues[1] == 1);
}

static void TestConfirmAndCancel(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    g_NameEntryChar = 9;
    g_PadPressed = PAD_CROSS;
    CHECK(UpdateRecordNameEntry(nameCodes) == 0);
    CHECK(nameCodes[0] == 9 && g_NameEntryCursor == 1);
    CHECK(g_NameEntryChar == nameCodes[1]);
    CHECK(s_cueCount == 1 && s_cues[0] == 2);

    g_PadPressed = PAD_TRIANGLE;
    CHECK(UpdateRecordNameEntry(nameCodes) == 0);
    CHECK(g_NameEntryCursor == 0 && g_NameEntryChar == 9);
    CHECK(s_cueCount == 2 && s_cues[1] == 3);

    g_PadPressed = PAD_TRIANGLE;
    CHECK(UpdateRecordNameEntry(nameCodes) == 0);
    CHECK(g_NameEntryCursor == 0 && s_cueCount == 2);
}

static void TestSixthCharacterCompletes(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    g_NameEntryCursor = 5;
    g_NameEntryChar = 17;
    g_PadPressed = PAD_START;
    CHECK(UpdateRecordNameEntry(nameCodes) == 1);
    CHECK(nameCodes[5] == 17 && g_NameEntryCursor == 6);
    CHECK(s_cueCount == 1 && s_cues[0] == 2);
}

static void TestWritesDriverName(void) {
    RaceRecord record;
    u8 nameCodes[6] = {0, 1, 2, 3, 4, 5};

    memset(&record, 0xCC, sizeof(record));
    WriteRecordDriverName(&record, nameCodes);
    CHECK(memcmp(record.driverName, "ABCDEF\0\0", 8) == 0);

    nameCodes[2] = 0xFF;
    WriteRecordDriverName(&record, nameCodes);
    CHECK(record.driverName[2] == g_NameEntryCharset[0xB]);
    WriteRecordDriverName(NULL, nameCodes);
    WriteRecordDriverName(&record, NULL);
}

static void TestInvalidStateIsBounded(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    g_NameEntryCursor = -1;
    g_NameEntryChar = INT_MAX;
    CHECK(UpdateRecordNameEntry(nameCodes) == 0);
    CHECK(g_NameEntryCursor == 0);
    CHECK(g_NameEntryChar == INT_MAX % 42);
    CHECK(nameCodes[0] == INT_MAX % 42);

    g_NameEntryCursor = RECORD_NAME_LENGTH;
    CHECK(UpdateRecordNameEntry(nameCodes) == 1);
    CHECK(UpdateRecordNameEntry(NULL) == 0);
}

int main(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    TestCharacterSelectionWraps();
    TestConfirmAndCancel();
    TestSixthCharacterCompletes();
    TestWritesDriverName();
    TestInvalidStateIsBounded();
    return s_failures != 0;
}
