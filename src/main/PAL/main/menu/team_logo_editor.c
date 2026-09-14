#include "game/menu_internal.h"
#include "game/menu.h"
#include "game/audio.h"
#include "game/state.h"

enum {
    TEAM_LOGO_FIRST_EDITABLE_COLOR = 1,
    TEAM_LOGO_EDITABLE_COLOR_COUNT = 15,
    TEAM_LOGO_LAST_EDITABLE_COLOR =
        TEAM_LOGO_FIRST_EDITABLE_COLOR + TEAM_LOGO_EDITABLE_COLOR_COUNT - 1,
    TEAM_LOGO_COLOR_CHANNEL_COUNT = 3,
    TEAM_LOGO_REPEAT_INITIAL_FRAME = 20,
    TEAM_LOGO_REPEAT_FRAME = 1,
    TEAM_LOGO_REPEAT_MAX_TIMER = 23,
};

static void PaintTeamLogoBrush(const TeamLogo *logo, u16 colour) {
    s32 row;
    s32 column;
    s32 x = logo->viewX + logo->cursorX;
    s32 y = logo->viewY + logo->cursorY;

    for (row = 0; row < logo->brushSize; row++) {
        for (column = 0; column < logo->brushSize; column++) {
            SetTeamLogoCanvasPixel(&g_TeamLogoCanvas, x + column, y + row,
                                   colour);
        }
    }
}

static void AdjustTeamLogoColour(const TeamLogo *logo, s32 step) {
    s32 shift = logo->colorChannel * 5;
    u16 mask = (u16)(0x1F << shift);
    u16 colour = g_TeamLogoClut[logo->penColor] | 0x8000;
    s32 component = (colour >> shift) & 0x1F;

    component = (component + step) & 0x1F;
    g_TeamLogoClut[logo->penColor] =
        (u16)((colour & ~mask) | (component << shift));
}

static s32 TeamLogoDpadRepeatsNow(const TeamLogo *logo) {
    return logo->repeatTimer == TEAM_LOGO_REPEAT_INITIAL_FRAME ||
           logo->repeatTimer == TEAM_LOGO_REPEAT_FRAME;
}

static void NormalizeTeamLogoEditorState(TeamLogo *logo) {
    if (logo->brushSize != 1 && logo->brushSize != 2 &&
        logo->brushSize != 4) {
        logo->brushSize = 1;
    }
    logo->penColor = AddClampedMenuValue(
        logo->penColor, 0, TEAM_LOGO_FIRST_EDITABLE_COLOR,
        TEAM_LOGO_LAST_EDITABLE_COLOR);
    logo->colorChannel = AddClampedMenuValue(
        logo->colorChannel, 0, 0, TEAM_LOGO_COLOR_CHANNEL_COUNT - 1);
    logo->viewX = AddClampedMenuValue(
        logo->viewX, 0, 0, TEAM_LOGO_EDITOR_VIEW_SIZE);
    logo->viewY = AddClampedMenuValue(
        logo->viewY, 0, 0, TEAM_LOGO_EDITOR_VIEW_SIZE);
    logo->cursorX = AddClampedMenuValue(
        logo->cursorX, 0, 0,
        TEAM_LOGO_EDITOR_VIEW_SIZE - logo->brushSize);
    logo->cursorY = AddClampedMenuValue(
        logo->cursorY, 0, 0,
        TEAM_LOGO_EDITOR_VIEW_SIZE - logo->brushSize);
    logo->repeatTimer = AddClampedMenuValue(
        logo->repeatTimer, 0, 0, TEAM_LOGO_REPEAT_MAX_TIMER);
    logo->repeatMask &= PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT;
    logo->expertMode = logo->expertMode != 0;
    logo->guideMode = AddClampedMenuValue(
        logo->guideMode, 0, 0, 2);
    logo->previousGuideMode = AddClampedMenuValue(
        logo->previousGuideMode, 0, 0, 2);
    logo->paintArmed = logo->paintArmed != 0;
    logo->paletteMode = logo->paletteMode != 0;
}

/* Mixing a colour: the cursor walks the editable palette slots and the three
 * five-bit channels of the selected colour. */
