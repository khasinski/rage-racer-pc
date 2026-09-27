#ifndef GAME_ENGINE_SOUND_H
#define GAME_ENGINE_SOUND_H
#include "game/car.h"

/* Presentation history/output for one car, independent of physics RNG/audio. */
typedef struct EngineSound {
    s32 rpm, jitter;
    int shiftLight, powered;
} EngineSound;

/* Caller owns state; car/spec are read-only and the noise seed is passed by
 * value. Invalid input leaves state unchanged. Call once per presentation tick. */
int StepEngineSound(EngineSound *sound, const GameCarDrive *drive,
                    const GameCarSpec *spec, u32 frame, u32 seed);
#endif
