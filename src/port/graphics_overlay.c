#include "graphics_overlay.h"

#include <SDL3/SDL.h>
#include <psyz/overlay_sdl3.h>
#include <psyz/overlay_sdl3_gpu.h>
#include <psyz/present_sdl3_gpu.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modern/modern_renderer.h"
#include "port_config.h"
#include "runtime_config.h"
#include "third_party/stb_truetype.h"

#include "fonts/ibm_plex_sans.ttf.inc"
#include "modern/shaders/overlay_frag_msl.h"
#include "modern/shaders/overlay_frag_spv.h"
#include "modern/shaders/overlay_vert_msl.h"
#include "modern/shaders/overlay_vert_spv.h"

/* ---- settings ---- */

typedef struct Setting {
    const char *label;
    const char *help;
    int count;
    const char *const *names;
    int (*get)(const RagePortConfig *config);
    void (*set)(RagePortConfig *config, int index);
} Setting;

static int NearestIndex(const float *values, int count, float value) {
    int best = 0;
    int i;

    for (i = 1; i < count; i++) {
        if (fabsf(values[i] - value) < fabsf(values[best] - value)) best = i;
    }
    return best;
}

static const char *const kRendererNames[] = {"Classic", "Modern"};
static int GetRenderer(const RagePortConfig *c) {
    return c->renderer == RAGE_RENDERER_MODERN;
}
static void SetRenderer(RagePortConfig *c, int i) {
    c->renderer = i ? RAGE_RENDERER_MODERN : RAGE_RENDERER_CLASSIC;
}

static const float kScales[] = {1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f};
static const char *const kScaleNames[] = {"1x", "2x", "3x", "4x", "6x", "8x"};
static int GetScale(const RagePortConfig *c) {
    return NearestIndex(kScales, 6, c->modernInternalScale);
}
static void SetScale(RagePortConfig *c, int i) {
    c->modernInternalScale = kScales[i];
}

static const char *const kAspectNames[] = {"Match window", "Classic 4:3",
                                           "Widescreen 16:9"};
static int GetAspect(const RagePortConfig *c) { return (int)c->modernAspect; }
static void SetAspect(RagePortConfig *c, int i) {
    c->modernAspect = (RageModernAspect)i;
}

static const float kFpsValues[] = {(float)RAGE_MODERN_FPS_VSYNC,
                                   (float)RAGE_MODERN_FPS_LOGIC, 60.0f,
                                   120.0f, 144.0f};
static const char *const kFpsNames[] = {"Display sync", "Original timing",
                                        "60 fps", "120 fps", "144 fps"};
static int GetFps(const RagePortConfig *c) {
    return NearestIndex(kFpsValues, 5, (float)c->modernFps);
}
static void SetFps(RagePortConfig *c, int i) {
    c->modernFps = (int)kFpsValues[i];
}

static const char *const kFilterNames[] = {"Crisp pixels", "Smooth"};
static int GetFilter(const RagePortConfig *c) {
    return c->modernTextureFilterLinear != 0;
}
static void SetFilter(RagePortConfig *c, int i) {
    c->modernTextureFilterLinear = i;
}

static const char *const kPostNames[] = {"Off", "FXAA"};
static int GetPost(const RagePortConfig *c) {
    return c->modernPost == RAGE_MODERN_POST_FXAA;
}
static void SetPost(RagePortConfig *c, int i) {
    c->modernPost = i ? RAGE_MODERN_POST_FXAA : RAGE_MODERN_POST_NONE;
}

static const char *const kGradingNames[] = {"Off", "Vibrant"};
static int GetGrading(const RagePortConfig *c) { return c->modernGrading != 0; }
static void SetGrading(RagePortConfig *c, int i) { c->modernGrading = i; }

static const char *const kShadowNames[] = {"Off", "Standard", "High"};
static int GetShadows(const RagePortConfig *c) { return c->modernShadows; }
static void SetShadows(RagePortConfig *c, int i) { c->modernShadows = i; }

static const char *const kSsaoNames[] = {"Off", "On"};
static int GetSsao(const RagePortConfig *c) { return c->modernSsao != 0; }
static void SetSsao(RagePortConfig *c, int i) { c->modernSsao = i; }

static const float kDistances[] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
static const char *const kDistanceNames[] = {"0.5x", "1x", "1.5x",
                                             "2x",   "3x", "4x"};
