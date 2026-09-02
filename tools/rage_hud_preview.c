/*
 * Draws the race HUD, and nothing else, with values you choose.
 *
 * rage-frame-replay answers "what did this captured frame look like", but a
 * captured frame holds no HUD: the HUD is 2D primitives the game builds after
 * the scene. So the one part of the picture that kept coming back wrong was
 * the one part no offline tool could show. This runs the game's own HUD
 * builders and drawing against the real rasteriser, with a VRAM snapshot for
 * the fonts and sprite sheets, and no game and no window.
 *
 * Every number on the HUD is an option, so a layout can be checked at the
 * values that expose it: a lap time that fills its field, a race position in
 * double digits, a split delta on either side of zero.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgpu.h>
#include <psyz/video.h>

#include "game/player_car_internal.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/save_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include "port_config.h"
#include "rage/hud_config.h"

void BuildRaceHudPrims(s32 mode);

enum { HUD_CANVAS_WIDTH = 320, HUD_HEIGHT = 240, HUD_LAP_SLOTS = 6 };

/*
 * The spread layout puts sprites at negative x and past 320, because the
 * widescreen picture is wider than the canvas the coordinates are authored
 * in. Drawing them into a 320-wide frame would clip exactly the sprites
 * under test, so the preview widens the frame by the same margin the layout
 * assumes and shifts the origin to match.
 */
/* The margin hud_config.c pushes an edge sprite out by. */
enum { RAGE_HUD_PREVIEW_MARGIN = 53 };

static int s_width = HUD_CANVAS_WIDTH;
static int s_originX;

/*
 * The port's own configuration, answered here rather than read from a file:
 * the anchoring is the thing under test, so it has to be selectable.
 */
static RagePortConfig s_config;
static int s_modernEnabled = 1;
static const char *s_anchor = "edges";

const RagePortConfig *PortActiveConfig(void) { return &s_config; }
int ModernIsEnabled(void) { return s_modernEnabled; }
const char *RuntimeConfigGet(const char *key) {
    if (strcmp(key, "hud.anchor") == 0) return s_anchor;
    return NULL;
}
int RuntimeConfigEnabled(const char *key) {
    const char *value = RuntimeConfigGet(key);
    return value != NULL && strcmp(value, "false") != 0;
}

/*
 * The HUD reaches the render state and the diagnostics; the state tables it
 * shares a translation unit with reach the attract demo and the music menu.
 * None of that runs here, so answer it rather than link half the game in.
 */
GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
CarTrackWork g_CarTrackWork;

int DiagnosticsEnabled(const char *key) { (void)key; return 0; }
const char *DiagnosticsValue(const char *key) { (void)key; return NULL; }
void Trace(const char *topic, const char *format, ...) {
    (void)topic;
    (void)format;
}

void UpdateAttractDemoRace(void) {}
void UpdateAttractDemoStart(void) {}
void UpdateBgmSelect(void) {}
void UpdateBgmSelectFadeIn(void) {}
void UpdateBgmSelectLoad(void) {}
void ExitBgmSelect(void) {}

static void Usage(const char *program) {
    fprintf(stderr,
        "usage: %s --vram VRAM.RAW [--output hud.ppm]\n"
        "  [--mode gp|time]        which HUD, grand prix or time trial\n"
        "  [--aspect 16:9|4:3]     16:9 is the layout that spreads out\n"
        "  [--anchor edges|center] where the spread layout puts things\n"
        "  [--lap N=MS]            one lap's time, repeatable, N from 0\n"
        "  [--lap-count N]         laps in the race\n"
        "  [--laps-done N]         laps finished, the rest draw as dashes\n"
        "  [--current-lap N]       which lap row is highlighted\n"
        "  [--best MS]             best lap this race\n"
        "  [--position N]          race position, grand prix only\n"
        "  [--split MS]            split delta; sign picks the marker\n"
        "  [--time-left TICKS]     time limit remaining\n"
        "  [--sector-time MS]      last sector time\n"
        "  [--target-time MS]      the split time being chased\n"
        "  [--total MS]            best total time\n"
        "  [--background R,G,B]    what to draw the HUD on top of\n",
        program);
}

static const char *OptionValue(int argc, char **argv, const char *name) {
    int index;
    for (index = 1; index + 1 < argc; index++)
        if (strcmp(argv[index], name) == 0) return argv[index + 1];
    return NULL;
}

static int OptionNumber(int argc, char **argv, const char *name, int fallback) {
    const char *value = OptionValue(argc, argv, name);
    return value != NULL ? (int)strtol(value, NULL, 0) : fallback;
}

/* Clearing to a flat colour is not what the game does; it is what makes a
 * HUD drawn over nothing readable. */
