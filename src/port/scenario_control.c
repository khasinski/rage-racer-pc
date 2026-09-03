#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game/asset.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/round_screen_internal.h"
#include "game/player_car_internal.h"
#include "game/save_internal.h"
#include "game/track.h"
#include "runtime_config.h"

extern int g_SceneId;

typedef struct RageScenarioState {
    int initialized, enabled;
    int mode, series, classIndex, course, car, transmission;
    int afterFinish, raceFinished, resultSeen, exitRequested;
    int grid[RACE_CAR_SLOT_COUNT], customGrid, gridApplied;
    int playerTrackPoint, rivalTrackPoints[RACE_CAR_SLOT_COUNT];
    int rivalTrackPointCount;
    int customStart, startApplied, freezeStarts;
    int exactX, exactZ, exactHeading, hasExact;
    int directBoot, directStep, skipSequences;
    int lastScene, lastFrontend, lastMenuScreen, stableFrames, retryFrames;
} RageScenarioState;

/* Steps of the direct-boot loader, in the order the frontend would trigger
 * their asset requests. */
enum {
    RAGE_DIRECT_PENDING,
    RAGE_DIRECT_BGM_ASSETS,
    RAGE_DIRECT_CAR_ASSETS,
    RAGE_DIRECT_ROUND_REQUEST,
    RAGE_DIRECT_ROUND_WAIT,
    RAGE_DIRECT_RACE_ASSETS,
    RAGE_DIRECT_DONE
};

enum {
    RAGE_SCENARIO_AFTER_MENU,
    RAGE_SCENARIO_AFTER_REPEAT,
    RAGE_SCENARIO_AFTER_EXIT,
};

static RageScenarioState s_scenario;

static int ScenarioParseTrackPoint(const char *text, int *result) {
    return RuntimeParseInt(text, 0, 0, INT_MAX, result);
}

