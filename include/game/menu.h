#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "common.h"

#include "game/menu_types.h"
#include "game/render_state.h"
#include "game/state.h"
#include "game/vector.h"
#include "psyq/gte.h"
#include "game/render.h"

/*
 * Menu / UI state block at 0x8009B200 (~0x550 bytes). Retail addresses it field
 * by field as absolute globals, never through a base pointer, so the named
 * externs below - not a struct - are the usable handles.
 */

enum MenuScreenId {
    MENU_SCREEN_BOOTSTRAP = 0,
    MENU_SCREEN_COURSE_SELECT,
    MENU_SCREEN_RANKING,
    MENU_SCREEN_ENTER_CAR_SELECT,
    MENU_SCREEN_CAR_SELECT,
    MENU_SCREEN_CUSTOMIZE,
    MENU_SCREEN_DESIGN_MODE,
    MENU_SCREEN_TEAM_LOGO,
    MENU_SCREEN_LOGO_SAMPLE,
    MENU_SCREEN_TEAM_NAME,
    MENU_SCREEN_PAINT_COLOR,
    MENU_SCREEN_CAR_SHOP,
    MENU_SCREEN_ENGINEER_SHOP,
    MENU_SCREEN_UNUSED,
    MENU_SCREEN_COUNT,
};

/* Grid-menu cursor (0..0x2B, a 4x11 grid; 0x2A/0x2B are the confirm buttons). */
extern s32 GameMenuCursor;
/* Non-zero while a screen transition is in progress; gates re-entry. */
extern s32 GameMenuBusy;
/* Cursor animation gate: input is only accepted while this is negative. */
extern s32 GameMenuCursorAnim;
/* Which g_MenuScreenDraw entry to run this frame, -1 for none; UpdateMenuMode
 * calls it with a step of 0x14. */
extern s32 g_MenuHandlerIndex;

/* Screen being faded out during a transition. UpdateMenuMode draws it with a
 * step of -10 and stores the result in g_MenuOutgoingScreenProgress. */
extern s32 g_MenuOutgoingHandlerIndex;

/* Which menu-mode screen is running; the id dispatched through
 * g_MenuScreenUpdate. */
extern s32 g_MenuScreen;

/*
 * The two parallel screen tables UpdateMenuMode dispatches through, both indexed
 * by the same screen id: g_MenuScreenUpdate holds the per-frame state machines
 * (selected by g_MenuScreen) and g_MenuScreenDraw the matching fade/transition
 * overlays (selected by g_MenuHandlerIndex / g_MenuOutgoingHandlerIndex). See the
 * screen-table block at the bottom of this header for the entries.
 */
extern void (*g_MenuScreenUpdate[MENU_SCREEN_COUNT])(void);
extern s32 (*g_MenuScreenDraw[MENU_SCREEN_COUNT])(s32 step);

/*
 * Title-menu cursor, 0..4 (UpdateMainMenuInput wraps it with `(sel + 5) % 5` on the
 * up/down pad edges and skips entry 1 while g_ExtraGrandPrixUnlocked is 0). 0 and 1 are the
 * two Grand Prix save files - they repoint g_CarTable / g_RaceProgress / g_CourseProgress
 * at that file's tables and set g_GrandPrixMode to 1 - 2 is Time Attack
 * (g_GrandPrixMode 0), 3 starts the attract demo and 4 opens the options.
 * DrawMainMenuRows draws the row whose index equals it as selected.
 */
extern s32 g_TitleMenuSelection;

/*
 * Element mask handed to DrawBitPatternOverlay by
 * UpdateMenuMode, selecting which parts of the current menu overlay are drawn.
 * -1 while a screen is still opening; screens then set their own pattern.
 */
extern s32 g_MenuOverlayPattern;

/* Debug/status phase code written through an asset-load state machine. */
extern s32 GameMenuLoadPhase;

/*
 * Alternate menu layout. The garage screens copy the setting into the live
 * flag on entry, RANKING / TEAM LOGO / LOGO SAMPLE force it to 0. Non-zero
 * pulls the 3D car view back (40 -> 64), shifts the HUD left by 0x2C, widens
 * the bottom bar and makes DrawScriptedSprite skip element types 9/19/29/39.
 * The setting is only ever written 0, so the layout is unreachable in retail.
 */
