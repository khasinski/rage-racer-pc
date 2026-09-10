#ifndef RAGE_RENDER_WORLD_SCENE_H
#define RAGE_RENDER_WORLD_SCENE_H

#include "game/scene.h"

/* Only live races and attract playback produce semantic 3D worlds. */
int GameRenderWorldSceneHas3d(GameSceneId scene);

#endif
