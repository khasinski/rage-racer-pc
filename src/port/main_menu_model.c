#include "main_menu_ui.h"
#include <math.h>

/* One ordered list shared by rendering, pointer hit testing and navigation. */
static const char *const s_labels[TITLE_MENU_ITEM_COUNT] = {
    "GRAND PRIX", "EXTRA GRAND PRIX", "TIME ATTACK", "CUSTOM RACE", "DUEL",
    "LOAD GAME", "SETTINGS", "QUIT GAME"
};
const char *MainMenuLabel(int item) {
    return item >= 0 && item < TITLE_MENU_ITEM_COUNT ? s_labels[item] : "";
}
MainMenuRect MainMenuRow(int width, int height, int item, int extraUnlocked) {
    if (item == TITLE_MENU_EXTRA_GRAND_PRIX && !extraUnlocked)
        return (MainMenuRect){0};
    int rowIndex = item - (!extraUnlocked && item > TITLE_MENU_EXTRA_GRAND_PRIX);
    float w = fminf(width * 0.39f, height * 0.646f);
    float row = height * 0.053f, gap = height * 0.011f;
    MainMenuRect r = {(width - w) * 0.5f, height * 0.27f + rowIndex * (row + gap), w, row};
    return r;
}
int MainMenuHit(int width, int height, float x, float y, int extraUnlocked) {
    if (width <= 0 || height <= 0) return -1;
    for (int i = 0; i < TITLE_MENU_ITEM_COUNT; i++) {
        MainMenuRect r = MainMenuRow(width, height, i, extraUnlocked);
        if (i == TITLE_MENU_EXTRA_GRAND_PRIX && !extraUnlocked) continue;
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return i;
    }
    return -1;
}
