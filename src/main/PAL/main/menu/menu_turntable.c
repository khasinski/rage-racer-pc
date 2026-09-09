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
#include "game/menu_internal.h"

int MenuCarViewSettled(void) {
    return MenuValueWithinWindow(g_MenuViewAngle, g_MenuViewAngleTarget,
                                 MENU_CAR_VIEW_SETTLE_WINDOW);
}

/*
 * Swings round to another car. The screen says which of its own indices is
 * the one on show and which car it was showing before.
 */
void MenuSpinToCar(s32 *shownCar, s32 fromIndex, s32 toIndex, s32 newTarget) {
    s32 previousTarget;

    if (shownCar == NULL || (u32)toIndex >= GAME_CAR_COUNT) {
        return;
    }
    if (!RequestCarModel(toIndex)) {
        return;
    }
    PlaySoundCue(8);
    *shownCar = toIndex;
    previousTarget = g_MenuViewAngleTarget;
    g_CarSwapFromIndex = fromIndex;
    g_MenuViewAngleTarget = newTarget;
    g_MenuLowerAltPanelStep = -1;
    g_CarSwapToIndex = *shownCar;
    g_MenuViewAngle = RebaseCarouselValue(
        g_MenuViewAngle, previousTarget, MENU_CAR_VIEW_REBASE_SPAN);
    /* Reversing shortly after the model swap must not put the next swap
     * directly ahead of us. Otherwise a brief held direction completes one
     * swap immediately and starts a second on the following frame. Move by
     * a whole revolution to preserve the visible orientation while retaining
     * a full browsing animation in the newly requested direction. */
    if (newTarget == MENU_CAR_VIEW_RIGHT_TARGET &&
        g_MenuViewAngle > MENU_CAR_VIEW_REBASE_SPAN)
        g_MenuViewAngle -= MENU_CAR_VIEW_REBASE_SPAN;
    else if (newTarget == 0 && g_MenuViewAngle < MENU_CAR_VIEW_REBASE_SPAN)
        g_MenuViewAngle += MENU_CAR_VIEW_REBASE_SPAN;
}

/*
 * Leaving the shop puts the player's own car back on the turntable. No cue and
 * no panel here: this runs under the sound the choice itself makes.
 */
void MenuSpinBackToPlayerCar(void) {
    s32 previousTarget;

    if ((u32)g_PlayerCarIndex >= GAME_CAR_COUNT) {
        return;
    }
    if (!RequestCarModel(g_PlayerCarIndex)) {
        return;
    }
    previousTarget = g_MenuViewAngleTarget;
    g_MenuViewAngleTarget = 0;
    g_CarSwapFromIndex = g_CarListCursor;
    g_CarSwapToIndex = g_PlayerCarIndex;
    g_MenuViewAngle = RebaseCarouselValue(
        g_MenuViewAngle, previousTarget, MENU_CAR_VIEW_REBASE_SPAN);
}
