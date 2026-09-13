#ifndef RAGE_RENDER_WORLD_SCENE_H
#define RAGE_RENDER_WORLD_SCENE_H

#include "game/scene.h"

/* Race and the three scripted driving presentations produce semantic 3D worlds. */
int GameRenderWorldSceneHas3d(GameSceneId scene);

/* The scripted Grand Prix flyby cuts between authored cameras.  Blending
 * adjacent snapshots there can combine geometry from one shot with the
 * camera from another, so it is presented at the logic rate. */
int GameRenderWorldSceneCanInterpolate(GameSceneId scene, int sceneTimer);

#endif