extern s32 g_MenuAltLayout;
extern s32 g_MenuAltLayoutSetting;

/* The two RaceRecord[series][course][5] high-score tables kept in the save block:
 * race ranking (+0x9A4) and time ranking (+0x8DC). */
extern RaceRecord g_RankingRecords[][4][5];
extern RaceRecord g_TimeRecords[][4][5];

/* The team-name entry buffer and its length, capped at 6 characters. The pair is
 * also the first bytes of the memory-card save header row. */
extern u8 g_TeamNameLength;
extern u8 g_TeamNameChars[];

/* g_McCardStatus is the last PollMemoryCardStatus result (0 no card yet, 1/2
 * card present, -1/-2/-3 error), not a record pointer; the others are
 * selection/phase words. */
extern s32 g_McMenuState;
extern s32 g_McCardStatus;
extern s32 g_McMenuSelection;

/* The two eased current/target pairs of the 3D menu view, in 1/1000 units:
 * an angle (carousel wraps at 500000 per entry) and a translation. Screens set
 * only the *Target words. */
extern s32 g_MenuViewAngle;
extern s32 g_MenuViewAngleTarget;
extern s32 g_MenuViewOffset;
extern s32 g_MenuViewOffsetTarget;

enum MenuLayout {
    MENU_FADE_MAX = 0x1FC,
    MENU_FADE_COMPLETE = MENU_FADE_MAX + 1,
    MENU_FADE_INTENSITY_DIVISOR = 4,
    MENU_TEAM_NAME_MAX_LENGTH = 6,
    MENU_TEAM_NAME_GRID_COLUMNS = 11,
    MENU_TEAM_NAME_GRID_ROWS = 4,
};

/*
 * One caption of the setup-screen hint bar. `width` both sizes the sprite and
 * centres it, and `advance` is how far down the bar the next element starts.
 * Records are indexed by the DrawOptionHintBar argument.
 */
extern OptionHintCaption g_OptionHintCaptions[MENU_OPTION_HINT_COUNT];

/*
 * Slide geometry shared by the CAR SHOP and ENGINEER SHOP money panels. Both
 * count 0..25; the panel appears at 11 and rises 35 lines a frame for the next
 * eleven frames.
 */
enum ShopPricePanelLayout {
    SHOP_PANEL_SLIDE_MAX = 25,
    SHOP_PANEL_VISIBLE_AT = 11,
    SHOP_PANEL_RISE_FRAMES = 11,
    SHOP_PANEL_RISE_PIXELS_PER_FRAME = 35,
    SHOP_PANEL_MONEY_BOX_Y = 492,
    SHOP_PANEL_MONEY_TEXT_Y = 502,
    SHOP_PANEL_PRICE_BOX_Y = 532,
    SHOP_PANEL_PRICE_TEXT_Y = 542
};

/* Slide counters for the two shop money panels. */
extern s32 g_CarShopPanelSlide;
extern s32 g_EngineerShopPanelSlide;
void DrawCarShopPricePanel(s32 step, s32 money, s32 price);
void DrawEngineerShopPricePanel(s32 step, s32 money, s32 price);

/* Menu widgets: an outlined filled box, the two-ring selection frame, and the
 * timeline sprites whose brightness decays through g_MenuRowFlashLevels[]. */
void GameDrawMenuButton(
    s32 x,
    s32 y,
    s32 width,
    s32 height,
    u8 r,
    u8 g,
    u8 b);
void DrawMenuCursorBox(
    s32 x0,
    s32 y0,
    s32 x1,
    s32 y1,
    s32 flash);
void DrawFadingMenuSprites(
    s32 progress,
    s32 lastRow,
    s32 selectedRow);

/* Menu-mode entry: reloads the live globals from g_RaceProgress, seeds the
 * render state, zeroes 0x8009B2F8..0x8009B378 and resets all
 * fourteen per-screen transition accumulators. */
