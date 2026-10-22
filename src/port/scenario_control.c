#include "game/angle.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <psyz/gpu.h>

#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/round_screen_internal.h"
#include "game/player_car_internal.h"
#include "game/save_internal.h"
#include "game/scene.h"
#include "game/scene_runtime.h"
#include "game/track.h"
#include "runtime_config.h"
#include "debug_autopilot.h"

extern int g_SceneId;

/* Immutable scenario input. It is deliberately separate from the mutable
 * game state so a scenario can request one race without owning menu, asset,
 * or renderer state. */
typedef struct RageRaceLaunchSelection {
    int mode, series, classIndex, course, car, transmission, variant;
} RageRaceLaunchSelection;

typedef struct RageScenarioState {
    int initialized, enabled;
    RageRaceLaunchSelection launch;
    int launchApplied, titleSelectionApplied;
    int afterFinish, raceFinished, resultSeen, exitRequested;
    int grid[RACE_CAR_SLOT_COUNT], customGrid, gridApplied;
    int playerTrackPoint, rivalTrackPoints[RACE_CAR_SLOT_COUNT];
    int rivalTrackPointCount;
    int customStart, startApplied, freezeStarts;
    int exactX, exactZ, exactHeading, hasExact;
    int directBoot, directStep, skipSequences;
    int lastScene, lastFrontend, lastMenuScreen, stableFrames, retryFrames;
} RageScenarioState;

