#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/car.h"
#include "game/input_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/track.h"
#include "runtime_config.h"
#include "scenario_control.h"

extern int g_SceneId;

typedef struct RageScenarioState {
    int initialized, enabled;
    int mode, series, classIndex, course, car;
    int afterFinish, raceFinished, resultSeen, exitRequested;
    int grid[11], customGrid, gridApplied;
    int playerTrackPoint, rivalTrackPoints[11], rivalTrackPointCount;
    int customStart, startApplied, freezeStarts;
    int lastScene, lastFrontend, lastMenuScreen, stableFrames, retryFrames;
} RageScenarioState;

enum {
    RAGE_SCENARIO_AFTER_MENU,
    RAGE_SCENARIO_AFTER_REPEAT,
    RAGE_SCENARIO_AFTER_EXIT,
};

static RageScenarioState s_scenario;

static int RageScenarioParseTrackPoint(const char *text, int *result) {
    char *end;
    long value;
    if (text == NULL || text[0] == '\0') return 0;
    value = strtol(text, &end, 0);
    if (*end != '\0' || value < 0 || value > INT_MAX) return 0;
    *result = (int)value;
    return 1;
}

static void RageScenarioParseTrackStarts(void) {
    const char *player = RageRuntimeConfigGet("start.player_track_point");
    const char *rivals = RageRuntimeConfigGet("start.rival_track_points");
    char buffer[512], *token;
    int value;
    if (player != NULL) {
        if (!RageScenarioParseTrackPoint(player, &value))
            fprintf(stderr, "rage-port: invalid start.player_track_point=%s\n", player);
        else {
            s_scenario.playerTrackPoint = value;
            s_scenario.customStart = 1;
        }
    }
    if (rivals == NULL || rivals[0] == '\0') return;
    if (strlen(rivals) >= sizeof(buffer)) goto invalid;
    strcpy(buffer, rivals);
    token = strtok(buffer, ",");
    while (token != NULL && s_scenario.rivalTrackPointCount < 11) {
        while (*token == ' ' || *token == '\t') token++;
        if (!strcmp(token, "-") || !strcmp(token, "default")) value = -1;
        else if (!RageScenarioParseTrackPoint(token, &value)) goto invalid;
        s_scenario.rivalTrackPoints[s_scenario.rivalTrackPointCount++] = value;
        token = strtok(NULL, ",");
    }
    if (token != NULL) goto invalid;
    s_scenario.customStart = 1;
    return;
invalid:
    s_scenario.rivalTrackPointCount = 0;
    fprintf(stderr, "rage-port: invalid start.rival_track_points\n");
}

static int RageScenarioPlaceCar(GameCarRuntime *car, int point) {
    CarTrackLimits limits = {0, 0, 0, 0};
    if (point < 0) return 1;
    if (point >= g_TrackPointCount) return 0;
    car->trackPointIndex = point;
    car->x = g_TrackPoints[point].x;
    car->z = g_TrackPoints[point].z;
    car->y = 0;
    car->bodyPitch = car->bodyRoll = 0;
    car->bodyYaw = (0xC00 - (g_RaceSeries << 11) -
                    g_TrackPoints[point].angle) & 0xFFF;
    car->headingAngle = car->bodyYaw;
    car->trackPointIndex = FindTrackSegment(car, point);
    SeedCarLapProgress(car, 0);
    UpdateCarTrackState(car, car->trackPointIndex, &limits);
    car->previousTrackProgress = car->trackProgress;
    car->modelY = car->y;
    CopyCarBodyRotationToModel(car);
    return 1;
}

