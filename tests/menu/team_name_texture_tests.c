#include <stdio.h>

#include "game/menu.h"

typedef struct ImageUpload {
    Rect rect;
    void *pixels;
} ImageUpload;

u8 g_TeamNameFontGlyphs
    [TEAM_NAME_FONT_GLYPH_COUNT * TEAM_NAME_FONT_GLYPH_BYTES];
u8 g_TeamNameBlankTile[192];

static ImageUpload s_uploads[8];
static s32 s_uploadCount;

#undef LoadImage
int LoadImage(RECT *rect, u_long *pixels) {
    if (s_uploadCount < 8) {
        s_uploads[s_uploadCount].rect = *rect;
        s_uploads[s_uploadCount].pixels = pixels;
    }
    s_uploadCount++;
    return 0;
}

static int Check(int condition, const char *message) {
    if (condition) return 1;
    printf("FAIL %s\n", message);
    return 0;
}

static void ResetUploads(void) {
    s_uploadCount = 0;
}

int main(void) {
    const u8 name[] = {2, 5, 83};
    const u8 tooLong[] = {0, 1, 2, 3, 4, 5, 6};
    const u8 invalid[] = {84};
    int ok = 1;

    ResetUploads();
    UploadTeamNameTexture(name, 3);
    ok &= Check(s_uploadCount == 4, "clear plus three glyph uploads");
    ok &= Check(s_uploads[0].rect.x == 0x282 &&
                    s_uploads[0].rect.y == 0x37 &&
                    s_uploads[0].rect.w == 12 && s_uploads[0].rect.h == 8 &&
                    s_uploads[0].pixels == g_TeamNameBlankTile,
                "clear upload covers the complete destination");
    ok &= Check(s_uploads[1].rect.x == 0x285 &&
                    s_uploads[2].rect.x == 0x287 &&
                    s_uploads[3].rect.x == 0x289,
                "three glyphs are centred and advance by their width");
    ok &= Check(s_uploads[1].pixels ==
                        &g_TeamNameFontGlyphs[2 * TEAM_NAME_FONT_GLYPH_BYTES] &&
                    s_uploads[2].pixels ==
                        &g_TeamNameFontGlyphs[5 * TEAM_NAME_FONT_GLYPH_BYTES] &&
                    s_uploads[3].pixels ==
                        &g_TeamNameFontGlyphs[
                            83 * TEAM_NAME_FONT_GLYPH_BYTES],
                "glyph indices address 32-byte atlas cells");

    ResetUploads();
    UploadTeamNameTexture(tooLong, 7);
    ok &= Check(s_uploadCount == 7 && s_uploads[1].rect.x == 0x282,
                "name upload is limited to six texture cells");

    ResetUploads();
    UploadTeamNameTexture(invalid, 1);
    ok &= Check(s_uploadCount == 2 &&
                    s_uploads[1].pixels == g_TeamNameBlankTile,
                "invalid saved glyph uses a blank cell");

    ResetUploads();
    UploadTeamNameTexture(NULL, 4);
    ok &= Check(s_uploadCount == 1, "null name only clears the texture");

    return ok ? 0 : 1;
}
