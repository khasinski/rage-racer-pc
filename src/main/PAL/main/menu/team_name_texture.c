#include "game/menu.h"

void ClearTeamNameTexture(void) {
    Rect rect;
    rect.x = 0x282;
    rect.y = 0x37;
    rect.w = 0xC;
    rect.h = 8;
    LoadImage(&rect, &g_TeamNameBlankTile);
}

void UploadTeamNameTexture(u8 *str, s32 len) {
    Rect rect;
    ClearTeamNameTexture();
    rect.x = 0x288 - len;
    rect.y = 0x37;
    rect.w = 2;
    rect.h = 8;
    while (len > 0) {
        LoadImage(&rect, &g_TeamNameFontGlyphs[*str++ << 5]);
        rect.x += 2;
        len--;
    }
}
