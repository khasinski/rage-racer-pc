/*
 * The showroom turntable, shared by the car select screen and the car shop.
 *
 * Both screens stand the car on a turntable that swings round to a new angle
 * whenever a different car is shown. The swing has to finish before the screen
 * will accept another one, and the two directions run the same arithmetic and
 * differ only in where the turntable is asked to stop.
 */

#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"

/* The rest window is a fifth of a turn either side of the target. */
#define TURNTABLE_REST_WINDOW 0x493DF
#define TURNTABLE_SWING 0x927C0

int MenuCarViewSettled(void) {
    s32 target = g_MenuViewAngleTarget;
    s32 angle = g_MenuViewAngle;

    return target < angle ? (angle - target <= TURNTABLE_REST_WINDOW)
                          : (target - angle <= TURNTABLE_REST_WINDOW);
}

/*
 * Swings round to another car. The screen says which of its own indices is
 * the one on show, and which car it was showing before, because a frame that
 * holds both directions at once swings twice from the same starting car.
 */
void MenuSpinToCar(s32 *shownCar, s32 fromIndex, s32 toIndex, s32 newTarget) {
    s32 previousTarget;

    PlaySoundCue(8);
    *shownCar = toIndex;
    RequestCarModel(toIndex);
    previousTarget = g_MenuViewAngleTarget;
    g_CarSwapFromIndex = fromIndex;
    g_MenuViewAngleTarget = newTarget;
    g_MenuLowerAltPanelStep = -1;
    g_CarSwapToIndex = *shownCar;
    g_MenuViewAngle = (g_MenuViewAngle - previousTarget) + TURNTABLE_SWING;
}

/*
 * Leaving the shop puts the player's own car back on the turntable. No cue and
 * no panel here: this runs under the sound the choice itself makes.
 */
void MenuSpinBackToPlayerCar(void) {
    s32 previousTarget;

    RequestCarModel(g_PlayerCarIndex);
    previousTarget = g_MenuViewAngleTarget;
    g_MenuViewAngleTarget = 0;
    g_CarSwapFromIndex = g_CarListCursor;
    g_CarSwapToIndex = g_PlayerCarIndex;
    g_MenuViewAngle = (g_MenuViewAngle - previousTarget) + TURNTABLE_SWING;
}
