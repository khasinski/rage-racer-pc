#include "render/car_lights.h"

#include <math.h>
#include <stdio.h>

static int failures;
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); \
    failures++; } } while (0)

int main(void) {
    CarLights car = {0}, rival = {0};
    UpdateCarLights(&car, 1, 1, 0, 1);
    CHECK(car.headlights == 0 && car.tail == 0 && car.stop == 0);
    UpdateCarLights(&car, 1, 1, 1, 0.02f);
    CHECK(car.stop == 1 && car.headlights == 0);
    UpdateCarLights(&car, 1, 0.25f, 0, 0.1f);
    CHECK(car.automatic && fabsf(car.headlights - 0.5f) < 0.0001f);
    CHECK(car.tail > 0 && car.stop == 0);
    UpdateCarLights(&car, 1, 0.32f, 0, 1);
    CHECK(car.automatic && car.headlights == 1);
    UpdateCarLights(&car, 1, 1, 0, 1);
    CHECK(!car.automatic && car.headlights == 0);
    UpdateCarLights(&car, 1, 0.32f, 0, 1);
    CHECK(!car.automatic && car.headlights == 0);
    UpdateCarLights(&car, 0.1f, 1, 1, 1);
    CHECK(car.headlights == 1 && car.stop == 1 && car.tail < car.stop);
    UpdateCarLights(&rival, 0.1f, 1, 1, 1);
    CHECK(rival.headlights == car.headlights && rival.stop == car.stop);
    UpdateCarLights(&car, 1, 1, 0, 0);
    CHECK(car.headlights == 1 && car.stop == 1);
    UpdateCarLights(&car, 1, 1, 0, NAN);
    CHECK(car.headlights == 1 && car.stop == 1);
    UpdateCarLights(&car, 1, 1, 0, -1);
    CHECK(car.headlights == 1 && car.stop == 1);
    /* One second produces the same settled state at PAL/NTSC tick rates. */
    car = (CarLights){0};
    rival = (CarLights){0};
    for (int i = 0; i < 50; i++) UpdateCarLights(&car, 0, 1, 0, 1.0f/50);
    for (int i = 0; i < 60; i++) UpdateCarLights(&rival, 0, 1, 0, 1.0f/60);
    CHECK(car.headlights == 1 && rival.headlights == 1);
    return failures != 0;
}
