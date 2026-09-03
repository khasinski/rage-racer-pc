#include "game/menu.h"
#include "game/asset_internal.h"

enum {
    TEAM_LOGO_BACKGROUND_SAMPLE_BASE = 10,
    TEAM_LOGO_CHARACTER_COLOR_FIRST = 1,
    TEAM_LOGO_BACKGROUND_COLOR_FIRST = 12,
    TEAM_LOGO_COLOR_COUNT = 16,
    TEAM_LOGO_HALFWORDS_PER_ROW = 16,
};

/* A zero character pixel is transparent and lets the background through. */
static u16 CompositeLogoPixels(u16 characterPixels, u16 backgroundPixels) {
    s32 shift;

    for (shift = 0; shift < 16; shift += 4) {
        u16 pixelMask = (u16)(0xF << shift);

        if ((characterPixels & pixelMask) == 0) {
            characterPixels |= backgroundPixels & pixelMask;
        }
    }
    return characterPixels;
}

/* Builds the editable logo from one character layer and one background layer. */
void ComposeSampleTeamLogo(s32 character, s32 background) {
    const TeamLogoSample *characterSample =
        &g_TeamLogoSampleData[character / 2];
    const TeamLogoSample *backgroundSample =
        &g_TeamLogoSampleData[TEAM_LOGO_BACKGROUND_SAMPLE_BASE + background / 2];
    const u16 *characterClut = characterSample->clut[character & 1];
    const u16 *backgroundClut = backgroundSample->clut[background & 1];
    s32 row;
    s32 word;
    s32 index;

    for (index = TEAM_LOGO_CHARACTER_COLOR_FIRST;
         index < TEAM_LOGO_BACKGROUND_COLOR_FIRST; index++) {
        g_TeamLogoSwatches[index - TEAM_LOGO_CHARACTER_COLOR_FIRST] =
            characterClut[index];
        g_TeamLogoClut[index] = characterClut[index];
    }
    for (; index < TEAM_LOGO_COLOR_COUNT; index++) {
        g_TeamLogoClut[index] = backgroundClut[index];
    }

    for (row = 0; row < 64; row++) {
        for (word = 0; word < TEAM_LOGO_HALFWORDS_PER_ROW; word++) {
            g_TeamLogoCanvas
                .halfwords[row * TEAM_LOGO_HALFWORDS_PER_ROW + word] =
                CompositeLogoPixels(characterSample->canvas[row][word],
                                    backgroundSample->canvas[row][word]);
        }
    }
}