static void FillBackground(int red, int green, int blue) {
    RECT area;
    unsigned short *row;
    unsigned short value;
    int x;

    row = malloc((size_t)s_width * sizeof(*row));
    if (row == NULL) return;
    value = (unsigned short)(((blue >> 3) << 10) | ((green >> 3) << 5) |
                             (red >> 3));
    for (x = 0; x < s_width; x++) row[x] = value;
    area.x = 0;
    area.w = (short)s_width;
    area.h = 1;
    for (area.y = 0; area.y < HUD_HEIGHT; area.y++)
        LoadImage(&area, (u_long *)row);
    DrawSync(0);
    free(row);
}

static int WritePpm(const char *path) {
    unsigned short *page;
    unsigned char *pixels;
    RECT area;
    FILE *file;
    size_t count = (size_t)s_width * HUD_HEIGHT;
    size_t index;

    page = malloc(count * sizeof(*page));
    pixels = malloc(count * 3);
    if (page == NULL || pixels == NULL) {
        free(page);
        free(pixels);
        return 0;
    }
    area.x = 0;
    area.y = 0;
    area.w = (short)s_width;
    area.h = HUD_HEIGHT;
    StoreImage(&area, (u_long *)page);
    DrawSync(0);
    for (index = 0; index < count; index++) {
        unsigned short value = page[index];
        pixels[index * 3 + 0] = (unsigned char)((value & 31) * 255 / 31);
        pixels[index * 3 + 1] = (unsigned char)(((value >> 5) & 31) * 255 / 31);
        pixels[index * 3 + 2] = (unsigned char)(((value >> 10) & 31) * 255 / 31);
    }
    file = fopen(path, "wb");
    if (file != NULL) {
        fprintf(file, "P6\n%d %d\n255\n", s_width, HUD_HEIGHT);
        fwrite(pixels, 3, count, file);
        fclose(file);
    }
    free(page);
    free(pixels);
    return file != NULL;
}

/* What the builder decided, printed beside the picture: a sprite in the wrong
 * place is easier to recognise as a number than as a few pixels. */
static void ReportSprites(s32 mode) {
    GameFrameContext *frame = g_DrawBuffer;
    GameSpriteDesc *descs = mode != 0 ? g_RaceHudSpriteDescsGp
                                      : g_RaceHudSpriteDescsTimeTrial;
    s32 rowCount = mode != 0 ? 12 : 11;
    s32 row;

    printf("row  authored  drawn  size     what\n");
    for (row = 0; row < rowCount; row++) {
        SPRT *sprite = row < HUD_LAP_SLOTS
            ? &frame->layout.raceHud.lapTimes[row]
            : &frame->layout.raceHud.labels[row - HUD_LAP_SLOTS];
        const char *what = "label";
        if (row < HUD_LAP_SLOTS) what = "lap time";
        else if (row == 9) what = mode != 0 ? "position tens / split time"
                                            : "split time";
        else if (row == 10) what = mode != 0 ? "position units / split sign"
                                             : "split sign";
        printf("%3d  %8d  %5d  %3dx%-3d  %s\n", row, (int)descs[row].x,
               (int)sprite->x0, (int)sprite->w, (int)sprite->h, what);
    }
}

