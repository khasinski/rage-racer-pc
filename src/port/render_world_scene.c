#include "rage/render_world_scene.h"

int GameRenderWorldSceneHas3d(GameSceneId scene) {
    return scene == GAME_SCENE_RACE || scene == GAME_SCENE_ATTRACT_DEMO;
}
