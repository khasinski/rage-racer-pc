#include "game/car_collision_internal.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const CarHullPoint hull[6] = {
        {-32, 64}, {32, 64}, {-24, -72}, {24, -72}, {-32, 16}, {32, 16},
    };
    const CarHullPoint corners[4] = {
        {-26, 96}, {26, 96}, {-26, -16}, {26, -16},
    };
    PlayerCarRuntime driver = {0};
    PlayerCarRuntime otherDriver = {0};
    driver.drive.dragScale = 1000;
    otherDriver.collisionFlag = 7;
    const CarCollider field[] = {
        {.car = NULL, .corners = corners},
        {.car = AsRivalCar(&driver), .corners = corners},
        {.car = AsRivalCar(&otherDriver), .corners = corners},
    };
    CarContact contact = FindCarContact(&driver, hull, field, 3, 32768);
    CHECK(contact.region > 0 && contact.opponent == AsRivalCar(&otherDriver));
    CHECK(otherDriver.collisionFlag == 7);
    CHECK(driver.drive.dragScale == 700);

    GameCarRuntime distant = {0};
    distant.trackProgress = 500;
    distant.x = 1000;
    const CarCollider otherField[] = {{.car = &distant, .corners = corners}};
    PlayerCarRuntime second = {0};
    second.drive.dragScale = 1000;
    contact = FindCarContact(&second, hull, otherField, 1, 10000);
    CHECK(contact.region == 0 && second.drive.dragScale == 875);
    CHECK(driver.drive.dragScale == 700);
    contact = FindCarContact(&driver, hull, field, 3, 32768);
    CHECK(contact.region > 0 && contact.opponent == AsRivalCar(&otherDriver));

    otherDriver.activeFlag = -1;
    CHECK(FindCarContact(&driver, hull, field, 3, 32768).region == 0);
    otherDriver.activeFlag = 0;
    otherDriver.verticalMotionState = CAR_VERTICAL_FALLING;
    otherDriver.y = 26;
    CHECK(FindCarContact(&driver, hull, field, 3, 32768).region == 0);
    otherDriver.y = 0;
    const CarHullPoint emptyHull[6] = {{0}};
    CHECK(FindCarContact(&driver, emptyHull, field, 3, 32768).region == 0);
    CHECK(FindCarContact(&driver, hull, field, 0, 32768).region == 0);
    CHECK(FindCarContact(&driver, hull, field, 3, 0).region == 0);
    CHECK(FindCarContact(NULL, hull, field, 3, 32768).region == 0);
    CHECK(FindCarContact(&driver, hull, NULL, 3, 32768).region == 0);
    puts("car contact tests passed");
    return 0;
}
