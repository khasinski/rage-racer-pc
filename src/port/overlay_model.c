#include "overlay_model.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/* ---- settings ---- */

/* A getter and setter pair over one value. */
#define ACCESSORS(Name, read, write)                                  \
    static int Get##Name(const OverlayValues *v) { return (read); }   \
    static void Set##Name(OverlayValues *v, int i) { write; }

static int NearestIndex(const float *values, int count, float value) {
    int best = 0;
    int i;

    for (i = 1; i < count; i++) {
        if (fabsf(values[i] - value) < fabsf(values[best] - value)) best = i;
    }
    return best;
}

static int ClampIndex(int index, int count) {
    if (index < 0) return 0;
    if (index >= count) return count - 1;
    return index;
}

#define COUNT_OF(array) ((int)(sizeof(array) / sizeof((array)[0])))
#define SETTING(label, help, names, Name) \
    {label, help, COUNT_OF(names), names, Get##Name, Set##Name}

static const char *const kSwitchNames[] = {"Off", "On"};

/* graphics */

static const char *const kRendererNames[] = {"Classic", "Modern"};
ACCESSORS(Renderer, v->config.renderer == RAGE_RENDERER_MODERN,
          v->config.renderer = i ? RAGE_RENDERER_MODERN : RAGE_RENDERER_CLASSIC)

static const float kScales[] = {1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f};
static const char *const kScaleNames[] = {"1x", "2x", "3x", "4x", "6x", "8x"};
ACCESSORS(Scale, NearestIndex(kScales, COUNT_OF(kScales), v->config.modernInternalScale),
          v->config.modernInternalScale = kScales[i])

static const char *const kAspectNames[] = {"Match window", "Classic 4:3",
                                           "Widescreen 16:9"};
ACCESSORS(Aspect, (int)v->config.modernAspect,
          v->config.modernAspect = (RageModernAspect)i)

static const float kFpsValues[] = {(float)RAGE_MODERN_FPS_VSYNC,
                                   (float)RAGE_MODERN_FPS_LOGIC, 60.0f,
                                   120.0f, 144.0f};
static const char *const kFpsNames[] = {"Display sync", "Original timing",
                                        "60 fps", "120 fps", "144 fps"};
ACCESSORS(Fps, NearestIndex(kFpsValues, COUNT_OF(kFpsValues), (float)v->config.modernFps),
          v->config.modernFps = (int)kFpsValues[i])

static const char *const kFilterNames[] = {"Crisp pixels", "Smooth"};
ACCESSORS(Filter, v->config.modernTextureFilterLinear != 0,
          v->config.modernTextureFilterLinear = i)

static const int kTextureUpscales[] = {1, 2, 4};
static const char *const kTextureUpscaleNames[] = {"Off", "2x", "4x"};
ACCESSORS(TextureUpscale,
          v->config.modernTextureUpscale >= 4
              ? 2
              : (v->config.modernTextureUpscale >= 2 ? 1 : 0),
          v->config.modernTextureUpscale = kTextureUpscales[i])

static const char *const kPostNames[] = {"Off", "FXAA"};
ACCESSORS(Post, v->config.modernPost == RAGE_MODERN_POST_FXAA,
          v->config.modernPost = i ? RAGE_MODERN_POST_FXAA : RAGE_MODERN_POST_NONE)

static const char *const kGradingNames[] = {"Off", "Vibrant"};
ACCESSORS(Grading, v->config.modernGrading != 0, v->config.modernGrading = i)

static const char *const kShadowNames[] = {"Off", "Standard", "High"};
ACCESSORS(Shadows, v->config.modernShadows, v->config.modernShadows = i)

ACCESSORS(Ssao, v->config.modernSsao != 0, v->config.modernSsao = i)
ACCESSORS(CarLights, v->config.carLights != 0, v->config.carLights = i)

static const float kDistances[] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
static const char *const kDistanceNames[] = {"0.5x", "1x", "1.5x",
                                             "2x",   "3x", "4x"};
ACCESSORS(Distance, NearestIndex(kDistances, COUNT_OF(kDistances), v->config.modernDrawDistance),
          v->config.modernDrawDistance = kDistances[i])

static const OverlaySetting kGraphics[] = {
    SETTING("Renderer", "Classic replays the original PlayStation image; Modern draws the scene natively.",
            kRendererNames, Renderer),
    SETTING("Render resolution", "Higher values increase image detail and GPU load.",
            kScaleNames, Scale),
    SETTING("Aspect ratio", "Menus keep their original layout.", kAspectNames, Aspect),
    SETTING("Frame rate", "Display sync follows your monitor. Original timing disables interpolation.",
            kFpsNames, Fps),
    SETTING("Texture filtering", "Choose crisp pixels or smoother transitions.",
            kFilterNames, Filter),
    SETTING("Texture upscaling", "Edge-directed upscaling for 3D car, track and sky textures.",
            kTextureUpscaleNames, TextureUpscale),
    SETTING("Anti-aliasing", "FXAA softens jagged edges.", kPostNames, Post),
    SETTING("Color grading", "A more vibrant palette.", kGradingNames, Grading),
    SETTING("Shadows", "Sun shadow detail. High uses the sharpest shadow maps.",
            kShadowNames, Shadows),
    SETTING("Ambient occlusion", "Soft contact shadows in creases and under cars.",
            kSwitchNames, Ssao),
    SETTING("Car lights", "Headlights, tail lamps and brake lights. On follows day, night and tunnels.",
            kSwitchNames, CarLights),
    SETTING("Draw distance", "Values above 1x may expose unfinished track geometry.",
            kDistanceNames, Distance),
};

/* controls */

ACCESSORS(Analog, v->session.analog != 0, v->session.analog = i)
ACCESSORS(Wheel, v->session.wheel != 0, v->session.wheel = i)

static const float kLinearity[] = {0.5f, 1.0f, 1.5f, 2.0f};
static const char *const kLinearityNames[] = {"0.5", "1.0", "1.5", "2.0"};
ACCESSORS(Linearity, NearestIndex(kLinearity, COUNT_OF(kLinearity), v->session.steeringLinearity),
          v->session.steeringLinearity = kLinearity[i])

static const char *const kGainNames[] = {
    "Off",  "5%",  "10%", "15%", "20%", "25%", "30%", "35%", "40%", "45%", "50%",
    "55%",  "60%", "65%", "70%", "75%", "80%", "85%", "90%", "95%", "100%"};
ACCESSORS(Gain, ClampIndex((v->session.forceFeedbackGain + 2) / 5, COUNT_OF(kGainNames)),
          v->session.forceFeedbackGain = i * 5)
ACCESSORS(Feedback, v->session.forceFeedback != 0, v->session.forceFeedback = i)

static const char *const kTickRateNames[] = {"30 Hz", "60 Hz"};
ACCESSORS(TickRate, v->session.tickMultiplier >= 2,
          v->session.tickMultiplier = i ? 2 : 1)

static const OverlaySetting kControls[] = {
    SETTING("Analog steering", "Use a gamepad stick or wheel. Keyboard bindings stay in rage-port.ini.",
            kSwitchNames, Analog),
    SETTING("Racing wheel", "A wheel replaces the gamepad while it is connected.",
            kSwitchNames, Wheel),
    SETTING("Steering feel", "Higher values soften the middle of the stick. 0.5 is the default.",
            kLinearityNames, Linearity),
    SETTING("Force feedback", "Steering forces on a wheel, rumble on a gamepad.",
            kSwitchNames, Feedback),
    SETTING("Feedback strength", "Remembered with the pause-menu force feedback switch.",
            kGainNames, Gain),
    SETTING("Race physics rate", "30 Hz is the original PlayStation cadence; 60 Hz steps race physics every field.",
            kTickRateNames, TickRate),
};

/* audio */

static const char *const kVolumeNames[] = {
    "Off", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", "10", "11", "12", "13", "14", "15"};
ACCESSORS(Music, ClampIndex(v->session.music, COUNT_OF(kVolumeNames)),
          v->session.music = i)
ACCESSORS(Effects, ClampIndex(v->session.effects, COUNT_OF(kVolumeNames)),
          v->session.effects = i)

static const OverlaySetting kAudio[] = {
    SETTING("Music", "In-game music. Saved with the game, same as the sound menu.",
            kVolumeNames, Music),
    SETTING("Effects", "Engine, skids and menu sounds. Saved with the game.",
            kVolumeNames, Effects),
};

/* view */

static const char *const kAnchorNames[] = {"Edges", "Center"};
ACCESSORS(Anchor, v->session.hudEdges ? 0 : 1, v->session.hudEdges = i == 0)
ACCESSORS(LapTimes, v->session.showLapTimes != 0, v->session.showLapTimes = i)
ACCESSORS(TimeLimit, v->session.showTimeLimit != 0, v->session.showTimeLimit = i)

static const float kChaseDistance[] = {0.75f, 1.0f, 1.5f, 2.0f, 3.0f};
static const char *const kChaseDistanceNames[] = {"0.75x", "1x", "1.5x", "2x", "3x"};
ACCESSORS(ChaseDistance, NearestIndex(kChaseDistance, COUNT_OF(kChaseDistance), v->session.chaseDistance),
          v->session.chaseDistance = kChaseDistance[i])

static const float kChaseHeight[] = {0.75f, 1.0f, 1.25f, 1.5f, 2.0f};
static const char *const kChaseHeightNames[] = {"0.75x", "1x", "1.25x", "1.5x", "2x"};
ACCESSORS(ChaseHeight, NearestIndex(kChaseHeight, COUNT_OF(kChaseHeight), v->session.chaseHeight),
          v->session.chaseHeight = kChaseHeight[i])

/* Lookahead 1 is about 30 degrees at full lock. These steps keep a tenth of
 * that and a twentieth of the old half step, about 3 and 0.7 degrees. */
static const float kLookahead[] = {0.0f, 0.025f, 0.1f};
static const char *const kLookaheadNames[] = {"Off", "Half", "Full"};
ACCESSORS(Lookahead, NearestIndex(kLookahead, COUNT_OF(kLookahead), v->session.chaseLookahead),
          v->session.chaseLookahead = kLookahead[i])

static const OverlaySetting kView[] = {
    SETTING("HUD alignment", "Edges follow a 16:9 picture. Center keeps the original layout.",
            kAnchorNames, Anchor),
    SETTING("Lap times", "Show the lap-time stack on the race HUD.",
            kSwitchNames, LapTimes),
    SETTING("Time limit", "Show the remaining time on the race HUD.",
            kSwitchNames, TimeLimit),
    SETTING("Chase distance", "How far behind the car the chase camera sits.",
            kChaseDistanceNames, ChaseDistance),
    SETTING("Chase height", "How high above the car the chase camera sits.",
            kChaseHeightNames, ChaseHeight),
    SETTING("Corner look", "Slightly turns the chase camera into the steering.",
            kLookaheadNames, Lookahead),
};

static const char *const kPageNames[OVERLAY_PAGE_COUNT] = {
    "Graphics", "Controls", "Audio", "View"};

const char *OverlayPageName(int page) {
    return page >= 0 && page < OVERLAY_PAGE_COUNT ? kPageNames[page] : "";
}

const OverlaySetting *OverlayPageSettings(int page, int *count) {
    switch (page) {
    case OVERLAY_PAGE_GRAPHICS:
        *count = COUNT_OF(kGraphics);
        return kGraphics;
    case OVERLAY_PAGE_CONTROLS:
        *count = COUNT_OF(kControls);
        return kControls;
    case OVERLAY_PAGE_AUDIO:
        *count = COUNT_OF(kAudio);
        return kAudio;
    case OVERLAY_PAGE_VIEW:
        *count = COUNT_OF(kView);
        return kView;
    default:
        *count = 0;
        return NULL;
    }
}

/* ---- state ---- */

int OverlayModelSettingCount(const OverlayModel *model) {
    int count = 0;

    (void)OverlayPageSettings(model->page, &count);
    return count;
}

int OverlayModelRowCount(const OverlayModel *model) {
    return OverlayModelSettingCount(model) + OVERLAY_ACTION_ROWS;
}

void OverlayModelOpen(OverlayModel *model, const RagePortConfig *config,
                      const PortSessionOptions *session) {
    *model = (OverlayModel){0};
    model->values.config = *config;
    model->values.session = *session;
    model->open = 1;
    model->dirty = 1;
}

void OverlayModelClose(OverlayModel *model) {
    model->open = 0;
    model->flushInput = 1;
}

void OverlayModelSelect(OverlayModel *model, int row) {
    if (row < 0 || row >= OverlayModelRowCount(model) || row == model->selected)
        return;
    model->selected = row;
    model->dirty = 1;
}

void OverlayModelSwitchPage(OverlayModel *model, int page) {
    if (page < 0 || page >= OVERLAY_PAGE_COUNT || page == model->page) return;
    model->page = page;
    model->selected = 0;
    model->dirty = 1;
}

static void Step(OverlayModel *model, int row, int direction, unsigned nowMs) {
    int count = 0;
    const OverlaySetting *setting =
        OverlayPageSettings(model->page, &count) + row;
    RagePortConfig beforeConfig = model->values.config;
    int index = setting->get(&model->values);

    index = (index + direction + setting->count) % setting->count;
    setting->set(&model->values, index);
    if (memcmp(&beforeConfig, &model->values.config, sizeof(beforeConfig)) != 0) {
        model->configPending = 1;
        model->changedAtMs = nowMs;
    } else {
        model->sessionPending = 1;
    }
    model->dirty = 1;
}

static void Activate(OverlayModel *model, int row, unsigned nowMs) {
    int settings = OverlayModelSettingCount(model);

    if (row == settings) {
        OverlayModelClose(model);
    } else if (row == settings + 1) {
        model->quitPending = 1;
    } else if (row >= 0 && row < settings) {
        Step(model, row, 1, nowMs);
    }
}

void OverlayModelDo(OverlayModel *model, OverlayAction action,
                    unsigned nowMs) {
    int rows = OverlayModelRowCount(model);
    int onSetting = model->selected < OverlayModelSettingCount(model);

    switch (action) {
    case OVERLAY_UP:
        OverlayModelSelect(model, (model->selected + rows - 1) % rows);
        break;
    case OVERLAY_DOWN:
        OverlayModelSelect(model, (model->selected + 1) % rows);
        break;
    case OVERLAY_LEFT:
        if (onSetting) Step(model, model->selected, -1, nowMs);
        break;
    case OVERLAY_RIGHT:
        if (onSetting) Step(model, model->selected, 1, nowMs);
        break;
    case OVERLAY_PAGE_PREV:
        OverlayModelSwitchPage(
            model, (model->page + OVERLAY_PAGE_COUNT - 1) % OVERLAY_PAGE_COUNT);
        break;
    case OVERLAY_PAGE_NEXT:
        OverlayModelSwitchPage(model, (model->page + 1) % OVERLAY_PAGE_COUNT);
        break;
    case OVERLAY_ACTIVATE:
        Activate(model, model->selected, nowMs);
        break;
    case OVERLAY_CLOSE:
        OverlayModelClose(model);
        break;
    case OVERLAY_NONE:
        break;
    }
}

/* ---- geometry ---- */

int OverlayDesignHeight(int rowCount) {
    return OVERLAY_HEADER_H + rowCount * OVERLAY_ROW_H + OVERLAY_ACTION_GAP +
           OVERLAY_FOOTER_H;
}

void OverlayComputeLayout(int rowCount, int windowW, int windowH,
                          OverlayLayout *layout) {
    float scale = (float)windowH / (float)OVERLAY_REFERENCE_H;
    float widthLimit = (float)windowW * 0.94f / (float)OVERLAY_PANEL_W;

    if (scale > widthLimit) scale = widthLimit;
    if (scale > 2.5f) scale = 2.5f;
    if (scale < 0.5f) scale = 0.5f;
    scale = floorf(scale * 8.0f) / 8.0f; /* no repaint on every resize pixel */
    layout->scale = scale;
    layout->w = (int)ceilf((float)OVERLAY_PANEL_W * scale);
    layout->h = (int)ceilf((float)OverlayDesignHeight(rowCount) * scale);
    layout->x = (windowW - layout->w) / 2;
    layout->y = (windowH - layout->h) / 2;
}

float OverlayRowTop(int settingCount, int row) {
    return (float)(OVERLAY_HEADER_H + row * OVERLAY_ROW_H +
                   (row >= settingCount ? OVERLAY_ACTION_GAP : 0));
}

OverlayRect OverlayTabRect(int page) {
    float width = (float)(OVERLAY_PANEL_W - 2 * OVERLAY_PAD) /
                  (float)OVERLAY_PAGE_COUNT;

    return (OverlayRect){(float)OVERLAY_PAD + width * (float)page,
                         (float)OVERLAY_TAB_TOP, width,
                         (float)OVERLAY_TAB_H};
}

float OverlayValueCentre(void) {
    return (float)(OVERLAY_PANEL_W - OVERLAY_PAD) - (float)OVERLAY_VALUE_W * 0.5f;
}

/* ---- pointer ---- */

static int TabAt(float x, float y) {
    int page;

    for (page = 0; page < OVERLAY_PAGE_COUNT; page++) {
        OverlayRect tab = OverlayTabRect(page);

        if (x >= tab.x && x < tab.x + tab.w && y >= tab.y &&
            y < tab.y + tab.h) {
            return page;
        }
    }
    return -1;
}

static int RowAt(const OverlayModel *model, float x, float y) {
    int rows = OverlayModelRowCount(model);
    int settings = OverlayModelSettingCount(model);
    int row;

    if (x < 0.0f || x > (float)OVERLAY_PANEL_W) return -1;
    for (row = 0; row < rows; row++) {
        float top = OverlayRowTop(settings, row);

        if (y >= top && y < top + (float)OVERLAY_ROW_H) return row;
    }
    return -1;
}

void OverlayModelPointer(OverlayModel *model, float x, float y, int click,
                         unsigned nowMs) {
    int tab = TabAt(x, y);
    int row;

    if (tab >= 0) {
        if (click) OverlayModelSwitchPage(model, tab);
        return;
    }
    row = RowAt(model, x, y);
    if (row >= 0) OverlayModelSelect(model, row);
    if (!click) return;
    if (row < 0) {
        OverlayModelClose(model);
    } else if (row < OverlayModelSettingCount(model)) {
        Step(model, row, x < OverlayValueCentre() ? -1 : 1, nowMs);
    } else {
        Activate(model, row, nowMs);
    }
}

/* ---- applying ---- */

OverlayPending OverlayModelTakePending(OverlayModel *model, unsigned nowMs) {
    OverlayPending due = {0, 0};

    if (model->sessionPending) {
        due.session = 1;
        model->sessionPending = 0;
    }
    if (model->configPending &&
        (!model->open || nowMs - model->changedAtMs >= OVERLAY_APPLY_DELAY_MS)) {
        due.config = 1;
        model->configPending = 0;
    }
    return due;
}

void OverlayModelAdopt(OverlayModel *model, const RagePortConfig *active) {
    if (active != NULL) model->values.config = *active;
    model->dirty = 1;
}

int OverlayModelTakeClosed(OverlayModel *model) {
    int closed = model->flushInput && !model->open;

    if (closed) model->flushInput = 0;
    return closed;
}

int OverlayModelTakeQuit(OverlayModel *model) {
    int quit = model->quitPending;

    model->quitPending = 0;
    return quit;
}
