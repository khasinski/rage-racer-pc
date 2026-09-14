#include "game/menu.h"
#include "game/menu_internal.h"

static u16 s_blankClut[16];

void RestoreTeamLogoClut(void) {
    LoadImage(&g_TeamLogoClutRect, s_blankClut);
}

void UploadTeamLogoClut(void) {
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
}
