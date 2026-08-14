#include "common.h"
#include "game/prim.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/input_internal.h"


/* The two 0..7 selections; g_PadType picks which one the screen edits. */


/*
 * Game mode 7's screen: either the "no controller" banner, or the two nudge
 * arrows (lit only while the selection can still move that way), the selection
 * panel and the diagram for whichever controller is plugged in. The NeGcon
 * variant adds a translucent caption strip under the diagram.
 */
void DrawControllerConfigScreen(void) {
    s32 selection;
    s32 leftLit;
    s32 rightLit;
    u8 *ot;
    u8 *prim;

    if (g_PadType == 0x23) {
        selection = g_NegconMappingIndex;
    } else {
        selection = g_PadMappingIndex;
    }
    /* The arrows light while the selection can still move that way; the xor
     * is folded into `selection` because retail lets it clobber the loaded
     * value rather than allocating a second register. */
    leftLit = selection != 0;
    selection ^= 7;
    rightLit = selection != 0;
    if (g_PadErrorState != PAD_ERROR_STATE_NONE) {
        if (g_PadErrorState == PAD_ERROR_STATE_DISCONNECTED) {
            DrawProportionalText(0x3A, 0xEA, g_MsgInsertController, 0x7812);
        } else {
            DrawProportionalText(0x40, 0xEA, g_MsgControllerError, 0x7812);
        }
    } else {
        ot = (u8 *)GamePrimaryOrderingTable(51);
        prim = SCRATCH_PRIM_CURSOR_AS(u8);
        prim = DrawLeftArrow(ot, prim, 0x28, 0xE0, leftLit);
        prim = DrawRightArrow(ot, prim, 0x108, 0xE0, rightLit);
        if (g_PadType == 0x23) {
            prim = DrawPadConfigSelector(ot, prim, 0xF0, 0x28, g_NegconMappingIndex);
            prim = DrawNegconConfigDiagram(ot, prim);
            prim = GameQueueSpriteTrans(
                ot, prim, 0x10, 0x40, 0xD8, 0x10, 0, 0xA8, 0x7F40);
            prim = QueueDrawModePrim(ot, prim, 0x3F);
        } else {
            prim = DrawPadConfigSelector(ot, prim, 0xF0, 0x28, g_PadMappingIndex);
            prim = DrawPadConfigDiagram(ot, prim);
        }
        SCRATCH_PRIM_CURSOR_AS(u8) = prim;
    }
}

void UpdateControllerConfigScreen(void) {
    u32 flipPhase;
    PadPressedView *pad = GetPadPressedView();

    g_AnimTimer++;
    g_SetupArrowPulse += 96;
    if (pad->buttons & 0x90) {
        PlaySoundCue(3);
        g_GameMode = 1;
        g_PadMappingIndex = g_PadMappingIndexSaved;
        g_NegconMappingIndex = g_NegconMappingIndexSaved;
    }
    if (pad->buttons & 0x860) {
        PlaySoundCue(2);
        LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
        if (g_PadType == 0x23) {
            g_GameMode = (pad->buttons & 0x800) ? 8 : 1;
        } else {
            g_GameMode = 1;
        }
    }
    if (g_PadPressed & PAD_LEFT) {
        if (g_PadType == 0x23) {
            if (g_NegconMappingIndex > 0) {
                PlaySoundCue(8);
                g_PadConfigFlipTimer = 30;
                g_PadConfigFlipDirection = 0;
                g_NegconMappingIndex--;
                g_ControllerSceneAngleY += 2048;
            }
        } else if (g_PadMappingIndex > 0) {
            PlaySoundCue(8);
            g_PadConfigFlipTimer = 30;
            g_PadConfigFlipDirection = 0;
            g_PadMappingIndex--;
            g_ControllerSceneAngleY += 2048;
        }
    }
    if (g_PadPressed & PAD_RIGHT) {
        if (g_PadType == 0x23) {
            if (g_NegconMappingIndex < 7) {
                PlaySoundCue(8);
                g_PadConfigFlipDirection = 1;
                g_PadConfigFlipTimer = 30;
                g_NegconMappingIndex++;
                g_ControllerSceneAngleY -= 2048;
            }
        } else if (g_PadMappingIndex < 7) {
            PlaySoundCue(8);
            g_PadConfigFlipDirection = 1;
            g_PadConfigFlipTimer = 30;
            g_PadMappingIndex++;
            g_ControllerSceneAngleY -= 2048;
        }
    }
    if (g_PadConfigFlipTimer > 0) {
        g_PadConfigFlipTimer--;
        flipPhase = g_PadConfigFlipTimer;
        g_PadConfigFlipPhase = (flipPhase / 4) & 1;
    }
    g_ControllerSceneAngleY = (g_ControllerSceneAngleY * 15) / 16;
    DrawControllerConfigScreen();
    DrawOptionHintBar(1);
    DrawControllerSetupScene(0);
}


