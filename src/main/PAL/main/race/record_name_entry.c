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

static const u8 s_nameCharacters[NAME_ENTRY_CHARACTER_COUNT] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', '.', '-', '!', '?', '@',
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
        record->driverName[character] = s_nameCharacters[code];
    }
    for (; character < (s32)sizeof(record->driverName); character++) {
        record->driverName[character] = '\0';
    }
}

s32 UpdateRecordNameEntry(RecordEntry *state, u8 *nameCodes) {
    s32 previousCharacter = state->nameCharacter;
    s32 step = 0;

    if (nameCodes == NULL) {
        return 0;
    }
    if (state->nameCursor == RECORD_NAME_LENGTH) {
        return 1;
    }
    if ((u32)state->nameCursor >= RECORD_NAME_LENGTH) {
        state->nameCursor = 0;
    }

    if (g_PadPressedRepeat & PAD_LEFT) {
        step = -1;
    } else if (g_PadPressedRepeat & PAD_RIGHT) {
        step = 1;
    }
    state->nameCharacter = WrapNameEntryCharacter(state->nameCharacter, step);
    if (previousCharacter != state->nameCharacter) {
        PlaySoundCue(NAME_ENTRY_MOVE_CUE);
    }

    nameCodes[state->nameCursor] = state->nameCharacter;
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(NAME_ENTRY_CONFIRM_CUE);
        state->nameCursor++;
        if (state->nameCursor == RECORD_NAME_LENGTH) {
            return 1;
        }
        state->nameCharacter = WrapNameEntryCharacter(nameCodes[state->nameCursor], 0);
    } else if ((g_PadPressed & PAD_CANCEL) && state->nameCursor > 0) {
        PlaySoundCue(NAME_ENTRY_CANCEL_CUE);
        state->nameCursor--;
        state->nameCharacter = WrapNameEntryCharacter(nameCodes[state->nameCursor], 0);
    }
    return 0;
}