static int GetDistance(const RagePortConfig *c) {
    return NearestIndex(kDistances, 6, c->modernDrawDistance);
}
static void SetDistance(RagePortConfig *c, int i) {
    c->modernDrawDistance = kDistances[i];
}

static const Setting kSettings[] = {
    {"Renderer", "Classic replays the original PlayStation image; Modern draws the scene natively.",
     2, kRendererNames, GetRenderer, SetRenderer},
    {"Render resolution", "Higher values increase image detail and GPU load.",
     6, kScaleNames, GetScale, SetScale},
    {"Aspect ratio", "Menus keep their original layout.",
     3, kAspectNames, GetAspect, SetAspect},
    {"Frame rate", "Display sync follows your monitor. Original timing disables interpolation.",
     5, kFpsNames, GetFps, SetFps},
    {"Texture filtering", "Choose crisp pixels or smoother transitions.",
     2, kFilterNames, GetFilter, SetFilter},
    {"Anti-aliasing", "FXAA softens jagged edges.",
     2, kPostNames, GetPost, SetPost},
    {"Color grading", "A more vibrant palette.",
     2, kGradingNames, GetGrading, SetGrading},
    {"Shadows", "Sun shadow detail. High uses the sharpest shadow maps.",
     3, kShadowNames, GetShadows, SetShadows},
    {"Ambient occlusion", "Soft contact shadows in creases and under cars.",
     2, kSsaoNames, GetSsao, SetSsao},
    {"Draw distance", "Values above 1x may expose unfinished track geometry.",
     6, kDistanceNames, GetDistance, SetDistance},
};

enum {
    SETTING_COUNT = (int)(sizeof(kSettings) / sizeof(kSettings[0])),
    ACTION_RESUME = SETTING_COUNT,
    ACTION_QUIT = SETTING_COUNT + 1,
    ROW_COUNT = SETTING_COUNT + 2,
};

/* ---- layout, in design units that scale with the window ---- */

enum {
    PANEL_W = 600,
    PAD = 32,
    HEADER_H = 92,
    ROW_H = 46,
    ACTION_GAP = 14,
    FOOTER_H = 74,
    VALUE_W = 250,
    PANEL_H = HEADER_H + ROW_COUNT * ROW_H + ACTION_GAP + FOOTER_H,
};

typedef struct Layout {
    float scale;
    int x, y, w, h; /* panel rectangle in window pixels */
} Layout;

static void ComputeLayout(int windowW, int windowH, Layout *layout) {
    float scale = (float)windowH / 820.0f;
    float widthLimit = (float)windowW * 0.94f / (float)PANEL_W;

    if (scale > widthLimit) scale = widthLimit;
    if (scale > 2.5f) scale = 2.5f;
    if (scale < 0.5f) scale = 0.5f;
    scale = floorf(scale * 8.0f) / 8.0f; /* avoid redrawing on every resize pixel */
    layout->scale = scale;
    layout->w = (int)ceilf((float)PANEL_W * scale);
    layout->h = (int)ceilf((float)PANEL_H * scale);
    layout->x = (windowW - layout->w) / 2;
    layout->y = (windowH - layout->h) / 2;
}

static float RowTop(int row) {
    return (float)(HEADER_H + row * ROW_H +
                   (row >= SETTING_COUNT ? ACTION_GAP : 0));
}

static int RowAtDesign(float x, float y) {
    int row;

    if (x < 0.0f || x > (float)PANEL_W) return -1;
    for (row = 0; row < ROW_COUNT; row++) {
        float top = RowTop(row);
        if (y >= top && y < top + (float)ROW_H) return row;
    }
    return -1;
}

/* ---- state ---- */

static int s_installed;
static int s_open;
static int s_selected;
static int s_dirty;
static int s_applyPending;
static int s_quitPending;
static int s_flushInput;
static RagePortConfig s_config;
static SDL_Window *s_window;
static int s_windowW, s_windowH;
static Layout s_layout;
static PsyzOverlayEventCB_SDL3 s_prevEvent;
static PsyzOverlayRenderCB_SDL3GPU s_prevRender;

int PortGraphicsOverlayActive(void) { return s_open; }

int PortGraphicsOverlayTakeClosed(void) {
    int closed = s_flushInput && !s_open;

    if (closed) s_flushInput = 0;
    return closed;
}

