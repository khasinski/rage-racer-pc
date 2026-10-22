#include "overlay_raster.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "third_party/stb_truetype.h"

#include "fonts/ibm_plex_sans.ttf.inc"

typedef struct Color {
    float r, g, b, a;
} Color;

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

/* ---- surface ---- */

int OverlaySurfaceResize(OverlaySurface *surface, int width, int height) {
    size_t bytes = (size_t)width * (size_t)(height + 1) * 4u;

    if (width == surface->w && height + 1 == surface->h &&
        surface->pixels != NULL) {
        return 1;
    }
    free(surface->pixels);
    surface->pixels = (uint8_t *)calloc(1, bytes);
    surface->w = surface->pixels != NULL ? width : 0;
    surface->h = surface->pixels != NULL ? height + 1 : 0;
    return surface->pixels != NULL;
}

void OverlaySurfaceRelease(OverlaySurface *surface) {
    free(surface->pixels);
    *surface = (OverlaySurface){0};
}

static float Clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

static void BlendPixel(OverlaySurface *s, int x, int y, Color c, float coverage) {
    uint8_t *p;
    float a = c.a * coverage;
    float keep = 1.0f - a;

    /* The last row is the solid texel, not part of the panel. */
    if (x < 0 || y < 0 || x >= s->w || y >= s->h - 1 || a <= 0.0f) return;
    p = s->pixels + ((size_t)y * (size_t)s->w + (size_t)x) * 4u;
    p[0] = (uint8_t)(c.r * a * 255.0f + p[0] * keep + 0.5f);
    p[1] = (uint8_t)(c.g * a * 255.0f + p[1] * keep + 0.5f);
    p[2] = (uint8_t)(c.b * a * 255.0f + p[2] * keep + 0.5f);
    p[3] = (uint8_t)(a * 255.0f + p[3] * keep + 0.5f);
}

/* ---- shapes ---- */

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

static void FillRoundRect(OverlaySurface *s, float x, float y, float w, float h,
                          float r, Color c) {
    int x0 = (int)floorf(x) - 1, x1 = (int)ceilf(x + w) + 1;
    int y0 = (int)floorf(y) - 1, y1 = (int)ceilf(y + h) + 1;
    int ix, iy;

    for (iy = y0; iy < y1; iy++) {
        for (ix = x0; ix < x1; ix++) {
            float d = RoundRectDistance((float)ix + 0.5f, (float)iy + 0.5f, x,
                                        y, w, h, r);
            BlendPixel(s, ix, iy, c, Clamp01(0.5f - d));
        }
    }
}

/* A capsule: the thick line segment the chevrons are made of. */
static void DrawSegment(OverlaySurface *s, float ax, float ay, float bx,
                        float by, float width, Color c) {
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
            BlendPixel(s, ix, iy, c, Clamp01(0.5f - d));
        }
    }
}

static void DrawChevron(OverlaySurface *s, float cx, float cy, float size,
                        int direction, float width, Color c) {
    float tipX = cx + (float)direction * size * 0.5f;
    float backX = cx - (float)direction * size * 0.5f;

    DrawSegment(s, backX, cy - size, tipX, cy, width, c);
    DrawSegment(s, backX, cy + size, tipX, cy, width, c);
}

/* ---- text ---- */

static stbtt_fontinfo s_font;
static int s_fontState;

int OverlayFontReady(void) {
    int offset;

    if (s_fontState != 0) return s_fontState > 0;
    offset = stbtt_GetFontOffsetForIndex(ibm_plex_sans_ttf, 0);
    if (offset < 0 || (size_t)offset >= ibm_plex_sans_ttf_len ||
        !stbtt_InitFont(&s_font, ibm_plex_sans_ttf, offset)) {
        s_fontState = -1;
        return 0;
    }
    s_fontState = 1;
    return 1;
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

typedef enum TextAlign { ALIGN_LEFT, ALIGN_CENTRE, ALIGN_RIGHT } TextAlign;

static void DrawText(OverlaySurface *s, float x, float baseline, float em,
                     const char *text, Color c, TextAlign align) {
    enum { GLYPH_MAX = 256 };
    static uint8_t glyph[GLYPH_MAX * GLYPH_MAX];
    float scale = stbtt_ScaleForMappingEmToPixels(&s_font, em);
    float width = TextWidth(text, em);

    x -= align == ALIGN_CENTRE ? width * 0.5f
         : align == ALIGN_RIGHT ? width : 0.0f;
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
                        BlendPixel(s, ox + gx, oy + gy, c,
                                   (float)coverage / 255.0f);
                    }
                }
            }
        }
        x += (float)advance * scale;
    }
}

