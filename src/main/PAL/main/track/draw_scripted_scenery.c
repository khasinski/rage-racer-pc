#include "game/race.h"
#include "game/track_internal.h"

void DrawScriptedScenery(s32 animate) {
    s32 sceneryTier = g_GrandPrixClass % 5;

    /* Preserve the retail switch's no-op for values outside its cases. */
    if (sceneryTier < 0 || sceneryTier > 4) {
        return;
    }

    if (animate != 0) {
        UpdateRouteScenery();
        if (sceneryTier >= 1) {
            UpdateFlybyScenery();
        }
        if (sceneryTier >= 3) {
            UpdatePathScenery();
        }
    }

    DrawRouteScenery();
    if (sceneryTier >= 1) {
        DrawFlybyScenery();
    }
    if (sceneryTier >= 3) {
        DrawPathScenery();
    }
}