static void EditLogoPalette(TeamLogo *logo) {
    u16 pressed = g_PadPressed;
    u16 held = g_PadHeld;

    if (pressed & (PAD_CROSS | PAD_CIRCLE)) {
        PlaySoundCue(2);
        logo->paletteMode = 0;
        logo->paintArmed = 0;
    }
    if ((pressed & PAD_SELECT) &&
        ((held & (PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1)) ==
         (PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1))) {
        logo->expertMode = logo->expertMode == 0;
        logo->guideMode = logo->previousGuideMode;
    }

    if (TeamLogoDpadRepeatsNow(logo)) {
        if (held & PAD_LEFT) {
            PlaySoundCue(1);
            logo->penColor =
                WrapMenuIndex(logo->penColor -
                                  TEAM_LOGO_FIRST_EDITABLE_COLOR,
                              -1, TEAM_LOGO_EDITABLE_COLOR_COUNT) +
                TEAM_LOGO_FIRST_EDITABLE_COLOR;
        }
        if (held & PAD_RIGHT) {
            PlaySoundCue(1);
            logo->penColor =
                WrapMenuIndex(logo->penColor -
                                  TEAM_LOGO_FIRST_EDITABLE_COLOR,
                              1, TEAM_LOGO_EDITABLE_COLOR_COUNT) +
                TEAM_LOGO_FIRST_EDITABLE_COLOR;
        }
    }

    if (logo->expertMode == 0) return;
    if (held & (PAD_R1 | PAD_R2)) {
        if (g_PadPressedRepeat & PAD_UP) {
            PlaySoundCue(4);
            AdjustTeamLogoColour(logo, -1);
        }
        if (g_PadPressedRepeat & PAD_DOWN) {
            PlaySoundCue(4);
            AdjustTeamLogoColour(logo, 1);
        }
        return;
    }

    if (pressed & PAD_UP) {
        PlaySoundCue(1);
        logo->colorChannel = WrapMenuIndex(
            logo->colorChannel, -1, TEAM_LOGO_COLOR_CHANNEL_COUNT);
    }
    if (pressed & PAD_DOWN) {
        PlaySoundCue(1);
        logo->colorChannel = WrapMenuIndex(
            logo->colorChannel, 1, TEAM_LOGO_COLOR_CHANNEL_COUNT);
    }
}

/*
 * Drawing on the canvas: the pad moves the pen, one button lays the
 * brush down and the other rubs it out, both at whatever size the brush
 * is set to. Every plot is a nibble inside a canvas word.
 */
