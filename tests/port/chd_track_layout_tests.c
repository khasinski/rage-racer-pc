#include "chd_track_layout.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK_EQ(actual, expected) do {                                      \
    if ((actual) != (expected)) {                                             \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__,       \
                #actual, (int)(actual), (int)(expected));                     \
        return 1;                                                             \
    }                                                                         \
} while (0)

static int CheckTrack(const RageChdTrack *track, int sector, int endSector,
                      uint32_t frameOffset) {
    CHECK_EQ(track->sector, sector);
    CHECK_EQ(track->endSector, endSector);
    CHECK_EQ(track->frameOffset, frameOffset);
    return 0;
}

int main(void) {
    RageChdTrackLayout layout;
    RageChdTrack track;
    RageChdTrackLayout beforeInvalid;

    ChdTrackLayoutInit(&layout);
    CHECK_EQ(layout.nextSector, -150);
    CHECK_EQ(layout.nextFrameOffset, 0);

    memset(&track, 0, sizeof(track));
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 0, 1, 1000, 0, 0, 0, &track), 1);
    if (CheckTrack(&track, 0, 1000, 0) != 0) return 1;
    CHECK_EQ(layout.nextSector, 1000);
    CHECK_EQ(layout.nextFrameOffset, 1000);

    /* A V pregap occupies CHD frames but is not exposed as track data. */
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 1, 2, 755, 150, 10, 1, &track), 1);
    if (CheckTrack(&track, 1150, 1755, 1150) != 0) return 1;
    CHECK_EQ(layout.nextSector, 1765);
    CHECK_EQ(layout.nextFrameOffset, 1766);

    /* A normal pregap advances logical sectors but consumes no stored frame. */
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 2, 3, 101, 75, 0, 0, &track), 1);
    if (CheckTrack(&track, 1840, 1941, 1766) != 0) return 1;
    CHECK_EQ(layout.nextSector, 1941);
    CHECK_EQ(layout.nextFrameOffset, 1870);

    beforeInvalid = layout;
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 3, 5, 100, 0, 0, 0, &track), 0);
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 3, 4, 100, 100, 0, 1, &track), 0);
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 3, 4, 100, -1, 0, 0, &track), 0);
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 3, 4, 100, 0, -1, 0, &track), 0);
    CHECK_EQ(layout.nextSector, beforeInvalid.nextSector);
    CHECK_EQ(layout.nextFrameOffset, beforeInvalid.nextFrameOffset);
    CHECK_EQ(ChdTrackLayoutAppend(
                 NULL, 0, 1, 1, 0, 0, 0, &track), 0);
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 0, 1, 1, 0, 0, 0, NULL), 0);

    layout.nextSector = INT_MAX;
    layout.nextFrameOffset = UINT32_MAX;
    beforeInvalid = layout;
    CHECK_EQ(ChdTrackLayoutAppend(
                 &layout, 0, 1, 1, 0, 0, 0, &track), 0);
    CHECK_EQ(layout.nextSector, beforeInvalid.nextSector);
    CHECK_EQ(layout.nextFrameOffset, beforeInvalid.nextFrameOffset);

    puts("CHD track layout tests passed");
    return 0;
}
