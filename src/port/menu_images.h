#ifndef MENU_IMAGES_H
#define MENU_IMAGES_H
#include <stdint.h>
enum { MENU_IMAGE_WIDTH=112, MENU_IMAGE_HEIGHT=16 };
/* Borrowed RGBA8 row-major pixels, valid until the mod session changes.
 * Menu layout remains 112x16 logical pixels; PNGs do not use a PS1 atlas. */
const uint8_t *MenuImage(int item,int selected);
#endif