void InitMenuMode(void);
void UpdateMenuMode(void);

/*
 * The menu-mode screen table pair: everything the front end shows once
 * g_GameMode == 3 is one of fourteen screens, dispatched from UpdateMenuMode
 * through g_MenuScreenUpdate[g_MenuScreen] (state machine) and
 * g_MenuScreenDraw[g_MenuHandlerIndex] (fade overlay). Each Draw entry owns a
 * private accumulator in 0x8009B2C4..0x8009B2EC, clamped to [0, 0x1FC]; a
 * `step` of 0 resets it, positive fades in, negative fades out.
 */

/* id 1 -- course + class picker; left/right change course, up/down the rows. */
void EnterCourseSelectScreen(void);
/* Whether the course before or after the current one may be picked; both
 * depend on the class and on whether a series is selected. In course_select.c,
 * next to the course list they read. */
s32 CanSelectPrevCourse(void);
s32 CanSelectNextCourse(void);
void UpdateCourseSelectScreen(void);
s32 DrawCourseSelectScreen(s32 step);

/* id 2 -- "RANKING": total time / lap time tables, or exit back to id 1. */
void UpdateRankingScreen(void);
s32 DrawRankingScreen(s32 step);
/* The five record rows: place number + suffix + holder + row background, from
 * the ranking table g_RankingRecords or the time table g_TimeRecords. */
s32 DrawRankingTable(s32 *accumulator, s32 step, s32 table);

/* id 3 -- runs for a single frame on the way from id 1 into id 4. */
void EnterCarSelectScreen(void);

/* id 4 -- "CAR SELECT"; the hub that starts a race or opens the shops. */
/* The yes/no prompt both shops put up before taking the player's money. */
void DrawShopPromptButtons(void *ot, s32 flash);

/* Rescans the owned-car list either side of the current one, and rechecks
 * what the shop and the engineer will accept. Both live in car_select.c. */
void UpdateOwnedCarNeighbours(void);
void RefreshCarUnlockState(void);
void UpdateCarSelectScreen(void);
s32 DrawCarSelectScreen(s32 step);

/* id 5 -- "CUSTOMIZE": tire compound (5 settings) and transmission (AT/MT). */
void UpdateCustomizeScreen(void);
s32 DrawCustomizeScreen(s32 step);

/* id 6 -- "DESIGN MODE": livery hub, branches to team logo / name / colour. */
void UpdateDesignModeScreen(void);
s32 DrawDesignModeScreen(s32 step);

/* id 7 -- "TEAM LOGO": pick a sample logo (id 8) or hand-paint one. */
void UpdateTeamLogoScreen(void);
s32 DrawTeamLogoScreen(s32 step);

/* id 8 -- "TEAM LOGO" sample picker: character and background, 20 each. */
void UpdateLogoSampleScreen(void);
s32 DrawLogoSampleScreen(s32 step);

/*
 * id 9 -- "TEAM NAME": the 4x11 character grid driven by GameMenuCursor, with
 * cell 0x2A = BS and 0x2B = ED. Accepted characters accumulate in
 * g_TeamNameChars[g_TeamNameLength].
 */
void UpdateTeamNameScreen(void);
s32 DrawTeamNameScreen(s32 step);

/* id 10 -- "PAINT COLOR": body colour 1 and 2, 18 choices each. */
void UpdatePaintColorScreen(void);
s32 DrawPaintColorScreen(s32 step);

/* id 11 -- "SHOP" (car shop): browse every car and buy the selected one. */
void UpdateCarShopScreen(void);
s32 DrawCarShopScreen(s32 step);

/* id 12 -- "SHOP" (engineer shop): pay the tune-up fee to grade the car up. */
void UpdateEngineerShopScreen(void);
s32 DrawEngineerShopScreen(s32 step);

/*
 * Menu widgets shared across those screens. Each keeps its own accumulator and
 * follows the same `step` convention as the Draw handlers above: 0 resets and
 * draws nothing, positive ramps in, negative ramps out.
 */
