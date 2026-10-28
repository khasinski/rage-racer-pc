#ifndef RAGE_PORT_TOUCH_MODEL_H
#define RAGE_PORT_TOUCH_MODEL_H

#include <stdint.h>

/* The on-screen pad of the phone build: where the buttons sit for a given
 * window and which of them a set of fingers holds. Pure rules, no SDL, so the
 * layout and the finger tracking are tested without a screen. */

typedef enum TouchControl {
    TOUCH_UP,
    TOUCH_DOWN,
    TOUCH_LEFT,
    TOUCH_RIGHT,
    TOUCH_CROSS,    /* gas, and confirm in menus */
    TOUCH_SQUARE,   /* brake */
    TOUCH_CIRCLE,
    TOUCH_TRIANGLE,
    TOUCH_L1,       /* shift down */
    TOUCH_R1,       /* shift up */
    TOUCH_START,
    TOUCH_SETTINGS, /* opens the graphics panel; not a pad button */
    TOUCH_CONTROL_COUNT
} TouchControl;

typedef struct TouchRect {
    float x, y, w, h;
} TouchRect;

typedef struct TouchLayout {
    TouchRect rect[TOUCH_CONTROL_COUNT];
    float unit; /* one button, in pixels */
} TouchLayout;

#define TOUCH_MAX_FINGERS 10

typedef struct TouchState {
    struct {
        int64_t id;
        int control; /* -1 for a finger on no control */
        int active;
    } finger[TOUCH_MAX_FINGERS];
} TouchState;

/* Places the pad inside the rectangle of the window that is safe to draw in
 * (clear of cut-outs and system bars), in window pixels. */
void TouchComputeLayout(int x, int y, int w, int h, TouchLayout *layout);

/* The control under a point, or -1. Buttons are hit a little beyond their
 * drawn edge so a thumb that lands between two still lands on one. */
int TouchControlAt(const TouchLayout *layout, float x, float y);

/* A finger arrived, moved or left. Moving onto another control hands the
 * finger over to it. Returns the controls newly pressed by this event as a
 * mask (bit n is control n), so one-shot controls can act on the press. */
uint32_t TouchFingerDown(TouchState *state, const TouchLayout *layout,
                         int64_t id, float x, float y);
uint32_t TouchFingerMove(TouchState *state, const TouchLayout *layout,
                         int64_t id, float x, float y);
void TouchFingerUp(TouchState *state, int64_t id);
void TouchReleaseAll(TouchState *state);

/* Every control some finger holds, bit n for control n. */
uint32_t TouchHeldMask(const TouchState *state);

/* Where a control's picture sits in the atlas the rasterizer paints: one tile
 * per control and state, tiles `tile` pixels square. */
int TouchAtlasTile(const TouchLayout *layout);
void TouchAtlasOrigin(const TouchLayout *layout, int control, int pressed,
                      int *x, int *y);
void TouchAtlasSize(const TouchLayout *layout, int *w, int *h);

#endif
