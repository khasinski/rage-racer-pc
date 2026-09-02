#include "game/audio.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/state.h"

enum {
    NAME_ENTRY_CHARACTER_COUNT = 42,
};

void WriteRecordDriverName(RaceRecord *record, const u8 *nameCodes) {
    s32 character;

    for (character = 0; character < RECORD_NAME_LENGTH; character++) {
        record->driverName[character] =
            g_NameEntryCharset[nameCodes[character]];
    }
}

s32 UpdateRecordNameEntry(u8 *nameCodes) {
    s32 previousCharacter = g_NameEntryChar;

    if (g_PadPressedRepeat & PAD_LEFT) {
        g_NameEntryChar--;
    } else if (g_PadPressedRepeat & PAD_RIGHT) {
        g_NameEntryChar++;
    }
    g_NameEntryChar =
        (g_NameEntryChar + NAME_ENTRY_CHARACTER_COUNT) %
        NAME_ENTRY_CHARACTER_COUNT;
    if (previousCharacter != g_NameEntryChar) {
        PlaySoundCue(1);
    }

    nameCodes[g_NameEntryCursor] = g_NameEntryChar;
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_NameEntryCursor++;
        if (g_NameEntryCursor == RECORD_NAME_LENGTH) {
            return 1;
        }
        g_NameEntryChar = nameCodes[g_NameEntryCursor];
    } else if ((g_PadPressed & PAD_CANCEL) && g_NameEntryCursor > 0) {
        PlaySoundCue(3);
        g_NameEntryCursor--;
        g_NameEntryChar = nameCodes[g_NameEntryCursor];
    }
    return 0;
}
