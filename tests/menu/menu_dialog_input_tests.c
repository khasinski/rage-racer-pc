#include "game/menu_internal.h"

#include <stdio.h>

#define CHECK_INPUT(buttons, expected)                                        \
    do {                                                                       \
        if (ChooseMenuDialogAction(buttons) != (expected)) {                  \
            fprintf(stderr, "wrong dialog action at line %d\n", __LINE__);  \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CHECK_INPUT(0, MENU_DIALOG_NO_ACTION);
    CHECK_INPUT(PAD_RIGHT, MENU_DIALOG_RIGHT);
    CHECK_INPUT(PAD_LEFT | PAD_RIGHT, MENU_DIALOG_LEFT);
    CHECK_INPUT(PAD_CANCEL | PAD_LEFT | PAD_RIGHT, MENU_DIALOG_CANCEL);
    CHECK_INPUT(PAD_CONFIRM | PAD_CANCEL | PAD_LEFT | PAD_RIGHT,
                MENU_DIALOG_CONFIRM);

    puts("menu dialog input tests passed");
    return 0;
}
