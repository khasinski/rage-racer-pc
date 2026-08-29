#ifndef GAME_SCREENS_H
#define GAME_SCREENS_H

#include "common.h"

/* Race / front-end screen drawing and text helpers, named from the format
 * strings each one references. Signatures of still-stubbed entries are
 * best-effort. */

/* Formats a lap/race time into dst as %1d'%02d"%03d, returns the write cursor. */
void *FormatLapTime(void *dst, s32 timeMs);

void DrawResultScreen(void);         /* "RESULT" */
void DrawRaceEndPrompt(void);        /* "PRESS START BUTTON" */
void DrawCourseIntro(void);          /* "COURSE IN" / "TIME ATTACK" */
void DrawGrandprixIntro(void);       /* "CLASS%d %s GRANDPRIX" */
/* The in-race option/pause overlay; `cursorRow` is g_RaceOptionCursor. The
 * "RAGE RACER GE" string is one half of a scrolling marquee, not a title -
 * this is not the title screen. */
void DrawRaceOptionMenu(s32 cursorRow);

/*
 * Title screen and main menu. UpdateFrontend runs one of the
 * four handlers below each frame via the jump table at g_FrontendDrawHandlers, indexed by
 * the sub-state g_FrontendState: 0 title -> 1 menu wipe-in -> 2 cursor/confirm ->
 * 3 fade out and request the selected scene.
 */

/* Enters the title screen (g_GameModeHandlers slot 3, requested when an attract
 * or real race ends); EnterFrontend is its twin on slot 2. */
void EnterTitleScreen(void);

/* The pulsing "PRESS START" sprite: a 112x16 cell at (0x68, 0xC8), brightness
 * from rsin(g_AnimTimer * 96). Also drains g_TitleFadeLevel. */
void DrawPressStartPrompt(void);

/* Frontend state 0: hold on the title screen until Start is pressed. */
void UpdateTitleScreen(void);

/* Draws the five main-menu rows (112x16 cells at x = 0x68, y = 0x64 + 0x18*row);
 * the cursor row uses CLUT 0x7E86 instead of 0x7E85 and entry 1 is skipped
 * while g_ExtraGrandPrixUnlocked == 0, leaving four visible rows. */
void DrawMainMenuRows(void);

/* Frontend state 1: the 48-frame menu wipe-in (counter g_MainMenuSlide to 0x30). */
void UpdateMainMenuOpen(void);

/* Frontend state 2: cursor (wrapped % 5, skipping the locked entry 1) and
 * confirm, which repoints g_CarTable / g_RaceProgress / g_CourseProgress at the
 * chosen mode's records and then enters state 3. */
void UpdateMainMenuInput(void);

/* Frontend state 3: fades out over 0x81 frames, then requests the scene for the
 * picked row (6 / 0x1F race, 0x19 SAVE&LOAD, 0x16 OPTION). */
void UpdateMainMenuExit(void);

#endif