static void RageScenarioApplyTrackStarts(void) {
    int index;
    if (s_scenario.playerTrackPoint >= 0 &&
        !RageScenarioPlaceCar((GameCarRuntime *)(void *)&g_PlayerCar,
                              s_scenario.playerTrackPoint)) {
        fprintf(stderr, "rage-port: player track point %d outside 0..%d\n",
                s_scenario.playerTrackPoint, g_TrackPointCount - 1);
    } else if (s_scenario.playerTrackPoint >= 0) {
        fprintf(stderr,
                "rage-port: scenario-start player point=%d pos=%d,%d progress=%d section=%d\n",
                s_scenario.playerTrackPoint, g_PlayerCar.x, g_PlayerCar.z,
                g_PlayerCar.trackProgress, g_PlayerCar.trackSection);
    }
    for (index = 0; index < s_scenario.rivalTrackPointCount; index++) {
        int point = s_scenario.rivalTrackPoints[index];
        if (point >= 0 && g_Cars[index].activeFlag &&
            !RageScenarioPlaceCar(&g_Cars[index], point)) {
            fprintf(stderr, "rage-port: rival %d track point %d outside 0..%d\n",
                    index, point, g_TrackPointCount - 1);
        } else if (point >= 0 && g_Cars[index].activeFlag) {
            fprintf(stderr,
                    "rage-port: scenario-start rival=%d point=%d pos=%d,%d progress=%d section=%d\n",
                    index, point, g_Cars[index].x, g_Cars[index].z,
                    g_Cars[index].trackProgress, g_Cars[index].trackSection);
        }
    }
    SetTrackTexturePageNow(g_PlayerCar.trackSection);
    s_scenario.startApplied = 1;
    fprintf(stderr,
            "rage-port: custom track start applied player=%d rivals=%d points=%d\n",
            s_scenario.playerTrackPoint, s_scenario.rivalTrackPointCount,
            g_TrackPointCount);
}

static void RageScenarioHoldTrackStarts(void) {
    int index;
    if (s_scenario.playerTrackPoint >= 0)
        RageScenarioPlaceCar((GameCarRuntime *)(void *)&g_PlayerCar,
                             s_scenario.playerTrackPoint);
    for (index = 0; index < s_scenario.rivalTrackPointCount; index++) {
        int point = s_scenario.rivalTrackPoints[index];
        if (point >= 0 && g_Cars[index].activeFlag)
            RageScenarioPlaceCar(&g_Cars[index], point);
    }
}

static int RageScenarioInt(const char *key, const char *legacyName,
                           int fallback, int low, int high) {
    const char *text = RageRuntimeConfigGetLegacy(key, legacyName);
    char *end = NULL;
    long value;
    if (text == NULL || text[0] == '\0') return fallback;
    value = strtol(text, &end, 10);
    if (*end != '\0' || value < low || value > high || value > INT_MAX) {
        fprintf(stderr, "rage-port: ignoring invalid %s=%s (expected %d..%d)\n",
                RageRuntimeConfigGet(key) ? key : legacyName, text, low, high);
        return fallback;
    }
    return (int)value;
}

static void RageScenarioParseGrid(const char *text) {
    char buffer[256], *token;
    int parsed[11], count = 0;
    if (text == NULL || text[0] == '\0') return;
    if (strcmp(text, "default") == 0) return;
    if (strlen(text) >= sizeof(buffer)) goto invalid;
    strcpy(buffer, text);
    token = strtok(buffer, ",");
    while (token != NULL && count < 11) {
        char *end = NULL;
        long value = strtol(token, &end, 10);
        if (*end != '\0' || value < -1 || value > 12) goto invalid;
        parsed[count++] = (int)value;
        token = strtok(NULL, ",");
    }
    if (count != 11 || token != NULL) goto invalid;
    memcpy(s_scenario.grid, parsed, sizeof(parsed));
    s_scenario.customGrid = 1;
    return;
invalid:
    fprintf(stderr, "rage-port: ignoring invalid RAGE_PORT_SCENARIO_GRID\n");
}