enum {
    RAGE_DIRECT_PENDING,
    RAGE_DIRECT_BGM_ASSETS,
    RAGE_DIRECT_CAR_ASSETS,
    RAGE_DIRECT_ROUND_REQUEST,
    RAGE_DIRECT_ROUND_WAIT,
    RAGE_DIRECT_RACE_ASSETS,
    RAGE_DIRECT_DONE,
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
                    TrackPoint(point)->angle) & ANGLE_MASK;
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
        if (point >= 0 && g_Cars[index].activeFlag != -1 &&
            !ScenarioPlaceCar(&g_Cars[index], point)) {
            fprintf(stderr, "rage-port: rival %d track point %d outside 0..%d\n",
                    index, point, g_TrackPointCount - 1);
        } else if (point >= 0 && g_Cars[index].activeFlag != -1) {
            fprintf(stderr,
                    "rage-port: scenario-start rival=%d point=%d pos=%d,%d progress=%d section=%d model=%d active=%d\n",
                    index, point, g_Cars[index].x, g_Cars[index].z,
                    g_Cars[index].trackProgress, g_Cars[index].trackSection,
                    g_Cars[index].modelIndex, g_Cars[index].activeFlag);
        }
    }
    if (s_scenario.hasExact) ScenarioPlaceExact();
    {
        const char *view = RuntimeConfigGet("start.camera");
        int parsedView;

        if (view != NULL &&
            RuntimeParseInt(view, 0, CAMERA_VIEW_CAR, CAMERA_VIEW_TRACK,
                            &parsedView)) {
            g_Camera.mode = (CameraViewMode)parsedView;
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
        car->bodyYaw = (s16)(s_scenario.exactHeading & ANGLE_MASK);
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
        if (point >= 0 && g_Cars[index].activeFlag != -1)
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

static void ScenarioApplyGrid(void) {
    int index;
    if (!s_scenario.customGrid || s_scenario.gridApplied) return;
    for (index=0;index<RACE_CAR_SLOT_COUNT;index++)
        g_RaceGridSlots[index] = s_scenario.grid[index];
    s_scenario.gridApplied=1;
    fprintf(stderr,"rage-port: custom rival grid applied\n");
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
        s_scenario.launch.mode = strcmp(mode, "time-attack") != 0;
    else
        s_scenario.launch.mode = ScenarioInt("race.mode", 1, 0, 1);
    if (series != NULL &&
        (!strcmp(series, "grand-prix") || !strcmp(series, "extra-gp")))
        s_scenario.launch.series = strcmp(series, "extra-gp") == 0;
    else
        s_scenario.launch.series = ScenarioInt("race.series", 0, 0, 1);
    s_scenario.launch.classIndex = ScenarioInt("race.class", 0, 0, 5);
    s_scenario.launch.course = ScenarioInt("race.course", 0, 0, 39);
    s_scenario.launch.car = ScenarioInt("race.car", 3, 0, 12);
    /* Select an asset variant within this car's catalog range, rather than
     * allowing an upgrade index to spill into the next car's assets. */
    {
        int first = g_CarModelBaseIndex[s_scenario.launch.car];
        int end = s_scenario.launch.car + 1 < GAME_CAR_COUNT ?
            g_CarModelBaseIndex[s_scenario.launch.car + 1] : CAR_MODEL_VARIANT_COUNT;
        s_scenario.launch.variant = ScenarioInt("race.variant", -1, 0, end - first - 1);
    }
    s_scenario.launch.transmission = -1;
    transmission = RuntimeConfigGet("race.transmission");
    if (transmission != NULL) {
        if (!strcmp(transmission, "automatic") || !strcmp(transmission, "auto"))
            s_scenario.launch.transmission = 0;
        else if (!strcmp(transmission, "manual"))
            s_scenario.launch.transmission = 1;
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
    if (!s_scenario.launch.mode && s_scenario.launch.series) {
        fprintf(stderr, "rage-port: Extra GP is unavailable in time attack; using Grand Prix\n");
        s_scenario.launch.series = 0;
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
    s_scenario.directBoot = RuntimeConfigEnabled("boot.direct");
    fprintf(stderr, "rage-port: scenario mode=%s series=%s class=%d course=%d car=%d grid=%s after_finish=%s\n",
            s_scenario.launch.mode ? "grand-prix" : "time-attack",
            s_scenario.launch.series ? "extra-gp" : "grand-prix",
            s_scenario.launch.classIndex, s_scenario.launch.course,
            s_scenario.launch.car,
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
            g_SceneId, MenuFrontend()->state, MenuRuntimeCurrent()->activeScreen);
}

static void ScenarioHideDirectBootDisplay(void) {
    SetDispMask(0);
}

static void ScenarioShowClearedRaceDisplay(void) {
    RECT framebuffers = {0, 0, 320, 480};
    ClearImage(&framebuffers, 0, 0, 0);
    DrawSync(0);
    SetDispMask(1);
}

static void ScenarioSelectDirectSeries(void) {
    const RageRaceLaunchSelection *selection = &s_scenario.launch;

    ResetTrackTextureSwap();
    SetupDisplay240(0, 0, 0);
    UploadLoadBufferImage();
    RefreshClassWinState();
    SetDefaultReverbDepth();

    g_RaceSession.kind = RACE_SESSION_STANDARD;
    g_GrandPrixMode = (s16)selection->mode;
    g_SeriesSelection = (s16)selection->series;
    g_GrandPrixSeries = (s16)(selection->mode
        ? GrandPrixAssetSeries(selection->series, selection->classIndex)
        : selection->series);
    g_GrandPrixClass = selection->classIndex;
    g_CourseIndex = selection->course >= 8
        ? selection->course
        : selection->course + selection->series * 4;

    if (!selection->mode) {
        g_CarTable = g_TimeAttackCars;
        g_RaceProgress = &g_TimeAttackSave;
        g_CourseProgress = &g_GrandPrixCourseProgress;
    } else if (selection->series) {
        g_CarTable = g_ExtraGrandPrixCars;
        g_RaceProgress = &g_ExtraGrandPrixSave;
        g_CourseProgress = &g_ExtraGrandPrixCourseProgress;
    } else {
        g_CarTable = g_GrandPrixCars;
        g_RaceProgress = &g_GrandPrixSave;
        g_CourseProgress = &g_GrandPrixCourseProgress;
    }

    g_PlayerCarIndex = (s16)selection->car;
    if (g_CarTable != NULL && selection->transmission >= 0)
        g_CarTable[selection->car].transmission =
            (u8)selection->transmission;
    if (g_CarTable != NULL && selection->variant >= 0)
        g_CarTable[selection->car].modelVariant = (u8)selection->variant;

    s_scenario.launchApplied = 1;
    fprintf(stderr,
            "rage-port: scenario launch selection applied mode=%d series=%d class=%d course=%d car=%d\n",
            selection->mode, selection->series, selection->classIndex,
            selection->course, selection->car);
}

static void ScenarioApplyCarSetup(void) {
    CarEntry *entry = &g_CarTable[s_scenario.launch.car];
    if (entry->transmission == 0 && g_CarModelAsset != NULL &&
        g_CarModelAsset->transmissionAvailable == 0) {
        fprintf(stderr,
                "rage-port: car %d does not offer automatic transmission; "
                "using manual\n",
                s_scenario.launch.car);
        entry->transmission = 1;
    }
    g_PlayerCar.showroomTireCompound = entry->tireCompound;
    g_PlayerCar.drive.manual = entry->transmission;
    fprintf(stderr,
            "rage-port: direct boot car setup tires=%d transmission=%s\n",
            entry->tireCompound,
            entry->transmission != 0 ? "manual" : "automatic");
}

static int ScenarioAssetRequestSucceeded(s32 result, const char *stage) {
    static s32 lastLoadState;

    if (g_AssetLoadState != 0) {
        lastLoadState = g_AssetLoadState;
    }
    if (result < 0) {
        fprintf(stderr,
                "rage-port: direct boot %s asset load failed in state %d\n",
                stage, lastLoadState);
        s_scenario.enabled = 0;
        return 0;
    }
    return result == 0;
}

static void ScenarioDirectBoot(void) {
    switch (s_scenario.directStep) {
    case RAGE_DIRECT_PENDING:
        ScenarioHideDirectBootDisplay();
        ScenarioSelectDirectSeries();
        ShuffleBgmOrder();
        s_scenario.directStep = RAGE_DIRECT_BGM_ASSETS;
        break;
    case RAGE_DIRECT_BGM_ASSETS:
        if (ScenarioAssetRequestSucceeded(
                RequestSelectBgmAssetsKeepAudioSlots(), "BGM")) {
            s_scenario.directStep = RAGE_DIRECT_CAR_ASSETS;
        }
        break;
    case RAGE_DIRECT_CAR_ASSETS:
        if (ScenarioAssetRequestSucceeded(RequestCarSelectAssets(),
                                          "car-select")) {
            ActivateShowroomCarModel((s32)g_CarModelSlot);
            {
                int sample = ScenarioInt("race.logo_sample", -1, -1,
                    TEAM_LOGO_SAMPLE_CHOICE_COUNT - 1);
                if (sample >= 0) ComposeSampleTeamLogo(MenuTeamLogo(), sample, 0);
            }
            LoadImage(&g_TeamLogoRect, &g_TeamLogoCanvas);
            UploadTeamLogoClut();
            UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
            s_scenario.directStep = RAGE_DIRECT_ROUND_REQUEST;
        }
        break;
    case RAGE_DIRECT_ROUND_REQUEST:
        RequestRoundAssets();
        g_GrandPrixClass = s_scenario.launch.classIndex;
        s_scenario.directStep = RAGE_DIRECT_ROUND_WAIT;
        break;
    case RAGE_DIRECT_ROUND_WAIT:
        if (AssetLoadHasFailed()) {
            fprintf(stderr,
                    "rage-port: direct boot round asset load failed\n");
            s_scenario.enabled = 0;
        } else if (IsRoundScreenAssetLoadComplete(g_AssetLoadState,
                                                  g_AssetLoadFailed)) {
            CloseLoadedAudioSlots();
            UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer),
                             g_ImageBlockSize);
            if (!RelocateCarModel()) {
                fprintf(stderr,
                        "rage-port: direct boot car relocation failed\n");
                s_scenario.enabled = 0;
                break;
            }
            g_GrandPrixRound = s_scenario.launch.course >= 8 ? 0 : DetermineGrandPrixRound(
                g_CourseProgress != NULL ? g_CourseProgress->bestPlace : NULL,
                g_GrandPrixClass,
                SeriesCourseIndex());
            s_scenario.directStep = RAGE_DIRECT_RACE_ASSETS;
        }
        break;
    case RAGE_DIRECT_RACE_ASSETS: {
        if (ScenarioAssetRequestSucceeded(RequestRaceAssets(), "race")) {
            RoundBgmChoice bgm = ChooseRoundBgm(
                g_BgmSelection, g_BgmShuffleOrder,
                ClampBgmTrackCount(g_BgmTrackCount),
                g_BgmShuffleIndex);

            g_BgmTrack = bgm.track;
            g_BgmShuffleIndex = bgm.shuffleIndex;
            ScenarioApplyCarSetup();
            g_MirrorMode = 0;
            g_FrameSyncThreshold = 0x180;
            g_SceneTimer = 0;
            ScenarioShowClearedRaceDisplay();
            g_SceneId = GAME_SCENE_ENTER_RACE;
            ScenarioApplyGrid();
            s_scenario.directStep = RAGE_DIRECT_DONE;
            fprintf(stderr, "rage-port: scenario direct boot entered the race\n");
        }
        break;
    }
    }
}

/* This is the sole adapter from a scenario request into the recovered game
 * state. It runs once when the normal menu is ready to consume the selection;
 * the menu and round screen then own every later asset and VRAM transition. */
static void ScenarioApplyLaunchSelection(void) {
    const RageRaceLaunchSelection *selection = &s_scenario.launch;

    if (s_scenario.launchApplied) return;
    g_GrandPrixMode = (s16)selection->mode;
    g_SeriesSelection = (s16)selection->series;
    g_GrandPrixSeries = (s16)(selection->mode
        ? GrandPrixAssetSeries(selection->series, selection->classIndex)
        : selection->series);
    g_GrandPrixClass = selection->classIndex;
    g_PlayerCarIndex = (s16)selection->car;
    /* Menu course indices retain the series in bit 2 until car select starts
     * the round; car_select.c then converts it to the physical course index. */
    g_CourseIndex = selection->course >= 8
        ? selection->course + selection->series * EXTRA_COURSE_SLOT_COUNT
        : selection->course + selection->series * 4;
    if (g_CarTable != NULL && selection->transmission >= 0)
        g_CarTable[selection->car].transmission =
            (u8)selection->transmission;
    if (g_CarTable != NULL && selection->variant >= 0)
        g_CarTable[selection->car].modelVariant = (u8)selection->variant;
    s_scenario.launchApplied = 1;
    fprintf(stderr,
            "rage-port: scenario launch selection applied mode=%d series=%d class=%d course=%d car=%d\n",
            selection->mode, selection->series, selection->classIndex,
            selection->course, selection->car);
}

void PortScenarioBeforeSceneHandler(void) {
    int changed;
    DebugAutopilotBeforeScene();
    if (!s_scenario.initialized) ScenarioInitialize();
    if (!s_scenario.enabled) return;

    /* A completed circuit race always hands off from the live race (12) to
     * replay (17). Restarts and pause-menu exits use other destinations. */
    if (s_scenario.lastScene == GAME_SCENE_RACE &&
        g_SceneId == GAME_SCENE_REPLAY) {
        s_scenario.raceFinished = 1;
        s_scenario.resultSeen = 1;
        s_scenario.directBoot = 0;
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
        if (g_SceneId >= GAME_SCENE_REPLAY &&
            g_SceneId <= GAME_SCENE_RECORD_ENTRY) {
            s_scenario.resultSeen = 1;
        } else if (s_scenario.resultSeen) {
            s_scenario.exitRequested = 1;
            fprintf(stderr, "rage-port: scenario results complete; exiting\n");
        }
        s_scenario.lastScene = g_SceneId;
        return;
    }

    /* Direct boot bypasses the boot logo, intro FMV, title screen, and all
     * menu screens: as soon as the initial boot asset load completes during
     * GAME_SCENE_BOOT_LOGO, it loads the race assets directly with the display
     * masked and transitions straight to GAME_SCENE_ENTER_RACE (Scene 11). */
    if (s_scenario.directBoot && s_scenario.directStep != RAGE_DIRECT_DONE) {
        if (g_SceneId == GAME_SCENE_BOOT_LOGO) {
            SceneRuntimeBootLogo()->timer = 0;
            SetDispMask(0);
            if (s_scenario.directStep != RAGE_DIRECT_PENDING ||
                AssetLoadCompletedSuccessfully()) {
                ScenarioDirectBoot();
            }
            return;
        }
        if (g_SceneId == GAME_SCENE_FRONTEND) {
            if (g_SceneTimer > 0x1C0) g_SceneTimer = 0x1C0;
            if (MenuFrontend()->idleTimer > 800) MenuFrontend()->idleTimer = 800;
            ScenarioDirectBoot();
            return;
        }
    }

    if (g_SceneId == GAME_SCENE_FRONTEND &&
        !s_scenario.titleSelectionApplied) {
        MenuFrontend()->selection = s_scenario.launch.mode
            ? s_scenario.launch.series : 2;
        s_scenario.titleSelectionApplied = 1;
    }
    if (g_SceneId == GAME_SCENE_MENU) ScenarioApplyLaunchSelection();

    changed = g_SceneId != s_scenario.lastScene ||
              (g_SceneId == GAME_SCENE_FRONTEND &&
               MenuFrontend()->state != s_scenario.lastFrontend) ||
              (g_SceneId == GAME_SCENE_MENU &&
               MenuRuntimeCurrent()->activeScreen != s_scenario.lastMenuScreen);
    if (changed) {
        if (s_scenario.lastScene == GAME_SCENE_RACE &&
            g_SceneId != GAME_SCENE_RACE) {
            s_scenario.startApplied = 0;
            s_scenario.gridApplied = 0;
            s_scenario.launchApplied = 0;
        }
        if (g_SceneId != GAME_SCENE_FRONTEND)
            s_scenario.titleSelectionApplied = 0;
        s_scenario.lastScene = g_SceneId;
        s_scenario.lastFrontend = MenuFrontend()->state;
        s_scenario.lastMenuScreen = MenuRuntimeCurrent()->activeScreen;
        s_scenario.stableFrames = s_scenario.retryFrames = 0;
    } else {
        s_scenario.stableFrames++;
        s_scenario.retryFrames++;
    }

    if (s_scenario.raceFinished &&
        s_scenario.afterFinish == RAGE_SCENARIO_AFTER_REPEAT &&
        (g_SceneId == GAME_SCENE_REPLAY || g_SceneId == GAME_SCENE_PRIZE) &&
        s_scenario.stableFrames >= 30 && s_scenario.retryFrames >= 60) {
        ScenarioConfirm();
    }
    if (s_scenario.raceFinished && g_SceneId == GAME_SCENE_ENTER_RACE) {
        s_scenario.raceFinished = 0;
        s_scenario.resultSeen = 0;
        fprintf(stderr, "rage-port: scenario repeat entered next race via menus\n");
    }

    /* Two non-interactive sequences sit between the boot logo and the first
     * race: the ~30 s intro movie (5) and the ~51 s prologue cutscene (32),
     * which UpdateMainMenuExit enters whenever the save is fresh. Both are
     * skippable by the player, so the automation skips them too; PAD_CONFIRM
     * carries PAD_START, which is what the movie player watches for. The
     * prologue ignores the button until its own timer passes 0x79, so holding
     * it costs nothing and takes effect at the first frame that accepts it. */
    /* Scene 5 is shared by boot and class/ending FMVs. Boot automation must
     * not press Start through a movie reached by the race reward flow. */
    if (s_scenario.skipSequences && !s_scenario.raceFinished) {
        if (g_SceneId == GAME_SCENE_FMV || g_SceneId == GAME_SCENE_PROLOGUE) {
            g_PadType = 0x41;
            g_PadPressed |= PAD_CONFIRM;
        } else if (g_SceneId == GAME_SCENE_BOOT_LOGO) {
            /* The boot logo drops its remaining hold as soon as a button is
             * down and the assets behind it have finished loading. What is
             * left after that is the load itself, which nothing can skip. */
            g_PadType = 0x41;
            g_PadHeld |= PAD_CONFIRM;
        }
    }

    if (g_SceneId == GAME_SCENE_FRONTEND &&
        MenuFrontend()->state == FRONTEND_STATE_TITLE &&
        s_scenario.stableFrames >= 20 && s_scenario.retryFrames >= 60) {
        ScenarioConfirm();
    } else if (g_SceneId == GAME_SCENE_FRONTEND &&
               MenuFrontend()->state == FRONTEND_STATE_MENU_INPUT &&
               s_scenario.stableFrames >= 10 && s_scenario.retryFrames >= 30) {
        ScenarioConfirm();
    } else if (g_SceneId == GAME_SCENE_MENU && s_scenario.stableFrames >= 20 &&
               s_scenario.retryFrames >= 60) {
        ScenarioConfirm();
    }

    if (g_SceneId == GAME_SCENE_ENTER_RACE) ScenarioApplyGrid();
    if (g_SceneId == GAME_SCENE_RACE && s_scenario.customStart &&
        !s_scenario.startApplied && g_TrackPointCount > 0) {
        ScenarioApplyTrackStarts();
    } else if (g_SceneId == GAME_SCENE_RACE && s_scenario.startApplied &&
               s_scenario.freezeStarts && g_TrackPointCount > 0) {
        ScenarioHoldTrackStarts();
    }
}

int PortScenarioShouldExit(void) {
    return s_scenario.exitRequested || DebugAutopilotShouldExit();
}