/* The four-bar car performance chart; only visible on CUSTOMIZE. */
void DrawCarSpecGraph(s32 step, u32 tireGrade);
/* "MAX POWER <n> ps / <n> rpm" and "MAX TORQUE <n>.<n> kgm / <n> rpm". */
void DrawCarEngineSpec(s32 slideRaw, s32 brightness);
/* The TEAM NAME 4x11 grid, its highlight and caret, and the typed name. */
void DrawTeamNameEntry(s32 step, s32 cursorIndex);
/* The 3D car view behind screens 3, 4, 5, 6, 10, 11, 12: eases
 * g_MenuViewAngle/Offset, then submits the car and the showroom floor. */
void DrawMenuCarView(void);
/* Draw and input halves of the logo painter. The canvas D_801E6F2C is a 64x64
 * 4bpp bitmap with its own 16-entry CLUT at g_TeamLogoClut. */
extern u16 g_TeamLogoClut[16];
extern TeamLogoCanvas g_TeamLogoCanvas;
void DrawTeamLogoCanvas(s32 panelStep, s32 editorStep);
void UpdateTeamLogoCanvas(void);

/*
 * The eight whole-canvas transforms UpdateTeamLogoCanvas offers, each
 * operating in place on g_TeamLogoCanvas (64 rows x 8 words x 8 nibbles).
 * The four scrolls wrap and play cue 1; the flips and rotations play cue 8.
 * Directions are derived from the arithmetic.
 */
void ScrollTeamLogoUp(void);
void ScrollTeamLogoDown(void);
void ScrollTeamLogoLeft(void);
void ScrollTeamLogoRight(void);
/* Mirror about the horizontal axis: row r <-> row 63 - r. */
void FlipTeamLogoVertical(void);
/* Mirror about the vertical axis: nibbles reversed inside each word and word
 * w swapped with word 7 - w. */
void FlipTeamLogoHorizontal(void);

/* dst(y, x) = src(x, 63 - y). */
void RotateTeamLogoCcw(void);
/* dst(y, x) = src(63 - x, y). */
void RotateTeamLogoCw(void);

/*
 * TEAM LOGO editor data, all per-file types.
 *   g_TeamLogoCanvas   D_801E6F2C  2048 bytes = 64x64 4bpp
 *   g_TeamLogoClut     g_TeamLogoClut  16 x u16
 *   g_TeamLogoRect     g_TeamLogoRect  RECT{0x290,0x30,64,16} for the canvas
 *   g_TeamLogoClutRect g_TeamLogoClutRect  RECT{16,480,16,1} for the CLUT
 * g_ClassRecords g_ClassRecords is the 11 x {s16 grade, s16 clears} table.
 */

extern u8 g_TeamLogoExpertMode;
extern s32 g_TeamLogoCursorY;
extern s32 g_TeamLogoViewY;
extern s32 g_TeamLogoGuideMode;
extern s32 g_TeamLogoBrushSize;
extern s32 g_TeamLogoPaletteMode;
extern s32 g_TeamLogoColorChannel;
extern s32 g_ClassChangeApplied;
extern s32 g_CarSpecGraphStep;
extern s32 g_MenuUpperAltPanelStep;
extern s32 g_MenuLowerAltPanelStep;
extern s32 g_CarShopUnlockAll;
extern s32 g_MenuOutgoingScreenProgress;
extern s32 g_CourseCardSpin;
extern s32 g_CourseCardSpinTarget;
extern s32 g_CourseCardPendingGrade;
extern const TimedDrawCommand *g_LogoSampleSubPanelScript;
extern s32 g_TeamLogoPaintArmed;
extern s32 g_CarSelectCursor;
extern const TimedDrawCommand *g_TeamLogoSubPanelScript;
extern s32 g_BgmChangeDelay;
extern s32 g_BgmSelectCdTrack;
extern s32 g_BgmSelectCursor;
extern s32 g_BgmSelectShowUi;
typedef enum BgmSelectStep {
    BGM_SELECT_STEP_INVALID = -1,
    BGM_SELECT_STEP_LOAD_ASSETS,
    BGM_SELECT_STEP_FADE_IN,
    BGM_SELECT_STEP_ACTIVE,
    BGM_SELECT_STEP_EXIT
} BgmSelectStep;

