#include "game/car.h"
#include "game/menu.h"
#include "game/state.h"

enum {
    MENU_ACTIVE_SCREEN_FADE_STEP = 20,
    MENU_OUTGOING_SCREEN_FADE_STEP = -10,
    MENU_NEAR_OT_SHIFT = 1,
    MENU_DEFAULT_OT_SHIFT = 5,
};

static MenuRuntime s_menuRuntime = {
    .activeScreen = MENU_SCREEN_BOOTSTRAP,
    .activeDrawScreen = -1,
    .outgoingDrawScreen = -1,
};

void MenuRuntimeReset(void) {
    s_menuRuntime = (MenuRuntime){
        .activeScreen = MENU_SCREEN_BOOTSTRAP,
        .activeDrawScreen = -1,
        .outgoingDrawScreen = -1,
    };
}

const MenuRuntime *MenuRuntimeCurrent(void) { return &s_menuRuntime; }

ControllerSetup *MenuControllerSetup(void) {
    return &s_menuRuntime.controllerSetup;
}

CourseSelectScreen *MenuCourseSelect(void) {
    return &s_menuRuntime.courseSelect;
}

BrowseArrows *MenuBrowseArrows(void) {
    return &s_menuRuntime.browseArrows;
}

EngineerShop *MenuEngineerShop(void) {
    return &s_menuRuntime.engineerShop;
}

PaintColor *MenuPaintColor(void) {
    return &s_menuRuntime.paintColor;
}

Customize *MenuCustomize(void) {
    return &s_menuRuntime.customize;
}

LogoSample *MenuLogoSample(void) {
    return &s_menuRuntime.logoSample;
}

TeamName *MenuTeamName(void) {
    return &s_menuRuntime.teamName;
}

CarSpecGraph *MenuCarSpecGraph(void) {
    return &s_menuRuntime.carSpecGraph;
}

CarSelect *MenuCarSelect(void) {
    return &s_menuRuntime.carSelect;
}

OptionMenu *MenuOption(void) {
    return &s_menuRuntime.optionMenu;
}

MenuWidgets *MenuWidgetState(void) {
    return &s_menuRuntime.widgets;
}

s32 MenuRuntimeScreenState(s32 screen) {
    if ((u32)screen >= MENU_SCREEN_COUNT) return 0;
    return s_menuRuntime.screenState[screen];
}

void MenuRuntimeSetScreenState(s32 screen, s32 state) {
    if ((u32)screen >= MENU_SCREEN_COUNT) return;
    s_menuRuntime.screenState[screen] = state;
}

void MenuActivateScreen(s32 screen) {
    if (screen <= MENU_SCREEN_BOOTSTRAP || screen >= MENU_SCREEN_COUNT) return;
    s_menuRuntime.activeScreen = screen;
    s_menuRuntime.activeDrawScreen = screen;
}

void MenuActivateEnteringScreen(s32 screen, s32 drawScreen) {
    if (screen <= MENU_SCREEN_BOOTSTRAP || screen >= MENU_SCREEN_COUNT ||
        drawScreen <= MENU_SCREEN_BOOTSTRAP || drawScreen >= MENU_SCREEN_COUNT) {
        return;
    }
    s_menuRuntime.activeScreen = screen;
    s_menuRuntime.activeDrawScreen = drawScreen;
}

void MenuBeginExit(s32 screen) {
    if (screen <= MENU_SCREEN_BOOTSTRAP || screen >= MENU_SCREEN_COUNT) return;
    s_menuRuntime.activeDrawScreen = -1;
    s_menuRuntime.outgoingDrawScreen = screen;
}

s32 MenuOutgoingProgress(void) {
    return s_menuRuntime.outgoingProgress;
}

static u32 CurrentMenuCarTireCompound(void) {
    s32 carIndex = s_menuRuntime.activeScreen == MENU_SCREEN_CAR_SHOP
                       ? g_CarListCursor
                       : g_PlayerCarIndex;

    if (g_CarTable == NULL || (u32)carIndex >= GAME_CAR_COUNT) {
        return 0;
    }
    return g_CarTable[carIndex].tireCompound;
}

static void DrawMenuTransitions(void) {
    if (s_menuRuntime.activeDrawScreen > MENU_SCREEN_BOOTSTRAP &&
        s_menuRuntime.activeDrawScreen < MENU_SCREEN_COUNT) {
        s32 screen = s_menuRuntime.activeDrawScreen;

        g_MenuScreenDraw[screen](&s_menuRuntime.drawProgress[screen],
                                 MENU_ACTIVE_SCREEN_FADE_STEP);
    }
    if (s_menuRuntime.outgoingDrawScreen > MENU_SCREEN_BOOTSTRAP &&
        s_menuRuntime.outgoingDrawScreen < MENU_SCREEN_COUNT) {
        s32 screen = s_menuRuntime.outgoingDrawScreen;

        s_menuRuntime.outgoingProgress =
            g_MenuScreenDraw[screen](&s_menuRuntime.drawProgress[screen],
                                     MENU_OUTGOING_SCREEN_FADE_STEP);
    }
}

static void DrawMenuHints(GameOrderingTableEntry *ot) {
    MenuWidgets *widgets = MenuWidgetState();

    if (widgets->hintStep == 0 ||
        RunTimedDrawScript(g_MenuHintBarScript, &widgets->hintProgress,
                           widgets->hintStep) == 0) {
        return;
    }

    if (widgets->hintButtonsVisible != 0 && ot != NULL) {
        GameOrderingTableEntry *hintOt = ot + 1;
        u16 textureV = g_PadType == PAD_TYPE_NEGCON ? 0xF4 : 0xE8;

        DrawSprite(hintOt, 0xC0, 0x1A1, 0x20, 0xC, 0x94, textureV, 0, 0,
                   0, 0x244, 1, 1, 0x3B);
        DrawSprite(hintOt, 0xF0, 0x1A1, 0x2C, 0xC, 0xB4, textureV, 0, 0,
                   0, 0x244, 1, 1, 0x3B);
    }
    DrawBitPatternOverlay(g_MenuOverlayPattern);
}

void UpdateMenuMode(void) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    g_SceneTimer = (s32)((u32)g_SceneTimer + 1u);
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    if (ot != NULL) {
        DrawSolidRect(ot, 0, 0, 0x140, 2, 0, 0, 0, 0xFF);
    }

    if ((u32)s_menuRuntime.activeScreen >= MENU_SCREEN_COUNT) {
        MenuRuntimeReset();
    }
    if (s_menuRuntime.activeScreen == MENU_SCREEN_COURSE_SELECT ||
        s_menuRuntime.activeScreen == MENU_SCREEN_RANKING) {
        g_RenderState.pass.otShift = MENU_NEAR_OT_SHIFT;
    } else {
        g_RenderState.pass.otShift = MENU_DEFAULT_OT_SHIFT;
    }

    DrawMenuTransitions();
    g_MenuScreenUpdate[s_menuRuntime.activeScreen]();
    DrawCarSpecGraph(MenuCarSpecGraph(), CurrentMenuCarTireCompound());
    DrawMenuHints(ot);
}
