#ifndef GAME_WORK_BUFFER_H
#define GAME_WORK_BUFFER_H

#include "common.h"
#include "game/replay.h"

typedef struct SpuTransferSampleBuffer {
    u8 transferArea[0x800];
    s16 channelA[2][0x100];
    s16 channelB[2][0x100];
} SpuTransferSampleBuffer;

typedef union GameWorkBuffer {
    u8 bytes[0x8CA0];
    SpuTransferSampleBuffer spuTransfer;
    ReplayGrandPrixFrame grandPrixReplay[GRAND_PRIX_REPLAY_SAMPLE_COUNT];
    ReplayTimeAttackFrame timeAttackReplay[TIME_ATTACK_REPLAY_SAMPLE_COUNT];
} GameWorkBuffer;

_Static_assert(sizeof(ReplayGrandPrixFrame) * GRAND_PRIX_REPLAY_SAMPLE_COUNT <=
                   0x8CA0,
               "Grand Prix replay frames must fit the shared work arena");
_Static_assert(sizeof(ReplayTimeAttackFrame) *
                       TIME_ATTACK_REPLAY_SAMPLE_COUNT <=
                   0x8CA0,
               "Time Attack replay frames must fit the shared work arena");

extern GameWorkBuffer g_ReplayFrameBuffer;

#endif