int main(int argc, char **argv) {
    const char *vramPath = OptionValue(argc, argv, "--vram");
    const char *outputPath = OptionValue(argc, argv, "--output");
    const char *mode = OptionValue(argc, argv, "--mode");
    const char *aspect = OptionValue(argc, argv, "--aspect");
    const char *anchor = OptionValue(argc, argv, "--anchor");
    const char *background = OptionValue(argc, argv, "--background");
    unsigned short *vram;
    RECT full = {0, 0, 1024, 512};
    DRAWENV draw;
    DISPENV display;
    FILE *file;
    OT_TYPE *ot;
    s32 hudMode;
    int red = 24, green = 24, blue = 32;
    int index;
    int split;

    if (vramPath == NULL) {
        Usage(argc > 0 ? argv[0] : "rage-hud-preview");
        return EXIT_FAILURE;
    }
    if (outputPath == NULL) outputPath = "hud.ppm";
    hudMode = mode != NULL && strcmp(mode, "gp") == 0 ? 1 : 0;
    if (anchor != NULL) s_anchor = anchor;
    s_config.modernAspect = aspect != NULL && strcmp(aspect, "4:3") == 0
        ? RAGE_MODERN_ASPECT_4_3
        : RAGE_MODERN_ASPECT_16_9;
    if (background != NULL &&
        sscanf(background, "%d,%d,%d", &red, &green, &blue) != 3) {
        fprintf(stderr, "rage-hud-preview: bad --background %s\n", background);
        return EXIT_FAILURE;
    }

    vram = malloc(1024u * 512u * sizeof(*vram));
    if (vram == NULL) return EXIT_FAILURE;
    file = fopen(vramPath, "rb");
    if (file == NULL ||
        fread(vram, sizeof(*vram), 1024u * 512u, file) != 1024u * 512u) {
        fprintf(stderr,
                "rage-hud-preview: need a 1024x512 RGB5551 VRAM snapshot: %s\n",
                vramPath);
        if (file != NULL) fclose(file);
        free(vram);
        return EXIT_FAILURE;
    }
    fclose(file);

    ResetGraph(0);
    if (s_config.modernAspect == RAGE_MODERN_ASPECT_16_9 &&
        strcmp(s_anchor, "center") != 0) {
        s_originX = RAGE_HUD_PREVIEW_MARGIN;
        s_width = HUD_CANVAS_WIDTH + RAGE_HUD_PREVIEW_MARGIN * 2;
    }
    SetDefDrawEnv(&draw, 0, 0, s_width, HUD_HEIGHT);
    SetDefDispEnv(&display, 0, 0, s_width, HUD_HEIGHT);
    draw.ofs[0] = (short)s_originX;
    draw.ofs[1] = 0;
    PutDrawEnv(&draw);
    PutDispEnv(&display);
    SetDispMask(1);
    LoadImage(&full, (u_long *)vram);
    DrawSync(0);
    FillBackground(red, green, blue);

    /* The game's frame context, wound up far enough for the HUD: one
     * ordering table, and a primitive cursor with room to build into. */
    g_DrawBuffer = &g_FrameContexts[0];
    g_FrameParity = 0;
    ot = GamePrimaryOrderingTable(0);
    ClearOTagR(ot, GAME_FRAME_OT_LENGTH);
    RENDER_PRIM_CURSOR_AS(u8) = g_FrameContexts[0].layout.primitiveBuffer;

    g_GrandPrixMode = hudMode;
    g_GrandPrixClass = OptionNumber(argc, argv, "--class", 0);
    g_LapCount = OptionNumber(argc, argv, "--lap-count", 3);
    g_PlayerCar.lap = OptionNumber(argc, argv, "--laps-done", g_LapCount);
    g_PlayerCar.drive.hudLapHighlightRow =
        OptionNumber(argc, argv, "--current-lap", 1);
    g_BestLapThisRace = OptionNumber(argc, argv, "--best", 92345);
    g_RacePosition = OptionNumber(argc, argv, "--position", 1);
    for (index = 0; index < HUD_LAP_SLOTS; index++)
        g_PlayerCar.lapTimes.table.milliseconds[index] = 95000 + index * 1234;
    for (index = 1; index + 1 < argc; index++) {
        int slot, value;
        if (strcmp(argv[index], "--lap") != 0) continue;
        if (sscanf(argv[index + 1], "%d=%d", &slot, &value) != 2 ||
            slot < 0 || slot >= HUD_LAP_SLOTS) {
            fprintf(stderr, "rage-hud-preview: bad --lap %s\n", argv[index + 1]);
            free(vram);
            return EXIT_FAILURE;
        }
        g_PlayerCar.lapTimes.table.milliseconds[slot] = value;
    }

    BuildRaceHudPrims(hudMode);
    DrawLapTimes();
    DrawRaceHudLabels(hudMode);
    if (hudMode != 0) DrawRacePosition();
    split = OptionNumber(argc, argv, "--split", 0);
    /* The split times and the total are a separate pass in the race, and
     * they are half of what the spread layout has to place. */
    g_SplitTimer = 0;
    g_SectorIndex = 1;
    /* The game keeps the delta as a magnitude and the direction in the
     * sign, so a negative delta is a state it never produces: feeding one in
     * makes DrawTimeValue print its unset-value dashes. */
    g_SplitSign = (s16)(split > 0 ? 1 : (split < 0 ? -1 : 0));
    g_SplitDelta = split < 0 ? -split : split;
    g_SplitSector = (s16)OptionNumber(argc, argv, "--sector", 1);
    g_LapTimeMs = OptionNumber(argc, argv, "--best", 92345);
    g_LastSectorTime = OptionNumber(argc, argv, "--sector-time", 31450);
    g_SplitTargetTime = OptionNumber(argc, argv, "--target-time", 91000);
    g_BestTotalTimes[0][0][0] = OptionNumber(argc, argv, "--total", 278900);
    g_RaceSeries = 0;
    /* DrawSplitTimes draws the delta itself. Adding the same primitive to
     * an ordering table twice links it to itself, and DrawOTag then walks a
     * loop until the GPU runs out of memory. */
    DrawSplitTimes();
    DrawTimeRemaining(OptionNumber(argc, argv, "--time-left", 4500));

    DrawOTag(ot + GAME_FRAME_OT_LENGTH - 1);
    DrawSync(0);

    if (!WritePpm(outputPath)) {
        fprintf(stderr, "rage-hud-preview: cannot write %s\n", outputPath);
        free(vram);
        return EXIT_FAILURE;
    }
    ReportSprites(hudMode);
    fprintf(stderr, "rage-hud-preview: wrote %s (%dx%d)\n", outputPath,
            s_width, HUD_HEIGHT);
    free(vram);
    ResetGraph(0);
    return EXIT_SUCCESS;
}
