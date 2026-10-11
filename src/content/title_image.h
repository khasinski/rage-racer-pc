#ifndef TITLE_IMAGE_H
#define TITLE_IMAGE_H
#include <stdint.h>
/* Stable six-item menu order, including CUSTOM at index 3. */
uint8_t TitleImageIndex(int item,int x,int y);
uint16_t TitleImageColor(int selected,int index);
#endif
