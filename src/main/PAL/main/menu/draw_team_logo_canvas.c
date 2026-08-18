#include "common.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render.h"
#include "game/render_types.h"
#include "game/render_workspace.h"
#include "psyq/gpu.h"

void DrawTeamLogoCanvasFade(s32 delta) {
    u8 *scratch;
    s32 value;
    s32 sum;
    s32 limit;
    s32 alpha;

    scratch = RENDER_OT_BASE_AS(u8);
    if (delta > 0) {
        value = g_MenuCurtainFade;
        sum = delta + value;
        value = sum;
        g_MenuCurtainFade = value;
        if (0xFFFF < value) {
            g_MenuCurtainFade = 0xFFFF;
        }
    } else {
        value = g_MenuCurtainFade;
        sum = delta + value;
        value = sum;
        g_MenuCurtainFade = value;
        if (value < 0) {
            g_MenuCurtainFade = 0;
        }
    }

    limit = 0x1E0;
    g_MenuCurtainShade = g_MenuCurtainFade >> 8;
    alpha = g_MenuCurtainShade;
    DrawSolidRect(scratch + 0x18, 0x48, 0, 0xF8, limit, alpha, alpha, alpha, 0x40);
}

void DrawTeamLogoCanvas(s32 panelStep, s32 editorStep)
{
  s32 kreg;
  s32 a0v;
  s32 a1v;
  RenderBufferAddress ot;
  s32 i;
  s32 d;
  s32 v;
  s32 ang;
  s32 gx;
  s32 gy;
  s32 gx2;
  s32 gy2;
  s32 pal;
  u8 clut;
  u16 x0;
  u16 x1;
  u16 y1;
  s32 x2;
  s32 yA0;
  s16 yA8;
  s16 yB8;
  s16 yC8;
  register s32 phaseValue asm("$2");
  register s32 secondaryValue asm("$3");
  register RenderBufferAddress drawValue asm("$4");
  a0v = panelStep;
  a1v = editorStep;
  ot.pointer = RENDER_OT_BASE_AS(void);
  if (panelStep == 0)
  {
    g_TeamLogoPanelStep = 0;
    g_TeamLogoEditorStep = 0;
    return;
  }
  g_TeamLogoClut[0] = 0x8000;
  g_TeamLogoClut[0] |= ((rsin(g_TeamLogoColorCycleAngle % 0x1000) / 128) + 0x20) >> 3;
  ang = g_TeamLogoColorCycleAngle + 0x55;
  g_TeamLogoClut[0] |= (((rsin(ang % 0x1000) / 128) + 0x20) >> 3) << 5;
  ang = g_TeamLogoColorCycleAngle + 0xAA;
  d = rsin(ang % 0x1000);
  i = 0;
  if (d < 0)
  {
    d += 0x7F;
  }
  {
    u16 *dst;
    s32 mul;
    u16 k8;
    k8 = 0x8000;
    g_TeamLogoClut[0] |= (((d >> 7) + 0x20) >> 3) << 10;
    g_TeamLogoFadedClut[0] = g_TeamLogoClut[0];
    dst = g_TeamLogoFadedClut;
    g_TeamLogoColorCycleAngle += 0x20;
    mul = g_TeamLogoFadeLevel;
    loop16:
    {
      u16 *src = &g_TeamLogoClut[i];
      *dst = k8;
      phaseValue = (((*src) & 0x1F) * mul) / 256;
      drawValue.value = phaseValue | ((u16) 0x8000);
      *dst = drawValue.value;
      drawValue.value |= (((((*src) >> 5) & 0x1F) * mul) / 256) << 5;
      *dst = drawValue.value;
      v = (((*src) >> 10) & 0x1F) * mul;
      if (v < 0)
      {
        v += 0xFF;
      }
      i++;
      phaseValue = drawValue.value | ((v >> 8) << 10);
      *dst = phaseValue;
      dst++;
      if (i < 16)
      {
        goto loop16;
      }
    }

  }
  LoadImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
  LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
  LoadImage(&g_TeamLogoFadedClutRect, g_TeamLogoFadedClut);
  if (a0v < 0)
  {
    g_TeamLogoPanelStep = a0v + g_TeamLogoPanelStep;
    if (g_TeamLogoPanelStep < 0)
    {
      g_TeamLogoPanelStep = 0;
    }
  }
  if (a1v < 0)
  {
    g_TeamLogoEditorStep = a1v + g_TeamLogoEditorStep;
    if (g_TeamLogoEditorStep < 0)
    {
      g_TeamLogoEditorStep = 0;
    }
  }
  d = g_TeamLogoPanelStep - 0xA;
  if (d >= 0)
  {
    s32 sy2;
    s32 sy;
    register s32 drawX asm("$5");
    register s32 sy2Arg;
    u8 ff;
    s32 sx;
    s32 x88;
    s32 w1;
    s32 delta;
    s32 scaleDelta;
    s32 texY;
    s32 gyTemp;
    u32 slidePhase;
    if (d >= 0xC)
    {
      d = 0xB;
    }
    x0 = 0x87;
    drawX = 0x87;
    asm("" : : "r"(drawX));
    slidePhase = d;
    sy = ((slidePhase * 0x460) >> 5) + 0xFEC9;
    ff = 0xFF;
    DrawRectOutline(ot.pointer, (s16)drawX, (s16)sy, (s16)0x82, 0x104, (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)ff);
    kreg = sy;
    if (g_TeamLogoZoomLevel >= 0x100)
    {
      if (g_TeamLogoPaletteMode == 0)
      {
        s32 syOffset;
        register s32 angleSource;
        register s32 angleValue;
        angleSource = g_TeamLogoColorCycleAngle;
        secondaryValue = g_TeamLogoCursorX;
        angleValue = angleSource * 2;
        drawValue.value = angleValue;
        secondaryValue *= 4;
        sy2 = secondaryValue + 0x88;
        sy2Arg = sy2;
        syOffset = (g_TeamLogoCursorY * 8) + 2;
        asm("" : "=r"(sy) : "0"(sy));
        sy += syOffset;
        if (angleValue < 0)
        {
          drawValue.value = angleValue + 0xFFF;
        }
        drawValue.value >>= 12;
        drawValue.value *= 0x1000;
        drawValue.value = angleValue - drawValue.value;
        clut = (rsin(drawValue.value) / 64) - 0x41;
        DrawRectOutline(ot.pointer, (s16)sy2Arg, (s16)sy, (s16)(g_TeamLogoBrushSize * 4), (s16)(g_TeamLogoBrushSize * 8), 0, (u8)clut, 0, (u8)ff);
      }
    }
    sx = (s16) kreg;
    x2 = ((s16) x0) - (g_TeamLogoZoomSpan < 0x220);
    if (g_TeamLogoZoomSpan < 0x220)
    {
      sx -= 2;
    }
    w1 = sx + 0x110;
    x88 = x2 + 0x88;
    delta = 0x220 - g_TeamLogoZoomSpan;
    scaleDelta = (delta * g_TeamLogoViewX) / 272;
    phaseValue = (g_TeamLogoRect.coordinate.x.value * 4) - 1;
    drawValue.value = phaseValue + scaleDelta;
    gx = drawValue.value;
    scaleDelta = (delta * g_TeamLogoViewY) / 272;
    texY = g_TeamLogoRect.coordinate.y.byte.low - 1;
    gyTemp = texY + scaleDelta;
    phaseValue = gyTemp;
    gy = phaseValue;
    gx2 = drawValue.value + (g_TeamLogoZoomSpan / 8);
    asm("" : : "r"(scaleDelta));
    gy2 = phaseValue + (g_TeamLogoZoomSpan / 8);
    clut = (g_TeamLogoRect.coordinate.y.value >> 4) & 0x10;
    clut |= (g_TeamLogoRect.coordinate.x.value & 0x3FF) >> 6;
    SetDrawClipRect(ot.pointer, (s16)0, (s16)0, (s16)0x140, (s16)0x1E0);
    GameDrawTexturedQuad(ot.pointer, (s16)x2, (s16)sx, (s16)x88, (s16)sx,
                         (s16)x2, (s16)w1, (s16)x88, (s16)w1,
                         (u8)gx, (u8)gy, (u8)gx2, (u8)gy,
                         (u8)gx, (u8)gy2, (u8)gx2, (u8)gy2,
                         (u8)0x7F, (u8)0x7F, (u8)0x7F, 0x27F, 1, 0, clut);
    SetDrawClipRect(ot.pointer, (s16)(x0 + 1), (s16)(kreg + 2), (s16)0x80, (s16)0x100);
  }
  d = g_TeamLogoPanelStep - 0xE;
  if (d >= 0)
  {
    register s32 sy asm("$16");
    u32 su;
    s32 ff;
    s16 w1;
    s16 xb;
    u32 slidePhase;
    if (d >= 8)
    {
      d = 7;
    }
    slidePhase = d;
    su = (slidePhase * -0x460) >> 5;
    sy = su + 0x1FB;
    x0 = 0x2F;
    ff = 0xFF;
    DrawRectOutline(ot.pointer, (s16)0x2F, (s16)sy, (s16)0x42, 0x84, (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)ff);
    kreg = sy;
    if ((g_TeamLogoZoomLevel >= 0x100) && (g_TeamLogoGuideMode != 0))
    {
      x1 = (u16)g_TeamLogoViewX + 0x30;
      y1 = sy + ((g_TeamLogoViewY * 2) + 2);
      clut = (rsin((g_TeamLogoColorCycleAngle * 2) % 0x1000) / 64) - 0x41;
      if (g_TeamLogoGuideMode == 2)
      {
        s16 ya;
        s16 yb;
        s16 xa;
        ya = su + 0x1FD;
        yb = su + 0x27D;
        xa = x1 + (u16)g_TeamLogoCursorX;
        DrawLine(ot.pointer, (s16)xa, (s16)ya, (s16)xa, (s16)yb, (u8)clut, (u8)clut, (u8)clut, (u8)ff);
        xa = ((x1 + (u16)g_TeamLogoCursorX) + ((u16) g_TeamLogoBrushSize)) - 1;
        DrawLine(ot.pointer, (s16)xa, (s16)ya, (s16)xa, (s16)yb, (u8)clut, (u8)clut, (u8)clut, (u8)ff);
        xa = y1 + (g_TeamLogoCursorY * 2);
        DrawLine(ot.pointer, (s16)0x30, (s16)xa, (s16)0x70, (s16)xa, (u8)clut, (u8)clut, (u8)clut, (u8)ff);
        {
          s32 odd;
          odd = g_TeamLogoCursorY * 2 + 1;
          xa = y1 + odd;
        }
        DrawLine(ot.pointer, (s16)0x30, (s16)xa, (s16)0x70, (s16)xa, (u8)clut, (u8)clut, (u8)clut, (u8)ff);
        xa = y1 + (((g_TeamLogoCursorY + g_TeamLogoBrushSize) - 1) * 2);
        DrawLine(ot.pointer, (s16)0x30, (s16)xa, (s16)0x70, (s16)xa, (u8)clut, (u8)clut, (u8)clut, (u8)ff);
        {
          s32 odd;
          odd = ((g_TeamLogoCursorY + g_TeamLogoBrushSize) - 1) * 2 + 1;
          xa = y1 + odd;
        }
        DrawLine(ot.pointer, (s16)0x30, (s16)xa, (s16)0x70, (s16)xa, (u8)clut, (u8)clut, (u8)clut, (u8)ff);
      }
      else
        if (g_TeamLogoBrushSize == 1)
      {
        s16 xa = x1 + (u16)g_TeamLogoCursorX;
        s16 ya = y1 + (g_TeamLogoCursorY * 2);
        DrawLine(ot.pointer, (s16)xa, (s16)ya, (s16)xa, (s16)(ya + 1), (u8)clut, (u8)clut, (u8)clut, (u8)ff);
      }
      else
      {
        DrawRectOutline(ot.pointer, (s16)(x1 + (u16)g_TeamLogoCursorX), (s16)(y1 + g_TeamLogoCursorY * 2), (s16)g_TeamLogoBrushSize, (s16)(g_TeamLogoBrushSize * 2), (u8)clut, (u8)clut, (u8)clut, (u8)0xFF);
      }
      DrawRectOutline(ot.pointer, (s16)x1, (s16)y1, (s16)0x20, 0x40, 0, (u8)clut, 0, (u8)0xFF);
    }
    gx = (g_TeamLogoRect.coordinate.x.value * 4) - 1;
    gy = g_TeamLogoRect.coordinate.y.byte.low - 1;
    gx2 = gx;
    gx2 += 0x41;
    gy2 = gy;
    gy2 += 0x41;
    pal = GetClut(g_TeamLogoClutRect.x, g_TeamLogoClutRect.y);
    clut = (g_TeamLogoRect.coordinate.y.value >> 4) & 0x10;
    clut |= (g_TeamLogoRect.coordinate.x.value & 0x3FF) >> 6;
    w1 = kreg + 0x83;
    xb = x0 + 0x41;
    SetDrawClipRect(ot.pointer, (s16)0, (s16)0, (s16)0x140, (s16)0x1E0);
    asm("" : "=r"(gx2) : "0"(gx2));
    GameDrawTexturedQuad(ot.pointer, (s16)x0, (s16)kreg, (s16)xb, (s16)kreg,
                         (s16)x0, (s16)w1, (s16)xb, (s16)w1,
                         (u8)gx, (u8)gy, (u8)gx2, (u8)gy,
                         (u8)gx, (u8)gy2, (u8)gx2, (u8)gy2,
                         (u8)0x7F, (u8)0x7F, (u8)0x7F,
                         pal & 0xFFFF, 1, 0, clut);
    SetDrawClipRect(ot.pointer, (s16)(x0 + 1), (s16)(kreg + 2), (s16)0x40, (s16)0x80);
  }
  d = g_TeamLogoEditorStep - 8;
  if (d >= 0)
  {
    s32 j;
    register s32 vs7 asm("$23");
    register s32 vs6 asm("$22");
    u32 panelY;
    u32 slidePhase;
    if (d >= 6)
    {
      d = 5;
    }
    x0 = 0x8A;
    slidePhase = d;
    panelY = (slidePhase * -0x3C0) >> 5;
    kreg = panelY + 0x1EA;
    y1 = panelY + 0x1E7;
    x1 = (g_TeamLogoPenColor * 8) + 0x80;
    if (g_TeamLogoPaletteMode == 1)
    {
      s32 panelAng;
      panelAng = g_TeamLogoColorCycleAngle * 2;
      clut = (rsin(panelAng % 0x1000) / 64) - 0x41;
      DrawRectOutline(ot.pointer, (s16)x1, (s16)y1, (s16)0xD, 0x1A, 0, (u8)clut, 0, (u8)0xFF);
    }
    else
    {
      DrawRectOutline(ot.pointer, (s16)x1, (s16)y1, (s16)0xD, 0x1A, (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    }
    DrawSolidRect(ot.pointer, (s16)(x1 + 1), (s16)(y1 + 2), (s16)0xB, (s16)0x16, (u8)((g_TeamLogoClut[g_TeamLogoPenColor] & 0xFF) * 8), (u8)((g_TeamLogoClut[g_TeamLogoPenColor] >> 2) & 0xF8), (u8)((g_TeamLogoClut[g_TeamLogoPenColor] >> 7) & 0xF8), (u8)0xFF);
    {
      s32 fy2 = (kreg + 2) << 16;
      i = 0;
      j = 1;
      loop15:
      DrawSolidRect(ot.pointer, (s16)(x0 + j), (s16)(fy2 >> 16), (s16)8, (s16)0x10, (u8)((g_TeamLogoSwatches[i] & 0xFF) * 8), (u8)((g_TeamLogoSwatches[i] >> 2) & 0xF8), (u8)((g_TeamLogoSwatches[i] >> 7) & 0xF8), (u8)0xFF);

      i++;
      j += 8;
      if (i < 15)
      {
        goto loop15;
      }
    }
    {
      DrawRectOutline(ot.pointer, (s16)x0, (s16)kreg, (s16)0x7A, 0x14, (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
      y1 = kreg + 0x1C;
    i = 0;
    yA0 = (kreg + 0x22) << 16;
    asm("" : : "r"(yA0));
    kreg = 0x60;
    vs7 = -0x30;
    vs6 = -0xF;
    for (; i < 4; i++)
    {
      register s32 shade asm("$21");
      register s32 swatchWidth asm("$7");
      s32 gxArg;
      s32 gyArg;
      s32 clutArg;

      x1 = x0 + vs6;
      if (g_PadType == 0x23)
      {
        gx = vs7;
        gy = 0;
        pal = 0x233;
        clut = 0x1E;
      }
      else
      {
        gx = kreg;
        gy = 0x58;
        pal = 0x1F6;
        clut = 0x1C;
      }
      gxArg = (u8) gx;
      gyArg = (u8) gy;
      asm("" : : "r"(gxArg), "r"(gyArg));
      asm("" : : "r"(pal));
      swatchWidth = 0xC;
      asm("" : : "r"(swatchWidth));
      shade = (u8) (i * 0x24);
      asm("" : : "r"(shade));
      drawValue.pointer = ot.pointer;
      asm("" : : "r"(drawValue.pointer));
      asm("" : : "r"(x1));
      clutArg = clut;
      DrawSprite(drawValue.pointer, (s16)(x1 + 0x13), (s16)(yA0 >> 16), (s16)swatchWidth, (s16)0x18,
                            gxArg, gyArg, 0, 0, 0, (u16) pal, 1, 0, clutArg);
      asm("" : "=r"(shade) : "0"(shade));
      secondaryValue = (u8) shade;
      DrawSprite(ot.pointer, (s16)x1, (s16)y1, (s16)0x22, (s16)0x32, secondaryValue, (u8)0xC0,
                    0, 0, 0, 0x1F5, 1, 0, 0x1D);
      kreg += 0xC;
      vs7 += 0xC;
      vs6 += 0x28;
      }
    }
  }
  d = g_TeamLogoEditorStep - 7;
  if (d >= 0)
  {
    u32 slidePhase;

    if (d >= 7)
    {
      d = 6;
    }
    slidePhase = d;
    DrawSprite(ot.pointer, (s16)(((slidePhase * 0x250) >> 5) + 0xFFA1), (s16)0xC0, (s16)0x61, (s16)0x32,
               (u8)0x90, (u8)0xC0, 0, 0, 0, 0x1F5, 1, 0, 0x1D);
  }
  d = g_TeamLogoEditorStep - 8;
  if ((d >= 0) && (g_TeamLogoExpertMode != 0))
  {
    s16 sy;
    s16 sx;
    s16 xa;
    s16 xb;
    s16 xc;
    s32 x0Calc;
    s32 syBase;
    register s32 tileSize;
    u32 slidePhase;
    kreg = 0xC8;
    if (d >= 6)
    {
      d = 5;
    }
    slidePhase = d;
    x0Calc = ((slidePhase * -0x140) >> 5) + 0x140;
    asm("" : : "r"(x0Calc));
    syBase = g_TeamLogoColorChannel;
    x0 = (u16) x0Calc;
    sy = (syBase * 0x30) + 0xD9;
    if (g_TeamLogoPaletteMode == 1)
    {
      clut = (rsin((g_TeamLogoColorCycleAngle * 2) % 0x1000) / 64) - 0x41;
      DrawRectOutline(ot.pointer, (s16)x0, (s16)sy, (s16)0x12, 0x15, 0, (u8)clut, 0, (u8)0xFF);
    }
    yA8 = (s16) (kreg + 0x14);
    sx = 0x3F;
    sx = x0 - sx;
    GameDrawNumber((s16)sx, yA8, (s16)3, g_TeamLogoClut[g_TeamLogoPenColor] & 0x1F,
                   (u8)0x7F, (u8)0x7F, (u8)0x7F, 0x244, 0x20);
    yB8 = (s16) (kreg + 0x44);
    GameDrawNumber((s16)sx, yB8, (s16)3,
                   (g_TeamLogoClut[g_TeamLogoPenColor] >> 5) & 0x1F,
                   (u8)0x7F, (u8)0x7F, (u8)0x7F, 0x244, 0x20);
    yC8 = (s16) (kreg + 0x74);
    GameDrawNumber((s16)sx, yC8, (s16)3,
                   (g_TeamLogoClut[g_TeamLogoPenColor] >> 10) & 0x1F,
                   (u8)0x7F, (u8)0x7F, (u8)0x7F, 0x244, 0x20);
    {
      s32 alpha;
      alpha = 0xFF;
      DrawRectOutline(ot.pointer, (s16)x0, (s16)kreg, (s16)0x12, 0x26, (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)alpha);
      DrawRectOutline(ot.pointer, (s16)x0, (s16)(kreg + 0x30), (s16)0x12, 0x26, (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)alpha);
      DrawRectOutline(ot.pointer, (s16)x0, (s16)(kreg + 0x60), (s16)0x12, 0x26, (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)alpha);
    }
    xa = x0 + 1;
    xb = x0 + 0x11;
    DrawLine(ot.pointer, (s16)xa, (s16)(kreg + 0x11), (s16)xb, (s16)(kreg + 0x11), (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    DrawLine(ot.pointer, (s16)xa, (s16)(kreg + 0x12), (s16)xb, (s16)(kreg + 0x12), (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    DrawLine(ot.pointer, (s16)xa, (s16)(kreg + 0x41), (s16)xb, (s16)(kreg + 0x41), (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    DrawLine(ot.pointer, (s16)xa, (s16)(kreg + 0x42), (s16)xb, (s16)(kreg + 0x42), (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    DrawLine(ot.pointer, (s16)xa, (s16)(kreg + 0x71), (s16)xb, (s16)(kreg + 0x71), (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    DrawLine(ot.pointer, (s16)xa, (s16)(kreg + 0x72), (s16)xb, (s16)(kreg + 0x72), (u8)0xB4, (u8)0xB4, (u8)0xB4, (u8)0xFF);
    xc = x0 + 5;
    tileSize = 0x10;
    DrawSprite(ot.pointer, (s16)xc, (s16)(kreg + 2), (s16)8, (s16)tileSize,
               (u8)0xD8, (u8)0x18, 0, 0, 0, 0x244, 1, 1, 0x5B);
    DrawSprite(ot.pointer, (s16)xc, (s16)(kreg + 0x32), (s16)8, (s16)tileSize,
               (u8)0x80, (u8)0x18, 0, 0, 0, 0x244, 1, 1, 0x5B);
    DrawSprite(ot.pointer, (s16)xc, (s16)(kreg + 0x62), (s16)8, (s16)tileSize,
               (u8)0x58, (u8)0x18, 0, 0, 0, 0x244, 1, 1, 0x5B);
    DrawSolidRect(ot.pointer, (s16)xa, (s16)(kreg + 2), (s16)0x10, (s16)tileSize, (u8)0xC0, 0, 0, (u8)0xFF);
    DrawSolidRect(ot.pointer, (s16)xa, (s16)yA8, (s16)0x10, (s16)tileSize, 0, 0, 0, (u8)0xFF);
    DrawSolidRect(ot.pointer, (s16)xa, (s16)(kreg + 0x32), (s16)0x10, (s16)tileSize, 0, (u8)0xC0, 0, (u8)0xFF);
    DrawSolidRect(ot.pointer, (s16)xa, (s16)yB8, (s16)0x10, (s16)tileSize, 0, 0, 0, (u8)0xFF);
    DrawSolidRect(ot.pointer, (s16)xa, (s16)(kreg + 0x62), (s16)0x10, (s16)tileSize, 0, 0, (u8)0xC0, (u8)0xFF);
    DrawSolidRect(ot.pointer, (s16)xa, (s16)yC8, (s16)0x10, (s16)tileSize, 0, 0, 0, (u8)0xFF);
  }
  if (a0v > 0)
  {
    g_TeamLogoPanelStep = a0v + g_TeamLogoPanelStep;
    if (g_TeamLogoPanelStep >= 0x1A)
    {
      g_TeamLogoPanelStep = 0x19;
    }
  }
  if (a1v > 0)
  {
    g_TeamLogoEditorStep = a1v + g_TeamLogoEditorStep;
    if (g_TeamLogoEditorStep >= 0x11)
    {
      g_TeamLogoEditorStep = 0x10;
    }
  }
}

void RampTeamLogoCanvas(s32 stepA, s32 stepB) {
    s32 temp;
    s32 y;

    if (stepA > 0) {
        temp = stepA + g_TeamLogoFadeLevel;
        g_TeamLogoFadeLevel = temp;
        if (temp >= 0x101) {
            g_TeamLogoFadeLevel = 0x100;
        }
    } else {
        temp = stepA + g_TeamLogoFadeLevel;
        g_TeamLogoFadeLevel = temp;
        if (temp < 0x40) {
            g_TeamLogoFadeLevel = 0x40;
        }
    }

    if (stepB > 0) {
        temp = stepB + g_TeamLogoZoomLevel;
        g_TeamLogoZoomLevel = temp;
        if (temp >= 0x101) {
            g_TeamLogoZoomLevel = 0x100;
        }
    } else {
        temp = stepB + g_TeamLogoZoomLevel;
        g_TeamLogoZoomLevel = temp;
        if (temp < 0) {
            g_TeamLogoZoomLevel = 0;
        }
    }

    y = g_TeamLogoZoomLevel;
    temp = (y * 17) << 4;
    if (temp < 0) {
        temp += 0xFF;
    }
    g_TeamLogoZoomSpan = 0x220 - (temp >> 8);
}


void ScrollTeamLogoUp(void) {
    s32 i;
    u32 *base;
    u32 saved[8];

    PlaySoundCue(1);

    base = g_TeamLogoCanvas.words[0];
    for (i = 0; i < 8; i++) {
        saved[i] = base[i];
    }
    for (i = 0; i < 0x1F8; i++) {
        base[i] = base[i + 8];
    }
    for (i = 0; i < 8; i++) {
        base[i + 0x1F8] = saved[i];
    }
}

void ScrollTeamLogoDown(void) {
    s32 i;
    u32 *newPtr;
    u32 *stackPtr;
    u32 *base;
    u32 *cursor;
    u32 saved[8];
    u32 value;

    PlaySoundCue(1);

    i = 0;
    stackPtr = saved;
    base = g_TeamLogoCanvas.words[0];
    cursor = base;
    do {
        value = cursor[0x1F8];
        cursor++;
        i++;
        *stackPtr = value;
        stackPtr++;
    } while (i < 8);

    i = 0x1F7;
    newPtr = base + 0x1F7;
    cursor = newPtr;
    do {
        value = *cursor;
        i--;
        cursor[8] = value;
        cursor--;
    } while (i >= 0);

    i = 0;
    stackPtr = base;
    cursor = saved;
    do {
        value = *cursor;
        cursor++;
        i++;
        newPtr = stackPtr;
        *newPtr = value;
        stackPtr++;
    } while (8 > i);
}

void ScrollTeamLogoLeft(void) {
    s32 row;
    u32 *savePtr;
    u32 *savePtr2;
    register u32 *rowBase asm("$8");
    s32 offset;
    s32 col;
    u32 *base;
    u32 *base2;
    u32 *addr;
    u32 *cursor;
    u32 saved[64];
    register u32 value asm("$2");
    register u32 next asm("$3");

    PlaySoundCue(1);

    row = 0;
    savePtr = saved;
    base = g_TeamLogoCanvas.words[0];
    cursor = base;
    do {
        value = *cursor;
        cursor += 8;
        row++;
        value <<= 28;
        *savePtr = value;
        savePtr++;
    } while (row < 0x40);

    row = 0;
    savePtr2 = saved;
    rowBase = base;
    offset = 0;
    do {
        col = 0;
        base2 = base;
        do {
            TeamLogoCanvasAddress address;

            address.wordPointer = base2;
            address.value = offset + address.value;
            addr = address.wordPointer;
            base2++;
            value = addr[0];
            next = addr[1];
            value >>= 4;
            next <<= 28;
            value |= next;
            addr[0] = value;
            col++;
        } while (col < 7);

        value = *savePtr2;
        savePtr2++;
        offset += 0x20;
        next = rowBase[7];
        row++;
        next >>= 4;
        next |= value;
        rowBase[7] = next;
        rowBase += 8;
    } while (row < 0x40);
}

void ScrollTeamLogoRight(void) {
    s32 row;
    u32 *savePtr;
    u32 *savePtr2;
    u32 *rowBase;
    s32 offset;
    s32 col;
    u32 *base;
    u32 *base2;
    u32 *cursor;
    u32 saved[64];

    PlaySoundCue(1);

    row = 0;
    savePtr = saved;
    base = g_TeamLogoCanvas.words[0];
    cursor = base;
    do {
        u32 last = cursor[7];
        cursor += 8;
        row++;
        last >>= 28;
        *savePtr = last;
        savePtr++;
    } while (row < 0x40);

    row = 0;
    rowBase = base;
    savePtr2 = saved;
    offset = 0;
    do {
        col = 7;
        base2 = base + 7;
        do {
            TeamLogoCanvasAddress address;
            u32 *word;
            u32 hi;
            u32 lo;

            address.wordPointer = base2;
            address.value = offset + address.value;
            word = address.wordPointer;
            base2--;
            col--;
            word--;
            hi = word[1];
            lo = word[0];
            hi <<= 4;
            lo >>= 28;
            hi |= lo;
            word[1] = hi;
        } while (col > 0);

        {
            u32 wrap = *savePtr2;
            u32 first;
            savePtr2++;
            offset += 0x20;
            first = rowBase[0];
            row++;
            first <<= 4;
            first |= wrap;
            rowBase[0] = first;
            rowBase += 8;
        }
    } while (row < 0x40);
}

void FlipTeamLogoVertical(void) {
    s32 i;
    s32 j;
    s32 mirror;
    u32 *base;

    PlaySoundCue(8);
    base = g_TeamLogoCanvas.words[0];
    i = 0;
    mirror = 0x3F;
    do {
        u8 *cursor;
        s32 leftOffset;
        s32 rightOffset;

        j = 0;
        leftOffset = i * 32;
        rightOffset = (mirror - i) << 5;
        cursor = GetTeamLogoCanvasBytes(base);
        do {
            u32 temp;
            u32 *left;
            u32 *right;
            TeamLogoCanvasAddress leftAddress;
            TeamLogoCanvasAddress rightAddress;

            leftAddress.bytePointer = cursor;
            leftAddress.value = leftOffset + leftAddress.value;
            left = leftAddress.wordPointer;
            rightAddress.bytePointer = cursor;
            rightAddress.value = rightOffset + rightAddress.value;
            right = rightAddress.wordPointer;
            cursor += 4;
            temp = *left;
            *left = *right;
            j++;
            *right = temp;
        } while (j < 8);
        i++;
    } while (i < 0x20);
}
