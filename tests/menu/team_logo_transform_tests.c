#include "common.h"
#include "game/menu.h"

#include <stdio.h>

TeamLogoCanvas g_TeamLogoCanvas;

static s32 s_lastCue;

void PlaySoundCue(s32 cue) { s_lastCue = cue; }

static TeamLogoCanvas Pattern(void) {
    TeamLogoCanvas canvas = {0};
    s32 x;
    s32 y;

    for (y = 0; y < TEAM_LOGO_HEIGHT; y++) {
        for (x = 0; x < TEAM_LOGO_WIDTH; x++) {
            SetTeamLogoCanvasPixel(
                &canvas, x, y,
                (u32)(x * 3 + y * 5 + (x / 7) + (y / 11)));
        }
    }
    return canvas;
}

typedef void (*Transform)(void);
typedef void (*SourceCoordinate)(s32 x, s32 y, s32 *sourceX, s32 *sourceY);

static int CheckTransform(const char *name, Transform transform,
                          SourceCoordinate sourceCoordinate, s32 cue) {
    TeamLogoCanvas source = Pattern();
    s32 x;
    s32 y;

    g_TeamLogoCanvas = source;
    s_lastCue = -1;
    transform();
    if (s_lastCue != cue) {
        printf("FAIL %s played cue %d, expected %d\n", name, s_lastCue, cue);
        return 0;
    }
    for (y = 0; y < TEAM_LOGO_HEIGHT; y++) {
        for (x = 0; x < TEAM_LOGO_WIDTH; x++) {
            s32 sourceX;
            s32 sourceY;
            u32 actual;
            u32 expected;

            sourceCoordinate(x, y, &sourceX, &sourceY);
            actual = GetTeamLogoCanvasPixel(&g_TeamLogoCanvas, x, y);
            expected = GetTeamLogoCanvasPixel(&source, sourceX, sourceY);
            if (actual != expected) {
                printf("FAIL %s at %d,%d: %u, expected source %d,%d=%u\n",
                       name, x, y, actual, sourceX, sourceY, expected);
                return 0;
            }
        }
    }
    return 1;
}

static void Up(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = x;
    *sy = (y + 1) & 63;
}

static void Down(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = x;
    *sy = (y + 63) & 63;
}

static void Left(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = (x + 1) & 63;
    *sy = y;
}

static void Right(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = (x + 63) & 63;
    *sy = y;
}

static void Vertical(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = x;
    *sy = 63 - y;
}

static void Horizontal(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = 63 - x;
    *sy = y;
}

static void CounterClockwise(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = 63 - y;
    *sy = x;
}

static void Clockwise(s32 x, s32 y, s32 *sx, s32 *sy) {
    *sx = y;
    *sy = 63 - x;
}

static int CheckPixelAccess(void) {
    TeamLogoCanvas canvas = {0};

    SetTeamLogoCanvasPixel(&canvas, 7, 0, 0x1F);
    SetTeamLogoCanvasPixel(&canvas, 8, 0, 2);
    if (GetTeamLogoCanvasPixel(&canvas, 7, 0) != 0xF ||
        GetTeamLogoCanvasPixel(&canvas, 8, 0) != 2 ||
        GetTeamLogoCanvasPixel(&canvas, 6, 0) != 0 ||
        GetTeamLogoCanvasPixel(&canvas, 9, 0) != 0) {
        puts("FAIL packed team logo pixel access");
        return 0;
    }
    return 1;
}

int main(void) {
    int ok = CheckPixelAccess();

    ok &= CheckTransform("scroll up", ScrollTeamLogoUp, Up, 1);
    ok &= CheckTransform("scroll down", ScrollTeamLogoDown, Down, 1);
    ok &= CheckTransform("scroll left", ScrollTeamLogoLeft, Left, 1);
    ok &= CheckTransform("scroll right", ScrollTeamLogoRight, Right, 1);
    ok &= CheckTransform("flip vertical", FlipTeamLogoVertical, Vertical, 8);
    ok &= CheckTransform("flip horizontal", FlipTeamLogoHorizontal, Horizontal, 8);
    ok &= CheckTransform("rotate counter-clockwise", RotateTeamLogoCcw,
                         CounterClockwise, 8);
    ok &= CheckTransform("rotate clockwise", RotateTeamLogoCw, Clockwise, 8);
    if (!ok) return 1;
    puts("all team logo transforms map every pixel correctly");
    return 0;
}
