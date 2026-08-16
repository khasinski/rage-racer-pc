#include "common.h"
#include "game/prim.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/input_internal.h"

/* Copied into a local solely to preserve the retail code shape. */

/* The 0..3 steering-play setting this screen edits. */


/* Local wide-parameter views; see GameQueueSprite.c / SetGteLightMatrix.c. */

/*
 * Game mode 10's overlay: the caption, the two nudge arrows, the three digit
 * cells of the setting, the framed panel, and the play gauge - two green rules
 * either side of the white centre line, spaced by the selected play scaled
 * into pixels.
 */
void DrawNegconSteerPlayScreen(void) {
    NegconUvTemplate unused;
    u8 *ot;
    u8 *prim;
    s32 span;
    s32 y;

    unused = g_NegconSteerPlayUvQuad;
    DrawSpriteString(0x18, 0x30, g_MsgNegconSteerPlay, 0x7F81);
    ot = (u8 *)GamePrimaryOrderingTable(51);
    prim = SCRATCH_PRIM_CURSOR_AS(u8);
    prim = DrawLeftArrow(ot, prim, 0x28, 0xE0, g_NegconSteerPlay != 0);
    prim = DrawRightArrow(ot, prim, 0x108, 0xE0, g_NegconSteerPlay != 3);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x70, 0x30, 0xC, 0x18, 0x8C, 0x18, 0x7F81);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x7C, 0x30, 0xC, 0x18, g_NegconSteerPlay * 12 + 152, 0x18, 0x7F81);
    prim = GameQueueSpriteTrans(
        ot, prim, 0x88, 0x30, 0xC, 0x18, 0x6C, 0x30, 0x7F81);
    prim = QueueDrawModePrim(ot, prim, 0x3F);
    prim = AddTilePrim(ot, prim, 0, 0x28, 0x124, 0x40, 0, 0, 0);
    prim = AddTilePrim(ot, prim, 0, 0x26, 0x125, 0x44, 0xFF, 0xFF, 0xFF);
    span = ((g_NegconPlayPercent[g_NegconSteerPlay] << 7) / 100) * 2;
    y = 230 - span;
    prim = GameQueueLine(ot, prim, 0x94, y, 0xA8, y, 0x20, 0x40, 0xFF);
    prim = GameQueueLine(ot, prim, 0x94, y + 1, 0xA8, y + 1, 0x20, 0x40, 0xFF);
    prim = GameQueueLine(
        ot, prim, 0x94, span + 230, 0xA8, span + 230, 0x20, 0x40, 0xFF);
    span = span + 231;
    prim = GameQueueLine(ot, prim, 0x94, span, 0xA8, span, 0x20, 0x40, 0xFF);
    prim = GameQueueLine(ot, prim, 0x94, 0xE6, 0xA8, 0xE6, 0, 0, 0);
    SCRATCH_PRIM_CURSOR_AS(u8) =
        GameQueueLine(ot, prim, 0x94, 0xE7, 0xA8, 0xE7, 0, 0, 0);
}


/*
 * Game mode 10: pick the steering play with left/right, confirm with
 * start/cross (on to mode 11, the max-twist screen) or cancel with
 * circle/square. Cancelling - and unplugging the NeGcon - restores the
 * backed-up setting on the way back to mode 1.
 */
void UpdateNegconSteerPlayScreen(void) {
    g_AnimTimer++;
    g_SetupArrowPulse += 96;
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = 1;
        RestoreNegconCalibrationSettings();
    } else if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = 11;
    }
    if (g_PadPressed & PAD_LEFT) {
        if (g_NegconSteerPlay > 0) {
            PlaySoundCue(8);
            g_NegconSteerPlay = g_NegconSteerPlay - 1;
        }
    }
    if (g_PadPressed & PAD_RIGHT) {
        if (g_NegconSteerPlay < 3) {
            PlaySoundCue(8);
            g_NegconSteerPlay = g_NegconSteerPlay + 1;
        }
    }
    if (g_PadType != 0x23) {
        g_GameMode = 1;
        RestoreNegconCalibrationSettings();
    }
    g_ControllerSceneAngleX = -896;
    DrawNegconSteerPlayScreen();
    DrawOptionHintBar(4);
    DrawControllerSetupScene(1);
}


/* The 0..3 maximum-twist setting this screen edits. */

/*
 * Game mode 11's overlay: the caption, the two nudge arrows (lit only while
 * the setting can still move that way), the gauge sprite whose width and texel
 * column follow the 0..3 setting, its end cap, and the framed panel.
 */
void DrawNegconMaxTwistScreen(void) {
    NegconUvTemplate unused;
    u8 *ot;
    u8 *prim;
    s32 xoff;
    s32 w;

    unused = g_NegconMaxTwistUvQuad;
    DrawSpriteString(0x18, 0x30, g_MsgNegconMaxTwist, 0x7F81);
    ot = (u8 *)GamePrimaryOrderingTable(51);
    prim = SCRATCH_PRIM_CURSOR_AS(u8);
    prim = DrawLeftArrow(ot, prim, 0x28, 0xE0, g_NegconMaxTwist != 0);
    prim = DrawRightArrow(ot, prim, 0x108, 0xE0, g_NegconMaxTwist != 3);
    if (g_NegconMaxTwist == 3) {
        xoff = 0;
        w = 0x24;
    } else {
        xoff = 0xC;
        w = 0x18;
    }
    prim = GameQueueSpriteTrans(
        ot, prim, xoff + 0x88, 0x30, w, 0x18, g_NegconMaxTwist * 24, 0x30, 0x7F81);
    prim = GameQueueSpriteTrans(ot, prim, 0xAC, 0x30, 4, 0x18, 0x78, 0x30, 0x7F81);
    prim = QueueDrawModePrim(ot, prim, 0x3F);
    prim = AddTilePrim(ot, prim, 0, 0x28, 0x124, 0x40, 0, 0, 0);
    SCRATCH_PRIM_CURSOR_AS(u8) =
        AddTilePrim(ot, prim, 0, 0x26, 0x125, 0x44, 0xFF, 0xFF, 0xFF);
}

/*
 * Game mode 11: pick the maximum twist range with left/right, confirm with
 * start/cross or cancel with circle/square. Cancelling - and unplugging the
 * NeGcon - restores the backed-up setting on the way back to mode 1.
 */
void UpdateNegconMaxTwistScreen(void) {
    g_AnimTimer++;
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = 1;
        RestoreNegconCalibrationSettings();
    } else if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = 1;
    }
    if (g_PadPressed & PAD_LEFT) {
        if (g_NegconMaxTwist > 0) {
            PlaySoundCue(8);
            g_NegconMaxTwist = g_NegconMaxTwist - 1;
        }
    }
    if (g_PadPressed & PAD_RIGHT) {
        if (g_NegconMaxTwist < 3) {
            PlaySoundCue(8);
            g_NegconMaxTwist = g_NegconMaxTwist + 1;
        }
    }
    if (g_PadType != 0x23) {
        g_GameMode = 1;
        RestoreNegconCalibrationSettings();
    }
    g_ControllerSceneAngleX = -896;
    DrawNegconMaxTwistScreen();
    DrawOptionHintBar(4);
    DrawControllerSetupScene(1);
}