static int StepSetting(int row, int direction) {
    const Setting *setting = &kSettings[row];
    int index = setting->get(&s_config);

    index = (index + direction + setting->count) % setting->count;
    setting->set(&s_config, index);
    s_applyPending = 1;
    s_dirty = 1;
    return index;
}

static void ActivateRow(int row) {
    if (row == ACTION_RESUME) {
        s_open = 0;
        s_flushInput = 1;
    } else if (row == ACTION_QUIT) {
        s_quitPending = 1;
    } else if (row >= 0 && row < SETTING_COUNT) {
        StepSetting(row, 1);
    }
}

static void Select(int row) {
    if (row < 0 || row >= ROW_COUNT || row == s_selected) return;
    s_selected = row;
    s_dirty = 1;
}

static void OpenPanel(void) {
    const RagePortConfig *active = PortActiveConfig();
    SDL_GPUDevice *device = NULL;

    s_window = NULL;
    if (active == NULL ||
        !Psyz_VideoGetPresentationDevice_SDL3GPU(&s_window, &device)) {
        return;
    }
    s_config = *active;
    s_selected = 0;
    s_open = 1;
    s_dirty = 1;
}

static void ClosePanel(void) {
    s_open = 0;
    s_flushInput = 1;
}

static void OnMouse(float windowX, float windowY, int click) {
    float density = s_window != NULL ? SDL_GetWindowPixelDensity(s_window) : 1.0f;
    float px = windowX * density - (float)s_layout.x;
    float py = windowY * density - (float)s_layout.y;
    float dx = px / s_layout.scale;
    float dy = py / s_layout.scale;
    int row = RowAtDesign(dx, dy);

    if (row >= 0) Select(row);
    if (!click) return;
    if (row < 0) {
        ClosePanel();
    } else if (row < SETTING_COUNT) {
        float valueCenter = (float)(PANEL_W - PAD) - (float)VALUE_W * 0.5f;
        StepSetting(row, dx < valueCenter ? -1 : 1);
    } else {
        ActivateRow(row);
    }
}

static void OnEvent(const SDL_Event *event) {
    if (s_prevEvent != NULL) s_prevEvent(event);
    if (event->type == SDL_EVENT_KEY_DOWN) {
        SDL_Scancode key = event->key.scancode;

        if (key == SDL_SCANCODE_ESCAPE) {
            if (event->key.repeat) return;
            if (s_open) ClosePanel();
            else OpenPanel();
            return;
        }
        if (!s_open) return;
        switch (key) {
        case SDL_SCANCODE_UP:
            Select((s_selected + ROW_COUNT - 1) % ROW_COUNT);
            break;
        case SDL_SCANCODE_DOWN:
            Select((s_selected + 1) % ROW_COUNT);
            break;
        case SDL_SCANCODE_LEFT:
            if (s_selected < SETTING_COUNT) StepSetting(s_selected, -1);
            break;
        case SDL_SCANCODE_RIGHT:
            if (s_selected < SETTING_COUNT) StepSetting(s_selected, 1);
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_SPACE:
            if (!event->key.repeat) ActivateRow(s_selected);
            break;
        default:
            break;
        }
    } else if (!s_open) {
        return;
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        OnMouse(event->motion.x, event->motion.y, 0);
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
               event->button.button == SDL_BUTTON_LEFT) {
        OnMouse(event->button.x, event->button.y, 1);
    } else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        switch (event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            Select((s_selected + ROW_COUNT - 1) % ROW_COUNT);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            Select((s_selected + 1) % ROW_COUNT);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            if (s_selected < SETTING_COUNT) StepSetting(s_selected, -1);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            if (s_selected < SETTING_COUNT) StepSetting(s_selected, 1);
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            ActivateRow(s_selected);
            break;
        case SDL_GAMEPAD_BUTTON_EAST:
        case SDL_GAMEPAD_BUTTON_START:
            ClosePanel();
            break;
        default:
            break;
        }
    }
}

/* ---- software rasteriser: premultiplied RGBA ---- */

typedef struct Color {
    float r, g, b, a;
} Color;

static uint8_t *s_pixels;
static int s_surfaceW, s_surfaceH; /* surface height includes the solid row */
static stbtt_fontinfo s_font;
static int s_fontState;

