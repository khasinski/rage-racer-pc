/* The on-screen pad's rules without a screen: every button lands inside the
 * window and clear of the others, a thumb finds the control it is on, several
 * fingers hold several controls, and a finger that slides moves its hold. */
#include <stdio.h>
#include <string.h>

#include "touch_model.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); \
    return 1; } } while (0)

static void Centre(const TouchLayout *layout, int control, float *x, float *y) {
    *x = layout->rect[control].x + layout->rect[control].w * 0.5f;
    *y = layout->rect[control].y + layout->rect[control].h * 0.5f;
}

static int Overlap(const TouchRect *a, const TouchRect *b) {
    return a->x < b->x + b->w && b->x < a->x + a->w &&
           a->y < b->y + b->h && b->y < a->y + a->h;
}

static int TestLayoutFitsAndDoesNotOverlap(void) {
    static const int sizes[][2] = {{2400, 1080}, {1920, 1080}, {1280, 720},
                                   {800, 480}, {2340, 1080}, {960, 540}};
    size_t s;

    for (s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        TouchLayout layout;
        int a, b;

        TouchComputeLayout(0, 0, sizes[s][0], sizes[s][1], &layout);
        for (a = 0; a < TOUCH_CONTROL_COUNT; a++) {
            const TouchRect *r = &layout.rect[a];

            if (r->x < 0 || r->y < 0 || r->x + r->w > (float)sizes[s][0] ||
                r->y + r->h > (float)sizes[s][1]) {
                fprintf(stderr, "%dx%d: control %d leaves the window\n",
                        sizes[s][0], sizes[s][1], a);
                return 1;
            }
            for (b = a + 1; b < TOUCH_CONTROL_COUNT; b++) {
                if (Overlap(r, &layout.rect[b])) {
                    fprintf(stderr, "%dx%d: controls %d and %d overlap\n",
                            sizes[s][0], sizes[s][1], a, b);
                    return 1;
                }
            }
        }
        /* The d-pad sits left of centre, the face buttons right of it. */
        CHECK(layout.rect[TOUCH_LEFT].x < layout.rect[TOUCH_RIGHT].x);
        CHECK(layout.rect[TOUCH_RIGHT].x + layout.rect[TOUCH_RIGHT].w <
              (float)sizes[s][0] * 0.5f);
        CHECK(layout.rect[TOUCH_SQUARE].x > (float)sizes[s][0] * 0.5f);
        /* Gas is under the right thumb, below the other face buttons. */
        CHECK(layout.rect[TOUCH_CROSS].y > layout.rect[TOUCH_CIRCLE].y);
    }
    return 0;
}

static int TestLayoutFollowsTheSafeArea(void) {
    TouchLayout plain, shifted;
    int control;

    TouchComputeLayout(0, 0, 2194, 933, &plain);
    TouchComputeLayout(128, 63, 2194, 933, &shifted);
    for (control = 0; control < TOUCH_CONTROL_COUNT; control++) {
        const TouchRect *r = &shifted.rect[control];

        CHECK(r->x == plain.rect[control].x + 128.0f);
        CHECK(r->y == plain.rect[control].y + 63.0f);
        CHECK(r->x >= 128.0f && r->y >= 63.0f);
        CHECK(r->x + r->w <= 128.0f + 2194.0f);
        CHECK(r->y + r->h <= 63.0f + 933.0f);
    }
    return 0;
}

static int TestEveryControlIsHitAtItsCentre(void) {
    TouchLayout layout;
    int control;

    TouchComputeLayout(0, 0, 1920, 1080, &layout);
    for (control = 0; control < TOUCH_CONTROL_COUNT; control++) {
        float x, y;

        Centre(&layout, control, &x, &y);
        CHECK(TouchControlAt(&layout, x, y) == control);
    }
    CHECK(TouchControlAt(&layout, 960.0f, 100.0f) == -1);
    CHECK(TouchControlAt(&layout, -5.0f, -5.0f) == -1);
    return 0;
}

