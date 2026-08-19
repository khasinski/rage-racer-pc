#include "common.h"
#include "game/game_input.h"
#include "game/audio.h"
/*
 * UpdateTeamLogoScreen: sound/menu state machine. The redundant
 * `if (g_GameInput.pressed) { ... } else { ... }` around the state>0 block is a
 * deliberate no-op guard (both arms identical): its presence forces GCC 2.6.3
 * to rematerialize the literal 2 at both the direction ternary and the switch
 * case instead of CSE-ing it into a saved register, reproducing retail codegen.
 * Verified byte-exact; both arms are semantically identical so behaviour is
 * unchanged. This is the accepted resolution while the legal-only permuter
 * searches for a cleaner shape.
 */
#include "game/menu.h"
#include "game/menu_controller.h"
#include "game/menu_dialog_controller.h"
#include "game/save_internal.h"
#include "game/render.h"
#include "game/render_workspace.h"
#include "game/state.h"
#include "psyq/gpu.h"

typedef enum TeamLogoState {
  TEAM_LOGO_CLOSE_PAINT = -4,
  TEAM_LOGO_PAINT = -3,
  TEAM_LOGO_CONFIRM_SAMPLE = -2,
  TEAM_LOGO_SAMPLE_PROMPT = -1,
  TEAM_LOGO_ACTIVE = 0,
  TEAM_LOGO_TO_SAMPLES = 1,
  TEAM_LOGO_BACK = 2
} TeamLogoState;