static int FontReady(void) {
    int offset;

    if (s_fontState != 0) return s_fontState > 0;
    offset = stbtt_GetFontOffsetForIndex(ibm_plex_sans_ttf, 0);
    if (offset < 0 || !stbtt_InitFont(&s_font, ibm_plex_sans_ttf, offset)) {
        s_fontState = -1;
        return 0;
    }
    s_fontState = 1;
    return 1;
}

static float Clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

static void BlendPixel(int x, int y, Color c, float coverage) {
    uint8_t *p;
    float a = c.a * coverage;
    float keep = 1.0f - a;

    if (x < 0 || y < 0 || x >= s_surfaceW || y >= s_surfaceH - 1 || a <= 0.0f) {
        return;
    }
    p = s_pixels + ((size_t)y * (size_t)s_surfaceW + (size_t)x) * 4u;
    p[0] = (uint8_t)(c.r * a * 255.0f + p[0] * keep + 0.5f);
    p[1] = (uint8_t)(c.g * a * 255.0f + p[1] * keep + 0.5f);
    p[2] = (uint8_t)(c.b * a * 255.0f + p[2] * keep + 0.5f);
    p[3] = (uint8_t)(a * 255.0f + p[3] * keep + 0.5f);
}

static float RoundRectDistance(float px, float py, float x, float y, float w,
                               float h, float r) {
    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float qx = fabsf(px - cx) - (w * 0.5f - r);
    float qy = fabsf(py - cy) - (h * 0.5f - r);
    float ox = qx > 0.0f ? qx : 0.0f;
    float oy = qy > 0.0f ? qy : 0.0f;
    float inside = qx > qy ? qx : qy;

    return sqrtf(ox * ox + oy * oy) + (inside < 0.0f ? inside : 0.0f) - r;
}

static void FillRoundRect(float x, float y, float w, float h, float r,
                          Color c) {
    int x0 = (int)floorf(x) - 1, x1 = (int)ceilf(x + w) + 1;
    int y0 = (int)floorf(y) - 1, y1 = (int)ceilf(y + h) + 1;
    int ix, iy;

    for (iy = y0; iy < y1; iy++) {
        for (ix = x0; ix < x1; ix++) {
            float d = RoundRectDistance((float)ix + 0.5f, (float)iy + 0.5f, x,
                                        y, w, h, r);
            BlendPixel(ix, iy, c, Clamp01(0.5f - d));
        }
    }
}

/* A capsule: the thick line segment the chevrons are made of. */
static void DrawSegment(float ax, float ay, float bx, float by, float width,
                        Color c) {
    float minX = (ax < bx ? ax : bx) - width - 1.0f;
    float maxX = (ax > bx ? ax : bx) + width + 1.0f;
    float minY = (ay < by ? ay : by) - width - 1.0f;
    float maxY = (ay > by ? ay : by) + width + 1.0f;
    float ex = bx - ax, ey = by - ay;
    float length2 = ex * ex + ey * ey;
    int ix, iy;

    for (iy = (int)floorf(minY); iy < (int)ceilf(maxY); iy++) {
        for (ix = (int)floorf(minX); ix < (int)ceilf(maxX); ix++) {
            float px = (float)ix + 0.5f - ax;
            float py = (float)iy + 0.5f - ay;
            float t = length2 > 0.0f ? (px * ex + py * ey) / length2 : 0.0f;
            float qx, qy, d;

            t = Clamp01(t);
            qx = px - ex * t;
            qy = py - ey * t;
            d = sqrtf(qx * qx + qy * qy) - width * 0.5f;
            BlendPixel(ix, iy, c, Clamp01(0.5f - d));
        }
    }
}

static void DrawChevron(float cx, float cy, float size, int direction,
                        float width, Color c) {
    float tipX = cx + direction * size * 0.5f;
    float backX = cx - direction * size * 0.5f;

    DrawSegment(backX, cy - size, tipX, cy, width, c);
    DrawSegment(backX, cy + size, tipX, cy, width, c);
}

static float TextWidth(const char *text, float em) {
    float scale = stbtt_ScaleForMappingEmToPixels(&s_font, em);
    float width = 0.0f;

    for (; *text != '\0'; text++) {
        int advance, bearing;

        stbtt_GetCodepointHMetrics(&s_font, (unsigned char)*text, &advance,
                                   &bearing);
        width += (float)advance * scale;
    }
    return width;
}