extern BgmSelectStep g_BgmSelectStep;
extern s32 g_BgmSelectTrack;
extern s32 g_BgmTrackCount;
extern s32 g_CarNamePlateStep;
extern s32 g_CarSwapFromIndex;
extern s32 g_CarSwapToIndex;
extern s32 g_CourseSelectOption;
extern s32 g_CourseSwapDelay;
extern s32 g_DesignModeOption;
typedef enum FrontendState {
    FRONTEND_STATE_INVALID = -1,
    FRONTEND_STATE_TITLE,
    FRONTEND_STATE_MENU_OPENING,
    FRONTEND_STATE_MENU_INPUT,
    FRONTEND_STATE_MENU_EXIT,
    FRONTEND_STATE_COUNT
} FrontendState;

extern FrontendState g_FrontendState;
extern s32 g_MainMenuSlide;
extern s32 g_MenuConfirmTimer;
extern s32 g_MenuCourseModelIndex;
extern s32 g_MenuHintBarProgress;
extern s32 g_MenuHintBarStep;
extern s32 g_MenuHintButtonsVisible;
extern s32 g_MenuPendingCourseIndex;
extern s32 g_MenuPlateCarIndex;
extern u8 g_MenuSubCursor;
extern s32 g_MenuViewSpin;
extern u16 g_NegconMaxTwistSaved;
extern u16 g_NegconSteerPlaySaved;
extern s16 g_NextOwnedCarIndex;
extern s32 g_OptionMenuCursor;
extern s32 g_PlayerMoney;
extern s16 g_PrevOwnedCarIndex;
extern s32 g_CustomizeOption;
extern s32 g_ScreenOffsetEditX;
extern s32 g_ScreenOffsetEditY;
extern s32 g_SoundOptionCursor;
extern u16 g_TeamLogoSwatches[15];
extern s32 g_TeamNameCharModel;
extern s32 g_TimeAttackPlateStep;
extern s32 g_TitleAttractTimer;
extern s32 g_TitleExitTimer;
extern s32 g_TitlePulse;
extern TimedDrawCommand g_UiChromeScript[];
extern TimedDrawCommand g_UiChromeScript2[];

void ClearTeamNameTexture(void);
void DrawCarNamePlate(s32 step, s32 model, s32 grade);
void DrawMenuAltPanel(s32 stepA, s32 stepB);
void DrawMenuCourseView(void);
void DrawOptionRootMenu(void);
void UpdateOptionRootMenu(void);
void UpdateClassRecordMenu(void);
void UpdateClassRecordBrowse(void);
void DrawPadTypeHint(void);
void UpdateSoundOptionMenu(void);
void UpdateSoundSettingAdjust(void);
void UpdateScreenAdjustScreen(void);
void DrawTimeAttackPlate(s32 stepArg);
/* Validate and activate one double-buffered showroom model for both CPU-side
 * model access and rendering. */
s32 ActivateShowroomCarModel(s32 slot);

extern char g_MsgInsertController[];
extern char g_MsgControllerError[];
extern char g_MsgNegconUntwistedLine1[];
extern char g_MsgNegconUntwistedLine2[];
extern char g_MsgNegconSteerPlay[];
extern char g_MsgNegconMaxTwist[];
extern char g_FmtRound[];
extern char g_CaptionPrizeMoney2[];
extern char g_FmtPrize1st[];
extern char g_FmtPrize2nd[];
extern char g_FmtPrize3rd[];
extern char g_CaptionBestTotalTime[];
extern char g_CaptionBestLapTime[];
extern char g_FmtBgmNumber[];
extern const char g_MsgOrdinalSt[4];
extern const char g_MsgOrdinalNd[4];
extern const char g_MsgOrdinalRd[4];
extern const char g_MsgOrdinalTh[8];
/* "%d": the only format string the menu code passes to sprintf. */
extern const char g_FormatDecimal[4];
extern s32 g_AttractCycleCount;
extern u8 g_TeamNameFontGlyphs
    [TEAM_NAME_FONT_GLYPH_COUNT * TEAM_NAME_FONT_GLYPH_BYTES];
