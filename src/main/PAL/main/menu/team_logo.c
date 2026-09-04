#include "game/menu.h"
#include "game/asset_internal.h"

enum {
    TEAM_LOGO_SAMPLE_CHOICE_COUNT = 20,
    TEAM_LOGO_BACKGROUND_SAMPLE_BASE = TEAM_LOGO_SAMPLE_RECORD_COUNT / 2,
    TEAM_LOGO_CHARACTER_COLOR_FIRST = 1,
    TEAM_LOGO_BACKGROUND_COLOR_FIRST = 12,
    TEAM_LOGO_COLOR_COUNT = 16,
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
    const TeamLogoSample *characterSample;
    const TeamLogoSample *backgroundSample;
    const u16 *characterClut;
    const u16 *backgroundClut;
    s32 row;
    s32 word;
    s32 index;

    if (g_TeamLogoSampleData == NULL ||
        (u32)character >= TEAM_LOGO_SAMPLE_CHOICE_COUNT ||
        (u32)background >= TEAM_LOGO_SAMPLE_CHOICE_COUNT) {
        return;
    }

    characterSample = &g_TeamLogoSampleData[character / 2];
    backgroundSample =
        &g_TeamLogoSampleData[TEAM_LOGO_BACKGROUND_SAMPLE_BASE + background / 2];
    characterClut = characterSample->clut[character & 1];
    backgroundClut = backgroundSample->clut[background & 1];

    for (index = TEAM_LOGO_CHARACTER_COLOR_FIRST;
         index < TEAM_LOGO_BACKGROUND_COLOR_FIRST; index++) {
        g_TeamLogoSwatches[index - TEAM_LOGO_CHARACTER_COLOR_FIRST] =
            characterClut[index];
        g_TeamLogoClut[index] = characterClut[index];
    }
    for (; index < TEAM_LOGO_COLOR_COUNT; index++) {
        g_TeamLogoClut[index] = backgroundClut[index];
    }

    for (row = 0; row < TEAM_LOGO_HEIGHT; row++) {
        for (word = 0; word < TEAM_LOGO_HALFWORDS_PER_ROW; word++) {
            g_TeamLogoCanvas.halfwordRows[row][word] =
                CompositeLogoPixels(characterSample->canvas[row][word],
                                    backgroundSample->canvas[row][word]);
        }
    }
}
