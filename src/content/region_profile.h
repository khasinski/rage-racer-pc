#ifndef RAGE_REGION_PROFILE_H
#define RAGE_REGION_PROFILE_H
/* Immutable built-in regional defaults, usable without game globals or GPU.
 * FMV offsets/frame counts remain derived from the actual mounted disc. */
typedef enum RageRegionId {
    RAGE_REGION_UNKNOWN, RAGE_REGION_PAL, RAGE_REGION_NTSC_U, RAGE_REGION_NTSC_J
} RageRegionId;
typedef struct RageRegionProfile {
    RageRegionId id;
    const char *name;
    unsigned baseHz;
} RageRegionProfile;
/* Returned profiles have process lifetime. Unknown input returns UNKNOWN,
 * whose 50 Hz value preserves the game's defensive PAL default. */
const RageRegionProfile *RageRegionByName(const char *name);
/* Classifies serial families, not whether a particular disc is supported. */
const RageRegionProfile *RageRegionByBootName(const char *boot);
#endif
