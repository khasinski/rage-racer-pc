#include "game/menu_internal.h"
#include "game/race.h"

enum { COURSE_SELECT_OPTION_COUNT = 3 };

/* Directions are applied before confirm on the main screen, so a combined
 * press chooses the row the cursor has just moved to. */
CourseSelectInputOutcome DecideCourseSelectInput(u16 pressed, u16 held,
                                                 s32 option) {
    CourseSelectInputOutcome out;

    out.cueCount = 0;
    out.option = AddClampedMenuValue(option, 0, 0,
                                     COURSE_SELECT_OPTION_COUNT - 1);
    if (pressed & PAD_UP) {
        out.cues[out.cueCount++] = 1;
        out.option = (out.option > 0) ? out.option - 1 : 2;
    }
    if (pressed & PAD_DOWN) {
        out.cues[out.cueCount++] = 1;
        out.option = (out.option < 2) ? out.option + 1 : 0;
    }
    out.wantsPrev = (held & PAD_LEFT) != 0;
    out.wantsNext = (held & PAD_RIGHT) != 0;
    out.choosesRow = (pressed & PAD_CONFIRM) != 0;
    return out;
}

/* Prompt confirm reads the old cursor before either direction changes it.
 * Every pressed button is acted on, matching the original simultaneous-input
 * behavior. */
MenuPromptOutcome DecideSavePrompt(u16 pressed, s32 busy, s32 confirmTimer,
                                   s32 subCursor) {
    MenuPromptOutcome out;

    out.cueCount = 0;
    out.busy = busy;
    out.confirmTimer = confirmTimer;
    out.subCursor = AddClampedMenuValue(subCursor, 0, 0, 1);
    if (pressed & PAD_CONFIRM) {
        out.cues[out.cueCount++] = (out.subCursor != 0) ? 2 : 3;
        out.busy = -3;
        out.confirmTimer = 0x23;
    }
    if (pressed & PAD_CANCEL) {
        out.cues[out.cueCount++] = 3;
        out.busy = -4;
    }
    if (pressed & PAD_LEFT) {
        out.cues[out.cueCount++] = 1;
        out.subCursor = 1;
    }
    if (pressed & PAD_RIGHT) {
        out.cues[out.cueCount++] = 1;
        out.subCursor = 0;
    }
    return out;
}

static void AddCue(MenuClassPromptOutcome *out, s32 cue) {
    out->effects[out->effectCount].kind = MENU_PROMPT_CUE;
    out->effects[out->effectCount++].value = cue;
}

MenuClassPromptOutcome DecideClassPrompt(u16 pressed, s32 busy,
                                         s32 confirmTimer, s32 subCursor,
                                         s32 currentClass, s32 maxClass,
                                         s32 changeApplied) {
    MenuClassPromptOutcome out;

    maxClass = AddClampedMenuValue(maxClass, 0, 0,
                                   GRAND_PRIX_FINAL_CLASS_INDEX);
    currentClass = AddClampedMenuValue(currentClass, 0, 0,
                                       GRAND_PRIX_FINAL_CLASS_INDEX);
    out.effectCount = 0;
    out.busy = busy;
    out.confirmTimer = confirmTimer;
    out.subCursor = AddClampedMenuValue(subCursor, 0, 0, maxClass);
    out.changeApplied = changeApplied != 0;
    if (pressed & PAD_CONFIRM) {
        AddCue(&out, 2);
        if (out.subCursor == currentClass) {
            out.busy = 0;
        } else {
            out.busy = -5;
            out.changeApplied = 0;
            out.confirmTimer = 0x23;
            out.effects[out.effectCount].kind = MENU_PROMPT_CURTAIN;
            out.effects[out.effectCount++].value = 0;
        }
    }
    if (pressed & PAD_CANCEL) {
        AddCue(&out, 3);
        out.busy = 0;
    }
    if (pressed & PAD_UP) {
        AddCue(&out, 1);
        out.subCursor = (out.subCursor != 0) ? out.subCursor - 1 : maxClass;
    }
    if (pressed & PAD_DOWN) {
        AddCue(&out, 1);
        out.subCursor = (out.subCursor < maxClass) ? out.subCursor + 1 : 0;
    }
    return out;
}