extern u8 g_TeamNameBlankTile[192];
extern s32 g_TeamLogoZoomSpan;
extern u16 g_TeamLogoFadedClutRect;
extern u16 g_TeamLogoBlankClut[16];
extern s32 g_TeamLogoPanelStep;
extern s32 g_TeamLogoEditorStep;
extern s32 g_TeamLogoDpadRepeatTimer;
extern s32 g_TeamLogoDpadRepeatMask;
extern s32 g_TeamLogoGuideModePrev;
extern s32 g_MenuLightBurstLevel;
extern s32 g_LogoSamplePanelSlide;
extern s32 g_TeamNameEntrySlide;
extern s32 g_OwnedCarCounterSlide;
extern s32 g_ClassChangeCurtainSlide;
extern s32 g_MenuUpperAltPanelProgress;
extern s32 g_MenuLowerAltPanelProgress;
extern s32 g_CourseCardFace;
extern s32 g_TimeAttackPlateProgress;
extern TimedDrawCommand g_CourseSelectGpScript[];
extern TimedDrawCommand g_CourseSelectTimeAttackScript[];
extern TimedDrawCommand g_CarSelectMenuScriptGp[];
extern TimedDrawCommand g_CarSelectMenuScriptTimeAttack[];
extern TimedDrawCommand g_UiEmptyScript[];
enum {
    CAR_PRICE_COUNT = 32,
    CAR_TUNE_UP_PRICE_COUNT = 31,
};
extern s32 g_CarTuneUpPriceTable[CAR_TUNE_UP_PRICE_COUNT];
extern const char *g_NativeCarManufacturerNames[];
#define g_CarManufacturerNames g_NativeCarManufacturerNames
/* The eight PS / rpm / kgm captions DrawEngineSpecLabel picks between. */
extern const char *g_NativeEngineSpecLabels[];
#define g_EngineSpecLabels g_NativeEngineSpecLabels
extern s32 g_LogoSampleCharIndex;
extern s32 g_LogoSampleBackIndex;
extern s32 g_LogoSampleSavedIndex;
extern s32 g_PaintColorIndex;
extern s32 g_TireSliderPulsePhase;
extern s32 g_BrowseArrowsPulsePhase;
extern s32 g_TeamLogoColorCycleAngle;
extern s32 g_TeamNameCursorPhase;
extern s32 g_PaintPalettePulsePhase;
extern s32 g_TeamLogoFadeLevel;
extern s32 g_TeamLogoZoomLevel;
extern u16 g_TeamLogoFadedClut[16];
extern s32 g_RankingScrollState;
extern s32 g_RankingPendingState;
extern s32 g_CarSelectFadeAccum;
extern s32 g_CustomizeFadeAccum;
extern s32 g_DesignModeScreenFade;
extern s32 g_TeamLogoScreenFade;
extern s32 g_LogoSampleScreenFade;
extern s32 g_TeamNameScreenProgress;
extern s32 g_PaintColorScreenProgress;
extern s32 g_CarShopScreenProgress;
extern s32 g_EngineSpecStep;
extern s32 g_LogoSampleCursor;
extern s32 g_ShopCarIndex;
extern s32 g_RankingCursor;
extern u16 g_DispEnv0ScreenX;
extern u16 g_DispEnv0ScreenY;
extern u16 g_DispEnv1ScreenX;
extern u16 g_DispEnv1ScreenY;
extern s32 g_EngineerShopOption;
extern s32 g_CarShopOption;
extern s32 g_TitleFadeLevel;
extern s32 g_PaintColorCursor;
extern s32 g_TeamLogoOption;
extern char *g_BgmTrackNames[];
extern s32 g_BrowseArrowsFade;
extern s32 g_CarNamePlateFade;
extern s32 g_CarPriceTable[CAR_PRICE_COUNT];
extern s32 g_CarSpecBars[4];
extern s32 g_CarSpecGraphProgress;
extern s32 g_ClassRecordMenuCursor;
extern u8 g_LastValidPadType;
extern TimedDrawCommand g_MenuHintBarScript[];