static void RageScenarioInitialize(void) {
    const char *mode, *series, *afterFinish;
    s_scenario.initialized = 1;
    s_scenario.playerTrackPoint = -1;
    s_scenario.lastScene = s_scenario.lastFrontend = s_scenario.lastMenuScreen = -1;
    if (!RageRuntimeConfigEnabled("race.enabled", "RAGE_PORT_SCENARIO")) return;
    s_scenario.enabled = 1;
    mode = RageRuntimeConfigGet("race.mode");
    series = RageRuntimeConfigGet("race.series");
    s_scenario.mode = mode ? strcmp(mode, "time-attack") != 0 :
        RageScenarioInt("race.mode", "RAGE_PORT_SCENARIO_MODE", 1, 0, 1);
    s_scenario.series = series ? strcmp(series, "extra-gp") == 0 :
        RageScenarioInt("race.series", "RAGE_PORT_SCENARIO_SERIES", 0, 0, 1);
    s_scenario.classIndex = RageScenarioInt(
        "race.class", "RAGE_PORT_SCENARIO_CLASS", 0, 0, 5);
    s_scenario.course = RageScenarioInt(
        "race.course", "RAGE_PORT_SCENARIO_COURSE", 0, 0, 3);
    s_scenario.car = RageScenarioInt(
        "race.car", "RAGE_PORT_SCENARIO_CAR", 3, 0, 12);
    afterFinish = RageRuntimeConfigGet("race.after_finish");
    s_scenario.afterFinish = RAGE_SCENARIO_AFTER_MENU;
    if (afterFinish != NULL && !strcmp(afterFinish, "repeat"))
        s_scenario.afterFinish = RAGE_SCENARIO_AFTER_REPEAT;
    else if (afterFinish != NULL && !strcmp(afterFinish, "exit"))
        s_scenario.afterFinish = RAGE_SCENARIO_AFTER_EXIT;
    else if (afterFinish != NULL && strcmp(afterFinish, "menu"))
        fprintf(stderr,
                "rage-port: invalid race.after_finish=%s (expected menu, repeat, or exit); using menu\n",
                afterFinish);
    if (!s_scenario.mode && s_scenario.series) {
        fprintf(stderr, "rage-port: Extra GP is unavailable in time attack; using Grand Prix\n");
        s_scenario.series = 0;
    }
    RageScenarioParseGrid(RageRuntimeConfigGetLegacy(
        "race.grid", "RAGE_PORT_SCENARIO_GRID"));
    RageScenarioParseTrackStarts();
    s_scenario.freezeStarts = RageRuntimeConfigEnabled("start.freeze", NULL);
    fprintf(stderr, "rage-port: scenario mode=%s series=%s class=%d course=%d car=%d grid=%s after_finish=%s\n",
            s_scenario.mode ? "grand-prix" : "time-attack",
            s_scenario.series ? "extra-gp" : "grand-prix",
            s_scenario.classIndex, s_scenario.course, s_scenario.car,
            s_scenario.customGrid ? "custom" : "default",
            s_scenario.afterFinish == RAGE_SCENARIO_AFTER_REPEAT ? "repeat" :
            s_scenario.afterFinish == RAGE_SCENARIO_AFTER_EXIT ? "exit" : "menu");
}

static void RageScenarioConfirm(void) {
    g_PadType = 0x41;
    g_PadPressed |= PAD_CONFIRM;
    s_scenario.retryFrames = 0;
    fprintf(stderr, "rage-port: scenario confirm scene=%d phase=%d screen=%d\n",
            g_SceneId, g_FrontendState, g_MenuScreen);
}

