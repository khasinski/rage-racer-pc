#include "rage/render_world_scene.h"

int GameRenderWorldSceneHas3d(GameSceneId scene) {
    return scene == GAME_SCENE_RACE || scene == GAME_SCENE_REPLAY ||
           scene == GAME_SCENE_ATTRACT_DEMO || scene == GAME_SCENE_PROLOGUE;
}

int GameRenderWorldSceneCanInterpolate(GameSceneId scene, int sceneTimer) {
    enum { GRAND_PRIX_INTRO_FRAMES = 90 };
    return scene != GAME_SCENE_RACE || sceneTimer >= GRAND_PRIX_INTRO_FRAMES;
}
