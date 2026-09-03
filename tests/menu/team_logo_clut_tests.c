#include "game/menu_internal.h"

#include <stdio.h>

u16 g_TeamLogoBlankClut[16];
u16 g_TeamLogoClut[16];
Rect g_TeamLogoClutRect;

static RECT *s_uploadedRect;
static u_long *s_uploadedPixels;

#undef LoadImage
int LoadImage(RECT *rect, u_long *pixels) {
    s_uploadedRect = rect;
    s_uploadedPixels = pixels;
    return 0;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    RestoreTeamLogoClut();
    CHECK(s_uploadedRect == &g_TeamLogoClutRect);
    CHECK(s_uploadedPixels == (u_long *)(void *)g_TeamLogoBlankClut);

    s_uploadedRect = NULL;
    s_uploadedPixels = NULL;
    UploadTeamLogoClut();
    CHECK(s_uploadedRect == &g_TeamLogoClutRect);
    CHECK(s_uploadedPixels == (u_long *)(void *)g_TeamLogoClut);

    puts("team logo CLUT tests passed");
    return 0;
}