/* ---- the panel ---- */

typedef struct Painter {
    OverlaySurface *surface;
    const OverlayModel *model;
    float s; /* design units to pixels */
} Painter;

static void PaintFrame(const Painter *p, float width, float height) {
    const float s = p->s;

    FillRoundRect(p->surface, 0.0f, 0.0f, width, height, 16.0f * s, kBorder);
    FillRoundRect(p->surface, 1.0f, 1.0f, width - 2.0f, height - 2.0f,
                  15.0f * s, kPanel);
}

static void PaintHeader(const Painter *p) {
    const float s = p->s;
    int page;

    DrawText(p->surface, (float)OVERLAY_PAD * s, 40.0f * s, 26.0f * s,
             "Settings", kText, ALIGN_LEFT);
    DrawText(p->surface, (float)(OVERLAY_PANEL_W - OVERLAY_PAD) * s, 38.0f * s,
             14.0f * s, "GAME PAUSED", kMuted, ALIGN_RIGHT);
    for (page = 0; page < OVERLAY_PAGE_COUNT; page++) {
        OverlayRect tab = OverlayTabRect(page);
        int current = page == p->model->page;

        if (current) {
            FillRoundRect(p->surface, tab.x * s, tab.y * s, tab.w * s,
                          tab.h * s, 8.0f * s, WithAlpha(kAccent, 0.18f));
        }
        DrawText(p->surface, (tab.x + tab.w * 0.5f) * s,
                 (tab.y + tab.h * 0.5f + 5.5f) * s, 15.0f * s,
                 OverlayPageName(page), current ? kText : kMuted, ALIGN_CENTRE);
    }
    FillRoundRect(p->surface, (float)OVERLAY_PAD * s,
                  (float)(OVERLAY_HEADER_H - 12) * s,
                  (float)(OVERLAY_PANEL_W - 2 * OVERLAY_PAD) * s, 1.0f, 0.0f,
                  WithAlpha(kText, 0.10f));
}

static void PaintSettingRow(const Painter *p, const OverlaySetting *setting,
                            float mid, float baseline, int selected,
                            Color label) {
    const float s = p->s;
    float centre = OverlayValueCentre() * s;
    float reach = ((float)OVERLAY_VALUE_W * 0.5f - 14.0f) * s;
    Color chevron = selected ? kAccent : WithAlpha(kMuted, 0.75f);
    int value = setting->get(&p->model->values);

    if (value < 0) value = 0;
    if (value >= setting->count) value = setting->count - 1;
    DrawText(p->surface, (float)OVERLAY_PAD * s, baseline, 17.0f * s,
             setting->label, label, ALIGN_LEFT);
    DrawChevron(p->surface, centre - reach, mid, 5.0f * s, -1, 2.0f * s,
                chevron);
    DrawChevron(p->surface, centre + reach, mid, 5.0f * s, 1, 2.0f * s,
                chevron);
    DrawText(p->surface, centre, baseline, 17.0f * s, setting->names[value],
             selected ? kText : WithAlpha(kText, 0.9f), ALIGN_CENTRE);
}

