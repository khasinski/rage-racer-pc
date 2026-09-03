#ifndef GAME_SCENERY_RENDER_INTERNAL_H
#define GAME_SCENERY_RENDER_INTERNAL_H

#include "common.h"
#include "psyq/gte.h"

void BuildSceneryObjectMatrix(Matrix *matrix, s32 rotationX, s32 rotationY,
                              s32 rotationZ);

#endif