/* Game mode 9's own overlay: the two lines of instructions over a black panel
 * inside a white border, both drawn into the 0xD0 sub-buffer of the current
 * draw buffer from the shared scratchpad packet cursor. */
void DrawNegconNeutralScreen(void) {
    u8 **cursor = &SCRATCH_PRIM_CURSOR_AS(u8);
    u8 *ot;
    u8 *prim;

    DrawSpriteString(0x18, 0x30, g_MsgNegconUntwistedLine1, 0x7F81);
    DrawSpriteString(0x18, 0x48, g_MsgNegconUntwistedLine2, 0x7F81);
    ot = (u8 *)GamePrimaryOrderingTable(52);
    prim = *cursor;
    prim = AddTilePrim(ot, prim, 0, 0x28, 0x124, 0x40, 0, 0, 0);
    *cursor = AddTilePrim(ot, prim, 0, 0x26, 0x125, 0x44, 0xFF, 0xFF, 0xFF);
}

/*
 * The six live NeGcon settings. All of them are persisted to the memory card,
 * so they also appear as fields of GameSaveBlock (game/memcard.h).
 */

/*
 * Entry hook for the NeGcon calibration sequence: snapshots the six live
 * settings, clears the three the neutral/twist screens are about to measure
 * along with the screen counters, and enters game mode 9 (the "hold the
 * controller still" screen).
 *
 * The pins and the empty barrier only hold the reads ahead of the clears and
 * keep the last two snapshots in the registers retail used; the code itself is
 * ordinary C.
 */
void BeginNegconCalibration(void) {
    register u16 twist asm("$3");
    register u16 mode asm("$4");
    NegconCalibrationValue *neutral = &g_NegconNeutralI;
    u16 steer = g_NegconSteerNeutral;
    u16 neutral0 = *neutral;
    u16 neutral1 = g_NegconNeutralII;
    u16 neutral2 = g_NegconNeutralL;

    twist = g_NegconSteerPlay;
    mode = g_NegconMaxTwist;
    asm("");

    *neutral = 0;
    g_ControllerSceneAngleY = 0;
    g_ControllerSceneAngleX = 0;
    g_PadConfigFlipTimer = 0;
    g_PadConfigFlipPhase = 0;
    g_NegconSteerNeutral = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    g_GameMode = 9;
    g_NegconSteerNeutralSaved = steer;
    g_NegconNeutralISaved = neutral0;
    g_NegconNeutralIISaved = neutral1;
    g_NegconNeutralLSaved = neutral2;
    g_NegconSteerPlaySaved = twist;
    g_NegconMaxTwistSaved = mode;
}

/*
 * Game mode 9: hold the NeGcon still and press start. Start latches the four
 * axes as the neutral point and advances to mode 10 (the steering play
 * screen); unplugging the NeGcon drops straight back to mode 1.
 */
void UpdateNegconNeutralScreen(void) {
    g_AnimTimer++;
    if (g_PadPressed & PAD_START) {
        PlaySoundCue(2);
        g_GameMode = 10;
        g_NegconSteerNeutral = g_NegconAxisSteer - 128;
        g_NegconNeutralI = g_NegconAxisI;
        g_NegconNeutralII = g_NegconAxisII;
        g_NegconNeutralL = g_NegconAxisL;
    }
    if (g_PadType != 0x23) {
        g_GameMode = 1;
    }
    DrawNegconNeutralScreen();
    DrawOptionHintBar(4);
    DrawControllerSetupScene(0);
}
