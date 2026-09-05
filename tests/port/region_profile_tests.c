#include "content/region_profile.h"
#include <stdio.h>
#include <string.h>
static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); } } while (0)
int main(void) {
    const char *boots[] = {"SCES", "SLES", "SCED", "SCUS", "SLUS",
                           "SCPS", "SLPS", "SLPM", "SCPM"};
    const char *names[] = {"PAL", "PAL", "PAL", "NTSC-U", "NTSC-U",
                           "NTSC-J", "NTSC-J", "NTSC-J", "NTSC-J"};
    for (unsigned i = 0; i < 9; ++i) {
        const RageRegionProfile *profile = RageRegionByBootName(boots[i]);
        CHECK(strcmp(profile->name, names[i]) == 0);
        CHECK(profile == RageRegionByName(names[i]));
        CHECK(profile->baseHz == (i < 3 ? 50u : 60u));
        char lower[5];
        for (unsigned j = 0; j < 4; ++j) lower[j] = boots[i][j] + ('a' - 'A');
        lower[4] = '\0';
        CHECK(RageRegionByBootName(lower) == profile);
        for (unsigned j = 0; j < 4; ++j) {
            char shortName[5] = {0};
            memcpy(shortName, boots[i], j);
            CHECK(RageRegionByBootName(shortName)->id == RAGE_REGION_UNKNOWN);
        }
    }
    CHECK(RageRegionByBootName("SLUS_004.03")->id == RAGE_REGION_NTSC_U);
    CHECK(RageRegionByBootName(NULL)->id == RAGE_REGION_UNKNOWN);
    CHECK(RageRegionByBootName("INVALID")->id == RAGE_REGION_UNKNOWN);
    CHECK(RageRegionByName(NULL)->baseHz == 50);
    CHECK(RageRegionByName("NTSC-invalid")->id == RAGE_REGION_UNKNOWN);
    CHECK(RageRegionByName("pal")->id == RAGE_REGION_UNKNOWN);
    return failures != 0;
}
