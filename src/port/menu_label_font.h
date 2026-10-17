#ifndef MENU_LABEL_FONT_H
#define MENU_LABEL_FONT_H

#include <stdint.h>

enum {
    MENU_LABEL_WIDTH = 112,
    MENU_LABEL_HEIGHT = 16,
};

/* RGB8, row-major, one retail 112x16 title button. NULL when the item is
 * outside the six-entry menu or the face cannot be read. The pointer stays
 * valid for the rest of the process. */
const uint8_t *MenuLabelFontPixels(int item, int selected);

#endif
