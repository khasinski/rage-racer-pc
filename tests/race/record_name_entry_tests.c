#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "game/race.h"
#include "game/records_internal.h"
#include "game/state.h"

static RecordEntry s_state;
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

    for (i = 0; i < 6; i++) {
        nameCodes[i] = i + 1;
    }
    s_state.nameCharacter = nameCodes[0];
    s_state.nameCursor = 0;
    g_PadPressed = 0;
    g_PadPressedRepeat = 0;
    s_cueCount = 0;
}

static void TestCharacterSelectionWraps(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    s_state.nameCharacter = 0;
    g_PadPressedRepeat = PAD_LEFT;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 0);
    CHECK(s_state.nameCharacter == 41 && nameCodes[0] == 41);
    CHECK(s_cueCount == 1 && s_cues[0] == 1);

    s_state.nameCharacter = 41;
    g_PadPressedRepeat = PAD_RIGHT;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 0);
    CHECK(s_state.nameCharacter == 0 && nameCodes[0] == 0);
    CHECK(s_cueCount == 2 && s_cues[1] == 1);
}

static void TestConfirmAndCancel(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    s_state.nameCharacter = 9;
    g_PadPressed = PAD_CROSS;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 0);
    CHECK(nameCodes[0] == 9 && s_state.nameCursor == 1);
    CHECK(s_state.nameCharacter == nameCodes[1]);
    CHECK(s_cueCount == 1 && s_cues[0] == 2);

    g_PadPressed = PAD_TRIANGLE;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 0);
    CHECK(s_state.nameCursor == 0 && s_state.nameCharacter == 9);
    CHECK(s_cueCount == 2 && s_cues[1] == 3);

    g_PadPressed = PAD_TRIANGLE;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 0);
    CHECK(s_state.nameCursor == 0 && s_cueCount == 2);
}

static void TestSixthCharacterCompletes(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    s_state.nameCursor = 5;
    s_state.nameCharacter = 17;
    g_PadPressed = PAD_START;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 1);
    CHECK(nameCodes[5] == 17 && s_state.nameCursor == 6);
    CHECK(s_cueCount == 1 && s_cues[0] == 2);
}

static void TestWritesDriverName(void) {
    RaceRecord record;
    u8 nameCodes[6] = {0, 1, 2, 3, 4, 5};

    memset(&record, 0xCC, sizeof(record));
    WriteRecordDriverName(&record, nameCodes);
    CHECK(memcmp(record.driverName, "012345\0\0", 8) == 0);

    nameCodes[2] = 0xFF;
    WriteRecordDriverName(&record, nameCodes);
    CHECK(record.driverName[2] == 'A');
    WriteRecordDriverName(NULL, nameCodes);
    WriteRecordDriverName(&record, NULL);
}

static void TestInvalidStateIsBounded(void) {
    u8 nameCodes[6];

    Reset(nameCodes);
    s_state.nameCursor = -1;
    s_state.nameCharacter = INT_MAX;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 0);
    CHECK(s_state.nameCursor == 0);
    CHECK(s_state.nameCharacter == INT_MAX % 42);
    CHECK(nameCodes[0] == INT_MAX % 42);

    s_state.nameCursor = RECORD_NAME_LENGTH;
    CHECK(UpdateRecordNameEntry(&s_state, nameCodes) == 1);
    CHECK(UpdateRecordNameEntry(&s_state, NULL) == 0);
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
