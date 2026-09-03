#ifndef RAGE_CHD_TRACK_LAYOUT_H
#define RAGE_CHD_TRACK_LAYOUT_H

#include <stdint.h>

typedef struct RageChdTrack {
    int sector;
    int endSector;
    uint32_t frameOffset;
    int audio;
} RageChdTrack;

typedef struct RageChdTrackLayout {
    int nextSector;
    uint32_t nextFrameOffset;
} RageChdTrackLayout;

void ChdTrackLayoutInit(RageChdTrackLayout *layout);
int ChdTrackLayoutAppend(RageChdTrackLayout *layout, int index, int number,
                         int frames, int pregap, int postgap,
                         int pregapStoredInTrack, RageChdTrack *track);

#endif
