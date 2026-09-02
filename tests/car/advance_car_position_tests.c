/*
 * What updating a car's travel velocity does to its heading and speed.
 *
 * The function had no test at all, and it is written in the shape it was
 * recovered in: a volatile array whose middle element is never used, the sine
 * and cosine of both angles worked out three times over, and a comment saying
 * it integrates the world position, which it does not. None of that can be
 * tidied safely without something that says the answers did not move, because
 * every step is integer arithmetic and rounding is part of the result.
 *
 * This folds a sweep of headings, body angles, speeds and accelerations into
 * one number, while separately asserting that a zero travel vector has no new
 * direction and therefore preserves the previous heading.
 */

#include "common.h"
#include "game/car.h"
#include "game/render.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

/*
 * Atan2 lives beside the camera code, which reaches for the render state and
 * the render world. None of that runs here, so answer it rather than link
 * the renderer in to divide two numbers.
 */
GameRenderState g_RenderState;
MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2) {
    (void)m0;
    (void)m1;
    return m2;
}
void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z, int32_t pitch,
                              int32_t yaw, int32_t roll) {
    (void)x; (void)y; (void)z; (void)pitch; (void)yaw; (void)roll;
}

static unsigned long s_digest = 2166136261UL;

static void Fold(FILE *out, s32 heading, s32 yaw, s32 speed, s32 accel,
                 const GameCarRuntime *car) {
    char line[256];
    const char *p;

    snprintf(line, sizeof(line),
             "heading=%d yaw=%d speed=%d accel=%d -> heading=%d speed=%d\n",
             heading, yaw, speed, accel, car->headingAngle, car->speed);
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (out != NULL) fputs(line, out);
}

int main(int argc, char **argv) {
    /*
     * Run the test with a file name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 498435994UL;
    static const s32 angles[] = {0, 0x100, 0x400, 0x7FF, 0x800, 0xC00, 0xFFF};
    static const s32 speeds[] = {0, 1, 100, 4096, -100, 20000};
    static const s32 accelerations[] = {0, 1, -1, 50, -50, 1000};
    FILE *out = NULL;
    size_t h, y, s, a;
    int cases = 0;
    GameCarRuntime stoppedCar;

    memset(&stoppedCar, 0, sizeof(stoppedCar));
    stoppedCar.headingAngle = 0x235;
    stoppedCar.bodyYaw = 0x900;
    UpdateCarTravelVelocity(&stoppedCar);
    if (stoppedCar.headingAngle != 0x235 || stoppedCar.speed != 0) {
        puts("stopped car lost its travel direction");
        return 1;
    }

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            fprintf(stderr, "cannot write %s\n", argv[1]);
            return 2;
        }
    }

    for (h = 0; h < sizeof(angles) / sizeof(angles[0]); h++)
        for (y = 0; y < sizeof(angles) / sizeof(angles[0]); y++)
            for (s = 0; s < sizeof(speeds) / sizeof(speeds[0]); s++)
                for (a = 0; a < sizeof(accelerations) / sizeof(accelerations[0]);
                     a++) {
                    GameCarRuntime car;
                    memset(&car, 0, sizeof(car));
                    car.headingAngle = (s16)angles[h];
                    car.bodyYaw = (s16)angles[y];
                    car.speed = speeds[s];
                    car.acceleration = accelerations[a];
                    UpdateCarTravelVelocity(&car);
                    Fold(out, angles[h], angles[y], speeds[s],
                         accelerations[a], &car);
                    cases++;
                }

    if (out != NULL) fclose(out);
    if (s_digest != expected) {
        printf("car travel velocity: %d cases folded to %lu, expected %lu\n",
               cases, s_digest, expected);
        return 1;
    }
    printf("car travel velocity: %d cases unchanged\n", cases);
    return 0;
}
