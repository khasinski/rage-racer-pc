#include <stdio.h>
#include <string.h>

#include "game/input_internal.h"
#include "game/menu.h"
#include "game/render_internal.h"
#include "game/render_state.h"
#include "game/state.h"

GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;
u8 g_PadType;
PadValidation g_PadValidation;
ControllerMappingIndex g_PadMappingIndex;
ControllerMappingIndex g_NegconMappingIndex;
static ControllerSetup s_controllerSetup;
ControllerSetup *MenuControllerSetup(void) { return &s_controllerSetup; }
const DVec g_PadLabelSlots[CONTROLLER_CONFIG_LABEL_SLOT_COUNT] = {
    {0, 0}, {10, 12}, {20, 24}, {30, 36}, {40, 48}, {50, 60},
};
const DVec g_PadCalloutLabelPoints[CONTROLLER_CONFIG_LABEL_SLOT_COUNT] = {
    {0, 0}, {10, 12}, {20, 24}, {30, 36}, {40, 48}, {50, 60},
};
const DVec g_PadCalloutButtonPoints[CONTROLLER_CONFIG_BUTTON_POINT_COUNT] = {
    {100, 80}, {101, 81}, {102, 82}, {103, 83},
    {104, 84}, {105, 85}, {106, 86}, {107, 87},
    {108, 88}, {109, 89}, {110, 90}, {111, 91},
    {112, 92}, {113, 93}, {114, 94}, {115, 95},
};
const u8 g_PadConfigLabelRows[CONTROLLER_CONFIG_ROW_COUNT] = {
    [0] = 1, [10] = 0, [11] = 1, [12] = 2, [13] = 3, [14] = 4,
};
const u8 g_PadConfigButtonRows[CONTROLLER_CONFIG_ROW_COUNT] = {
    [10] = 0, [11] = 1, [12] = 2, [13] = 3, [14] = 4,
};
const u8 g_NegconConfigLabelRows[CONTROLLER_CONFIG_ROW_COUNT] = {
    [15] = 0, [16] = 1, [17] = 2, [18] = 3, [19] = 4, [35] = 2,
};
const u8 g_NegconConfigButtonRows[CONTROLLER_CONFIG_ROW_COUNT] = {
    [15] = 0, [16] = 1, [17] = 2, [18] = 3, [19] = 4,
};

static GameFrameContext s_frame;
static u8 s_packets[128];
static s32 s_spriteCount;
static s32 s_firstSpriteX;
static s32 s_tileCount;
static s32 s_lineCount;
static s32 s_modeCount;
static s32 s_leftPulse;
static s32 s_rightPulse;
static s32 s_selectorSelection;
static s32 s_selectorCount;
static s32 s_proportionalX;
static const char *s_proportionalText;
static s32 s_textCount;
static s32 s_failures;

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
            s_failures++;                                                                 \
        }                                                                                 \
    } while (0)

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                         s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    if (s_spriteCount == 0) s_firstSpriteX = x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    s_spriteCount++;
    return prim + 1;
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                s32 width, s32 height, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    s_tileCount++;
    return prim + 1;
}

u8 *GameQueueLine(GameOrderingTableEntry *ot, u8 *prim, s32 x0, s32 y0,
                  s32 x1, s32 y1, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)r;
    (void)g;
    (void)b;
    s_lineCount++;
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    s_modeCount++;
    return prim + 1;
}

u8 *DrawLeftArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                  s32 pulse, s32 phase) {
    (void)ot;
    (void)x;
    (void)y;
    (void)phase;
    s_leftPulse = pulse;
    return prim + 1;
}

u8 *DrawRightArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                   s32 pulse, s32 phase) {
    (void)ot;
    (void)x;
    (void)y;
    (void)phase;
    s_rightPulse = pulse;
    return prim + 1;
}

u8 *DrawPadConfigSelector(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                          s32 selection) {
    (void)ot;
    (void)x;
    (void)y;
    s_selectorCount++;
    s_selectorSelection = selection;
    return prim + 1;
}

void DrawProportionalText(s32 x, s32 y, const char *text, s32 clut) {
    (void)y;
    (void)clut;
    s_proportionalX = x;
    s_proportionalText = text;
}

