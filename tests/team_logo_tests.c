#include <assert.h>
#include <string.h>

#include "common.h"
#include "game/asset_internal.h"
#include "game/menu.h"

TeamLogoSample *g_TeamLogoSampleData;
TeamLogoCanvas g_TeamLogoCanvas;
u16 g_TeamLogoClut[16];
u16 g_TeamLogoSwatches[15];

static TeamLogoSample samples[20];

int main(void)
{
    s32 index;

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

    assert(g_TeamLogoClut[0] == 0xA5A5);
    for (index = 1; index < 12; index++) {
        assert(g_TeamLogoSwatches[index - 1] == (u16)(0x100 + index));
        assert(g_TeamLogoClut[index] == (u16)(0x100 + index));
    }
    for (index = 12; index < 16; index++) {
        assert(g_TeamLogoClut[index] == (u16)(0x200 + index));
    }

    return 0;
}