void UpdateTeamLogoScreen(void)
{
  void *ot;
  s32 state;
  s32 sel;
  s32 cnt;
  int buttonHeight;
  MenuSession menu;
  MenuSessionCommands commands;
  ot = RENDER_OT_BASE;
  g_MenuAltLayout = 0;
  state = GameMenuBusy;
  if (state == TEAM_LOGO_ACTIVE)
  {
    RampTeamLogoCanvas(-13, -21);
    RunTimedDrawScript(&g_TeamLogoScreenScript2, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
    RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 0);
    DrawTeamLogoCanvas(1, -1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    RunTimedDrawScript(&g_TeamLogoScreenScript, &g_UiScriptProgress, 0);
    if ((RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) != 0) && (g_UiScriptProgress2 <= 0))
    {
      g_MenuHintButtonsVisible = 1;
      g_MenuOverlayPattern = -1;
      menu = (MenuSession){g_TeamLogoOption, 3, 0};
      commands = MenuSessionStepVertical(&menu, g_GameInput.pressed);
      g_TeamLogoOption = menu.selection;
      for (cnt = 0; cnt < commands.moveCount; cnt++) PlaySoundCue(1);
      if (commands.action == MENU_ACTION_CONFIRM)
      {
        sel = g_TeamLogoOption;
        if (sel == 0)
        {
          PlaySoundCue(2);
          GameMenuBusy = TEAM_LOGO_SAMPLE_PROMPT;
          g_MenuSubCursor = 0;
          g_UiScriptProgress2 = 0;
          g_TeamLogoSubPanelScript = &g_MenuDialogPanelUpperScript;
        }
        else
          if (sel == 1)
        {
          PlaySoundCue(2);
          ApplyDuckedSequenceAudio();
          GameMenuBusy = TEAM_LOGO_PAINT;
          g_TeamLogoPaintArmed = 0;
          g_UiScriptProgress2 = 0;
          g_TeamLogoSubPanelScript = &g_MenuRow1MarkerScript;
        }
        else
          if (sel == 2)
        {
          PlaySoundCue(3);
          GameMenuBusy = TEAM_LOGO_BACK;
          g_MenuOverlayPattern = sel;
        }
      }
      else
        if (commands.action == MENU_ACTION_CANCEL)
      {
        PlaySoundCue(3);
        GameMenuBusy = TEAM_LOGO_BACK;
        g_MenuOverlayPattern = 2;
      }
    }
  }
  else
    if (state < 0)
  {
    if (state == TEAM_LOGO_SAMPLE_PROMPT)
    {
      RunTimedDrawScript(&g_TeamLogoScreenScript2, &g_UiScriptProgress2, 0);
      RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
      if (RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 1) != 0)
      {
        MenuDialogInputResult dialog = MenuDialogHandleBinary(
            g_MenuSubCursor, 1, 0, g_GameInput.pressed);
        s32 soundIndex;

        if (dialog.confirmed)
        {
          if (g_MenuSubCursor != 0)
          {
            PlaySoundCue(2);
            GameMenuBusy = TEAM_LOGO_CONFIRM_SAMPLE;
            g_MenuConfirmTimer = 0x23;
          }
          else
          {
            PlaySoundCue(3);
            GameMenuBusy = TEAM_LOGO_ACTIVE;
          }
        }
        if (dialog.cancelled)
        {
          PlaySoundCue(3);
          GameMenuBusy = TEAM_LOGO_ACTIVE;
        }
        g_MenuSubCursor = dialog.value;
        for (soundIndex = 0; soundIndex < dialog.moveCount; soundIndex++)
          PlaySoundCue(1);
        DrawMenuCursorBox((g_MenuSubCursor != 0) ? (0xB8) : (0xDA), 0x44, 0x20, 0x20, 0);
        DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        buttonHeight = 0x20;
        GameDrawMenuButton(0xB8, 0x44, 0x20, buttonHeight, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
        GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x3A, 0x1E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
      }
      DrawTeamLogoCanvas(1, 0);
    }
    else
      if (state == TEAM_LOGO_CONFIRM_SAMPLE)
    {
      cnt = g_MenuConfirmTimer;
      if (cnt <= 0)
      {
        RunTimedDrawScript(&g_TeamLogoScreenScript2, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 0);
        if (g_UiScriptProgress2 <= 0)
        {
          GameMenuBusy = TEAM_LOGO_TO_SAMPLES;
          g_MenuOverlayPattern = 1;
        }
      }
      else
      {
        g_MenuConfirmTimer = cnt - 1;
        RunTimedDrawScript(&g_TeamLogoScreenScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 1);
        DrawMenuCursorBox((g_MenuSubCursor != 0) ? (0xB8) : (0xDA), 0x44, 0x20, 0x20, 1);
        DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
        GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x3A, 0x1E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
      }
      DrawTeamLogoCanvas(1, 0);
    }
    else
      if (state == TEAM_LOGO_PAINT)
    {
      RampTeamLogoCanvas(9, 0x15);
      if (RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 1) != 0)
      {
        if (g_GameInput.pressed & PAD_START)
        {
          PlaySoundCue(3);
          ApplyCurrentSequenceAudio();
          GameMenuBusy = TEAM_LOGO_CLOSE_PAINT;
        }
        UpdateTeamLogoCanvas();
      }
      if (g_UiScriptProgress2 >= 8)
      {
        g_MenuHintButtonsVisible = 0;
      }
      DrawTeamLogoCanvas(1, 1);
    }
    else
    {
      RampTeamLogoCanvas(-13, -21);
      RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, -1);
      DrawTeamLogoCanvas(1, -1);
      if (g_UiScriptProgress2 < 7)
      {
        g_MenuHintButtonsVisible = 1;
      }
      if (g_UiScriptProgress2 <= 0)
      {
        GameMenuBusy = TEAM_LOGO_ACTIVE;
      }
    }
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    RunTimedDrawScript(&g_TeamLogoScreenScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
  }
  else
  {
    if (g_GameInput.pressed)
    {
      MenuFlowFadeOut(MENU_SCREEN_TEAM_LOGO);
      DrawTeamLogoCanvas((state == TEAM_LOGO_BACK) ? (-1) : (1), 0);
      RunTimedDrawScript(&g_TeamLogoScreenScript, &g_UiScriptProgress, -1);
      RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
      DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    }
    else
    {
      MenuFlowFadeOut(MENU_SCREEN_TEAM_LOGO);
      DrawTeamLogoCanvas((state == TEAM_LOGO_BACK) ? (-1) : (1), 0);
      RunTimedDrawScript(&g_TeamLogoScreenScript, &g_UiScriptProgress, -1);
      RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
      DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    }
    if (g_UiScriptProgress <= 0)
    {
      switch (GameMenuBusy)
      {
        case TEAM_LOGO_TO_SAMPLES:
          MenuFlowOpen(MENU_SCREEN_LOGO_SAMPLE);
          DrawLogoSamplePanel(0, 0);
          break;

        case TEAM_LOGO_BACK:
          MenuFlowOpen(MENU_SCREEN_DESIGN_MODE);
          g_TeamLogoOption = 0;
          g_TeamLogoClut[0] = 0;
          LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
          break;

        default:
            break;

      }

      g_UiScriptProgress = 0;
      GameMenuBusy = TEAM_LOGO_ACTIVE;
    }
  }
}

s32 DrawLogoSampleScreen(s32 step) {
    s32 value;

    if (step == 0) {
        g_LogoSampleScreenFade = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_LogoSampleScreenFade;
        g_LogoSampleScreenFade = value;
        if (value >= MENU_FADE_COMPLETE) {
            g_LogoSampleScreenFade = MENU_FADE_MAX;
        }
    } else {
        value = step + g_LogoSampleScreenFade;
        g_LogoSampleScreenFade = value;
        if (value < 0) {
            g_LogoSampleScreenFade = 0;
        }
    }

    return g_LogoSampleScreenFade;
}
