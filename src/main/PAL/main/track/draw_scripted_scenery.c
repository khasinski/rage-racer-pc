#include "game/race.h"
#include "game/track_internal.h"

void DrawScriptedScenery(s32 flags) {
    switch (g_GrandPrixClass % 5) {
    case 0:
        if (flags != 0) {
            UpdateRouteScenery();
        }
        DrawRouteScenery();
        break;
    case 1:
    case 2:
        if (flags != 0) {
            UpdateRouteScenery();
            UpdateFlybyScenery();
        }
        DrawRouteScenery();
        DrawFlybyScenery();
        break;
    case 3:
    case 4:
        if (flags != 0) {
            UpdateRouteScenery();
            UpdateFlybyScenery();
            UpdatePathScenerySound();
        }
        DrawRouteScenery();
        DrawFlybyScenery();
        DrawPathScenery();
        break;
    }
}
