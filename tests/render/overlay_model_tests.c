/* The settings panel's rules, driven without a window: every row reads back
 * what it wrote, navigation wraps and resets, graphics changes wait for the
 * player to stop stepping while session options apply at once, and the
 * pointer lands on the rows and tabs the layout puts there. */
#include <stdio.h>
#include <string.h>

#include "overlay_model.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", \
    __LINE__, #x); return 1; } } while (0)

static void Defaults(RagePortConfig *config, PortSessionOptions *session) {
    memset(config, 0, sizeof(*config));
    memset(session, 0, sizeof(*session));
    config->renderer = RAGE_RENDERER_MODERN;
    config->modernInternalScale = 4.0f;
    config->modernDrawDistance = 1.0f;
    config->modernShadows = 2;
    session->steeringLinearity = 0.5f;
    session->forceFeedbackGain = 55;
    session->music = 15;
    session->effects = 15;
    session->chaseDistance = 1.0f;
    session->chaseHeight = 1.0f;
}

static void Open(OverlayModel *model) {
    RagePortConfig config;
    PortSessionOptions session;

    Defaults(&config, &session);
    OverlayModelOpen(model, &config, &session);
}

/* Every value a row offers must read back as itself, whatever the page. */
static int TestEverySettingRoundTrips(void) {
    int page;

    for (page = 0; page < OVERLAY_PAGE_COUNT; page++) {
        int count = 0;
        const OverlaySetting *settings = OverlayPageSettings(page, &count);
        int row;

        CHECK(settings != NULL && count > 0);
        for (row = 0; row < count; row++) {
            OverlayModel model;
            int index;

            Open(&model);
            CHECK(settings[row].count > 0 && settings[row].names != NULL);
            for (index = 0; index < settings[row].count; index++) {
                settings[row].set(&model.values, index);
                if (settings[row].get(&model.values) != index) {
                    fprintf(stderr, "page %d row %d (%s) value %d reads %d\n",
                            page, row, settings[row].label, index,
                            settings[row].get(&model.values));
                    return 1;
                }
                CHECK(settings[row].names[index] != NULL);
            }
        }
    }
    CHECK(OverlayPageSettings(OVERLAY_PAGE_COUNT, &(int){0}) == NULL);
    return 0;
}

static int TestNavigation(void) {
    OverlayModel model;
    int rows;

    Open(&model);
    rows = OverlayModelRowCount(&model);
    CHECK(model.open && model.page == OVERLAY_PAGE_GRAPHICS &&
          model.selected == 0);
    CHECK(rows == OverlayModelSettingCount(&model) + OVERLAY_ACTION_ROWS);

    OverlayModelDo(&model, OVERLAY_UP, 0);
    CHECK(model.selected == rows - 1);
    OverlayModelDo(&model, OVERLAY_DOWN, 0);
    CHECK(model.selected == 0);

    /* Paging wraps and starts the new page at its first row. */
    OverlayModelDo(&model, OVERLAY_DOWN, 0);
    OverlayModelDo(&model, OVERLAY_PAGE_PREV, 0);
    CHECK(model.page == OVERLAY_PAGE_VIEW && model.selected == 0);
    CHECK(OverlayModelRowCount(&model) != rows);
    OverlayModelDo(&model, OVERLAY_PAGE_NEXT, 0);
    CHECK(model.page == OVERLAY_PAGE_GRAPHICS);

    /* The action rows take no left or right. */
    model.selected = OverlayModelSettingCount(&model);
    OverlayModelDo(&model, OVERLAY_RIGHT, 0);
    CHECK(!model.configPending && !model.sessionPending);
    return 0;
}