void RagePortScenarioBeforeSceneHandler(void) {
    int changed, index;
    if (!s_scenario.initialized) RageScenarioInitialize();
    if (!s_scenario.enabled) return;

    /* A completed circuit race always hands off from the live race (12) to
     * replay (17). Restarts and pause-menu exits use other destinations. */
    if (s_scenario.lastScene == 12 && g_SceneId == 17) {
        s_scenario.raceFinished = 1;
        s_scenario.resultSeen = 1;
        fprintf(stderr, "rage-port: scenario race finished after_finish=%s\n",
                s_scenario.afterFinish == RAGE_SCENARIO_AFTER_REPEAT ? "repeat" :
                s_scenario.afterFinish == RAGE_SCENARIO_AFTER_EXIT ? "exit" : "menu");
        if (s_scenario.afterFinish == RAGE_SCENARIO_AFTER_MENU) {
            s_scenario.enabled = 0;
            fprintf(stderr, "rage-port: scenario automation stopped after finish\n");
            return;
        }
    }
    if (s_scenario.raceFinished &&
        s_scenario.afterFinish == RAGE_SCENARIO_AFTER_EXIT) {
        if (g_SceneId >= 17 && g_SceneId <= 21) {
            s_scenario.resultSeen = 1;
        } else if (s_scenario.resultSeen) {
            s_scenario.exitRequested = 1;
            fprintf(stderr, "rage-port: scenario results complete; exiting\n");
        }
        s_scenario.lastScene = g_SceneId;
        return;
    }

    g_GrandPrixMode = (s16)s_scenario.mode;
    g_SeriesSelection = (s16)s_scenario.series;
    g_GrandPrixSeries = (s16)s_scenario.series;
    g_GrandPrixClass = s_scenario.classIndex;
    g_PlayerCarIndex = (s16)s_scenario.car;
    if (g_SceneId < 11) g_CourseIndex = s_scenario.course + s_scenario.series * 4;
    if (g_SceneId == 4) g_TitleMenuSelection = s_scenario.mode ? s_scenario.series : 2;
    /* Scene 5 is movie playback, which START skips. The opening movie runs
     * about eighty seconds and a scenario exists to reach a race, so waiting
     * it out costs more than every menu step that follows put together. */
    if (g_SceneId == 5) g_PadPressed |= PAD_START;

    changed = g_SceneId != s_scenario.lastScene ||
              (g_SceneId == 4 && g_FrontendState != s_scenario.lastFrontend) ||
              (g_SceneId == 8 && g_MenuScreen != s_scenario.lastMenuScreen);
    if (changed) {
        if (s_scenario.lastScene == 12 && g_SceneId != 12) {
            s_scenario.startApplied = 0;
            s_scenario.gridApplied = 0;
        }
        s_scenario.lastScene = g_SceneId;
        s_scenario.lastFrontend = g_FrontendState;
        s_scenario.lastMenuScreen = g_MenuScreen;
        s_scenario.stableFrames = s_scenario.retryFrames = 0;
    } else {
        s_scenario.stableFrames++;
        s_scenario.retryFrames++;
    }

    if (g_SceneId == 4 && g_FrontendState == FRONTEND_STATE_TITLE &&
        s_scenario.stableFrames >= 20 && s_scenario.retryFrames >= 60) {
        RageScenarioConfirm();
    } else if (g_SceneId == 4 && g_FrontendState == FRONTEND_STATE_MENU_INPUT &&
               s_scenario.stableFrames >= 10 && s_scenario.retryFrames >= 30) {
        RageScenarioConfirm();
    } else if (g_SceneId == 8 && s_scenario.stableFrames >= 20 &&
               s_scenario.retryFrames >= 60) {
        RageScenarioConfirm();
    }

    if (g_SceneId == 11 && s_scenario.customGrid && !s_scenario.gridApplied) {
        for (index = 0; index < 11; index++) g_RaceGridSlots[index].value = s_scenario.grid[index];
        s_scenario.gridApplied = 1;
        fprintf(stderr, "rage-port: custom rival grid applied\n");
    }
    if (g_SceneId == 12 && s_scenario.customStart &&
        !s_scenario.startApplied && g_TrackPointCount > 0) {
        RageScenarioApplyTrackStarts();
    } else if (g_SceneId == 12 && s_scenario.startApplied &&
               s_scenario.freezeStarts && g_TrackPointCount > 0) {
        RageScenarioHoldTrackStarts();
    }
}

int RagePortScenarioShouldExit(void) {
    return s_scenario.exitRequested;
}