/* align: 0 left, 1 centre, 2 right of `x`. */
static void DrawText(float x, float baseline, float em, const char *text,
                     Color c, int align) {
    enum { GLYPH_MAX = 256 };
    static uint8_t glyph[GLYPH_MAX * GLYPH_MAX];
    float scale = stbtt_ScaleForMappingEmToPixels(&s_font, em);
    float width = TextWidth(text, em);

    x -= align == 1 ? width * 0.5f : align == 2 ? width : 0.0f;
    for (; *text != '\0'; text++) {
        int codepoint = (unsigned char)*text;
        int advance, bearing, x0, y0, x1, y1, gx, gy;

        stbtt_GetCodepointHMetrics(&s_font, codepoint, &advance, &bearing);
        stbtt_GetCodepointBitmapBox(&s_font, codepoint, scale, scale, &x0, &y0,
                                    &x1, &y1);
        if (x1 - x0 > 0 && y1 - y0 > 0 && x1 - x0 <= GLYPH_MAX &&
            y1 - y0 <= GLYPH_MAX) {
            int gw = x1 - x0, gh = y1 - y0;
            int ox = (int)floorf(x + 0.5f) + x0;
            int oy = (int)floorf(baseline + 0.5f) + y0;

            stbtt_MakeCodepointBitmap(&s_font, glyph, gw, gh, gw, scale, scale,
                                      codepoint);
            for (gy = 0; gy < gh; gy++) {
                for (gx = 0; gx < gw; gx++) {
                    uint8_t coverage = glyph[gy * gw + gx];
                    if (coverage != 0) {
                        BlendPixel(ox + gx, oy + gy, c, (float)coverage / 255.0f);
                    }
                }
            }
        }
        x += (float)advance * scale;
    }
}

static const Color kPanel = {0.075f, 0.082f, 0.102f, 0.96f};
static const Color kBorder = {1.0f, 1.0f, 1.0f, 0.10f};
static const Color kAccent = {0.27f, 0.62f, 1.0f, 1.0f};
static const Color kText = {0.93f, 0.95f, 0.98f, 1.0f};
static const Color kMuted = {0.60f, 0.65f, 0.73f, 1.0f};
static const Color kDanger = {1.0f, 0.45f, 0.42f, 1.0f};

static Color WithAlpha(Color c, float a) {
    c.a = a;
    return c;
}

static void PaintPanel(void) {
    float s = s_layout.scale;
    float width = (float)PANEL_W * s;
    float height = (float)PANEL_H * s;
    int row;
    float footerTop = (float)(PANEL_H - FOOTER_H) * s;
    const char *help = s_selected < SETTING_COUNT
                           ? kSettings[s_selected].help
                           : s_selected == ACTION_RESUME
                                 ? "Return to the game."
                                 : "Close the game.";

    memset(s_pixels, 0, (size_t)s_surfaceW * (size_t)s_surfaceH * 4u);
    FillRoundRect(0.0f, 0.0f, width, height, 16.0f * s, kBorder);
    FillRoundRect(1.0f, 1.0f, width - 2.0f, height - 2.0f, 15.0f * s, kPanel);

    DrawText((float)PAD * s, 52.0f * s, 28.0f * s, "Graphics", kText, 0);
    DrawText((float)(PANEL_W - PAD) * s, 50.0f * s, 14.0f * s, "GAME PAUSED",
             kMuted, 2);
    FillRoundRect((float)PAD * s, (float)(HEADER_H - 12) * s,
                  (float)(PANEL_W - 2 * PAD) * s, 1.0f, 0.0f,
                  WithAlpha(kText, 0.10f));

    for (row = 0; row < ROW_COUNT; row++) {
        float top = RowTop(row) * s;
        float mid = top + (float)ROW_H * 0.5f * s;
        float baseline = mid + 6.0f * s;
        int selected = row == s_selected;
        Color label = selected ? kText : WithAlpha(kText, 0.82f);

        if (selected) {
            FillRoundRect((float)(PAD / 2) * s, top + 3.0f * s,
                          (float)(PANEL_W - PAD) * s, (float)(ROW_H - 6) * s,
                          9.0f * s, WithAlpha(kAccent, 0.16f));
            FillRoundRect((float)(PAD / 2) * s + 4.0f * s, mid - 10.0f * s,
                          3.0f * s, 20.0f * s, 1.5f * s, kAccent);
        }
        if (row < SETTING_COUNT) {
            const Setting *setting = &kSettings[row];
            float centre = (float)(PANEL_W - PAD) * s - (float)VALUE_W * 0.5f * s;
            float reach = ((float)VALUE_W * 0.5f - 14.0f) * s;
            Color chevron = selected ? kAccent : WithAlpha(kMuted, 0.75f);

            DrawText((float)PAD * s, baseline, 17.0f * s, setting->label, label, 0);
            DrawChevron(centre - reach, mid, 5.0f * s, -1, 2.0f * s, chevron);
            DrawChevron(centre + reach, mid, 5.0f * s, 1, 2.0f * s, chevron);
            DrawText(centre, baseline, 17.0f * s,
                     setting->names[setting->get(&s_config)],
                     selected ? kText : WithAlpha(kText, 0.9f), 1);
        } else {
            DrawText((float)PAD * s, baseline, 17.0f * s,
                     row == ACTION_RESUME ? "Resume game" : "Quit game",
                     row == ACTION_QUIT ? kDanger : label, 0);
        }
    }

    FillRoundRect((float)PAD * s, footerTop, (float)(PANEL_W - 2 * PAD) * s,
                  1.0f, 0.0f, WithAlpha(kText, 0.10f));
    DrawText((float)PAD * s, footerTop + 28.0f * s, 14.0f * s, help, kMuted, 0);
    DrawText((float)PAD * s, footerTop + 54.0f * s, 13.0f * s,
             "Up/Down select    Left/Right change    Esc resume",
             WithAlpha(kMuted, 0.7f), 0);
}