static void PaintRows(const Painter *p) {
    const float s = p->s;
    int settingCount = 0;
    const OverlaySetting *settings =
        OverlayPageSettings(p->model->page, &settingCount);
    int rows = settingCount + OVERLAY_ACTION_ROWS;
    int row;

    for (row = 0; row < rows; row++) {
        float top = OverlayRowTop(settingCount, row) * s;
        float mid = top + (float)OVERLAY_ROW_H * 0.5f * s;
        float baseline = mid + 6.0f * s;
        int selected = row == p->model->selected;
        Color label = selected ? kText : WithAlpha(kText, 0.82f);

        if (selected) {
            FillRoundRect(p->surface, (float)(OVERLAY_PAD / 2) * s,
                          top + 3.0f * s, (float)(OVERLAY_PANEL_W - OVERLAY_PAD) * s,
                          (float)(OVERLAY_ROW_H - 6) * s, 9.0f * s,
                          WithAlpha(kAccent, 0.16f));
            FillRoundRect(p->surface, (float)(OVERLAY_PAD / 2) * s + 4.0f * s,
                          mid - 10.0f * s, 3.0f * s, 20.0f * s, 1.5f * s,
                          kAccent);
        }
        if (row < settingCount) {
            PaintSettingRow(p, &settings[row], mid, baseline, selected, label);
        } else {
            int quit = row == settingCount + 1;

            DrawText(p->surface, (float)OVERLAY_PAD * s, baseline, 17.0f * s,
                     quit ? "Quit game" : "Resume game", quit ? kDanger : label,
                     ALIGN_LEFT);
        }
    }
}

static void PaintFooter(const Painter *p, int rowCount) {
    const float s = p->s;
    int settingCount = 0;
    const OverlaySetting *settings =
        OverlayPageSettings(p->model->page, &settingCount);
    float footerTop = (float)(OverlayDesignHeight(rowCount) - OVERLAY_FOOTER_H) * s;
    const char *help = p->model->selected < settingCount
                           ? settings[p->model->selected].help
                           : p->model->selected == settingCount
                                 ? "Return to the game."
                                 : "Close the game.";

    FillRoundRect(p->surface, (float)OVERLAY_PAD * s, footerTop,
                  (float)(OVERLAY_PANEL_W - 2 * OVERLAY_PAD) * s, 1.0f, 0.0f,
                  WithAlpha(kText, 0.10f));
    DrawText(p->surface, (float)OVERLAY_PAD * s, footerTop + 28.0f * s,
             14.0f * s, help, kMuted, ALIGN_LEFT);
    DrawText(p->surface, (float)OVERLAY_PAD * s, footerTop + 54.0f * s,
             13.0f * s,
             "Up/Down select   Left/Right change   PgUp/PgDn page   Esc resume",
             WithAlpha(kMuted, 0.7f), ALIGN_LEFT);
}

void OverlayPaint(OverlaySurface *surface, const OverlayModel *model,
                  const OverlayLayout *layout) {
    Painter painter = {surface, model, layout->scale};
    int rowCount = OverlayModelRowCount(model);
    uint8_t *solid;

    memset(surface->pixels, 0, (size_t)surface->w * (size_t)surface->h * 4u);
    PaintFrame(&painter, (float)OVERLAY_PANEL_W * layout->scale,
               (float)OverlayDesignHeight(rowCount) * layout->scale);
    PaintHeader(&painter);
    PaintRows(&painter);
    PaintFooter(&painter, rowCount);
    solid = surface->pixels + (size_t)(surface->h - 1) * (size_t)surface->w * 4u;
    solid[3] = 255;
}

int OverlaySurfaceDump(const OverlaySurface *surface, const char *path) {
    FILE *file = fopen(path, "wb");
    int x, y;

    if (file == NULL) return 0;
    fprintf(file, "P6\n%d %d\n255\n", surface->w, surface->h - 1);
    for (y = 0; y < surface->h - 1; y++) {
        for (x = 0; x < surface->w; x++) {
            const uint8_t *p = surface->pixels +
                               ((size_t)y * (size_t)surface->w + (size_t)x) * 4u;
            int keep = 255 - p[3];
            uint8_t rgb[3];

            rgb[0] = (uint8_t)(p[0] + 40 * keep / 255);
            rgb[1] = (uint8_t)(p[1] + 40 * keep / 255);
            rgb[2] = (uint8_t)(p[2] + 40 * keep / 255);
            fwrite(rgb, 1, 3, file);
        }
    }
    fclose(file);
    return 1;
}
