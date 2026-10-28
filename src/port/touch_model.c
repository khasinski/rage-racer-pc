#include "touch_model.h"

#include <math.h>
#include <string.h>

static TouchRect Square(float cx, float cy, float size) {
    return (TouchRect){cx - size * 0.5f, cy - size * 0.5f, size, size};
}

void TouchComputeLayout(int originX, int originY, int areaW, int areaH,
                        TouchLayout *layout) {
    const float w = (float)areaW, h = (float)areaH;
    int control;
    /* A button is an eleventh of the short side, but never so big that the two
     * clusters meet on a narrow window. */
    float unit = (h < w ? h : w) / 11.0f;
    const float margin = unit * 0.4f;
    float dpadX, dpadY, faceX, faceY, pitch;

    if (unit * 10.0f > w * 0.5f) unit = w * 0.05f;
    pitch = unit * 1.15f;
    layout->unit = unit;
    dpadX = margin + pitch * 1.5f;
    dpadY = h - margin - pitch * 1.5f;
    faceX = w - margin - pitch * 1.5f;
    faceY = h - margin - pitch * 1.5f;

    layout->rect[TOUCH_UP] = Square(dpadX, dpadY - pitch, unit);
    layout->rect[TOUCH_DOWN] = Square(dpadX, dpadY + pitch, unit);
    layout->rect[TOUCH_LEFT] = Square(dpadX - pitch, dpadY, unit);
    layout->rect[TOUCH_RIGHT] = Square(dpadX + pitch, dpadY, unit);
    /* Gas under the thumb, brake beside it, as on a console pad's face. */
    layout->rect[TOUCH_CROSS] = Square(faceX, faceY + pitch, unit * 1.25f);
    layout->rect[TOUCH_SQUARE] = Square(faceX - pitch * 1.25f, faceY, unit);
    layout->rect[TOUCH_CIRCLE] = Square(faceX + pitch * 1.25f, faceY, unit);
    layout->rect[TOUCH_TRIANGLE] = Square(faceX, faceY - pitch, unit);
    layout->rect[TOUCH_L1] = (TouchRect){dpadX - unit * 1.5f,
        dpadY - pitch * 2.0f - unit * 0.9f, unit * 3.0f, unit * 0.8f};
    layout->rect[TOUCH_R1] = (TouchRect){faceX - unit * 1.5f,
        faceY - pitch * 2.0f - unit * 0.9f, unit * 3.0f, unit * 0.8f};
    layout->rect[TOUCH_START] = (TouchRect){w * 0.5f - unit * 1.9f,
        h - margin - unit * 0.8f, unit * 1.8f, unit * 0.8f};
    layout->rect[TOUCH_SETTINGS] = (TouchRect){w * 0.5f + unit * 0.1f,
        h - margin - unit * 0.8f, unit * 1.8f, unit * 0.8f};
    for (control = 0; control < TOUCH_CONTROL_COUNT; control++) {
        layout->rect[control].x += (float)originX;
        layout->rect[control].y += (float)originY;
    }
}

int TouchControlAt(const TouchLayout *layout, float x, float y) {
    const float slop = layout->unit * 0.2f;
    int best = -1;
    float bestDistance = 0.0f;
    int control;

    for (control = 0; control < TOUCH_CONTROL_COUNT; control++) {
        const TouchRect *r = &layout->rect[control];
        float cx, cy, d;

        if (x < r->x - slop || x > r->x + r->w + slop ||
            y < r->y - slop || y > r->y + r->h + slop) continue;
        cx = r->x + r->w * 0.5f;
        cy = r->y + r->h * 0.5f;
        d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
        if (best < 0 || d < bestDistance) {
            best = control;
            bestDistance = d;
        }
    }
    return best;
}

static int FindFinger(const TouchState *state, int64_t id) {
    int i;

    for (i = 0; i < TOUCH_MAX_FINGERS; i++) {
        if (state->finger[i].active && state->finger[i].id == id) return i;
    }
    return -1;
}

static int Held(const TouchState *state, int control) {
    int i;

    for (i = 0; i < TOUCH_MAX_FINGERS; i++) {
        if (state->finger[i].active && state->finger[i].control == control)
            return 1;
    }
    return 0;
}

uint32_t TouchFingerDown(TouchState *state, const TouchLayout *layout,
                         int64_t id, float x, float y) {
    int slot = FindFinger(state, id);
    int control = TouchControlAt(layout, x, y);
    int newlyHeld;

    if (slot < 0) {
        for (slot = 0; slot < TOUCH_MAX_FINGERS; slot++) {
            if (!state->finger[slot].active) break;
        }
        if (slot == TOUCH_MAX_FINGERS) return 0;
    }
    newlyHeld = control >= 0 && !Held(state, control);
    state->finger[slot].id = id;
    state->finger[slot].active = 1;
    state->finger[slot].control = control;
    return newlyHeld ? 1u << control : 0u;
}

uint32_t TouchFingerMove(TouchState *state, const TouchLayout *layout,
                         int64_t id, float x, float y) {
    int slot = FindFinger(state, id);
    int control;
    int newlyHeld;

    if (slot < 0) return 0;
    control = TouchControlAt(layout, x, y);
    if (control == state->finger[slot].control) return 0;
    newlyHeld = control >= 0 && !Held(state, control);
    state->finger[slot].control = control;
    return newlyHeld ? 1u << control : 0u;
}

void TouchFingerUp(TouchState *state, int64_t id) {
    int slot = FindFinger(state, id);

    if (slot >= 0) state->finger[slot].active = 0;
}

void TouchReleaseAll(TouchState *state) { memset(state, 0, sizeof(*state)); }

uint32_t TouchHeldMask(const TouchState *state) {
    uint32_t mask = 0;
    int i;

    for (i = 0; i < TOUCH_MAX_FINGERS; i++) {
        if (state->finger[i].active && state->finger[i].control >= 0)
            mask |= 1u << state->finger[i].control;
    }
    return mask;
}

int TouchAtlasTile(const TouchLayout *layout) {
    return (int)ceilf(layout->unit * 3.0f) + 2;
}

void TouchAtlasOrigin(const TouchLayout *layout, int control, int pressed,
                      int *x, int *y) {
    const int tile = TouchAtlasTile(layout);

    *x = pressed ? tile : 0;
    *y = control * tile;
}

void TouchAtlasSize(const TouchLayout *layout, int *w, int *h) {
    const int tile = TouchAtlasTile(layout);

    *w = tile * 2;
    *h = tile * TOUCH_CONTROL_COUNT;
}