static void DumpSurface(const char *path) {
    FILE *file = fopen(path, "wb");
    int x, y;

    if (file == NULL) return;
    fprintf(file, "P6\n%d %d\n255\n", s_surfaceW, s_surfaceH - 1);
    for (y = 0; y < s_surfaceH - 1; y++) {
        for (x = 0; x < s_surfaceW; x++) {
            const uint8_t *p = s_pixels + ((size_t)y * (size_t)s_surfaceW +
                                           (size_t)x) * 4u;
            int keep = 255 - p[3];
            uint8_t rgb[3];

            rgb[0] = (uint8_t)(p[0] + 40 * keep / 255);
            rgb[1] = (uint8_t)(p[1] + 40 * keep / 255);
            rgb[2] = (uint8_t)(p[2] + 40 * keep / 255);
            fwrite(rgb, 1, 3, file);
        }
    }
    fclose(file);
}

/* ---- GPU ---- */

typedef struct OverlayQuad {
    float rect[4];
    float uv[4];
    float tint[4];
} OverlayQuad;

static SDL_GPUDevice *s_device;
static SDL_GPUGraphicsPipeline *s_pipeline;
static SDL_GPUSampler *s_sampler;
static SDL_GPUTexture *s_texture;
static int s_textureW, s_textureH;

void PortGraphicsOverlayReleaseGpu(void) {
    if (s_device != NULL) {
        if (s_texture != NULL) SDL_ReleaseGPUTexture(s_device, s_texture);
        if (s_sampler != NULL) SDL_ReleaseGPUSampler(s_device, s_sampler);
        if (s_pipeline != NULL) SDL_ReleaseGPUGraphicsPipeline(s_device, s_pipeline);
    }
    s_texture = NULL;
    s_sampler = NULL;
    s_pipeline = NULL;
    s_device = NULL;
    s_textureW = s_textureH = 0;
    s_dirty = 1;
}

static SDL_GPUShader *CreateShader(SDL_GPUDevice *device, SDL_GPUShaderStage stage,
                                   const unsigned char *spirv, size_t spirvSize,
                                   const unsigned char *msl, size_t mslSize,
                                   const char *mslEntry, Uint32 samplers,
                                   Uint32 uniforms) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderCreateInfo info;

    SDL_zero(info);
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniforms;
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.code = spirv;
        info.code_size = spirvSize;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.code = msl;
        info.code_size = mslSize;
        info.entrypoint = mslEntry;
        info.format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        return NULL;
    }
    return SDL_CreateGPUShader(device, &info);
}