static int TestFingersHoldAndRelease(void) {
    TouchLayout layout;
    TouchState state;
    float x, y;
    uint32_t pressed;

    TouchComputeLayout(0, 0, 1920, 1080, &layout);
    TouchReleaseAll(&state);
    CHECK(TouchHeldMask(&state) == 0);

    /* Gas and right together, two thumbs. */
    Centre(&layout, TOUCH_CROSS, &x, &y);
    pressed = TouchFingerDown(&state, &layout, 1, x, y);
    CHECK(pressed == 1u << TOUCH_CROSS);
    Centre(&layout, TOUCH_RIGHT, &x, &y);
    pressed = TouchFingerDown(&state, &layout, 2, x, y);
    CHECK(pressed == 1u << TOUCH_RIGHT);
    CHECK(TouchHeldMask(&state) == ((1u << TOUCH_CROSS) | (1u << TOUCH_RIGHT)));

    /* A second finger on the same control adds nothing new. */
    Centre(&layout, TOUCH_CROSS, &x, &y);
    CHECK(TouchFingerDown(&state, &layout, 3, x, y) == 0);
    TouchFingerUp(&state, 1);
    CHECK(TouchHeldMask(&state) & (1u << TOUCH_CROSS)); /* finger 3 still holds it */
    TouchFingerUp(&state, 3);
    TouchFingerUp(&state, 2);
    CHECK(TouchHeldMask(&state) == 0);
    return 0;
}

static int TestSlidingMovesTheHold(void) {
    TouchLayout layout;
    TouchState state;
    float x, y;

    TouchComputeLayout(0, 0, 1920, 1080, &layout);
    TouchReleaseAll(&state);
    Centre(&layout, TOUCH_LEFT, &x, &y);
    TouchFingerDown(&state, &layout, 7, x, y);
    CHECK(TouchHeldMask(&state) == 1u << TOUCH_LEFT);

    /* Sliding across to the next button hands the finger over. */
    Centre(&layout, TOUCH_RIGHT, &x, &y);
    CHECK(TouchFingerMove(&state, &layout, 7, x, y) == 1u << TOUCH_RIGHT);
    CHECK(TouchHeldMask(&state) == 1u << TOUCH_RIGHT);

    /* Off every control it holds nothing; a move that changes nothing is quiet. */
    CHECK(TouchFingerMove(&state, &layout, 7, 960.0f, 100.0f) == 0);
    CHECK(TouchHeldMask(&state) == 0);
    CHECK(TouchFingerMove(&state, &layout, 7, 960.0f, 110.0f) == 0);
    /* An unknown finger moving is ignored. */
    CHECK(TouchFingerMove(&state, &layout, 99, x, y) == 0);

    /* A finger that lands on nothing and then slides onto a button presses it. */
    TouchFingerUp(&state, 7);
    TouchFingerDown(&state, &layout, 8, 960.0f, 100.0f);
    CHECK(TouchHeldMask(&state) == 0);
    Centre(&layout, TOUCH_START, &x, &y);
    CHECK(TouchFingerMove(&state, &layout, 8, x, y) == 1u << TOUCH_START);
    return 0;
}

static int TestMoreFingersThanSlots(void) {
    TouchLayout layout;
    TouchState state;
    int i;

    TouchComputeLayout(0, 0, 1920, 1080, &layout);
    TouchReleaseAll(&state);
    for (i = 0; i < TOUCH_MAX_FINGERS + 3; i++)
        TouchFingerDown(&state, &layout, 100 + i, 960.0f, 100.0f);
    /* Extra fingers are dropped, and releasing frees their slots again. */
    for (i = 0; i < TOUCH_MAX_FINGERS + 3; i++) TouchFingerUp(&state, 100 + i);
    CHECK(TouchFingerDown(&state, &layout, 500,
                          layout.rect[TOUCH_UP].x + 5.0f,
                          layout.rect[TOUCH_UP].y + 5.0f) == 1u << TOUCH_UP);
    return 0;
}

static int TestAtlasHoldsEveryTile(void) {
    TouchLayout layout;
    int w, h, control, tile;

    TouchComputeLayout(0, 0, 1920, 1080, &layout);
    TouchAtlasSize(&layout, &w, &h);
    tile = TouchAtlasTile(&layout);
    for (control = 0; control < TOUCH_CONTROL_COUNT; control++) {
        int pressed;

        CHECK(layout.rect[control].w <= (float)tile &&
              layout.rect[control].h <= (float)tile);
        for (pressed = 0; pressed < 2; pressed++) {
            int x, y;

            TouchAtlasOrigin(&layout, control, pressed, &x, &y);
            CHECK(x >= 0 && y >= 0 && x + tile <= w && y + tile <= h);
        }
    }
    return 0;
}

int main(void) {
    if (TestLayoutFitsAndDoesNotOverlap()) return 1;
    if (TestLayoutFollowsTheSafeArea()) return 1;
    if (TestEveryControlIsHitAtItsCentre()) return 1;
    if (TestFingersHoldAndRelease()) return 1;
    if (TestSlidingMovesTheHold()) return 1;
    if (TestMoreFingersThanSlots()) return 1;
    if (TestAtlasHoldsEveryTile()) return 1;
    puts("touch model rules hold");
    return 0;
}
