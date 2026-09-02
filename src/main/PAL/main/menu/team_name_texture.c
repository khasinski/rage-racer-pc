#include "game/menu.h"

enum {
    TEAM_NAME_TEXTURE_X = 0x282,
    TEAM_NAME_TEXTURE_Y = 0x37,
    TEAM_NAME_TEXTURE_WIDTH = 12,
    TEAM_NAME_TEXTURE_HEIGHT = 8,
    TEAM_NAME_GLYPH_WIDTH = 2,
    TEAM_NAME_GLYPH_BYTES = 32,
    TEAM_NAME_GLYPH_COUNT =
        sizeof(g_TeamNameFontGlyphs) / TEAM_NAME_GLYPH_BYTES,
    TEAM_NAME_TEXTURE_CAPACITY =
        TEAM_NAME_TEXTURE_WIDTH / TEAM_NAME_GLYPH_WIDTH,
};

void ClearTeamNameTexture(void) {
    Rect rect = {
        TEAM_NAME_TEXTURE_X,
        TEAM_NAME_TEXTURE_Y,
        TEAM_NAME_TEXTURE_WIDTH,
        TEAM_NAME_TEXTURE_HEIGHT,
    };

    LoadImage(&rect, g_TeamNameBlankTile);
}

void UploadTeamNameTexture(const u8 *str, s32 len) {
    Rect rect = {
        0,
        TEAM_NAME_TEXTURE_Y,
        TEAM_NAME_GLYPH_WIDTH,
        TEAM_NAME_TEXTURE_HEIGHT,
    };

    ClearTeamNameTexture();

    if (str == NULL || len <= 0) return;
    if (len > TEAM_NAME_TEXTURE_CAPACITY) len = TEAM_NAME_TEXTURE_CAPACITY;

    rect.x = TEAM_NAME_TEXTURE_X + TEAM_NAME_TEXTURE_CAPACITY - len;
    while (len > 0) {
        u8 glyph = *str++;
        void *pixels = glyph < TEAM_NAME_GLYPH_COUNT
                           ? &g_TeamNameFontGlyphs[glyph * TEAM_NAME_GLYPH_BYTES]
                           : g_TeamNameBlankTile;

        LoadImage(&rect, pixels);
        rect.x += TEAM_NAME_GLYPH_WIDTH;
        len--;
    }
}
