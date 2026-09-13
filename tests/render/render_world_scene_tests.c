#include "rage/render_world_scene.h"

#include <stdio.h>

int main(void) {
    if (!GameRenderWorldSceneHas3d(GAME_SCENE_RACE) ||
        !GameRenderWorldSceneHas3d(GAME_SCENE_REPLAY) ||
        !GameRenderWorldSceneHas3d(GAME_SCENE_ATTRACT_DEMO) ||
        !GameRenderWorldSceneHas3d(GAME_SCENE_PROLOGUE) ||
        GameRenderWorldSceneHas3d(GAME_SCENE_ENTER_LOST_RACE) ||
        GameRenderWorldSceneHas3d(GAME_SCENE_LOST_RACE) ||
        GameRenderWorldSceneHas3d(GAME_SCENE_RACE_END) ||
        GameRenderWorldSceneHas3d(GAME_SCENE_MENU)) {
        fputs("render world scene selection failed\n", stderr);
        return 1;
    }
    if (GameRenderWorldSceneCanInterpolate(GAME_SCENE_RACE, 0) ||
        GameRenderWorldSceneCanInterpolate(GAME_SCENE_RACE, 89) ||
        !GameRenderWorldSceneCanInterpolate(GAME_SCENE_RACE, 90) ||
        !GameRenderWorldSceneCanInterpolate(GAME_SCENE_REPLAY, 0) ||
        !GameRenderWorldSceneCanInterpolate(GAME_SCENE_ATTRACT_DEMO, 0)) {
        fputs("render world interpolation policy failed\n", stderr);
        return 1;
    }
    puts("render world only follows scenes with live 3D packets");
    return 0;
}
