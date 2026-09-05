#include "region_profile.h"
#include <stddef.h>
#include <string.h>

static const RageRegionProfile profiles[] = {
    {RAGE_REGION_UNKNOWN, "unknown", 50},
    {RAGE_REGION_PAL, "PAL", 50},
    {RAGE_REGION_NTSC_U, "NTSC-U", 60},
    {RAGE_REGION_NTSC_J, "NTSC-J", 60},
};
static const struct { const char *prefix; RageRegionId id; } families[] = {
    {"SCES", RAGE_REGION_PAL}, {"SLES", RAGE_REGION_PAL},
    {"SCED", RAGE_REGION_PAL}, {"SCUS", RAGE_REGION_NTSC_U},
    {"SLUS", RAGE_REGION_NTSC_U}, {"SCPS", RAGE_REGION_NTSC_J},
    {"SLPS", RAGE_REGION_NTSC_J}, {"SLPM", RAGE_REGION_NTSC_J},
    {"SCPM", RAGE_REGION_NTSC_J},
};
const RageRegionProfile *RageRegionByName(const char *name) {
    if (name != NULL)
        for (size_t i = 1; i < sizeof(profiles) / sizeof(profiles[0]); ++i)
            if (strcmp(name, profiles[i].name) == 0) return &profiles[i];
    return &profiles[RAGE_REGION_UNKNOWN];
}
const RageRegionProfile *RageRegionByBootName(const char *boot) {
    if (boot != NULL) {
        for (size_t i = 0; i < sizeof(families) / sizeof(families[0]); ++i) {
            size_t j;
            for (j = 0; j < 4; ++j) {
                unsigned char c = (unsigned char)boot[j];
                if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
                if (c != (unsigned char)families[i].prefix[j]) break;
            }
            if (j == 4) return &profiles[families[i].id];
        }
    }
    return &profiles[RAGE_REGION_UNKNOWN];
}