static int TestStepsAndPending(void) {
    OverlayModel model;
    OverlayPending due;
    int before;

    Open(&model);
    before = OverlayPageSettings(OVERLAY_PAGE_GRAPHICS, &(int){0})[1].get(
        &model.values);
    model.selected = 1; /* render resolution */
    OverlayModelDo(&model, OVERLAY_RIGHT, 1000);
    CHECK(OverlayPageSettings(OVERLAY_PAGE_GRAPHICS, &(int){0})[1].get(
              &model.values) == (before + 1) % 6);
    OverlayModelDo(&model, OVERLAY_LEFT, 1100);
    OverlayModelDo(&model, OVERLAY_LEFT, 1200);
    CHECK(model.configPending && !model.sessionPending);

    /* Still being stepped: nothing is due. After the delay it is, once. */
    due = OverlayModelTakePending(&model, 1200 + OVERLAY_APPLY_DELAY_MS - 1);
    CHECK(!due.config && !due.session && model.configPending);
    due = OverlayModelTakePending(&model, 1200 + OVERLAY_APPLY_DELAY_MS);
    CHECK(due.config && !model.configPending);
    due = OverlayModelTakePending(&model, 99999);
    CHECK(!due.config);

    /* Closing the panel applies a waiting change without the delay. */
    OverlayModelDo(&model, OVERLAY_RIGHT, 5000);
    OverlayModelDo(&model, OVERLAY_CLOSE, 5001);
    due = OverlayModelTakePending(&model, 5001);
    CHECK(due.config);

    /* Session options apply on the next take, whatever the delay. */
    Open(&model);
    OverlayModelDo(&model, OVERLAY_PAGE_NEXT, 0); /* controls */
    OverlayModelDo(&model, OVERLAY_RIGHT, 7000);
    CHECK(model.sessionPending && !model.configPending);
    due = OverlayModelTakePending(&model, 7000);
    CHECK(due.session && !due.config && !model.sessionPending);
    return 0;
}

static int TestAdoptingTheRunningConfig(void) {
    OverlayModel model;
    RagePortConfig running;
    PortSessionOptions session;

    Open(&model);
    Defaults(&running, &session);
    running.modernInternalScale = 2.0f;
    model.dirty = 0;
    OverlayModelAdopt(&model, &running);
    CHECK(model.values.config.modernInternalScale == 2.0f && model.dirty);
    model.dirty = 0;
    OverlayModelAdopt(&model, NULL);
    CHECK(model.dirty);
    return 0;
}

static int TestClosingAndQuitting(void) {
    OverlayModel model;
    int resume;

    Open(&model);
    resume = OverlayModelSettingCount(&model);
    model.selected = resume;
    OverlayModelDo(&model, OVERLAY_ACTIVATE, 0);
    CHECK(!model.open && OverlayModelTakeClosed(&model));
    CHECK(!OverlayModelTakeClosed(&model));

    Open(&model);
    model.selected = resume + 1;
    OverlayModelDo(&model, OVERLAY_ACTIVATE, 0);
    CHECK(model.open && OverlayModelTakeQuit(&model));
    CHECK(!OverlayModelTakeQuit(&model));

    Open(&model);
    CHECK(!OverlayModelTakeClosed(&model));
    OverlayModelDo(&model, OVERLAY_CLOSE, 0);
    CHECK(OverlayModelTakeClosed(&model));
    return 0;
}

