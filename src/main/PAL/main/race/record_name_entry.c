#include "game/audio.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/state.h"

enum {
    NAME_ENTRY_CHARACTER_COUNT = 42,
    NAME_ENTRY_MOVE_CUE = 1,
    NAME_ENTRY_CONFIRM_CUE = 2,
    NAME_ENTRY_CANCEL_CUE = 3,
    NAME_ENTRY_DEFAULT_CHARACTER = 0xB,
};

static s32 WrapNameEntryCharacter(s32 character, s32 step) {
    int64_t wrapped = ((int64_t)character + step) % NAME_ENTRY_CHARACTER_COUNT;

    if (wrapped < 0) {
        wrapped += NAME_ENTRY_CHARACTER_COUNT;
    }
    return (s32)wrapped;
}

void WriteRecordDriverName(RaceRecord *record, const u8 *nameCodes) {
    s32 character;

    if (record == NULL || nameCodes == NULL) {
        return;
    }
    for (character = 0; character < RECORD_NAME_LENGTH; character++) {
        u8 code = nameCodes[character];

        if (code >= NAME_ENTRY_CHARACTER_COUNT) {
            code = NAME_ENTRY_DEFAULT_CHARACTER;
        }
        record->driverName[character] =
            g_NameEntryCharset[code];
    }
    for (; character < (s32)sizeof(record->driverName); character++) {
        record->driverName[character] = '\0';
    }
}

s32 UpdateRecordNameEntry(u8 *nameCodes) {
    s32 previousCharacter = g_NameEntryChar;
    s32 step = 0;

    if (nameCodes == NULL) {
        return 0;
    }
    if (g_NameEntryCursor == RECORD_NAME_LENGTH) {
        return 1;
    }
    if ((u32)g_NameEntryCursor >= RECORD_NAME_LENGTH) {
        g_NameEntryCursor = 0;
    }

    if (g_PadPressedRepeat & PAD_LEFT) {
        step = -1;
    } else if (g_PadPressedRepeat & PAD_RIGHT) {
        step = 1;
    }
    g_NameEntryChar = WrapNameEntryCharacter(g_NameEntryChar, step);
    if (previousCharacter != g_NameEntryChar) {
        PlaySoundCue(NAME_ENTRY_MOVE_CUE);
    }

    nameCodes[g_NameEntryCursor] = g_NameEntryChar;
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(NAME_ENTRY_CONFIRM_CUE);
        g_NameEntryCursor++;
        if (g_NameEntryCursor == RECORD_NAME_LENGTH) {
            return 1;
        }
        g_NameEntryChar = WrapNameEntryCharacter(nameCodes[g_NameEntryCursor], 0);
    } else if ((g_PadPressed & PAD_CANCEL) && g_NameEntryCursor > 0) {
        PlaySoundCue(NAME_ENTRY_CANCEL_CUE);
        g_NameEntryCursor--;
        g_NameEntryChar = WrapNameEntryCharacter(nameCodes[g_NameEntryCursor], 0);
    }
    return 0;
}