/* Retail stores timed-draw commands as packed 12-byte records containing
 * 32-bit PSX addresses. Native pointers widen those records, so the host must
 * use decoded TimedDrawCommand arrays instead of walking the serialized data
 * in host_state.c directly. */
#define RAGE_NATIVE_UI_SCRIPT(name, count) \
    extern TimedDrawCommand g_Native##name[count]
RAGE_NATIVE_UI_SCRIPT(RankingPanelScript, 5);
RAGE_NATIVE_UI_SCRIPT(CustomizeMenuScriptGp, 13);
RAGE_NATIVE_UI_SCRIPT(CustomizeMenuScriptTimeAttack, 11);
RAGE_NATIVE_UI_SCRIPT(DesignModeScript, 16);
RAGE_NATIVE_UI_SCRIPT(TeamLogoScreenScript, 12);
RAGE_NATIVE_UI_SCRIPT(LogoSampleScreenScript, 12);
RAGE_NATIVE_UI_SCRIPT(TeamNameScreenScript, 61);
RAGE_NATIVE_UI_SCRIPT(PaintColorScreenScript, 15);
RAGE_NATIVE_UI_SCRIPT(CarShopScreenScript, 9);
RAGE_NATIVE_UI_SCRIPT(EngineerShopScreenScript, 68);
RAGE_NATIVE_UI_SCRIPT(MenuDialogPanelUpperScript, 4);
RAGE_NATIVE_UI_SCRIPT(MenuDialogPanelLowerScript, 8);
RAGE_NATIVE_UI_SCRIPT(CourseSelectSavePromptScript, 4);
RAGE_NATIVE_UI_SCRIPT(CourseSelectSavePromptBanner, 2);
RAGE_NATIVE_UI_SCRIPT(MenuRow0MarkerScript, 4);
RAGE_NATIVE_UI_SCRIPT(MenuRow1MarkerScript, 16);
RAGE_NATIVE_UI_SCRIPT(RankingMenuScript, 9);
RAGE_NATIVE_UI_SCRIPT(TransmissionUnavailableScript, 4);
RAGE_NATIVE_UI_SCRIPT(TeamLogoScreenScript2, 2);
RAGE_NATIVE_UI_SCRIPT(CarShopUnavailableScript, 2);
RAGE_NATIVE_UI_SCRIPT(EngineerShopUnavailableScript, 3);
RAGE_NATIVE_UI_SCRIPT(EngineerShopNoFundsScript, 2);
RAGE_NATIVE_UI_SCRIPT(CarShopNoFundsScript, 5);
RAGE_NATIVE_UI_SCRIPT(DesignModeDeniedScript, 2);
RAGE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript2, 7);
RAGE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript1, 7);
RAGE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript3, 7);
RAGE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript4, 7);
RAGE_NATIVE_UI_SCRIPT(EngineerShopTuneUpPromptScript, 5);
#undef RAGE_NATIVE_UI_SCRIPT

