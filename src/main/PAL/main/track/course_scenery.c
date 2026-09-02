#include "game/race.h"
#include "game/state.h"
#include "game/track_internal.h"

enum {
    REPLAY_SCENE_ID = 0x11,
};

static void DrawCourseObjects(s32 course, s32 timer, s32 animate,
                              s32 useAlternateAnimation) {
    if (g_GrandPrixClass == 5) {
        animate = 0;
    }

    switch (course) {
    case 0:
        DrawSpinningScenery(timer, animate);
        if (g_GrandPrixClass >= 4) {
            DrawHighClassScenery();
        }
        DrawStaticScenery(0);
        break;
    case 1:
        if (g_GrandPrixClass >= 2) {
            DrawSpinningScenery(timer, animate);
        }
        if (animate != 0) {
            UpdateShuttleScenery(0);
        }
        DrawShuttleScenery(0);
        DrawStaticScenery(0);
        break;
    case 2:
        if (animate != 0) {
            UpdateShuttleScenery(0);
            UpdateShuttleScenery(1);
        }
        DrawShuttleScenery(0);
        DrawShuttleScenery(1);
        DrawStaticScenery(0);
        break;
    case 3:
        if (useAlternateAnimation) {
            DrawAnimatedScenery2(timer, 1,
                                 g_SceneId == REPLAY_SCENE_ID, animate);
        } else {
            DrawAnimatedScenery(timer, 1);
        }
        DrawStaticScenery(1);
        break;
    }
}

void DrawCourseScenery(s32 course, s32 timer, s32 animate) {
    DrawAnimatedScenery(timer, 0);
    DrawCourseObjects(course, timer, animate, 0);
}

void DrawCourseScenery2(s32 timer, s32 animate) {
    DrawAnimatedScenery2(timer, 0, g_SceneId == REPLAY_SCENE_ID,
                         g_GrandPrixClass == 5 ? 0 : animate);
    DrawCourseObjects(SeriesCourseIndex(), timer, animate, 1);
}
