#include "game/frontend_internal.h"

s32 MoveTitleMenuSelection(s32 selection, s32 direction,
                           int extraGrandPrixUnlocked) {
    selection %= TITLE_MENU_ITEM_COUNT;
    if (selection < 0) {
        selection += TITLE_MENU_ITEM_COUNT;
    }

    if (direction < 0) {
        selection = selection > 0 ? selection - 1 : TITLE_MENU_ITEM_COUNT - 1;
        if (!extraGrandPrixUnlocked &&
            selection == TITLE_MENU_EXTRA_GRAND_PRIX) {
            selection = TITLE_MENU_GRAND_PRIX;
        }
    } else if (direction > 0) {
        selection = selection + 1 < TITLE_MENU_ITEM_COUNT ? selection + 1 : 0;
        if (!extraGrandPrixUnlocked &&
            selection == TITLE_MENU_EXTRA_GRAND_PRIX) {
            selection = TITLE_MENU_TIME_ATTACK;
        }
    }
    return selection;
}