void DrawSpriteString(s32 x, s32 y, const char *text, s32 clut) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
    s_textCount++;
}

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_DrawBuffer = &s_frame;
    g_RenderState.draw.packetCursor = s_packets;
    g_PadValidation.error = PAD_ERROR_STATE_NONE;
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadMappingIndex = 2;
    g_NegconMappingIndex = 3;
    MenuControllerSetup()->angleY = 0;
    s_spriteCount = 0;
    s_firstSpriteX = -1;
    s_tileCount = 0;
    s_lineCount = 0;
    s_modeCount = 0;
    s_leftPulse = -1;
    s_rightPulse = -1;
    s_selectorCount = 0;
    s_selectorSelection = -1;
    s_proportionalX = -1;
    s_proportionalText = NULL;
    s_textCount = 0;
}

static void TestPadAndNegconScreens(void) {
    Reset();
    DrawControllerConfigScreen(&s_controllerSetup);
    CHECK(s_leftPulse == 1 && s_rightPulse == 1);
    CHECK(s_selectorCount == 1 && s_selectorSelection == 2);
    CHECK(s_spriteCount == 6 && s_tileCount == 10 && s_lineCount == 15);
    CHECK(s_modeCount == 1);
    CHECK(g_RenderState.draw.packetCursor == s_packets + 35);

    Reset();
    g_PadType = PAD_TYPE_NEGCON;
    g_NegconMappingIndex = 7;
    DrawControllerConfigScreen(&s_controllerSetup);
    CHECK(s_leftPulse == 1 && s_rightPulse == 0);
    CHECK(s_selectorSelection == 7);
    CHECK(s_spriteCount == 7 && s_tileCount == 10 && s_lineCount == 15);
    CHECK(s_modeCount == 2);
    CHECK(g_RenderState.draw.packetCursor == s_packets + 37);
}

static void TestErrorScreens(void) {
    Reset();
    g_PadValidation.error = PAD_ERROR_STATE_DISCONNECTED;
    DrawControllerConfigScreen(&s_controllerSetup);
    CHECK(s_proportionalX == 0x3A &&
          strcmp(s_proportionalText, "INSERT CONTROLLER") == 0);
    CHECK(s_selectorCount == 0 && g_RenderState.draw.packetCursor == s_packets);

    Reset();
    g_PadValidation.error = PAD_ERROR_STATE_INVALID_INPUT;
    DrawControllerConfigScreen(&s_controllerSetup);
    CHECK(s_proportionalX == 0x40 &&
          strcmp(s_proportionalText, "CONTROLLER ERROR") == 0);
    CHECK(s_selectorCount == 0);
}

static void TestCalloutVisibilityAndNeutralPanel(void) {
    Reset();
    MenuControllerSetup()->angleY = 16;
    DrawPadConfigDiagram(GameSecondaryOrderingTable(51), s_packets, s_controllerSetup.angleY);
    CHECK(s_lineCount == 0);
    CHECK(s_spriteCount == 6 && s_tileCount == 10 && s_modeCount == 1);

    Reset();
    DrawNegconNeutralScreen();
    CHECK(s_textCount == 2 && s_tileCount == 2);
    CHECK(g_RenderState.draw.packetCursor == s_packets + 2);
}

static void TestDiagramClampsMappingRows(void) {
    Reset();
    g_PadMappingIndex = -1;
    DrawPadConfigDiagram(GameSecondaryOrderingTable(51), s_packets, s_controllerSetup.angleY);
    CHECK(s_firstSpriteX == g_PadLabelSlots[1].vx + 4);

    Reset();
    g_NegconMappingIndex = 99;
    DrawNegconConfigDiagram(GameSecondaryOrderingTable(51), s_packets, s_controllerSetup.angleY);
    CHECK(s_firstSpriteX == g_PadLabelSlots[2].vx + 4);
}

int main(void) {
    TestPadAndNegconScreens();
    TestErrorScreens();
    TestCalloutVisibilityAndNeutralPanel();
    TestDiagramClampsMappingRows();
    return s_failures != 0;
}
