#ifndef GAME_PROLOGUE_STATE_H
#define GAME_PROLOGUE_STATE_H

#include "common.h"

typedef enum PrologueStep {
    PROLOGUE_STEP_LOAD_TEXTURES,
    PROLOGUE_STEP_LOAD_TRACK,
    PROLOGUE_STEP_WAIT_FOR_FADE,
    PROLOGUE_STEP_ACTIVE,
} PrologueStep;

typedef struct Prologue {
    PrologueStep step;
    s32 cameraCut;
} Prologue;

#endif
