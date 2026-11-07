#include "game/track_look.h"
#include <string.h>

int ReadTrackLook(const void *data, size_t size, TrackLook *look) {
    if (!data || !look || size < sizeof(*look)) return 0;
    TrackLook candidate;
    memcpy(&candidate, data, sizeof(candidate));
    *look = candidate;
    return 1;
}

int ReadRivalLook(const TrackLook *track, s32 course, s32 slot,
                  s32 modelCount, RivalLook *look) {
    if (!track || !look || (u32)course >= CAR_MODEL_COURSE_COUNT ||
        (u32)slot >= RACE_CAR_SLOT_COUNT) return 0;
    const u32 model = g_CarModelByCourse[course][slot];
    const s16 *bank = g_CarModelBankTable[model];
    if (modelCount <= bank[0] + 4) return 0;
    const CarModelRenderParams *params = &track->models[model];
    *look = (RivalLook){
        .shape = {params->axis0, (s16)params->axis1, (s16)params->axis2, params->horizon},
        .bodyMesh = (u32)bank[0], .palette = (u8)bank[1]};
    return 1;
}

const s16 g_CarModelBankTable[CAR_MODEL_BANK_ENTRY_COUNT][CAR_MODEL_BANK_FIELDS] = {
    {0, 0},
    {5, 0},
    {10, 0},
    {15, 0},
    {20, 0},
    {20, 1},
    {25, 0},
    {25, 1},
    {30, 0},
    {30, 1},
    {30, 2},
};

const u8 g_CarModelByCourse[CAR_MODEL_COURSE_COUNT][RACE_CAR_SLOT_COUNT] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
    {1, 2, 0, 3, 4, 5, 6, 7, 8, 9, 10},
    {2, 0, 1, 3, 4, 5, 6, 7, 8, 9, 10},
    {3, 0, 1, 2, 4, 5, 6, 7, 8, 9, 10},
};
