#include <stdint.h>
#include <stdio.h>

#include "rage/track_asset_identity.h"

static int failures;

#define EXPECT_EQ(expected, actual) do {                                      \
    uint32_t expected_value = (uint32_t)(expected);                            \
    uint32_t actual_value = (uint32_t)(actual);                                \
    if (expected_value != actual_value) {                                      \
        fprintf(stderr, "%s:%d: expected %u, got %u\n", __FILE__, __LINE__,  \
                expected_value, actual_value);                                 \
        failures++;                                                            \
    }                                                                          \
} while (0)

int main(void) {
    uint64_t revision;

    /* The prologue loads OVAL1 (94), then changes the live course index so a
     * fresh calculation would point at BIG1 (88).  Rendering must retain 94. */
    RageTrackAssetIdentitySet(-1);
    EXPECT_EQ(88, RageTrackAssetIdentityResolve(88));
    revision = RageTrackAssetIdentityRevision();
    RageTrackAssetIdentitySet(94);
    EXPECT_EQ(94, RageTrackAssetIdentityResolve(88));
    EXPECT_EQ(revision + 1, RageTrackAssetIdentityRevision());

    /* Reloading the same track still starts a new resident asset lifetime.
     * GPU resources from the previous scene must therefore be invalidated. */
    revision = RageTrackAssetIdentityRevision();
    RageTrackAssetIdentitySet(94);
    EXPECT_EQ(revision + 1, RageTrackAssetIdentityRevision());

    return failures == 0 ? 0 : 1;
}