static int EnsurePipeline(SDL_GPUDevice *device, SDL_GPUTextureFormat format) {
    SDL_GPUShader *vertex, *fragment;
    SDL_GPUColorTargetDescription color;
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUSamplerCreateInfo sampler;

    if (s_device != NULL && s_device != device) PortGraphicsOverlayReleaseGpu();
    if (s_pipeline != NULL && s_sampler != NULL) return 1;
    s_device = device;
    vertex = CreateShader(device, SDL_GPU_SHADERSTAGE_VERTEX, overlay_vert_spv,
                          overlay_vert_spv_len, overlay_vert_msl,
                          overlay_vert_msl_len, "vs_overlay", 0, 1);
    fragment = CreateShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, overlay_frag_spv,
                            overlay_frag_spv_len, overlay_frag_msl,
                            overlay_frag_msl_len, "fs_overlay", 1, 0);
    if (vertex == NULL || fragment == NULL) {
        if (vertex != NULL) SDL_ReleaseGPUShader(device, vertex);
        if (fragment != NULL) SDL_ReleaseGPUShader(device, fragment);
        fprintf(stderr, "rage-port: graphics overlay shaders failed: %s\n",
                SDL_GetError());
        return 0;
    }
    SDL_zero(color);
    color.format = format;
    color.blend_state.enable_blend = true;
    color.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    SDL_zero(info);
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.target_info.color_target_descriptions = &color;
    info.target_info.num_color_targets = 1;
    s_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    SDL_zero(sampler);
    sampler.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    s_sampler = SDL_CreateGPUSampler(device, &sampler);
    if (s_pipeline == NULL || s_sampler == NULL) {
        fprintf(stderr, "rage-port: graphics overlay pipeline failed: %s\n",
                SDL_GetError());
        PortGraphicsOverlayReleaseGpu();
        return 0;
    }
    return 1;
}

static int UploadSurface(SDL_GPUDevice *device) {
    SDL_GPUTransferBufferCreateInfo transferInfo;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *command;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureTransferInfo source;
    SDL_GPUTextureRegion destination;
    Uint32 bytes = (Uint32)((size_t)s_surfaceW * (size_t)s_surfaceH * 4u);
    void *mapped;

    if (s_texture == NULL || s_textureW != s_surfaceW || s_textureH != s_surfaceH) {
        SDL_GPUTextureCreateInfo info;

        if (s_texture != NULL) SDL_ReleaseGPUTexture(device, s_texture);
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_surfaceW;
        info.height = (Uint32)s_surfaceH;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        s_texture = SDL_CreateGPUTexture(device, &info);
        s_textureW = s_surfaceW;
        s_textureH = s_surfaceH;
        if (s_texture == NULL) return 0;
    }
    SDL_zero(transferInfo);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bytes;
    transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (transfer == NULL) return 0;
    mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped == NULL) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    memcpy(mapped, s_pixels, bytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    command = SDL_AcquireGPUCommandBuffer(device);
    if (command == NULL) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    copy = SDL_BeginGPUCopyPass(command);
    SDL_zero(source);
    source.transfer_buffer = transfer;
    source.pixels_per_row = (Uint32)s_surfaceW;
    source.rows_per_layer = (Uint32)s_surfaceH;
    SDL_zero(destination);
    destination.texture = s_texture;
    destination.w = (Uint32)s_surfaceW;
    destination.h = (Uint32)s_surfaceH;
    destination.d = 1;
    SDL_UploadToGPUTexture(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(command);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return 1;
}

static int ResizeSurface(int width, int height) {
    size_t bytes = (size_t)width * (size_t)(height + 1) * 4u;

    if (width == s_surfaceW && height + 1 == s_surfaceH && s_pixels != NULL) {
        return 1;
    }
    free(s_pixels);
    s_pixels = (uint8_t *)calloc(1, bytes);
    s_surfaceW = s_pixels != NULL ? width : 0;
    s_surfaceH = s_pixels != NULL ? height + 1 : 0;
    return s_pixels != NULL;
}

static void RefreshSurface(SDL_GPUDevice *device, SDL_Window *window) {
    int w = 0, h = 0;
    Layout layout;

    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (w <= 0 || h <= 0 || !FontReady()) return;
    ComputeLayout(w, h, &layout);
    if (w != s_windowW || h != s_windowH || layout.w != s_layout.w ||
        layout.h != s_layout.h) {
        s_dirty = 1;
    }
    s_windowW = w;
    s_windowH = h;
    s_layout = layout;
    if (!s_dirty) return;
    if (!ResizeSurface(layout.w, layout.h)) return;
    PaintPanel();
    /* The extra bottom row is one solid texel the backdrop quad samples. */
    memset(s_pixels + (size_t)(s_surfaceH - 1) * (size_t)s_surfaceW * 4u, 0,
           (size_t)s_surfaceW * 4u);
    {
        uint8_t *solid = s_pixels + (size_t)(s_surfaceH - 1) * (size_t)s_surfaceW * 4u;
        solid[3] = 255;
    }
    if (EnsurePipeline(device, SDL_GetGPUSwapchainTextureFormat(device, window))) {
        UploadSurface(device);
    }
    {
        const char *dump = RuntimeConfigGet("diagnostics.overlay_dump");
        if (dump != NULL && dump[0] != '\0') DumpSurface(dump);
    }
    s_dirty = 0;
}

void PortGraphicsOverlayTick(void) {
    SDL_GPUDevice *device = NULL;
    SDL_Window *window = NULL;

    if (s_applyPending) {
        const RagePortConfig *active;

        s_applyPending = 0;
        ModernApplyConfig(&s_config);
        active = PortActiveConfig();
        if (active != NULL) s_config = *active;
        s_dirty = 1;
    }
    if (s_quitPending) {
        SDL_Event quit;

        s_quitPending = 0;
        SDL_zero(quit);
        quit.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit);
    }
    if (!s_open) return;
    if (!Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device)) return;
    s_window = window;
    RefreshSurface(device, window);
}

