#ifndef GAME_MENU_INTERNAL_H
#define GAME_MENU_INTERNAL_H

#include "common.h"
#include "game/menu_types.h"
#include "game/team_logo.h"
#include "psyq/gpu.h"

/*
 * What a prompt makes of a button press: the sound cues it plays, in the
 * order it plays them, and the state it leaves behind.
 *
 * A prompt's screen function draws, reads the pad, plays sounds and moves
 * state in one body, which means none of it can be exercised without a
 * renderer and three hundred frames of boot to reach the screen. Deciding is
 * the part that can be wrong in a way a player notices, and it is a pure
 * function of the press and the state it acts on, so it is kept separate and
 * every combination of buttons can be checked directly.
 */
typedef struct MenuPromptOutcome {
    s32 cues[4];
    s32 cueCount;
    s32 busy;
    s32 confirmTimer;
    s32 subCursor;
} MenuPromptOutcome;

/* The save prompt offered when a course is chosen: confirm, cancel, and the
 * two directions that pick between its buttons. */
MenuPromptOutcome DecideSavePrompt(u16 pressed, s32 busy, s32 confirmTimer,
                                   s32 subCursor);

/*
 * The class prompt does two kinds of thing, not one, and every press is acted
 * on, so confirming while a direction is held starts the curtain between two
 * sounds rather than before or after both. The order is therefore part of the
 * answer and the outcome carries a list rather than a count of cues.
 */
/* The things a menu decision can ask for, in the order it asks for them. */
enum {
    MENU_PROMPT_CUE = 0,      /* play this sound cue */
    MENU_PROMPT_CURTAIN = 1,  /* start the curtain over the class change */
    MENU_BROWSE_PREV = 2,     /* step the course card to the previous one */
    MENU_BROWSE_NEXT = 3,     /* and to the next */
    MENU_CHOOSE_ROW = 4       /* act on the row the cursor is on */
};

typedef struct MenuPromptEffect {
    s32 kind;
    s32 value;
} MenuPromptEffect;

typedef struct MenuClassPromptOutcome {
    MenuPromptEffect effects[5];
    s32 effectCount;
    s32 busy;
    s32 confirmTimer;
    s32 subCursor;
    s32 changeApplied;
} MenuClassPromptOutcome;

/*
 * The idle screen: moving between its three rows, browsing the courses, and
 * choosing. Unlike the prompts, confirm here acts on the row the directions
 * have just moved to rather than the one that was showing, which is a real
 * difference in feel and is why this is worth stating rather than assuming.
 */
typedef struct CourseSelectInputOutcome {
    s32 cues[2];
    s32 cueCount;
    s32 option;
    /* Whether the player is asking to browse. Whether they may is a question
     * about live state that the screen answers itself, in the order and only
     * as often as it always did. */
    int wantsPrev;
    int wantsNext;
    int choosesRow;
} CourseSelectInputOutcome;

CourseSelectInputOutcome DecideCourseSelectInput(u16 pressed, u16 held,
                                                 s32 option);

/* Choosing the Grand Prix class, which resets the series progress, so picking
 * the class already in use has to close the prompt and change nothing. */
MenuClassPromptOutcome DecideClassPrompt(u16 pressed, s32 busy,
                                         s32 confirmTimer, s32 subCursor,
                                         s32 currentClass, s32 maxClass,
                                         s32 changeApplied);

extern PaintColorTable g_PaintColorTable;
extern s32 g_PaintPalettePulsePhase;
extern s32 g_MenuAltLayout;
extern s32 g_OwnedCarCounterSlide;
extern u16 g_TeamLogoClut[16];
extern TeamLogoCanvas g_TeamLogoCanvas;
extern u8 g_TeamLogoExpertMode;
extern s32 g_TeamLogoCursorY;
extern s32 g_TeamLogoViewY;
extern s32 g_TeamLogoGuideMode;
extern s32 g_TeamLogoBrushSize;
extern TeamLogoCoordinate g_TeamLogoCursorX;
extern TeamLogoCoordinate g_TeamLogoViewX;
extern TeamLogoColorIndex g_TeamLogoPenColor;
extern s32 g_TeamLogoPaletteMode;
extern s32 g_TeamLogoColorChannel;
extern s32 g_TeamLogoDpadRepeatTimer;
extern s32 g_TeamLogoDpadRepeatMask;
extern s32 g_TeamLogoGuideModePrev;
extern s32 g_TeamLogoPaintArmed;
extern s32 g_TeamLogoBlankClut;
extern s32 g_MenuLightBurstLevel;
extern const MenuLightBurstBand g_MenuLightBurstBandX;
extern const MenuLightBurstBand g_MenuLightBurstBandY;
extern const char g_MsgOrdinalSt[4];
extern const char g_MsgOrdinalNd[4];
extern const char g_MsgOrdinalRd[4];
extern const char g_MsgOrdinalTh[8];
extern RaceRecord g_RankingRecords[2][4][5];
extern RaceRecord g_TimeRecords[2][4][5];
extern ClassRecordSprite g_ClassRecordCellSprites[];
extern ScoreRecord g_ClassRecords[11];
extern s32 g_ClassWinCount;
extern NegconUvTemplate g_NegconSteerPlayUvQuad;
extern NegconUvTemplate g_NegconMaxTwistUvQuad;
extern DesignModeCellMask g_DesignModeCellMask;

#endif
