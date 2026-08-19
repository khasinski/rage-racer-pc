#include "common.h"
#include "game/game_input.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/car_select_controller.h"
#include "game/asset_internal.h"
#include "game/menu.h"
#include "game/menu_controller.h"
#include "game/menu_controller.h"
#include "game/menu_dialog_controller.h"
#include "game/menu_scripts_internal.h"
#include "game/save_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_workspace.h"
#include "game/state.h"
#include "game/game_context.h"

typedef enum RankingScreenState {
    RANKING_CLOSE_COURSE_TABLE = -6,
    RANKING_COURSE_TABLE = -5,
    RANKING_CLOSE_LAP_TABLE = -4,
    RANKING_LAP_TABLE = -3,
    RANKING_CLOSE_MENU = -2,
    RANKING_MENU = -1,
    RANKING_ENTER = 0,
    RANKING_BACK = 1
} RankingScreenState;

void UpdateRankingScreen(void) {
    s32 state;

    g_MenuAltLayout = 0;
    DrawMenuCourseView();
    DrawMenuLightBurst(-9);
    state = GameMenuBusy;
    if (state == RANKING_ENTER) {
        g_UiScriptProgress2 = 0;
        GameMenuBusy = RANKING_MENU;
        DrawFadingMenuSprites(0, 2, g_RankingCursor);
        RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, 1);
    } else if (state < 0) {
        switch (state) {
        case RANKING_MENU:
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, 1) != 0) {
                MenuSession menu = {g_RankingCursor, 3, 0};
                MenuSessionCommands commands = MenuSessionStepVertical(
                    &menu, g_GameInput.pressed);
                s32 soundIndex;

                g_MenuOverlayPattern = -1;
                g_RankingCursor = menu.selection;
                for (soundIndex = 0; soundIndex < commands.moveCount;
                     soundIndex++) {
                    PlaySoundCue(1);
                }
                if (commands.action == MENU_ACTION_CONFIRM) {
                        s32 x = g_RankingCursor;
                        if (x == 0) {
                            PlaySoundCue(2);
                            GameMenuBusy = RANKING_CLOSE_MENU;
                            g_RankingPendingState = RANKING_LAP_TABLE;
                        } else if (x == 1) {
                            PlaySoundCue(2);
                            GameMenuBusy = RANKING_CLOSE_MENU;
                            g_RankingPendingState = RANKING_COURSE_TABLE;
                        } else if (x == 2) {
                            PlaySoundCue(3);
                            GameMenuBusy = RANKING_BACK;
                            g_MenuOverlayPattern = x;
                        }
                } else if (commands.action == MENU_ACTION_CANCEL) {
                    PlaySoundCue(3);
                    GameMenuBusy = RANKING_BACK;
                    g_MenuOverlayPattern = 2;
                }
            }
            break;
        case RANKING_CLOSE_MENU:
            RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, -1);
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = g_RankingPendingState;
            break;
        case RANKING_LAP_TABLE:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 0) == 0) {
                break;
            }
            if (!(g_GameInput.pressed & (PAD_START | PAD_SQUARE | PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = RANKING_CLOSE_LAP_TABLE;
            break;
        case RANKING_CLOSE_LAP_TABLE:
            DrawRankingTable(&g_UiScriptProgress2, -1, 0);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = RANKING_MENU;
            break;
        case RANKING_COURSE_TABLE:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 1) == 0) {
                break;
            }
            if (!(g_GameInput.pressed & (PAD_START | PAD_SQUARE | PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = RANKING_CLOSE_COURSE_TABLE;
            break;
        case RANKING_CLOSE_COURSE_TABLE:
            DrawRankingTable(&g_UiScriptProgress2, -1, 1);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = RANKING_MENU;
            break;
        }
    } else {
        goto pos;
    }
    RunTimedDrawScript(&g_RankingPanelScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
    return;
pos:
    MenuFlowFadeOut(MENU_SCREEN_RANKING);
    RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    RunTimedDrawScript(&g_RankingPanelScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    if (g_UiScriptProgress > 0) {
        return;
    }
    MenuFlowOpen(MENU_SCREEN_COURSE_SELECT);
    g_RankingCursor = 0;
    g_UiScriptProgress = 0;
    GameMenuBusy = RANKING_ENTER;
    DrawTimeAttackPlate(0);
    if (g_CourseIndex >= 4) {
        g_TimeAttackPlateStep = 1;
    } else {
        g_TimeAttackPlateStep = -1;
    }
}

s32 DrawCarSelectScreen(s32 step) {
    OT_TYPE *ot = RENDER_OT_BASE_AS(OT_TYPE);
    OrderingTableAddress otAddress;
    s32 p;
    u32 *buf = (u32 *)(ot + 1);
    s32 v;
    s32 col;
    s32 xpos;
    s32 mode;
    u8 tex;

    otAddress.pointer = ot;
    p = otAddress.value;

    if (step == 0) {
        g_CarSelectFadeAccum = 0;
        return p;
    }

    if (step > 0) {
        g_CarSelectFadeAccum += step;
        if (g_CarSelectFadeAccum >= 509) {
            g_CarSelectFadeAccum = 508;
        }
    } else {
        g_CarSelectFadeAccum += step;
        if (g_CarSelectFadeAccum < 0) {
            g_CarSelectFadeAccum = 0;
        }
    }

    v = g_CarSelectFadeAccum / 4U;
    col = v & 0xff;
    DrawRectOutline(buf, 0xa3, 0x180, 0x1a, 0x19, col, col, col, 0x20);

    tex = g_CarTable[g_PlayerCarIndex].transmission;
    if (tex != 0) {
        DrawSprite(buf, 0xad, 0x185, 0x10, 0x10, 0x6c, 0x7c, col, col, col,
                      0x244, 0, 1, 0x3b);
        xpos = 0xa5;
    } else {
        DrawSprite(buf, 0xae, 0x185, 0xc, 0x10, 0x60, 0x7c, col, col, col,
                      0x244, 0, 1, 0x3b);
        xpos = 0xa6;
    }

    mode = g_CarModelAsset->gearCount;
    switch (mode) {
    case 4:
        DrawSprite(buf, xpos, 0x185, 8, 0x10, 0x20, 0x18, v & 0xff, v & 0xff,
                      v & 0xff, 0x244, 0, 1, 0x3b);
        break;
    case 5:
        DrawSprite(buf, xpos, 0x185, 8, 0x10, 0x28, 0x18, v & 0xff, v & 0xff,
                      v & 0xff, 0x244, 0, 1, 0x3b);
        break;
    case 6:
        DrawSprite(buf, xpos, 0x185, 8, 0x10, 0x30, 0x18, v & 0xff, v & 0xff,
                      v & 0xff, 0x244, 0, 1, 0x3b);
        break;
    }

    return g_CarSelectFadeAccum;
}

void UpdateOwnedCarNeighbours(void) {
    s32 index;
    CarEntry *ptr;

    g_PrevOwnedCarIndex = -1;
    index = g_PlayerCarIndex - 1;
    if (index >= 0) {
        s32 one = 1;
        ptr = &g_CarTable[index];
        while (index >= 0) {
            if (ptr->enabled == one) {
                g_PrevOwnedCarIndex = index;
                break;
            }
            index--;
            ptr--;
        }
    }

    g_NextOwnedCarIndex = -1;
    index = g_PlayerCarIndex + 1;
    if (index < 13) {
        s32 one = 1;
        ptr = &g_CarTable[index];
        while (index < 13) {
            if (ptr->enabled == one) {
                g_NextOwnedCarIndex = index;
                break;
            }
            index++;
            ptr++;
        }
    }
}
void RefreshCarUnlockState(void) {
    s32 index;
    s32 value;
    CarEntry *ptr;
    CarEntry *enabledPtr;
    s32 byte;

    g_ShopCarIndex = -1;

    if (g_CarShopUnlockAll != 0) {
        index = 12;
        enabledPtr = &g_CarTable[12];
while (1) {
        byte = enabledPtr->enabled;
        enabledPtr--;
        if (byte == 0) {
            g_ShopCarIndex = index;
        }
        index--;
        if (index < 0) {
            return;
        }
        }
    }

    index = 12;
do {
    {
        value = GetCarUnlockLevel(index);
        ptr = &g_CarTable[index];
        if (ptr->enabled == 0) {
            if (g_RaceProgress->maxClassReached < 4) {
                if ((g_RaceProgress->maxClassReached + 1) < value) {
                    index--;
                    continue;
                }
            } else if (g_RaceProgress->maxClassReached < value) {
                index--;
                continue;
            }
            g_ShopCarIndex = index;
        }
        index--;
    }
    } while (index >= 0);

}

void EnterCarSelectScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    InstallCarModelSlot();
    g_MenuScreen = 4;
    g_UiScriptProgress = 0;
    UpdateOwnedCarNeighbours();
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    DrawMenuLightBurst(-9);
}


static void ApplyCarSelectCommand(CarSelectCommand command) {
    switch (command) {
    case CAR_SELECT_COMMAND_START_RACE: {
        u16 series;

        PlaySoundCue(2);
        StartSequenceFadeOut();
        if (g_GrandPrixMode != 0) {
            series = 0;
            if (g_GrandPrixClass < 5) series = (u16)g_GrandPrixSeries;
            g_GrandPrixSeries = series;
        } else {
            g_GrandPrixSeries = g_CourseIndex >> 2;
        }
        RequestRoundAssets();
        GameMenuBusy = CAR_SELECT_START_RACE;
        g_MenuHintBarStep = -1;
        g_CarNamePlateStep = -10;
        g_MenuOverlayPattern = 0;
        g_CarSpecGraphStep = -3;
        g_MenuViewOffsetTarget = 0x3D090;
        break;
    }
    case CAR_SELECT_COMMAND_CUSTOMIZE:
        PlaySoundCue(2);
        GameMenuBusy = CAR_SELECT_TO_CUSTOMIZE;
        g_MenuOverlayPattern = 1;
        g_CarNamePlateStep = -10;
        break;
    case CAR_SELECT_COMMAND_CAR_SHOP: {
        s32 previousAngle = g_MenuViewAngleTarget;

        PlaySoundCue(2);
        g_CarListCursor = g_ShopCarIndex;
        RequestCarModel(g_CarListCursor);
        g_MenuViewAngleTarget = 0x124F80;
        GameMenuBusy = CAR_SELECT_TO_CAR_SHOP;
        g_MenuOverlayPattern = 1;
        g_CarSwapFromIndex = g_PlayerCarIndex;
        g_CarSwapToIndex = g_CarListCursor;
        g_MenuViewAngle = 0x927C0 - (previousAngle - g_MenuViewAngle);
        break;
    }
    case CAR_SELECT_COMMAND_CAR_SHOP_UNAVAILABLE:
        PlaySoundCue(5);
        g_CarSelectPopupScript = (u8 *)&g_CarShopUnavailableScript;
        GameMenuBusy = CAR_SELECT_SHOP_UNAVAILABLE;
        g_UiScriptProgress2 = 0;
        break;
    case CAR_SELECT_COMMAND_ENGINEER_SHOP:
        GameMenuBusy = CAR_SELECT_TO_ENGINEER_SHOP;
        g_MenuOverlayPattern = 1;
        PlaySoundCue(2);
        break;
    case CAR_SELECT_COMMAND_ENGINEER_SHOP_UNAVAILABLE:
        PlaySoundCue(5);
        g_CarSelectPopupScript = (u8 *)&g_EngineerShopUnavailableScript;
        GameMenuBusy = CAR_SELECT_ENGINEER_UNAVAILABLE;
        g_UiScriptProgress2 = 0;
        break;
    case CAR_SELECT_COMMAND_BACK:
        PlaySoundCue(3);
        GameMenuBusy = CAR_SELECT_BACK;
        g_MenuOverlayPattern = 2;
        g_CarNamePlateStep = -10;
        g_CarSpecGraphStep = -3;
        g_MenuViewOffsetTarget = 0x3D090;
        break;
    case CAR_SELECT_COMMAND_NONE:
        break;
    }
}

static CarSelectScreenResult ReduceCurrentCarSelectInput(
    u16 pressed, s32 requiredClass) {
    CarSelectScreenState state;
    CarSelectScreenInput input;

    state.phase = (CarSelectState)GameMenuBusy;
    state.selection = g_CarSelectCursor;
    input.pressed = pressed;
    input.grandPrixMode = g_GrandPrixMode;
    input.shopCarIndex = g_ShopCarIndex;
    input.upgradesAvailable = g_CarModelAsset->upgradesAvailable;
    input.maxClassReached = g_RaceProgress->maxClassReached;
    input.requiredClass = requiredClass;
    return CarSelectReduceInput(&state, &input);
}

static void ApplyCarSelectScreenState(const CarSelectScreenState *state) {
    GameMenuBusy = state->phase;
    g_CarSelectCursor = state->selection;
}

void UpdateCarSelectScreen(void) {
    s32 mode;
    u8 *cmdList;
    s32 lowMode;
    s32 t;
    s32 u;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    mode = 2;
    DrawMenuCarView();
    DrawMenuLightBurst(-9);
    if (g_GrandPrixMode != 0) {
        mode = 4;
    }
    cmdList = (u8 *)&g_CarSelectMenuScriptTimeAttack;
    if (g_GrandPrixMode != 0) {
        cmdList = (u8 *)&g_CarSelectMenuScriptGp;
    }

    if (GameMenuBusy == CAR_SELECT_ACTIVE) {
        g_CarNamePlateStep = 0x14;
        g_CarSpecGraphStep = 3;
        g_MenuPlateCarIndex = g_PlayerCarIndex;
        RunTimedDrawScript(g_CarSelectPopupScript, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        DrawBrowseArrows(
            1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        {
            s32 initial;

            initial = -1;
            if (g_GrandPrixMode == 0) {
                DrawOwnedCarCounter(1, CountOwnedCars());
            }
            lowMode = mode & 0xFF;
            DrawFadingMenuSprites(g_UiScriptProgress, lowMode, g_CarSelectCursor);
            RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
            if ((RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) !=
                 0) &&
                (g_UiScriptProgress2 <= 0)) {
                CarSelectScreenResult input;
                s32 requiredClass = 0;
                s32 selectedCar;
                s32 soundIndex;
                u16 commandButtons = g_GameInput.pressed;

                g_MenuOverlayPattern = initial;
                UpdateOwnedCarNeighbours();
                RefreshCarUnlockState();
                if (g_CarModelAsset->upgradesAvailable != 0) {
                    requiredClass = GetCarUnlockLevel(g_PlayerCarIndex);
                }
                if ((u32)(g_MenuViewAngle - 0x2710) <= 0x120160U) {
                    commandButtons &= (u16)~PAD_CANCEL;
                }
                input = ReduceCurrentCarSelectInput(
                    commandButtons, requiredClass);
                ApplyCarSelectScreenState(&input.state);
                for (soundIndex = 0; soundIndex < input.moveCount; soundIndex++) {
                    PlaySoundCue(1);
                }
                selectedCar = g_PlayerCarIndex;
                if ((g_GameInput.held & PAD_LEFT) && (g_PrevOwnedCarIndex != -1)) {
                    t = g_MenuViewAngleTarget;
                    u = g_MenuViewAngle;
                    if (MenuViewIsSettled(u, t, 0x493DF)) {
                        if (g_CarSwapToIndex < 0) {
                            s32 prev;

                            PlaySoundCue(8);
                            g_PlayerCarIndex = g_PrevOwnedCarIndex;
                            RequestCarModel(g_PrevOwnedCarIndex);
                            prev = g_MenuViewAngleTarget;
                            g_CarSwapFromIndex = selectedCar;
                            g_MenuViewAngleTarget = 0;
                            g_MenuAltPanelStep2 = -1;
                            g_CarSwapToIndex = g_PlayerCarIndex;
                            g_MenuViewAngle =
                                (g_MenuViewAngle - prev) + 0x927C0;
                        }
                    }
                }
                if ((g_GameInput.held & PAD_RIGHT) && (g_NextOwnedCarIndex != -1)) {
                    t = g_MenuViewAngleTarget;
                    u = g_MenuViewAngle;
                    if (MenuViewIsSettled(u, t, 0x493DF)) {
                        if (g_CarSwapToIndex < 0) {
                            s32 base;
                            s32 prev;

                            PlaySoundCue(8);
                            g_PlayerCarIndex = g_NextOwnedCarIndex;
                            RequestCarModel(g_NextOwnedCarIndex);
                            base = 0x927C0;
                            prev = g_MenuViewAngleTarget;
                            g_MenuViewAngleTarget = 0x124F80;
                            g_CarSwapFromIndex = selectedCar;
                            g_MenuAltPanelStep2 = -1;
                            g_CarSwapToIndex = g_PlayerCarIndex;
                            g_MenuViewAngle =
                                base - (prev - g_MenuViewAngle);
                        }
                    }
                }
                t = g_MenuViewAngleTarget;
                u = g_MenuViewAngle;
                if (MenuViewIsSettled(u, t, 0x493DF)) {
                    if (g_CarSwapToIndex < 0) {
                        ApplyCarSelectCommand(input.command);
                        if (input.command != CAR_SELECT_COMMAND_NONE) return;
                    }
                }
            }
        }
        return;
    }

    if (GameMenuBusy < 0) {
        RunTimedDrawScript(g_CarSelectPopupScript, &g_UiScriptProgress2, 0);
        if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
            CarSelectScreenResult result = ReduceCurrentCarSelectInput(
                g_GameInput.pressed, 0);
            ApplyCarSelectScreenState(&result.state);
        }
        DrawBrowseArrows(
            1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        if (g_GrandPrixMode == 0) {
            DrawOwnedCarCounter(1, CountOwnedCars());
        }
        DrawFadingMenuSprites(g_UiScriptProgress, mode, g_CarSelectCursor);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    MenuFlowFadeOut(MENU_SCREEN_CAR_SELECT);
    DrawBrowseArrows(
        -1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
    if (g_GrandPrixMode == 0) {
        DrawOwnedCarCounter(-1, CountOwnedCars());
    }
    RunTimedDrawScript(cmdList, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, mode, g_CarSelectCursor);
    if (g_UiScriptProgress <= 0) {
        switch (GameMenuBusy) {
        case CAR_SELECT_START_RACE:
            if ((g_MenuOutgoingScreenProgress > 0) &&
                (g_MenuViewOffset <= 0x3D08F)) {
                return;
            }
            GameSceneSet(SCENE_ROUND_ENTER);
            g_CourseIndex &= 3;
            g_RaceProgress->course = g_CourseIndex;
            g_RaceProgress->carIndex = g_PlayerCarIndex;
            g_RaceProgress->classIndex = g_GrandPrixClass;
            if (g_GrandPrixMode != 0) {
                g_RaceProgress->money.value = g_PlayerMoney;
            } else {
                g_RaceProgress->money.value = g_GrandPrixSeries;
            }
            break;
        case CAR_SELECT_TO_CUSTOMIZE:
            MenuFlowOpen(MENU_SCREEN_CUSTOMIZE);
            break;
        case CAR_SELECT_TO_CAR_SHOP:
            MenuFlowOpen(MENU_SCREEN_CAR_SHOP);
            DrawCarShopPricePanel(0, 0, 0);
            DrawBrowseArrows(0, 0, 0, 0);
            DrawMenuAltPanel(0, 0);
            g_MenuAltPanelStep = 0;
            g_MenuAltPanelStep2 = 0;
            ClearTeamNameTexture();
            RestoreTeamLogoClut();
            break;
        case CAR_SELECT_TO_ENGINEER_SHOP:
            MenuFlowOpen(MENU_SCREEN_ENGINEER_SHOP);
            DrawEngineerShopPricePanel(0, 0, 0);
            break;
        case CAR_SELECT_BACK:
        {
            s32 angle;
            s32 offset;
            s32 largeValue;
            s32 course;

            if (g_MenuViewOffset <= 0x3D08F) {
                return;
            }
            angle = 0x7A120;
            offset = 0x3D090;
            largeValue = 0x1F4000;
            course = g_CourseIndex;
            g_MenuViewAngle = angle;
            g_MenuViewAngleTarget = angle;
            MenuFlowOpen(MENU_SCREEN_COURSE_SELECT);
            g_CarSelectCursor = 0;
            g_MenuPendingCourseIndex = -1;
            g_MenuViewOffset = offset;
            g_MenuViewOffsetTarget = 0;
            g_CourseCardSpin = largeValue;
            g_MenuCourseModelIndex = course;
            g_CourseCardPendingGrade = g_CourseProgress->bestPlace[course & 3];
            DrawTimeAttackPlate(0);
            if (g_CourseIndex >= 4) {
                g_TimeAttackPlateStep = 1;
            } else {
                g_TimeAttackPlateStep = -1;
            }
            break;
        }
        }
        g_UiScriptProgress = 0;
        GameMenuBusy = CAR_SELECT_ACTIVE;
    }
}

s32 DrawCustomizeScreen(s32 step) {
    s32 value;

    if (step == 0) {
        g_CustomizeFadeAccum = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_CustomizeFadeAccum;
        g_CustomizeFadeAccum = value;
        if (value >= 0x1FD) {
            g_CustomizeFadeAccum = 0x1FC;
        }
        value = 0;
    } else {
        s32 limit;
        u32 product;

        value = step + g_CustomizeFadeAccum;
        g_CustomizeFadeAccum = value;
        limit = 0x1FC;
        if (value < 0) {
            g_CustomizeFadeAccum = 0;
        }
        limit = limit - g_CustomizeFadeAccum;
        product = limit * limit;
        value = product / 2048;
    }

    DrawCarEngineSpec((s16)value, (g_CustomizeFadeAccum / 4U) & 0xFF);
    return g_CustomizeFadeAccum;
}


static void ApplyCustomizeCommand(CustomizeCommand command) {
    switch (command) {
    case CUSTOMIZE_COMMAND_TIRES:
        PlaySoundCue(2);
        g_CustomizePopupScript = (u8 *)&g_MenuDialogPanelUpperScript;
        GameMenuBusy = CUSTOMIZE_TIRE_PROMPT;
        g_UiScriptProgress2 = 0;
        g_MenuSubCursor = g_CarTable[g_PlayerCarIndex].tireCompound;
        break;
    case CUSTOMIZE_COMMAND_TRANSMISSION:
        PlaySoundCue(2);
        g_CustomizePopupScript = (u8 *)&g_MenuDialogPanelLowerScript;
        GameMenuBusy = CUSTOMIZE_TRANSMISSION_PROMPT;
        g_UiScriptProgress2 = 0;
        g_MenuSubCursor = g_CarTable[g_PlayerCarIndex].transmission;
        break;
    case CUSTOMIZE_COMMAND_TRANSMISSION_UNAVAILABLE:
        PlaySoundCue(5);
        g_CustomizePopupScript = (u8 *)&g_TransmissionUnavailableScript;
        GameMenuBusy = CUSTOMIZE_TRANSMISSION_UNAVAILABLE;
        g_UiScriptProgress2 = 0;
        break;
    case CUSTOMIZE_COMMAND_DESIGN:
        PlaySoundCue(2);
        GameMenuBusy = CUSTOMIZE_TO_DESIGN;
        g_MenuOverlayPattern = 1;
        g_CarSpecGraphStep = -3;
        g_MenuViewOffsetTarget = 0x3D090;
        break;
    case CUSTOMIZE_COMMAND_BACK:
        PlaySoundCue(3);
        GameMenuBusy = CUSTOMIZE_TO_CAR_SELECT;
        g_MenuOverlayPattern = 2;
        break;
    case CUSTOMIZE_COMMAND_NONE:
        break;
    }
}

static CustomizeScreenState CurrentCustomizeScreenState(void) {
    CustomizeScreenState state;

    state.phase = (CustomizeState)GameMenuBusy;
    state.selection = g_RankingOption;
    state.modalCursor = g_MenuSubCursor;
    state.confirmTimer = g_MenuConfirmTimer;
    return state;
}

static void ApplyCustomizeScreenState(const CustomizeScreenState *state) {
    GameMenuBusy = state->phase;
    g_RankingOption = state->selection;
    g_MenuSubCursor = state->modalCursor;
    g_MenuConfirmTimer = state->confirmTimer;
}

static CustomizeScreenResult ReduceCurrentCustomizeInput(void) {
    CustomizeScreenState state = CurrentCustomizeScreenState();
    CustomizeScreenInput input;

    input.pressed = g_GameInput.pressed;
    input.grandPrixMode = g_GrandPrixMode;
    input.transmissionAvailable = g_CarModelAsset->transmissionAvailable;
    return CustomizeReduceInput(&state, &input);
}

void UpdateCustomizeScreen(void) {
    void *ot;
    s32 mode;
    s32 lowMode;
    u8 *cmdList;

    ot = RENDER_OT_BASE_AS(void);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    mode = 2;
    DrawMenuCarView();
    if (g_GrandPrixMode != 0) {
        mode = 3;
    }
    cmdList = (u8 *)&g_CustomizeMenuScriptTimeAttack;
    if (g_GrandPrixMode != 0) {
        cmdList = (u8 *)&g_CustomizeMenuScriptGp;
    }

    if (GameMenuBusy == CUSTOMIZE_ACTIVE) {
        g_CarSpecGraphStep = 3;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
        lowMode = mode & 0xFF;
        DrawFadingMenuSprites(g_UiScriptProgress, lowMode, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        if ((RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) != 0) && (g_UiScriptProgress2 <= 0)) {
            CustomizeScreenResult input;
            s32 soundIndex;

            g_MenuOverlayPattern = -1;
            input = ReduceCurrentCustomizeInput();
            ApplyCustomizeScreenState(&input.state);
            for (soundIndex = 0; soundIndex < input.moveCount; soundIndex++) {
                PlaySoundCue(1);
            }
            ApplyCustomizeCommand(input.command);
            if (input.command != CUSTOMIZE_COMMAND_NONE) return;
        }
        return;
    }

    if (GameMenuBusy < 0) {
        if (GameMenuBusy == CUSTOMIZE_TIRE_PROMPT) {
            if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) != 0) {
                CustomizeScreenResult dialog = ReduceCurrentCustomizeInput();
                s32 soundIndex;

                if ((dialog.effects & CUSTOMIZE_EFFECT_ACCEPT) != 0) {
                    PlaySoundCue(2);
                }
                if ((dialog.effects & CUSTOMIZE_EFFECT_CANCEL) != 0) {
                    PlaySoundCue(3);
                }
                ApplyCustomizeScreenState(&dialog.state);
                for (soundIndex = 0;
                     soundIndex < dialog.moveCount; soundIndex++) {
                    PlaySoundCue(1);
                }
                DrawTireCompoundSlider(g_MenuSubCursor, 0);
            }
        } else if (GameMenuBusy == CUSTOMIZE_TRANSMISSION_PROMPT) {
            if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) != 0) {
                s32 previousCursor = g_MenuSubCursor;
                CustomizeScreenResult dialog = ReduceCurrentCustomizeInput();
                s32 soundIndex;

                if ((dialog.effects & CUSTOMIZE_EFFECT_ACCEPT) != 0) {
                    PlaySoundCue(2);
                }
                if ((dialog.effects & CUSTOMIZE_EFFECT_CANCEL) != 0) {
                    PlaySoundCue(3);
                }
                if ((dialog.effects & CUSTOMIZE_EFFECT_APPLY_TRANSMISSION) != 0) {
                    g_CarTable[g_PlayerCarIndex].transmission = previousCursor;
                    g_TimeAttackCarTransmissions[g_PlayerCarIndex * 8] = previousCursor;
                }
                ApplyCustomizeScreenState(&dialog.state);
                for (soundIndex = 0;
                     soundIndex < dialog.moveCount; soundIndex++) {
                    PlaySoundCue(1);
                }
                DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xDA : 0xB8, 0x68, 0x20, 0x20, 0);
                DrawSprite(ot, 0xC2, 0x70, 0xC, 0x10, 0x60, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                DrawSprite(ot, 0xE3, 0x70, 0x10, 0x10, 0x6C, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                GameDrawMenuButton(0xB8, 0x68, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                GameDrawMenuButton(0xDA, 0x68, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
            }
        } else if (GameMenuBusy == CUSTOMIZE_TRANSMISSION_UNAVAILABLE) {
            RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 0);
            if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                CustomizeScreenResult result = ReduceCurrentCustomizeInput();
                ApplyCustomizeScreenState(&result.state);
            }
        } else if (GameMenuBusy == CUSTOMIZE_CLOSE_UNAVAILABLE) {
            RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
            RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
            if (g_UiScriptProgress2 <= 0) {
                CustomizeScreenState current = CurrentCustomizeScreenState();
                CustomizeScreenState next = CustomizeFinishPopup(&current);
                ApplyCustomizeScreenState(&next);
            }
        } else if (GameMenuBusy == CUSTOMIZE_CONFIRM_TIRES) {
            if (g_MenuConfirmTimer <= 0) {
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
                if (g_UiScriptProgress2 <= 0) {
                    CustomizeScreenState current = CurrentCustomizeScreenState();
                    CustomizeScreenState next = CustomizeFinishPopup(&current);
                    ApplyCustomizeScreenState(&next);
                    g_CarTable[g_PlayerCarIndex].tireCompound = g_MenuSubCursor;
                    g_TimeAttackCarTires[g_PlayerCarIndex * 8] = g_MenuSubCursor;
                }
            } else {
                CustomizeScreenState current = CurrentCustomizeScreenState();
                CustomizeScreenState next = CustomizeTickConfirmTimer(&current);
                ApplyCustomizeScreenState(&next);
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1);
                DrawTireCompoundSlider(g_MenuSubCursor, 1);
            }
        } else if (GameMenuBusy == CUSTOMIZE_CONFIRM_TRANSMISSION) {
            if (g_MenuConfirmTimer <= 0) {
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
                if (g_UiScriptProgress2 <= 0) {
                    CustomizeScreenState current = CurrentCustomizeScreenState();
                    CustomizeScreenState next = CustomizeFinishPopup(&current);
                    ApplyCustomizeScreenState(&next);
                }
            } else {
                CustomizeScreenState current = CurrentCustomizeScreenState();
                CustomizeScreenState next = CustomizeTickConfirmTimer(&current);
                ApplyCustomizeScreenState(&next);
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1);
                DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xDA : 0xB8, 0x68, 0x20, 0x20, 1);
                DrawSprite(ot, 0xC2, 0x70, 0xC, 0x10, 0x60, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                DrawSprite(ot, 0xE3, 0x70, 0x10, 0x10, 0x6C, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                GameDrawMenuButton(0xB8, 0x68, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                GameDrawMenuButton(0xDA, 0x68, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
            }
        }
        DrawFadingMenuSprites(g_UiScriptProgress, mode, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    MenuFlowFadeOut(MENU_SCREEN_CUSTOMIZE);
    RunTimedDrawScript(cmdList, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, mode, g_RankingOption);
    if (g_UiScriptProgress <= 0) {
        switch (GameMenuBusy) {
        case CUSTOMIZE_TO_DESIGN:
            if (g_MenuViewOffset <= 0x3D08F) {
                return;
            }
            MenuFlowOpen(MENU_SCREEN_DESIGN_MODE);
            break;
        case CUSTOMIZE_TO_CAR_SELECT:
            MenuFlowOpen(MENU_SCREEN_CAR_SELECT);
            g_RankingOption = 0;
            break;
        }
        g_UiScriptProgress = 0;
        GameMenuBusy = CUSTOMIZE_ACTIVE;
    }
}