static void ScenarioParseTrackStarts(void) {
    const char *player = RuntimeConfigGet("start.player_track_point");
    const char *rivals = RuntimeConfigGet("start.rival_track_points");
    char buffer[512], *token;
    int value;
    if (player != NULL) {
        if (!ScenarioParseTrackPoint(player, &value))
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
    while (token != NULL &&
           s_scenario.rivalTrackPointCount < RACE_CAR_SLOT_COUNT) {
        while (*token == ' ' || *token == '\t') token++;
        if (!strcmp(token, "-") || !strcmp(token, "default")) value = -1;
        else if (!ScenarioParseTrackPoint(token, &value)) goto invalid;
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

static int ScenarioPlaceCar(GameCarRuntime *car, int point) {
    CarTrackLimits limits = {0, 0, 0, 0};
    if (point < 0) return 1;
    if (point >= g_TrackPointCount) return 0;
    car->trackPointIndex = point;
    car->x = TrackPoint(point)->x;
    car->z = TrackPoint(point)->z;
    car->y = 0;
    car->bodyPitch = car->bodyRoll = 0;
    car->bodyYaw = (0xC00 - (g_RaceSeries << 11) -
                    TrackPoint(point)->angle) & 0xFFF;
    car->headingAngle = car->bodyYaw;
    car->trackPointIndex = FindTrackSegment(car, point);
    SeedCarLapProgress(car, 0);
    UpdateCarTrackState(car, car->trackPointIndex, &limits);
    car->previousTrackProgress = car->trackProgress;
    car->modelY = car->y;
    CopyCarBodyRotationToModel(car);
    return 1;
}

static void ScenarioPlaceExact(void);

static void ScenarioApplyTrackStarts(void) {
    int index;
    if (s_scenario.playerTrackPoint >= 0 &&
        !ScenarioPlaceCar(AsRivalCar(&g_PlayerCar),
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
            !ScenarioPlaceCar(&g_Cars[index], point)) {
            fprintf(stderr, "rage-port: rival %d track point %d outside 0..%d\n",
                    index, point, g_TrackPointCount - 1);
        } else if (point >= 0 && g_Cars[index].activeFlag) {
            fprintf(stderr,
                    "rage-port: scenario-start rival=%d point=%d pos=%d,%d progress=%d section=%d\n",
                    index, point, g_Cars[index].x, g_Cars[index].z,
                    g_Cars[index].trackProgress, g_Cars[index].trackSection);
        }
    }
    if (s_scenario.hasExact) ScenarioPlaceExact();
    {
        const char *view = RuntimeConfigGet("start.camera");
        int parsedView;

        if (view != NULL &&
            RuntimeParseInt(view, 0, CAMERA_VIEW_CAR, CAMERA_VIEW_TRACK,
                            &parsedView)) {
            g_CameraViewMode = (CameraViewMode)parsedView;
        } else if (view != NULL) {
            fprintf(stderr, "rage-port: invalid start.camera=%s\n", view);
        }
    }
    SetTrackTexturePageNow(g_PlayerCar.trackSection);
    s_scenario.startApplied = 1;
    fprintf(stderr,
            "rage-port: custom track start applied player=%d rivals=%d points=%d\n",
            s_scenario.playerTrackPoint, s_scenario.rivalTrackPointCount,
            g_TrackPointCount);
}

/* Put the car exactly where a mark said it was, keeping the track state the
 * placement computed so the camera and collision follow. */
static void ScenarioPlaceExact(void) {
    CarTrackLimits limits = {0, 0, 0, 0};
    PlayerCarRuntime *car = &g_PlayerCar;
    GameCarRuntime *rivalView = AsRivalCar(car);

    car->x = s_scenario.exactX;
    car->z = s_scenario.exactZ;
    if (s_scenario.exactHeading >= 0) {
        car->bodyYaw = (s16)(s_scenario.exactHeading & 0xFFF);
        car->headingAngle = car->bodyYaw;
    }
    car->trackPointIndex = FindTrackSegment(rivalView, car->trackPointIndex);
    UpdateCarTrackState(rivalView, car->trackPointIndex, &limits);
    CopyCarBodyRotationToModel(rivalView);
    car->modelY = car->y;
}

static void ScenarioHoldTrackStarts(void) {
    int index;
    if (s_scenario.hasExact) {
        ScenarioPlaceExact();
        return;
    }
    if (s_scenario.playerTrackPoint >= 0) {
        ScenarioPlaceCar(AsRivalCar(&g_PlayerCar),
                         s_scenario.playerTrackPoint);
    }
    for (index = 0; index < s_scenario.rivalTrackPointCount; index++) {
        int point = s_scenario.rivalTrackPoints[index];
        if (point >= 0 && g_Cars[index].activeFlag)
            ScenarioPlaceCar(&g_Cars[index], point);
    }
}

static int ScenarioInt(const char *key, int fallback, int low, int high) {
    const char *text = RuntimeConfigGet(key);
    int value;
    if (text == NULL || text[0] == '\0') return fallback;
    if (!RuntimeParseInt(text, 10, low, high, &value)) {
        fprintf(stderr, "rage-port: ignoring invalid %s=%s (expected %d..%d)\n",
                key, text, low, high);
        return fallback;
    }
    return value;
}

static void ScenarioParseGrid(const char *text) {
    char buffer[256], *token;
    int parsed[RACE_CAR_SLOT_COUNT], count = 0;
    if (text == NULL || text[0] == '\0') return;
    if (strcmp(text, "default") == 0) return;
    if (strlen(text) >= sizeof(buffer)) goto invalid;
    strcpy(buffer, text);
    token = strtok(buffer, ",");
    while (token != NULL && count < RACE_CAR_SLOT_COUNT) {
        int value;

        if (!RuntimeParseInt(token, 10, -1, 12, &value)) goto invalid;
        parsed[count++] = value;
        token = strtok(NULL, ",");
    }
    if (count != RACE_CAR_SLOT_COUNT || token != NULL) goto invalid;
    memcpy(s_scenario.grid, parsed, sizeof(parsed));
    s_scenario.customGrid = 1;
    return;
invalid:
    fprintf(stderr, "rage-port: ignoring invalid race.grid\n");
}

static void ScenarioInitialize(void) {
    const char *mode, *series, *transmission, *afterFinish;
    s_scenario.initialized = 1;
    s_scenario.playerTrackPoint = -1;
    s_scenario.lastScene = s_scenario.lastFrontend = s_scenario.lastMenuScreen = -1;
    if (!RuntimeConfigEnabled("race.enabled")) return;
    s_scenario.enabled = 1;
    /* Both settings are written either as a word or as the retail index.
     * A word that is neither of the two falls through to the numeric path,
     * which is the one that reports what it could not use. */
    mode = RuntimeConfigGet("race.mode");
    series = RuntimeConfigGet("race.series");
    if (mode != NULL &&
        (!strcmp(mode, "grand-prix") || !strcmp(mode, "time-attack")))
        s_scenario.mode = strcmp(mode, "time-attack") != 0;
    else
        s_scenario.mode = ScenarioInt("race.mode", 1, 0, 1);
    if (series != NULL &&
        (!strcmp(series, "grand-prix") || !strcmp(series, "extra-gp")))
        s_scenario.series = strcmp(series, "extra-gp") == 0;
    else
        s_scenario.series = ScenarioInt("race.series", 0, 0, 1);
    s_scenario.classIndex = ScenarioInt("race.class", 0, 0, 5);
    s_scenario.course = ScenarioInt("race.course", 0, 0, 3);
    s_scenario.car = ScenarioInt("race.car", 3, 0, 12);
    s_scenario.transmission = -1;
    transmission = RuntimeConfigGet("race.transmission");
    if (transmission != NULL) {
        if (!strcmp(transmission, "automatic") || !strcmp(transmission, "auto"))
            s_scenario.transmission = 0;
        else if (!strcmp(transmission, "manual"))
            s_scenario.transmission = 1;
        else if (strcmp(transmission, "default"))
            fprintf(stderr,
                    "rage-port: invalid race.transmission=%s (expected default, automatic, or manual)\n",
                    transmission);
    }
    afterFinish = RuntimeConfigGet("race.after_finish");
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
    ScenarioParseGrid(RuntimeConfigGet("race.grid"));
    ScenarioParseTrackStarts();
    s_scenario.freezeStarts = RuntimeConfigEnabled("start.freeze");
    {
        /* A mark taken while driving records where the car actually was, which
         * a track point alone cannot express: the car is rarely on the centre
         * line and its heading is its own. These place it exactly, so a
         * reported frame can be reproduced. */
        const char *x = RuntimeConfigGet("start.player_x");
        const char *z = RuntimeConfigGet("start.player_z");
        const char *heading = RuntimeConfigGet("start.player_heading");
        if (x != NULL && z != NULL) {
            int valid = RuntimeParseInt(
                x, 0, INT_MIN, INT_MAX, &s_scenario.exactX);
            valid = valid && RuntimeParseInt(
                z, 0, INT_MIN, INT_MAX, &s_scenario.exactZ);
            s_scenario.exactHeading = -1;
            if (heading != NULL) {
                valid = valid && RuntimeParseInt(
                    heading, 0, INT_MIN, INT_MAX, &s_scenario.exactHeading);
            }
            if (valid) {
                s_scenario.hasExact = 1;
                s_scenario.customStart = 1;
                fprintf(stderr, "rage-port: exact start %d,%d heading=%d\n",
                        s_scenario.exactX, s_scenario.exactZ,
                        s_scenario.exactHeading);
            } else {
                fprintf(stderr, "rage-port: invalid exact start coordinates\n");
            }
        } else if (x != NULL || z != NULL) {
            fprintf(stderr,
                    "rage-port: exact start requires both start.player_x and start.player_z\n");
        }
    }
    s_scenario.skipSequences = RuntimeConfigGet("boot.skip_sequences") == NULL
                                   ? 1
                                   : RuntimeConfigEnabled("boot.skip_sequences");
    s_scenario.directBoot = RuntimeConfigGet("boot.direct") == NULL
                                ? 1
                                : RuntimeConfigEnabled("boot.direct");
    if (s_scenario.directBoot && !s_scenario.mode) {
        fprintf(stderr,
                "rage-port: direct boot covers Grand Prix only; time attack uses the menus\n");
        s_scenario.directBoot = 0;
    }
    fprintf(stderr, "rage-port: scenario mode=%s series=%s class=%d course=%d car=%d grid=%s after_finish=%s\n",
            s_scenario.mode ? "grand-prix" : "time-attack",
            s_scenario.series ? "extra-gp" : "grand-prix",
            s_scenario.classIndex, s_scenario.course, s_scenario.car,
            s_scenario.customGrid ? "custom" : "default",
            s_scenario.afterFinish == RAGE_SCENARIO_AFTER_REPEAT ? "repeat" :
            s_scenario.afterFinish == RAGE_SCENARIO_AFTER_EXIT ? "exit" : "menu");
    fprintf(stderr, "rage-port: scenario boot=%s skip=%s\n",
            s_scenario.directBoot ? "direct" : "menus",
            s_scenario.skipSequences ? "on" : "off");
}

static void ScenarioConfirm(void) {
    g_PadType = 0x41;
    g_PadPressed |= PAD_CONFIRM;
    s_scenario.retryFrames = 0;
    fprintf(stderr, "rage-port: scenario confirm scene=%d phase=%d screen=%d\n",
            g_SceneId, g_FrontendState, g_MenuScreen);
}

/* Seconds since the first traced frame. The automation is judged by how long
 * it takes to reach a race, so the trace carries wall time rather than frames:
 * scene handlers tick at different rates. */
static double ScenarioElapsed(void) {
    struct timespec now;
    static struct timespec start;
    static int started;
    if (timespec_get(&now, TIME_UTC) != TIME_UTC) return 0.0;
    if (!started) {
        started = 1;
        start = now;
    }
    return (double)(now.tv_sec - start.tv_sec) +
           (double)(now.tv_nsec - start.tv_nsec) / 1e9;
}

/* One line per state change, plus one when a screen the automation is still
 * navigating outlasts every timeout the confirm ladder uses. A scenario that
 * never reaches a race then names the screen it died on instead of just going
 * quiet. Scene 11 and up hold their state for as long as the race and the
 * result screens last, so they are never reported. */
static void ScenarioTrace(void) {
    static int lastScene = -1, lastFrontend = -1, lastScreen = -1;
    static int held;
    if (g_SceneId != lastScene || g_FrontendState != lastFrontend ||
        g_MenuScreen != lastScreen) {
        lastScene = g_SceneId;
        lastFrontend = g_FrontendState;
        lastScreen = g_MenuScreen;
        held = 0;
        fprintf(stderr,
                "rage-port: scenario state t=%.1fs scene=%d phase=%d screen=%d\n",
                ScenarioElapsed(), g_SceneId, g_FrontendState, g_MenuScreen);
    } else if (++held == 600 && g_SceneId < 11) {
        fprintf(stderr,
                "rage-port: scenario stalled t=%.1fs scene=%d phase=%d screen=%d\n",
                ScenarioElapsed(), g_SceneId, g_FrontendState, g_MenuScreen);
    }
}

/* Every title-screen series confirm repoints the same three tables
 * (title_screen.c cases 0 and 1), and everything downstream reads the race
 * through them. Direct boot never shows that screen, so it repoints them
 * itself. Time attack is not covered: its confirm leaves g_CourseProgress
 * pointing wherever a previous Grand Prix selection left it, which is nothing
 * at all on a cold boot. */
static void ScenarioSelectSeries(void) {
    if (s_scenario.series) {
        g_CarTable = g_ExtraGrandPrixCars;
        g_RaceProgress = &g_ExtraGrandPrixSave;
        g_CourseProgress = &g_ExtraGrandPrixCourseProgress;
    } else {
        g_CarTable = g_GrandPrixCars;
        g_RaceProgress = &g_GrandPrixSave;
        g_CourseProgress = &g_GrandPrixCourseProgress;
    }
    if (s_scenario.transmission >= 0)
        g_CarTable[s_scenario.car].transmission =
            (u8)s_scenario.transmission;
}

/* DrawMenuCarView normally copies the selected setup into the player object.
 * Direct boot deliberately skips that screen, so do the same non-UI work
 * immediately before the race initializes the car. */
static void ScenarioApplyCarSetup(void) {
    CarEntry *entry = &g_CarTable[s_scenario.car];
    if (entry->transmission == 0 && g_CarModelAsset != NULL &&
        g_CarModelAsset->transmissionAvailable == 0) {
        fprintf(stderr,
                "rage-port: car %d does not offer automatic transmission; "
                "using manual\n",
                s_scenario.car);
        entry->transmission = 1;
    }
    g_PlayerCar.showroomTireCompound = entry->tireCompound;
    g_PlayerCar.drive.manual = entry->transmission;
    fprintf(stderr,
            "rage-port: direct boot car setup tires=%d transmission=%s\n",
            entry->tireCompound,
            entry->transmission != 0 ? "manual" : "automatic");
}


/* Load a race without the frontend. Of the retail route to scene 11, the
 * screens are what costs the time and the asset requests behind them are what
 * a race actually needs, so issue those in the order the menus do and skip
 * the rest. Car-select assets carry the player's car model, the round-screen
 * request leaves behind the block pointers LoadRaceAssets reads out of, and
 * the race request loads the course, the track data and the car audio.
 *
 * The two request styles differ and cannot be polled the same way.
 * RequestCarSelectAssets and RequestRaceAssets return 1 while busy, 0 after
 * success and -1 after failure, on the call after the load lands.
 * RequestRoundAssets has no busy guard: it
 * resets the loader whenever it is called mid-load, so it is issued once and
 * waited on through g_AssetLoadState. */
static int ScenarioAssetRequestSucceeded(s32 result, const char *stage) {
    if (result < 0) {
        fprintf(stderr, "rage-port: direct boot %s asset load failed\n",
                stage);
        s_scenario.enabled = 0;
        return 0;
    }
    return result == 0;
}

static void ScenarioDirectBoot(void) {
    static const char *const stepNames[] = {
        "setup", "bgm-assets", "car-assets", "round-request", "round-wait",
        "race-assets", "done"
    };
    static int lastStep = -1;

    if (s_scenario.directStep != lastStep) {
        lastStep = s_scenario.directStep;
        fprintf(stderr, "rage-port: direct boot %s t=%.1fs\n",
                stepNames[s_scenario.directStep], ScenarioElapsed());
    }
    switch (s_scenario.directStep) {
    case RAGE_DIRECT_PENDING:
        ScenarioSelectSeries();
        ShuffleBgmOrder();
        s_scenario.directStep = RAGE_DIRECT_BGM_ASSETS;
        break;
    case RAGE_DIRECT_BGM_ASSETS:
        /* Asset 7 carries the sequence bank, and leaves behind the three block
         * pointers LoadCarSelectAssets opens its own first state with. */
        if (ScenarioAssetRequestSucceeded(RequestSelectBgmAssets(),
                                          "BGM")) {
            s_scenario.directStep = RAGE_DIRECT_CAR_ASSETS;
        }
        break;
    case RAGE_DIRECT_CAR_ASSETS:
        if (ScenarioAssetRequestSucceeded(RequestCarSelectAssets(),
                                          "car-select")) {
            /* EnterCarSelectScreen's one piece of non-UI work. The load above
             * leaves the player's model in slot 0 but its texture still out of
             * VRAM; RelocateCarModel below moves the model without uploading
             * that image, and the race draws the player's own car from bank 0
             * only in the outside views. Skipping this loses the car there. */
            ActivateShowroomCarModel((s32)g_CarModelSlot);
            s_scenario.directStep = RAGE_DIRECT_ROUND_REQUEST;
        }
        break;
    case RAGE_DIRECT_ROUND_REQUEST:
        RequestRoundAssets();
        s_scenario.directStep = RAGE_DIRECT_ROUND_WAIT;
        break;
    case RAGE_DIRECT_ROUND_WAIT:
        if (AssetLoadHasFailed()) {
            fprintf(stderr,
                    "rage-port: direct boot round asset load failed\n");
            s_scenario.enabled = 0;
        } else if (g_AssetLoadState == 0) {
            /* EnterRoundScreen's own work, minus the screen it draws. */
            CloseLoadedAudioSlots();
            UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer),
                             g_ImageBlockSize);
            if (!RelocateCarModel()) {
                fprintf(stderr,
                        "rage-port: direct boot car relocation failed\n");
                s_scenario.enabled = 0;
                break;
            }
            g_GrandPrixRound = DetermineGrandPrixRound(
                g_CourseProgress->bestPlace, g_GrandPrixClass,
                SeriesCourseIndex());
            s_scenario.directStep = RAGE_DIRECT_RACE_ASSETS;
        }
        break;
    case RAGE_DIRECT_RACE_ASSETS: {
        if (ScenarioAssetRequestSucceeded(RequestRaceAssets(), "race")) {
            RoundBgmChoice bgm = ChooseRoundBgm(
                g_BgmSelection, g_BgmShuffleOrder, g_BgmTrackCount,
                g_BgmShuffleIndex);

            g_BgmTrack = bgm.track;
            g_BgmShuffleIndex = bgm.shuffleIndex;
            ScenarioApplyCarSetup();
            g_MirrorMode = 0;
            g_FrameSyncThreshold = 0x180;
            g_SceneTimer = 0;
            g_SceneId = 11;
            s_scenario.directStep = RAGE_DIRECT_DONE;
            fprintf(stderr, "rage-port: scenario direct boot entered the race t=%.1fs\n",
                    ScenarioElapsed());
        }
        break;
    }
    }
}

void PortScenarioBeforeSceneHandler(void) {
    int changed, index;
    if (!s_scenario.initialized) ScenarioInitialize();
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

    /* The scenario steers the frontend by holding its race selection in the
     * globals the menus read, but only the scenes that actually choose a race
     * may be steered. Two others read the same globals for their own purpose
     * and load assets from them: the Grand Prix prologue runs at its own class
     * and course, and the result flow reports the race just finished. Holding
     * the scenario's class across those made them fetch another class's
     * assets. The title screen's hand-off phase is excluded for the same
     * reason - it is where UpdateMainMenuExit picks the prologue's class. */
    if (g_SceneId >= 2 && g_SceneId <= 12 &&
        !(g_SceneId == 4 && g_FrontendState == FRONTEND_STATE_MENU_EXIT)) {
        g_GrandPrixMode = (s16)s_scenario.mode;
        g_SeriesSelection = (s16)s_scenario.series;
        g_GrandPrixSeries = (s16)s_scenario.series;
        g_GrandPrixClass = s_scenario.classIndex;
        g_PlayerCarIndex = (s16)s_scenario.car;
        if (g_CarTable != NULL && s_scenario.transmission >= 0)
            g_CarTable[s_scenario.car].transmission =
                (u8)s_scenario.transmission;
        if (g_SceneId < 11) {
            /* The menus index course progress with the series in bit 2, and
             * the retail car-select confirm masks it back to the physical
             * course before the race loads (car_select.c). Asset slots run
             * class * 8 + course * 2 with four courses to a class, so an
             * unmasked index reads the next class's data: that is where the
             * reverse grid went, leaving every rival at the origin with no
             * track segment, which deactivates them. Direct boot shows none
             * of those menus, so it uses the physical course throughout. */
            int menuIndexed = !s_scenario.directBoot && g_SceneId <= 8;
            g_CourseIndex =
                s_scenario.course + (menuIndexed ? s_scenario.series * 4 : 0);
        }
    }
    if (g_SceneId == 4) g_TitleMenuSelection = s_scenario.mode ? s_scenario.series : 2;

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

    ScenarioTrace();

    /* Two non-interactive sequences sit between the boot logo and the first
     * race: the ~30 s intro movie (5) and the ~51 s prologue cutscene (32),
     * which UpdateMainMenuExit enters whenever the save is fresh. Both are
     * skippable by the player, so the automation skips them too; PAD_CONFIRM
     * carries PAD_START, which is what the movie player watches for. The
     * prologue ignores the button until its own timer passes 0x79, so holding
     * it costs nothing and takes effect at the first frame that accepts it. */
    if (s_scenario.skipSequences) {
        if (g_SceneId == 5 || g_SceneId == 32) {
            g_PadType = 0x41;
            g_PadPressed |= PAD_CONFIRM;
        } else if (g_SceneId == 1) {
            /* The boot logo drops its remaining hold as soon as a button is
             * down and the assets behind it have finished loading. What is
             * left after that is the load itself, which nothing can skip. */
            g_PadType = 0x41;
            g_PadHeld |= PAD_CONFIRM;
        }
    }

    /* Direct boot takes over the moment the title screen appears, which is the
     * first point where the boot assets are in and the game is otherwise idle.
     * The scene stays on 4 so its handler keeps drawing a still title, but its
     * timers are held short of the two attract triggers: UpdateFrontend starts
     * loading an attract demo at g_SceneTimer 0x1cc and cuts to one at
     * g_FrontendIdleTimer 900, and both would fight the loader for the asset
     * pipeline. They are clamped rather than pinned because the title screen
     * turns the display on at its own g_SceneTimer 0xf, and a timer held at
     * zero never gets there - which leaves the race that follows drawing into
     * a masked display. */
    if (s_scenario.directBoot && g_SceneId == 4 &&
        s_scenario.directStep != RAGE_DIRECT_DONE) {
        if (g_SceneTimer > 0x1C0) g_SceneTimer = 0x1C0;
        if (g_FrontendIdleTimer > 800) g_FrontendIdleTimer = 800;
        ScenarioDirectBoot();
        return;
    }

    if (g_SceneId == 4 && g_FrontendState == FRONTEND_STATE_TITLE &&
        s_scenario.stableFrames >= 20 && s_scenario.retryFrames >= 60) {
        ScenarioConfirm();
    } else if (g_SceneId == 4 && g_FrontendState == FRONTEND_STATE_MENU_INPUT &&
               s_scenario.stableFrames >= 10 && s_scenario.retryFrames >= 30) {
        ScenarioConfirm();
    } else if (g_SceneId == 8 && s_scenario.stableFrames >= 20 &&
               s_scenario.retryFrames >= 60) {
        ScenarioConfirm();
    }

    if (g_SceneId == 11 && s_scenario.customGrid && !s_scenario.gridApplied) {
        for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
            g_RaceGridSlots[index].value = s_scenario.grid[index];
        }
        s_scenario.gridApplied = 1;
        fprintf(stderr, "rage-port: custom rival grid applied\n");
    }
    if (g_SceneId == 12 && s_scenario.customStart &&
        !s_scenario.startApplied && g_TrackPointCount > 0) {
        ScenarioApplyTrackStarts();
    } else if (g_SceneId == 12 && s_scenario.startApplied &&
               s_scenario.freezeStarts && g_TrackPointCount > 0) {
        ScenarioHoldTrackStarts();
    }
}

int PortScenarioShouldExit(void) {
    return s_scenario.exitRequested;
}