static int TestPointer(void) {
    OverlayModel model;
    OverlayRect tab;
    float centre = OverlayValueCentre();
    int settings;
    int before;

    Open(&model);
    settings = OverlayModelSettingCount(&model);

    /* Hovering a row selects it; moving over nothing changes nothing. */
    OverlayModelPointer(&model, 100.0f, OverlayRowTop(settings, 3) + 5.0f, 0, 0);
    CHECK(model.selected == 3 && !model.configPending);
    OverlayModelPointer(&model, 100.0f, 5.0f, 0, 0);
    CHECK(model.selected == 3);

    /* Left and right of the value column step either way. */
    before = OverlayPageSettings(0, &(int){0})[3].get(&model.values);
    OverlayModelPointer(&model, centre + 40.0f, OverlayRowTop(settings, 3) + 5.0f, 1, 10);
    CHECK(OverlayPageSettings(0, &(int){0})[3].get(&model.values) ==
          (before + 1) % 5);
    OverlayModelPointer(&model, centre - 40.0f, OverlayRowTop(settings, 3) + 5.0f, 1, 20);
    CHECK(OverlayPageSettings(0, &(int){0})[3].get(&model.values) == before);

    /* A click on a tab changes page; hovering does not. */
    tab = OverlayTabRect(OVERLAY_PAGE_AUDIO);
    OverlayModelPointer(&model, tab.x + tab.w * 0.5f, tab.y + tab.h * 0.5f, 0, 0);
    CHECK(model.page == OVERLAY_PAGE_GRAPHICS);
    OverlayModelPointer(&model, tab.x + tab.w * 0.5f, tab.y + tab.h * 0.5f, 1, 0);
    CHECK(model.page == OVERLAY_PAGE_AUDIO);

    /* The resume row closes, quit asks to quit, outside closes. */
    settings = OverlayModelSettingCount(&model);
    OverlayModelPointer(&model, 100.0f, OverlayRowTop(settings, settings + 1) + 5.0f, 1, 0);
    CHECK(model.open && model.quitPending);
    OverlayModelPointer(&model, 100.0f, OverlayRowTop(settings, settings) + 5.0f, 1, 0);
    CHECK(!model.open);
    Open(&model);
    OverlayModelPointer(&model, -50.0f, 10.0f, 1, 0);
    CHECK(!model.open && OverlayModelTakeClosed(&model));
    return 0;
}

/* The middle of every row and tab must hit that row or tab. */
static int TestHitsFollowTheLayout(void) {
    int page;

    for (page = 0; page < OVERLAY_PAGE_COUNT; page++) {
        OverlayModel model;
        OverlayRect tab = OverlayTabRect(page);
        int row;

        Open(&model);
        OverlayModelPointer(&model, tab.x + tab.w * 0.5f,
                            tab.y + tab.h * 0.5f, 1, 0);
        CHECK(model.page == page);
        for (row = 0; row < OverlayModelRowCount(&model); row++) {
            float top = OverlayRowTop(OverlayModelSettingCount(&model), row);

            OverlayModelPointer(&model, 100.0f,
                                top + (float)OVERLAY_ROW_H * 0.5f, 0, 0);
            CHECK(model.selected == row);
        }
    }
    return 0;
}

static int TestLayout(void) {
    OverlayLayout layout;
    int rows = 12;

    OverlayComputeLayout(rows, 1280, OVERLAY_REFERENCE_H, &layout);
    CHECK(layout.scale >= 0.99f && layout.scale <= 1.01f);
    CHECK(layout.w == OVERLAY_PANEL_W && layout.h == OverlayDesignHeight(rows));
    CHECK(layout.x == (1280 - layout.w) / 2 && layout.y == (820 - layout.h) / 2);

    /* A narrow window limits the scale by width, never below half. */
    OverlayComputeLayout(rows, 400, 2000, &layout);
    CHECK(layout.scale <= 400.0f * 0.94f / (float)OVERLAY_PANEL_W + 0.01f);
    CHECK(layout.scale >= 0.5f);
    OverlayComputeLayout(rows, 100, 100, &layout);
    CHECK(layout.scale == 0.5f);
    OverlayComputeLayout(rows, 8000, 8000, &layout);
    CHECK(layout.scale == 2.5f);

    /* The action rows sit below a gap; settings rows do not. */
    CHECK(OverlayRowTop(10, 10) - OverlayRowTop(10, 9) ==
          (float)(OVERLAY_ROW_H + OVERLAY_ACTION_GAP));
    CHECK(OverlayRowTop(10, 9) - OverlayRowTop(10, 8) == (float)OVERLAY_ROW_H);
    return 0;
}

int main(void) {
    if (TestEverySettingRoundTrips()) return 1;
    if (TestNavigation()) return 1;
    if (TestStepsAndPending()) return 1;
    if (TestAdoptingTheRunningConfig()) return 1;
    if (TestClosingAndQuitting()) return 1;
    if (TestPointer()) return 1;
    if (TestHitsFollowTheLayout()) return 1;
    if (TestLayout()) return 1;
    puts("overlay model rules hold");
    return 0;
}