#define g_RankingPanelScript g_NativeRankingPanelScript
#define g_CustomizeMenuScriptGp g_NativeCustomizeMenuScriptGp
#define g_CustomizeMenuScriptTimeAttack g_NativeCustomizeMenuScriptTimeAttack
#define g_DesignModeScript g_NativeDesignModeScript
#define g_TeamLogoScreenScript g_NativeTeamLogoScreenScript
#define g_LogoSampleScreenScript g_NativeLogoSampleScreenScript
#define g_TeamNameScreenScript g_NativeTeamNameScreenScript
#define g_PaintColorScreenScript g_NativePaintColorScreenScript
#define g_CarShopScreenScript g_NativeCarShopScreenScript
#define g_EngineerShopScreenScript g_NativeEngineerShopScreenScript
#define g_MenuDialogPanelUpperScript g_NativeMenuDialogPanelUpperScript
#define g_MenuDialogPanelLowerScript g_NativeMenuDialogPanelLowerScript
#define g_CourseSelectSavePromptScript g_NativeCourseSelectSavePromptScript
#define g_CourseSelectSavePromptBanner g_NativeCourseSelectSavePromptBanner
#define g_MenuRow0MarkerScript g_NativeMenuRow0MarkerScript
#define g_MenuRow1MarkerScript g_NativeMenuRow1MarkerScript
#define g_RankingMenuScript g_NativeRankingMenuScript
#define g_TransmissionUnavailableScript g_NativeTransmissionUnavailableScript
#define g_TeamLogoScreenScript2 g_NativeTeamLogoScreenScript2
#define g_CarShopUnavailableScript g_NativeCarShopUnavailableScript
#define g_EngineerShopUnavailableScript g_NativeEngineerShopUnavailableScript
#define g_EngineerShopNoFundsScript g_NativeEngineerShopNoFundsScript
#define g_CarShopNoFundsScript g_NativeCarShopNoFundsScript
#define g_DesignModeDeniedScript g_NativeDesignModeDeniedScript
#define g_CarShopBuyPromptScript2 g_NativeCarShopBuyPromptScript2
#define g_CarShopBuyPromptScript1 g_NativeCarShopBuyPromptScript1
#define g_CarShopBuyPromptScript3 g_NativeCarShopBuyPromptScript3
#define g_CarShopBuyPromptScript4 g_NativeCarShopBuyPromptScript4
#define g_EngineerShopTuneUpPromptScript g_NativeEngineerShopTuneUpPromptScript
extern u8 g_NegconAxisI;
extern u8 g_NegconAxisII;
extern u8 g_NegconAxisL;
extern u8 g_NegconAxisSteer;
extern u16 g_NegconNeutralIISaved;
extern u16 g_NegconNeutralISaved;
extern u16 g_NegconNeutralLSaved;
extern s16 g_NegconPlayPercent[];
extern u16 g_NegconSteerNeutralSaved;
extern u32 g_OptionMenuExitScene;
extern s16 g_RoundScreenFadeDelays[];

void AdvanceGrandPrixClass(void);
s32 CountOwnedCars(void);
void ComposeSampleTeamLogo(s32 character, s32 background);
void DrawBrowseArrows(s32 step, s32 wide, s32 drawLeft, s32 drawRight);
void DrawLogoSamplePanel(s32 step, s32 sample);
void DrawMenuCursorArrow(s32 x, s32 y);
void DrawMenuLightBurst(s32 arg);
void DrawOptionHintBar(s32 variant);
void RestoreNegconCalibrationSettings(void);
void DrawOwnedCarCounter(s32 direction, s32 ownedCount);
void DrawSpriteString(s32 x, s32 y, const char *str, s32 clutIndex);
void RampTeamLogoCanvas(s32 from, s32 to);
void ShuffleBgmOrder(void);
void StartOptionMenuExit(u32 scene);
void UploadTeamNameTexture(const u8 *str, s32 len);
s32 DrawClassChangeCurtain(s32 step);
void DrawOptionSceneOverlay(void);
void UpdateOptionScene(void);
void UpdateOptionMenuFade(void);
s32 DrawPaintColorPalette(s32 *counter, s32 step, s32 index);
void DrawTeamNameCharModel(void);
void DrawTireCompoundSlider(u8 compound, s32 confirming);
void DrawVolumeBar(s32 level, s32 y);
void UpdateAndDrawCourseCard(void);
void TickClassClearFanfare(void);
void UpdateCarListCursor(void);
void UpdateFrontend(void);
void UpdateTitleAttract(void);

extern SVec g_CourseCardVerts[];
extern Vec4 g_MenuCarPivotOffset;
extern const Vec4 g_TeamNameCharScale;
extern DVec g_ClassRecordCellPoints[];
extern Rgb g_ClassRecordNameSprites[];
extern Matrix g_MenuColorMatrix;
extern Matrix g_MenuLightMatrix;
extern Vec4 g_MenuViewScale;

void DrawPrizeMoneyPanel(s32 y);
extern void (*g_FrontendDrawHandlers[FRONTEND_STATE_COUNT])(void);

#endif
