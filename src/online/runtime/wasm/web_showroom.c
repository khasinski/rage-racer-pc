/* The garage preview: a race prepared for one car (rw_start_race, alone on
 * the grid), drawn without its field from a camera circling the car,
 * `angle` degrees round it. Its paint follows rw_set_showroom_paint; its logo
 * and name rw_set_showroom_logo/tag (web_looks.c). */
#include <emscripten/emscripten.h>
#include <math.h>
#include "game/angle.h"
#include "render/car_paint.h"
#include "web_bridge.h"
#include "web_showroom.h"

/* The middle of the car as drawn (game coordinates), found from its own
 * vertices the first time a preview frame is built: the car's origin sits by
 * its rear axle, so circling that would swing the car round it. */
static struct { int valid; Vec3 centre; } s_showroom;

void WebShowroomForget(void) { s_showroom.valid = 0; }

EMSCRIPTEN_KEEPALIVE void rw_set_showroom_paint(int first, int second) {
    ClientRace *race = WebRace();
    if (!race || !race->view) return;
    RaceCarLook *look = &race->view->looks[0];
    look->hasPaint = first >= 0 && first < RAGE_CAR_PAINT_COLOR_COUNT &&
                     second >= 0 && second < RAGE_CAR_PAINT_COLOR_COUNT;
    look->paint.paintColor1 = (u8)(look->hasPaint ? first : 0);
    look->paint.paintColor2 = (u8)(look->hasPaint ? second : 0);
}

static void FindShowroomCentre(void) {
    float low[3], high[3];
    if (!WebDrawnCarBounds(low, high)) return;
    /* Drawn coordinates flip y and z relative to the game's. */
    s_showroom.centre = (Vec3){(low[0] + high[0]) / 2, -(low[1] + high[1]) / 2, -(low[2] + high[2]) / 2};
    s_showroom.valid = 1;
}

EMSCRIPTEN_KEEPALIVE int rw_build_showroom(float aspect, float angle) {
    enum { RADIUS = 330, HEIGHT = 105, FOCUS_HEIGHT = 30 };
    const ClientRace *race = WebRace();
    if (!race || !(aspect > 0.0f)) return -1;
    const PlayerCarRuntime *car = &race->sim.drivers[0].car;
    /* Game coordinates: y points down, so the eye is HEIGHT above the focus
     * when it is that far negative; the view angles are the retail chase
     * camera's (yaw and pitch from the eye-to-focus direction). */
    const float radians = angle * 0.017453292519943295f;
    const Vec3 toFocus = {-RADIUS * sinf(radians), HEIGHT, -RADIUS * cosf(radians)};
    const Vec3 focus = s_showroom.valid ? s_showroom.centre
                                        : (Vec3){(float)car->x, (float)car->y - FOCUS_HEIGHT, (float)car->z};
    const Vec3 eye = {focus.x - toFocus.x, focus.y - toFocus.y, focus.z - toFocus.z};
    const s32 horizontal = (s32)lroundf(sqrtf(toFocus.x * toFocus.x + toFocus.z * toFocus.z));
    const s32 yaw = 0x400 - (Atan2((s32)lroundf(toFocus.x), (s32)lroundf(toFocus.z)) & ANGLE_MASK);
    const s32 pitch = 0x400 - (Atan2((s32)lroundf(toFocus.y), horizontal) & ANGLE_MASK);
    const int count = WebDrawFieldScene(aspect, eye, pitch, yaw, 30.0f);
    if (count >= 0 && !s_showroom.valid) {
        FindShowroomCentre();
        if (s_showroom.valid) return rw_build_showroom(aspect, angle);
    }
    return count;
}
