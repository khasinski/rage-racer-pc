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
                            Color label, int available) {
    const float s = p->s;
    float centre = OverlayValueCentre() * s;
    float reach = ((float)OVERLAY_VALUE_W * 0.5f - 14.0f) * s;
    Color chevron = !available ? WithAlpha(kMuted, 0.25f) : selected ? kAccent : WithAlpha(kMuted, 0.75f);
    int value = setting->get(&p->model->values);

    if (value < 0) value = 0;
    if (value >= setting->count) value = setting->count - 1;
    if (!available) label = WithAlpha(kMuted, 0.45f);
    DrawText(p->surface, (float)OVERLAY_PAD * s, baseline, 17.0f * s,
             setting->label, label, ALIGN_LEFT);
    if (setting->slider) {
        float start = centre - OVERLAY_SLIDER_W * s * 0.5f;
        float length = OVERLAY_SLIDER_W * s;
        float t = (float)value / (float)(setting->count - 1);
        DrawText(p->surface, centre, mid - 2.0f * s, 15.0f * s,
                 setting->names[value], !available ? label : selected ? kText : kMuted, ALIGN_CENTRE);
        FillRoundRect(p->surface, start, mid + 8.0f * s, length,
                      4.0f * s, 2.0f * s, WithAlpha(kText, 0.18f));
        FillRoundRect(p->surface, start, mid + 8.0f * s, length * t,
                      4.0f * s, 2.0f * s, available ? kAccent : WithAlpha(kMuted, 0.25f));
        FillRoundRect(p->surface, start + length * t - 5.0f * s,
                      mid + 5.0f * s, 10.0f * s, 10.0f * s, 5.0f * s, available ? kText : label);
        return;
    }
    DrawChevron(p->surface, centre - reach, mid, 5.0f * s, -1, 2.0f * s,
                chevron);
    DrawChevron(p->surface, centre + reach, mid, 5.0f * s, 1, 2.0f * s,
                chevron);
    DrawText(p->surface, centre, baseline, 17.0f * s, setting->names[value],
             !available ? label : selected ? kText : WithAlpha(kText, 0.9f), ALIGN_CENTRE);
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
            PaintSettingRow(p, &settings[row], mid, baseline, selected, label,
                            OverlaySettingAvailable(p->model, row));
        } else {
            int quit = row == settingCount + 2;

            DrawText(p->surface, (float)OVERLAY_PAD * s, baseline, 17.0f * s,
                     quit ? "Quit game" : row == settingCount ? "Reset this page" : "Resume game", quit ? kDanger : label,
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
                                 ? "Restore this page only. Installed packs are kept."
                                 : p->model->selected == settingCount + 1 ? "Return to the game."
                                 : "Close the game.";

    if (p->model->selected < settingCount && !OverlaySettingAvailable(p->model, p->model->selected))
        help = "Available with the Modern renderer.";
    if (p->model->settingsStatus[0]) help = p->model->settingsStatus;
    if(p->model->page==OVERLAY_PAGE_MODS&&p->model->selected<4&&p->model->packStatus[0])help=p->model->packStatus;
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

static void PaintDownloader(const Painter *p, int rowCount) {
    const float s = p->s;
    const OverlayModel *m = p->model;
    const float left = OVERLAY_PAD * s;
    const float width = (OVERLAY_PANEL_W - 2 * OVERLAY_PAD) * s;
    const float height = (float)OverlayDesignHeight(rowCount);
    const char *title = m->packDownloader == 1 ? "HD texture downloader" :
                        m->packDownloader == 2 ? "HD textures ready" : "Download stopped";
    DrawText(p->surface, left, 48*s, 26*s, title, kText, ALIGN_LEFT);
    DrawText(p->surface, left, 92*s, 16*s, "Faithful Refresh", kMuted, ALIGN_LEFT);
    FillRoundRect(p->surface, left, 116*s, width, s, 0, WithAlpha(kText, .1f));
    DrawText(p->surface, left, 172*s, 20*s, m->packPhase,
             m->packDownloader == 3 ? kDanger : kText, ALIGN_LEFT);
    int percent = m->packPercent < 0 ? 0 : m->packPercent > 100 ? 100 : m->packPercent;
    FillRoundRect(p->surface, left, 198*s, width, 12*s, 6*s, WithAlpha(kText,.1f));
    if (percent) FillRoundRect(p->surface,left,198*s,width*percent/100.0f,12*s,6*s,kAccent);
    char text[96];
    if (m->packTotalBytes)
        snprintf(text,sizeof(text),"%.1f / %.1f MB",m->packDownloadedBytes/1000000.0,m->packTotalBytes/1000000.0);
    else snprintf(text,sizeof(text),"%s",m->packDownloader==1?"Preparing your download...":"");
    DrawText(p->surface,left,246*s,16*s,text,kMuted,ALIGN_LEFT);
    snprintf(text,sizeof(text),"%d%%",percent);
    DrawText(p->surface,left+width,246*s,18*s,text,kText,ALIGN_RIGHT);
    if (m->packDownloader != 1) {
        const char *remaining = m->packStatus;
        for (int line = 0; line < 3 && *remaining; ++line) {
            char message[160];
            size_t count = strlen(remaining);
            if (count >= sizeof(message)) count = sizeof(message)-1;
            memcpy(message,remaining,count);message[count]=0;
            while (count && TextWidth(message,14*s)>width) message[--count]=0;
            if (remaining[count] && count) {
                size_t word=count;
                while(word && message[word]!=' ')--word;
                if(word)message[count=word]=0;
            }
            if (!count) break;
            DrawText(p->surface,left,(296+22*line)*s,14*s,message,kMuted,ALIGN_LEFT);
            remaining+=count;while(*remaining==' ')++remaining;
        }
    }
    if (m->packCanCancel || m->packDownloader != 1) {
        FillRoundRect(p->surface,left,(height-110)*s,width,46*s,8*s,WithAlpha(kAccent,.18f));
        DrawText(p->surface,left+width*.5f,(height-80)*s,17*s,
                 m->packDownloader==1?"Cancel":"Back to Mods",kText,ALIGN_CENTRE);
        DrawText(p->surface,left,(height-28)*s,13*s,
                 m->packDownloader==1?"Enter to cancel":"Enter / Esc to return",kMuted,ALIGN_LEFT);
    }
}

void OverlayPaint(OverlaySurface *surface, const OverlayModel *model,
                  const OverlayLayout *layout) {
    Painter painter = {surface, model, layout->scale};
    int rowCount = OverlayModelRowCount(model);
    uint8_t *solid;

    memset(surface->pixels, 0, (size_t)surface->w * (size_t)surface->h * 4u);
    PaintFrame(&painter, (float)OVERLAY_PANEL_W * layout->scale,
               (float)OverlayDesignHeight(rowCount) * layout->scale);
    if (model->packDownloader) PaintDownloader(&painter, rowCount);
    else {
        PaintHeader(&painter);
        PaintRows(&painter);
        PaintFooter(&painter, rowCount);
    }
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

/* ---- the touch pad ---- */

static void StrokePolygon(OverlaySurface *s, const float *points, int count,
                          float width, Color c) {
    int i;

    for (i = 0; i < count; i++) {
        const float *a = &points[i * 2];
        const float *b = &points[((i + 1) % count) * 2];

        DrawSegment(s, a[0], a[1], b[0], b[1], width, c);
    }
}

static void StrokeCircle(OverlaySurface *s, float cx, float cy, float radius,
                         float width, Color c) {
    enum { SEGMENTS = 28 };
    float points[SEGMENTS * 2];
    int i;

    for (i = 0; i < SEGMENTS; i++) {
        float angle = (float)i * 6.2831853f / (float)SEGMENTS;

        points[i * 2] = cx + cosf(angle) * radius;
        points[i * 2 + 1] = cy + sinf(angle) * radius;
    }
    StrokePolygon(s, points, SEGMENTS, width, c);
}

static void PaintTouchSymbol(OverlaySurface *s, int control, float cx, float cy,
                             float size, Color c) {
    const float stroke = size * 0.1f;
    const float r = size * 0.28f;

    switch (control) {
    case TOUCH_UP:
    case TOUCH_DOWN:
    case TOUCH_LEFT:
    case TOUCH_RIGHT: {
        float dx = control == TOUCH_LEFT ? -1.0f : control == TOUCH_RIGHT ? 1.0f : 0.0f;
        float dy = control == TOUCH_UP ? -1.0f : control == TOUCH_DOWN ? 1.0f : 0.0f;
        float tipX = cx + dx * r, tipY = cy + dy * r;
        float backX = cx - dx * r, backY = cy - dy * r;

        DrawSegment(s, backX - dy * r, backY + dx * r, tipX, tipY, stroke, c);
        DrawSegment(s, backX + dy * r, backY - dx * r, tipX, tipY, stroke, c);
        break;
    }
    case TOUCH_CROSS:
        DrawSegment(s, cx - r, cy - r, cx + r, cy + r, stroke, c);
        DrawSegment(s, cx - r, cy + r, cx + r, cy - r, stroke, c);
        break;
    case TOUCH_SQUARE: {
        const float p[8] = {cx - r, cy - r, cx + r, cy - r, cx + r, cy + r,
                            cx - r, cy + r};

        StrokePolygon(s, p, 4, stroke, c);
        break;
    }
    case TOUCH_CIRCLE:
        StrokeCircle(s, cx, cy, r * 1.1f, stroke, c);
        break;
    case TOUCH_TRIANGLE: {
        const float p[6] = {cx, cy - r * 1.15f, cx + r * 1.1f, cy + r * 0.8f,
                            cx - r * 1.1f, cy + r * 0.8f};

        StrokePolygon(s, p, 3, stroke, c);
        break;
    }
    default:
        break;
    }
}

static const char *TouchLabel(int control) {
    switch (control) {
    case TOUCH_L1: return "L1";
    case TOUCH_R1: return "R1";
    case TOUCH_START: return "START";
    case TOUCH_SETTINGS: return "MENU";
    default: return NULL;
    }
}

static Color TouchTint(int control) {
    switch (control) {
    case TOUCH_CROSS: return (Color){0.55f, 0.72f, 1.0f, 1.0f};
    case TOUCH_SQUARE: return (Color){1.0f, 0.62f, 0.82f, 1.0f};
    case TOUCH_CIRCLE: return (Color){1.0f, 0.5f, 0.48f, 1.0f};
    case TOUCH_TRIANGLE: return (Color){0.5f, 0.9f, 0.62f, 1.0f};
    default: return (Color){1.0f, 1.0f, 1.0f, 1.0f};
    }
}

void OverlayPaintTouchAtlas(OverlaySurface *surface, const TouchLayout *layout) {
    int control, pressed;

    memset(surface->pixels, 0, (size_t)surface->w * (size_t)surface->h * 4u);
    for (control = 0; control < TOUCH_CONTROL_COUNT; control++) {
        for (pressed = 0; pressed < 2; pressed++) {
            const TouchRect *rect = &layout->rect[control];
            const Color tint = TouchTint(control);
            const float w = rect->w, h = rect->h;
            const char *label = TouchLabel(control);
            int ox, oy;
            Color fill = WithAlpha(tint, pressed ? 0.55f : 0.14f);
            Color edge = WithAlpha(tint, pressed ? 0.95f : 0.45f);
            Color ink = WithAlpha(tint, pressed ? 1.0f : 0.7f);
            float radius = label != NULL ? h * 0.5f : w * 0.5f;
            float em = h * 0.42f;

            TouchAtlasOrigin(layout, control, pressed, &ox, &oy);
            FillRoundRect(surface, (float)ox, (float)oy, w, h, radius, fill);
            if (label == NULL) {
                StrokeCircle(surface, (float)ox + w * 0.5f, (float)oy + h * 0.5f,
                             w * 0.5f - 1.0f, 2.0f, edge);
            }
            if (label != NULL) {
                DrawText(surface, (float)ox + w * 0.5f,
                         (float)oy + h * 0.5f + em * 0.35f, em, label, ink,
                         ALIGN_CENTRE);
            } else {
                PaintTouchSymbol(surface, control, (float)ox + w * 0.5f,
                                 (float)oy + h * 0.5f, w, ink);
            }
        }
    }
}

#include "online/model.h"
void OverlayPaintOnline(OverlaySurface *surface,const OnlineView *view,float scale) {
    if(!surface||!surface->pixels||!OverlayFontReady())return;
    memset(surface->pixels,0,(size_t)surface->w*(size_t)surface->h*4u);
    uint8_t *solid=surface->pixels+(size_t)(surface->h-1)*(size_t)surface->w*4u;
    for(int x=0;x<surface->w;x++){solid[x*4]=solid[x*4+1]=solid[x*4+2]=solid[x*4+3]=255;}
    float s=scale;
    FillRoundRect(surface,0,0,(float)surface->w,(float)(surface->h-1),14*s,kPanel);
    if((OnlineViewJoining(view)||OnlineViewStarting(view))&&!view->panel){
        int starting=OnlineViewStarting(view),seconds=starting?(view->hud[1]+49)/50:view->takeoverSeconds;
        DrawText(surface,28*s,43*s,24*s,starting?"Get ready":"Joining race",kText,ALIGN_LEFT);
        if(!view->raceReady){
            DrawText(surface,28*s,98*s,23*s,"Synchronizing...",kAccent,ALIGN_LEFT);
            DrawText(surface,28*s,139*s,15*s,"Preparing the track and your car.",kMuted,ALIGN_LEFT);
        }else if(seconds>0){
            char text[80];snprintf(text,sizeof(text),starting?"Race starts in %d":"Taking control in %d",seconds);
            DrawText(surface,28*s,98*s,29*s,text,kAccent,ALIGN_LEFT);
            DrawText(surface,28*s,139*s,15*s,"Get ready. Your car is already on track.",kMuted,ALIGN_LEFT);
        }else{
            DrawText(surface,28*s,98*s,23*s,"Watching this race",kAccent,ALIGN_LEFT);
            DrawText(surface,28*s,139*s,15*s,"You'll join when a car becomes available.",kMuted,ALIGN_LEFT);
        }
        DrawText(surface,28*s,183*s,14*s,"ESC: menu",kMuted,ALIGN_LEFT);
        return;
    }
    if(view->racing&&!view->panel){
        char text[96];
        if(view->hud[9]==2)snprintf(text,sizeof(text),"FINISH   POS %d/%d",view->hud[4],view->hud[5]);
        else if(view->finishing)snprintf(text,sizeof(text),"RACE COMPLETE");
        else snprintf(text,sizeof(text),"POS %d/%d   LAP %d/%d",view->hud[4],view->hud[5],view->hud[2]>0?view->hud[2]:1,view->hud[3]);
        DrawText(surface,24*s,35*s,18*s,text,kText,ALIGN_LEFT);
        int ms=view->hud[6];if(ms<0)ms=0;snprintf(text,sizeof(text),"%d:%02d.%03d",ms/60000,ms/1000%60,ms%1000);
        DrawText(surface,24*s,72*s,26*s,text,kText,ALIGN_LEFT);
        if(view->hud[0]==1){snprintf(text,sizeof(text),"START %d",(view->hud[1]+49)/50);DrawText(surface,200*s,72*s,18*s,text,kAccent,ALIGN_LEFT);}
        char notification[256];snprintf(notification,sizeof(notification),"%s",view->finishing?"Results in a moment...":view->hud[9]==2?"Waiting for other drivers...":view->status[0]?view->status:"ESC: menu");
        if(TextWidth(notification,12*s)>272*s){
            while(notification[0]&&TextWidth(notification,12*s)>256*s){
                size_t n=strlen(notification);do{--n;}while(n&&((unsigned char)notification[n]&0xc0)==0x80);notification[n]=0;
            }
            size_t n=strlen(notification);if(n+3<sizeof(notification))memcpy(notification+n,"...",4);
        }
        DrawText(surface,24*s,108*s,12*s,notification,kMuted,ALIGN_LEFT);
        return;
    }
    DrawText(surface,32*s,45*s,29*s,view->title,kText,ALIGN_LEFT);
    DrawText(surface,32*s,82*s,14*s,view->subtitle,kMuted,ALIGN_LEFT);
    FillRoundRect(surface,32*s,112*s,536*s,1,0,WithAlpha(kText,0.1f));
    for(int row=0;row<view->count;row++){
        const OnlineRow *r=&view->rows[row];float top=(128+row*46)*s,base=top+29*s;
        if(row==view->selected){FillRoundRect(surface,16*s,top+3*s,568*s,40*s,9*s,WithAlpha(kAccent,0.16f));FillRoundRect(surface,20*s,top+13*s,3*s,20*s,1.5f*s,kAccent);}
        Color c=r->enabled||view->screen==OS_RESULTS?kText:WithAlpha(kMuted,0.6f);
        DrawText(surface,32*s,base,view->screen==OS_CHAT&&r->action==OA_NONE?12*s:17*s,r->label,c,ALIGN_LEFT);
        char value[80];snprintf(value,sizeof(value),"%s",r->value);
        float em=16*s;if(TextWidth(value,em)>310*s)em=12*s;
        while(value[0]&&TextWidth(value,em)>310*s){
            size_t n=strlen(value);do{--n;}while(n&&((unsigned char)value[n]&0xc0)==0x80);value[n]=0;
        }
        DrawText(surface,568*s,base,em,value,r->editable&&view->editing&&row==view->selected?kAccent:kMuted,ALIGN_RIGHT);
    }
    float bottom=(128+view->count*46+20)*s;
    if(view->screen==OS_ROOM||view->screen==OS_ARCADE){
        int first=view->chatCount>3?view->chatCount-3:0;
        for(int i=first;i<view->chatCount;i++){
            char line[100];snprintf(line,sizeof(line),"%.88s",view->messages[i]);
            DrawText(surface,32*s,bottom+16*s,12*s,line,kMuted,ALIGN_LEFT);bottom+=22*s;
        }
        bottom+=10*s;
    }
    char status[100];size_t n=strlen(view->status);if(n>86)n=86;memcpy(status,view->status,n);status[n]=0;
    DrawText(surface,32*s,bottom+16*s,13*s,status,kMuted,ALIGN_LEFT);
    DrawText(surface,32*s,bottom+40*s,12*s,view->editing?"Type to edit / Enter: done / Ctrl+A: replace":"Arrows / D-pad: select   Enter / A: confirm   Esc / B: back",kMuted,ALIGN_LEFT);
}
