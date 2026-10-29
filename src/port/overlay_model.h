#ifndef RAGE_PORT_OVERLAY_MODEL_H
#define RAGE_PORT_OVERLAY_MODEL_H

#include "port_config.h"
#include "session_options.h"

/* The settings panel as data: its pages and rows, what is selected, what is
 * waiting to be applied, and where everything lies. Nothing here knows SDL or
 * the GPU, so every rule can be tested by feeding the model actions. */

typedef struct OverlayValues {
    RagePortConfig config;
    PortSessionOptions session;
} OverlayValues;

typedef struct OverlaySetting {
    const char *label;
    const char *help;
    int count;
    const char *const *names;
    int (*get)(const OverlayValues *values);
    void (*set)(OverlayValues *values, int index);
} OverlaySetting;

enum {
    OVERLAY_PAGE_GRAPHICS = 0,
    OVERLAY_PAGE_CONTROLS,
    OVERLAY_PAGE_AUDIO,
    OVERLAY_PAGE_VIEW,
    OVERLAY_PAGE_COUNT,
    /* Rows after a page's settings: resume the game, quit the game. */
    OVERLAY_ACTION_ROWS = 2,
    /* A graphics change rebuilds the presentation, so it waits for the player
     * to stop stepping through values. */
    OVERLAY_APPLY_DELAY_MS = 300,
};

const char *OverlayPageName(int page);
/* The settings of a page, or NULL for a page that does not exist. */
const OverlaySetting *OverlayPageSettings(int page, int *count);

typedef enum OverlayAction {
    OVERLAY_NONE = 0,
    OVERLAY_UP,
    OVERLAY_DOWN,
    OVERLAY_LEFT,
    OVERLAY_RIGHT,
    OVERLAY_PAGE_PREV,
    OVERLAY_PAGE_NEXT,
    OVERLAY_ACTIVATE,
    OVERLAY_CLOSE,
} OverlayAction;

typedef struct OverlayModel {
    OverlayValues values;
    int open;
    int page;
    int selected;
    int dirty;         /* the panel needs repainting */
    int configPending; /* values.config differs from what the game runs */
    int sessionPending;
    unsigned changedAtMs;
    int quitPending;
    int flushInput;    /* closed since the caller last asked */
} OverlayModel;

void OverlayModelOpen(OverlayModel *model, const RagePortConfig *config,
                      const PortSessionOptions *session);
void OverlayModelClose(OverlayModel *model);

int OverlayModelSettingCount(const OverlayModel *model);
int OverlayModelRowCount(const OverlayModel *model);

void OverlayModelSelect(OverlayModel *model, int row);
void OverlayModelSwitchPage(OverlayModel *model, int page);
void OverlayModelDo(OverlayModel *model, OverlayAction action,
                    unsigned nowMs);

/* A pointer at (x, y) in design units from the panel's top left. A click on
 * a tab, a row or the area around the panel acts; moving only selects. */
void OverlayModelPointer(OverlayModel *model, float x, float y, int click,
                         unsigned nowMs);

typedef struct OverlayPending {
    int config;
    int session;
} OverlayPending;

/* What the caller should apply now. Session options are due at once; a
 * graphics change is due once it has been left alone for the delay, or when
 * the panel is closed. Returns, and forgets, what is due. */
OverlayPending OverlayModelTakePending(OverlayModel *model, unsigned nowMs);

/* Adopts the configuration the game ended up running. */
void OverlayModelAdopt(OverlayModel *model, const RagePortConfig *active);

int OverlayModelTakeClosed(OverlayModel *model);
int OverlayModelTakeQuit(OverlayModel *model);

/* ---- geometry, in design units that scale with the window ---- */

enum {
    OVERLAY_PANEL_W = 600,
    OVERLAY_PAD = 32,
    OVERLAY_HEADER_H = 128,
    OVERLAY_ROW_H = 46,
    OVERLAY_ACTION_GAP = 14,
    OVERLAY_FOOTER_H = 74,
    OVERLAY_VALUE_W = 250,
    OVERLAY_TAB_TOP = 62,
    OVERLAY_TAB_H = 34,
    OVERLAY_REFERENCE_H = 820,
};

typedef struct OverlayLayout {
    float scale;
    int x, y, w, h; /* the panel rectangle in window pixels */
} OverlayLayout;

typedef struct OverlayRect {
    float x, y, w, h;
} OverlayRect;

int OverlayDesignHeight(int rowCount);
void OverlayComputeLayout(int rowCount, int windowW, int windowH,
                          OverlayLayout *layout);
/* Top of a row; the action rows sit below a gap that follows the settings. */
float OverlayRowTop(int settingCount, int row);
OverlayRect OverlayTabRect(int page);
/* Centre of the value column, where the arrows either side step. */
float OverlayValueCentre(void);

#endif
