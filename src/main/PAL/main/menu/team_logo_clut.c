#include "game/menu.h"
#include "game/menu_internal.h"

void RestoreTeamLogoClut(void) {
    LoadImage(&g_TeamLogoClutRect, &g_TeamLogoBlankClut);
}

void UploadTeamLogoClut(void) {
    LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
}