static void DrawPanel(SDL_GPUCommandBuffer *command, SDL_GPURenderPass *pass) {
    OverlayQuad quad;
    SDL_GPUTextureSamplerBinding binding;
    SDL_GPUViewport viewport;
    SDL_Rect scissor;
    float w, h;

    if (s_pipeline == NULL || s_texture == NULL || s_windowW <= 0) {
        return;
    }
    w = (float)s_windowW;
    h = (float)s_windowH;
    viewport = (SDL_GPUViewport){0.0f, 0.0f, w, h, 0.0f, 1.0f};
    scissor = (SDL_Rect){0, 0, s_windowW, s_windowH};
    SDL_SetGPUViewport(pass, &viewport);
    SDL_SetGPUScissor(pass, &scissor);
    SDL_BindGPUGraphicsPipeline(pass, s_pipeline);
    binding.texture = s_texture;
    binding.sampler = s_sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);

    /* Backdrop: the solid texel, dimmed. */
    quad.rect[0] = -1.0f; quad.rect[1] = 1.0f;
    quad.rect[2] = 1.0f;  quad.rect[3] = -1.0f;
    quad.uv[0] = quad.uv[2] = 0.5f / (float)s_textureW;
    quad.uv[1] = quad.uv[3] = ((float)s_textureH - 0.5f) / (float)s_textureH;
    quad.tint[0] = quad.tint[1] = quad.tint[2] = 1.0f;
    quad.tint[3] = 0.62f;
    SDL_PushGPUVertexUniformData(command, 0, &quad, sizeof(quad));
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);

    /* The panel at one texel per pixel. */
    quad.rect[0] = (float)s_layout.x / w * 2.0f - 1.0f;
    quad.rect[1] = 1.0f - (float)s_layout.y / h * 2.0f;
    quad.rect[2] = (float)(s_layout.x + s_layout.w) / w * 2.0f - 1.0f;
    quad.rect[3] = 1.0f - (float)(s_layout.y + s_layout.h) / h * 2.0f;
    quad.uv[0] = 0.0f;
    quad.uv[1] = 0.0f;
    quad.uv[2] = 1.0f;
    quad.uv[3] = (float)(s_textureH - 1) / (float)s_textureH;
    quad.tint[3] = 1.0f;
    SDL_PushGPUVertexUniformData(command, 0, &quad, sizeof(quad));
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
}

static void OnRender(SDL_GPUCommandBuffer *command, SDL_GPURenderPass *pass) {
    if (s_prevRender != NULL) s_prevRender(command, pass);
    if (s_open) DrawPanel(command, pass);
}

void PortGraphicsOverlayInstall(void) {
    if (s_installed) return;
    s_installed = 1;
    s_prevEvent = Psyz_OverlayEvent_SDL3(OnEvent);
    s_prevRender = Psyz_OverlayRender_SDL3GPU(OnRender);
}
