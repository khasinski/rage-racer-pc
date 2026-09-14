#include "game/prim.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/fmv.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/frontend_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/grand_prix_content.h"
#include "game/random.h"
#include "game/render_internal.h"
#include "game/screens.h"
#include "psyq/cd.h"

enum {
    FRONTEND_ATTRACT_LOAD_TRACK = 0x1CC,
    FRONTEND_ATTRACT_START_RACE = 0x1CD,
    FRONTEND_ATTRACT_WAIT_RACE = 0x1CE,
    FRONTEND_ATTRACT_READY = 0x1CF,
    FRONTEND_IDLE_FRAMES = 900,
};

void UpdateMainMenuExit(void) {
    Frontend *frontend = MenuFrontend();

    frontend->pulse++;
    DrawFullscreenFadeTile(frontend->pulse * 2, 0x59);

    if (frontend->pulse >= 0x81) {
        switch (frontend->selection) {
        case TITLE_MENU_GRAND_PRIX:
        case TITLE_MENU_EXTRA_GRAND_PRIX:
            g_GrandPrixMode = 1;
            if (g_RaceProgress->maxClassReached == -1) {
                g_RaceProgress->maxClassReached = 0;
                g_SceneId = 0x1F;
                g_GrandPrixSeries = 0;
            } else {
                g_SceneId = 6;
            }
            break;
        case TITLE_MENU_TIME_ATTACK:
            g_GrandPrixMode = 0;
            g_SceneId = 6;
            break;
        case TITLE_MENU_LOAD_SAVE:
            g_SceneId = 0x19;
            break;
        case TITLE_MENU_OPTIONS:
            g_SceneId = 0x16;
            break;
        }
    }

    DrawMainMenuRows();
}

void UpdateTitleAttract(void) {
    const Frontend *frontend = MenuFrontend();
    s32 alpha;
    u16 panelClut = 0x7E00;
    void *orderingTable = GamePrimaryOrderingTable(1);
    void *next;

    alpha = 0x7F - frontend->menuSlide * 2;
    if (alpha < 0x30) {
        alpha = 0x30;
    } else if (alpha > 0x7F) {
        alpha = 0x7F;
    }

    next = RENDER_PRIM_CURSOR;
    next = GameQueueShadedSprite(orderingTable, next, 0x28, 0xA0, 0xF0,
                                 0x18, 0, 0x88, 0x7DC0, alpha);
    next = GameQueueShadedSprite(orderingTable, next, 0x20, 0xB8, 0x100,
                                 0x10, 0, 0xF0, 0x7DC1, alpha);
    next = GameQueueShadedSprite(orderingTable, next, 0x11A, 0xAF, 0xC, 8,
                                 0xE0, 0xB0, 0x7DC0, alpha);
    next = QueueDrawModePrim(orderingTable, next, 0x19);

    if (CountClassWins(g_ClassRecords, CLASS_RECORD_COUNT) >=
        CLASS_RECORD_COUNT) {
        panelClut = 0x7D80;
    }

    next = GameQueueShadedSprite(orderingTable, next, 0x34, 0x18, 0x6C,
                                 0x88, 0, 0, panelClut, alpha);
    g_RenderState.draw.packetCursor = GameQueueShadedTexturedRect(
        orderingTable, next, 0xA0, 0x18, -0x6C, 0x88, 0, 0, panelClut, 0x99,
        alpha);
}


static void UpdateAttractRaceLoading(void) {
    const Frontend *frontend = MenuFrontend();
    s32 randomCourse;
    const GrandPrixClassDefinition *definition;

    if (frontend->state == FRONTEND_STATE_MENU_EXIT ||
        (frontend->attractCycle % 2) != 0) {
        return;
    }

    switch (g_SceneTimer) {
    case FRONTEND_ATTRACT_LOAD_TRACK:
        g_GrandPrixSeries = 0;
        g_GrandPrixClass = RandomIndex(5);
        definition = GrandPrixContentClass(g_GrandPrixClass);
        randomCourse = RandomIndex(4);
        g_CourseIndex = randomCourse;
        if (randomCourse >= definition->courseCount) {
            g_CourseIndex = RandomIndex(definition->courseCount);
        }
        RequestCourseTextureAssets();
        g_SceneTimer++;
        break;
    case FRONTEND_ATTRACT_START_RACE:
        if (AssetLoadCompletedSuccessfully()) {
            RequestRaceStart();
            g_SceneTimer++;
        }
        break;
    case FRONTEND_ATTRACT_WAIT_RACE:
        if (AssetLoadCompletedSuccessfully()) {
            g_SceneTimer = FRONTEND_ATTRACT_READY;
        }
        break;
    }
}

static void UpdateFrontendIdleAttract(void) {
    Frontend *frontend = MenuFrontend();

    if (frontend->idleTimer < FRONTEND_IDLE_FRAMES) {
        frontend->idleTimer++;
        return;
    }

    if ((frontend->attractCycle % 2) != 0) {
        BeginIntroFmv(3);
        frontend->attractCycle++;
    } else if (g_SceneTimer == FRONTEND_ATTRACT_READY) {
        g_GrandPrixMode = 1;
        g_SceneId = 0x1D;
        frontend->attractCycle++;
    }
}

void UpdateFrontend(void) {
    Frontend *frontend = MenuFrontend();
    u32 sceneTimer;

    g_AnimTimer++;
    Random15();

    if (frontend->attractTimer > 0) {
        frontend->attractTimer--;
    }
    if (frontend->attractTimer == 0 && CdControl(9, 0, 0) == 1) {
        frontend->attractTimer--;
    }
    if (frontend->exitTimer != 0 && --frontend->exitTimer == 0) {
        PlaySoundCue(0x1A);
    }

    sceneTimer = g_SceneTimer;
    if (sceneTimer < FRONTEND_ATTRACT_LOAD_TRACK) {
        g_SceneTimer = sceneTimer + 1;
    } else {
        UpdateAttractRaceLoading();
    }

    sceneTimer = g_SceneTimer;
    if (sceneTimer == 0xF) {
        SetDispMask(1);
    }
    if (sceneTimer == 1) {
        SetupDisplay240(0, 0, 0);
    }

    if ((u32)frontend->state >= FRONTEND_STATE_COUNT) {
        frontend->state = FRONTEND_STATE_TITLE;
    }
    g_FrontendDrawHandlers[frontend->state]();
    UpdateFrontendIdleAttract();
    UpdateTitleAttract();
}
