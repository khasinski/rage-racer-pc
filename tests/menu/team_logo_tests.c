#include <stdio.h>
#include <string.h>

#include "common.h"
#include "game/asset_internal.h"
#include "game/menu.h"

const TeamLogoSample *g_TeamLogoSampleData;
TeamLogoCanvas g_TeamLogoCanvas;
u16 g_TeamLogoClut[16];
u16 g_TeamLogoSwatches[15];

static TeamLogoSample samples[20];

static int Check(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
    }
    return condition;
}

int main(void) {
    TeamLogoCanvas savedCanvas;
    u16 savedClut[16];
    u16 savedSwatches[15];
    s32 index;
    int ok = 1;

    memset(samples, 0, sizeof(samples));
    memset(g_TeamLogoClut, 0xA5, sizeof(g_TeamLogoClut));
    memset(g_TeamLogoSwatches, 0, sizeof(g_TeamLogoSwatches));
    g_TeamLogoSampleData = samples;

    for (index = 1; index < 12; index++) {
        samples[0].clut[0][index] = (u16)(0x100 + index);
    }
    for (index = 12; index < 16; index++) {
        samples[10].clut[0][index] = (u16)(0x200 + index);
    }

    ComposeSampleTeamLogo(0, 0);

    ok &= Check(g_TeamLogoClut[0] == 0xA5A5, "colour zero was overwritten");
    for (index = 1; index < 12; index++) {
        ok &= Check(g_TeamLogoSwatches[index - 1] == (u16)(0x100 + index),
                    "character swatch differs");
        ok &= Check(g_TeamLogoClut[index] == (u16)(0x100 + index),
                    "character CLUT differs");
    }
    for (index = 12; index < 16; index++) {
        ok &= Check(g_TeamLogoClut[index] == (u16)(0x200 + index),
                    "background CLUT differs");
    }

    memset(samples, 0, sizeof(samples));
    for (index = 1; index < 12; index++) {
        samples[1].clut[1][index] = (u16)(0x300 + index);
    }
    for (index = 12; index < 16; index++) {
        samples[12].clut[1][index] = (u16)(0x400 + index);
    }
    samples[1].canvas[0][0] = 0x1020;
    samples[12].canvas[0][0] = 0xABCD;
    samples[1].canvas[63][15] = 0x0000;
    samples[12].canvas[63][15] = 0x5678;

    ComposeSampleTeamLogo(3, 5);

    ok &= Check(g_TeamLogoCanvas.halfwords[0] == 0x1B2D,
                "transparent character pixels were not composited");
    ok &= Check(g_TeamLogoCanvas.halfwords[1023] == 0x5678,
                "last background word was not copied");
    for (index = 1; index < 12; index++) {
        ok &= Check(g_TeamLogoClut[index] == (u16)(0x300 + index),
                    "odd character variant was not selected");
    }
    for (index = 12; index < 16; index++) {
        ok &= Check(g_TeamLogoClut[index] == (u16)(0x400 + index),
                    "odd background variant was not selected");
    }

    savedCanvas = g_TeamLogoCanvas;
    memcpy(savedClut, g_TeamLogoClut, sizeof(savedClut));
    memcpy(savedSwatches, g_TeamLogoSwatches, sizeof(savedSwatches));
    ComposeSampleTeamLogo(-1, 20);
    ok &= Check(memcmp(&g_TeamLogoCanvas, &savedCanvas, sizeof(savedCanvas)) == 0,
                "invalid sample changed the canvas");
    ok &= Check(memcmp(g_TeamLogoClut, savedClut, sizeof(savedClut)) == 0,
                "invalid sample changed the CLUT");
    ok &= Check(memcmp(g_TeamLogoSwatches, savedSwatches,
                       sizeof(savedSwatches)) == 0,
                "invalid sample changed the swatches");

    g_TeamLogoSampleData = NULL;
    ComposeSampleTeamLogo(0, 0);
    ok &= Check(memcmp(&g_TeamLogoCanvas, &savedCanvas, sizeof(savedCanvas)) == 0,
                "missing sample bank changed the canvas");

    if (!ok) {
        return 1;
    }
    puts("team logo samples compose palettes and pixels correctly");
    return 0;
}
