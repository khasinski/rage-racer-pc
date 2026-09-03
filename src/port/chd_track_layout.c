#include "chd_track_layout.h"

#include <limits.h>
#include <stddef.h>

enum {
    CD_LEAD_IN_SECTORS = 150,
    CHD_FRAME_ALIGNMENT = 4,
};

void ChdTrackLayoutInit(RageChdTrackLayout *layout) {
    layout->nextSector = -CD_LEAD_IN_SECTORS;
    layout->nextFrameOffset = 0;
}

int ChdTrackLayoutAppend(RageChdTrackLayout *layout, int index, int number,
                         int frames, int pregap, int postgap,
                         int pregapStoredInTrack, RageChdTrack *track) {
    int logicalPregap;
    int storedPregap;
    int playableFrames;
    uint32_t alignedFrames;
    int64_t sector;
    int64_t endSector;
    int64_t nextSector;
    uint64_t frameOffset;
    uint64_t nextFrameOffset;

    if (layout == NULL || track == NULL || index < 0 ||
        (int64_t)number != (int64_t)index + 1 || frames <= 0 ||
        pregap < 0 || postgap < 0) {
        return 0;
    }
    logicalPregap = index == 0
        ? CD_LEAD_IN_SECTORS
        : pregapStoredInTrack ? 0 : pregap;
    storedPregap = pregapStoredInTrack ? pregap : 0;
    playableFrames = frames - storedPregap;
    if (playableFrames <= 0) return 0;

    alignedFrames = ((uint32_t)frames + CHD_FRAME_ALIGNMENT - 1U) &
                    ~(CHD_FRAME_ALIGNMENT - 1U);
    sector = (int64_t)layout->nextSector + logicalPregap + storedPregap;
    endSector = sector + playableFrames;
    nextSector = endSector + postgap;
    frameOffset = (uint64_t)layout->nextFrameOffset + storedPregap;
    nextFrameOffset = frameOffset + (uint32_t)playableFrames +
                      (uint32_t)postgap +
                      (alignedFrames - (uint32_t)frames);
    if (sector < INT_MIN || sector > INT_MAX ||
        endSector < INT_MIN || endSector > INT_MAX ||
        nextSector < INT_MIN || nextSector > INT_MAX ||
        frameOffset > UINT32_MAX || nextFrameOffset > UINT32_MAX) {
        return 0;
    }

    track->sector = (int)sector;
    track->endSector = (int)endSector;
    track->frameOffset = (uint32_t)frameOffset;
    layout->nextSector = (int)nextSector;
    layout->nextFrameOffset = (uint32_t)nextFrameOffset;
    return 1;
}