static void EditLogoCanvas(TeamLogo *logo) {
    s32 movedHorizontally;
    u16 sampledColour;
    s32 movedVertically;
    u16 pressed = g_PadPressed;
    u16 held = g_PadHeld;

    if ((held & PAD_CIRCLE) && (logo->paintArmed != 0)) {
        if (pressed & PAD_CIRCLE) {
            PlaySoundCue(4);
        }
        PaintTeamLogoBrush(logo, (u16)logo->penColor);
    }
    if (held & PAD_SQUARE) {
        if (pressed & PAD_SQUARE) {
            PlaySoundCue(4);
        }
        PaintTeamLogoBrush(logo, 0);
    }
    if (pressed & PAD_CROSS) {
        PlaySoundCue(2);
        logo->paletteMode = 1;
    }
    if (pressed & PAD_TRIANGLE) {
        PlaySoundCue(2);
        switch (logo->brushSize) {
        case 1:
            logo->brushSize = 2;
            break;
        case 2:
            logo->brushSize = 4;
            break;
        case 4:
            logo->brushSize = 1;
            break;
        }
        if ((logo->cursorX + logo->brushSize) >=
            TEAM_LOGO_EDITOR_VIEW_SIZE) {
            logo->cursorX =
                TEAM_LOGO_EDITOR_VIEW_SIZE - logo->brushSize;
        }
        if ((logo->cursorY + logo->brushSize) >=
            TEAM_LOGO_EDITOR_VIEW_SIZE) {
            logo->cursorY =
                TEAM_LOGO_EDITOR_VIEW_SIZE - logo->brushSize;
        }
    }
    if ((held & PAD_R1) && (logo->expertMode != 0)) {
        if (held & PAD_L1) {
            if (pressed & PAD_UP) {
                RotateTeamLogoCw();
            }
            if (pressed & PAD_DOWN) {
                FlipTeamLogoVertical();
            }
            if (pressed & PAD_LEFT) {
                RotateTeamLogoCcw();
            }
            if (pressed & PAD_RIGHT) {
                FlipTeamLogoHorizontal();
            }
        } else if (TeamLogoDpadRepeatsNow(logo)) {
            if (held & PAD_UP) {
                ScrollTeamLogoUp();
            }
            if (held & PAD_DOWN) {
                ScrollTeamLogoDown();
            }
            if (held & PAD_LEFT) {
                ScrollTeamLogoLeft();
            }
            if (held & PAD_RIGHT) {
                ScrollTeamLogoRight();
            }
        }
    } else {
        movedHorizontally = 0;
        if (TeamLogoDpadRepeatsNow(logo) || (held & (PAD_L2 | PAD_L1))) {
            movedVertically = 0;
            if (held & PAD_UP) {
                if (logo->cursorY > 0) {
                    logo->cursorY -= 1;
                    movedVertically = 1;
                } else if (logo->viewY > 0) {
                    logo->viewY -= 1;
                    movedVertically = 1;
                }
            }
            if (held & PAD_DOWN) {
                if ((logo->cursorY + logo->brushSize) <
                    TEAM_LOGO_EDITOR_VIEW_SIZE) {
                    logo->cursorY += 1;
                    movedVertically = 1;
                } else if (logo->viewY < TEAM_LOGO_EDITOR_VIEW_SIZE) {
                    logo->viewY += 1;
                    movedVertically = 1;
                }
            }
            if (held & PAD_LEFT) {
                if (logo->cursorX > 0) {
                    logo->cursorX -= 1;
                    movedHorizontally = 1;
                } else if (logo->viewX > 0) {
                    logo->viewX -= 1;
                    movedHorizontally = 1;
                }
            }
            if (held & PAD_RIGHT) {
                if ((logo->cursorX + logo->brushSize) <
                    TEAM_LOGO_EDITOR_VIEW_SIZE) {
                    logo->cursorX += 1;
                    movedHorizontally = 1;
                } else if (logo->viewX < TEAM_LOGO_EDITOR_VIEW_SIZE) {
                    logo->viewX += 1;
                    movedHorizontally = 1;
                }
            }
            if ((held & (PAD_SQUARE | PAD_CIRCLE)) &&
                (movedHorizontally || movedVertically)) {
                PlaySoundCue(4);
            }
        }
    }
    if ((pressed & PAD_R2) && (logo->expertMode != 0)) {
        PlaySoundCue(4);
        sampledColour = (u16)GetTeamLogoCanvasPixel(
            &g_TeamLogoCanvas, logo->viewX + logo->cursorX,
            logo->viewY + logo->cursorY);
        if (sampledColour == 0) {
            sampledColour = logo->penColor;
        }
        logo->penColor = sampledColour;
    }
}

void UpdateTeamLogoCanvas(TeamLogo *logo) {
    u16 held = g_PadHeld;
    s32 repeatDelay = (held & (PAD_L2 | PAD_L1)) ? 0 : 3;

    NormalizeTeamLogoEditorState(logo);
    if (held & logo->repeatMask) {
        if (logo->repeatTimer <
            TEAM_LOGO_REPEAT_INITIAL_FRAME + repeatDelay) {
            logo->repeatTimer++;
        }
    } else {
        logo->repeatTimer = 0;
    }

    logo->repeatMask = held &
        (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (!(held & PAD_CIRCLE)) {
        logo->paintArmed = 1;
    }

    if (logo->expertMode != 0) {
        if (g_PadPressed & PAD_SELECT) {
            logo->previousGuideMode = logo->guideMode;
            logo->guideMode = logo->guideMode < 2
                                      ? logo->guideMode + 1
                                      : 0;
        }
    } else {
        logo->guideMode = 1;
    }

    if (logo->paletteMode == 1) {
        EditLogoPalette(logo);
    } else {
        EditLogoCanvas(logo);
    }
}
