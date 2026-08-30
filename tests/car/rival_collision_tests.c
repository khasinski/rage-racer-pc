/*
 * Cars hitting each other, swept.
 *
 * CollideRivalCars is what stops two AI cars occupying the same piece of
 * track. It builds the hull of the car in its own frame, cuts it into four
 * quadrants, transforms the other car's hull into the same frame, and asks
 * which quadrant, if any, the other car is inside. Which quadrant decides
 * which of the two gets shoved. Two hundred and sixty-five lines nested eight
 * deep, most of it the same work written out twice, and nothing tested it.
 *
 * The geometry is real here rather than stubbed: the rotation matrix is the
 * library's, and transforming a corner is the multiply the GTE does. Every
 * quadrant test is recorded with all five of its packed points, so the digest
 * pins the whole hull construction, not just the answer.
 */

#include "common.h"
#include "game/car.h"
#include "game/track.h"
#include "psyq/gte.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[11];
CarCollisionPoint g_CarCollisionCorners[4];
s32 g_TrackLength;

static unsigned long s_digest = 2166136261UL;
static FILE *s_out;
static int s_calls;

static void Fold(unsigned char byte) {
    s_digest = ((s_digest ^ byte) * 16777619UL) & 0xFFFFFFFFUL;
}

static void Record(const char *name, const s32 *values, int count) {
    const char *p;
    int i;

    for (p = name; *p != '\0'; p++) {
        Fold((unsigned char)*p);
    }
    for (i = 0; i < count; i++) {
        u32 value = (u32)values[i];

        Fold((unsigned char)value);
        Fold((unsigned char)(value >> 8));
        Fold((unsigned char)(value >> 16));
        Fold((unsigned char)(value >> 24));
    }
    if (s_out != NULL) {
        fputs(name, s_out);
        for (i = 0; i < count; i++) {
            fprintf(s_out, " %d", values[i]);
        }
        fputc('\n', s_out);
    }
    s_calls++;
}

#define RECORD(name, ...)                                                      \
    do {                                                                       \
        s32 v[] = {__VA_ARGS__};                                               \
        Record(name, v, (int)(sizeof(v) / sizeof(v[0])));                       \
    } while (0)

/*
 * The port's own transform, which is the emulated GTE doing the multiply the
 * console does. Copied here rather than linked because the file it lives in is
 * the whole host platform layer.
 */
void TransformCollisionVector(const s16 *input, s32 *output) {
    SVECTOR vector;
    VECTOR transformed;

    vector.vx = input[0];
    vector.vy = input[1];
    vector.vz = input[2];
    vector.pad = 0;
    ApplyRotMatrix(&vector, &transformed);
    output[0] = transformed.vx;
    output[1] = transformed.vy;
    output[2] = transformed.vz;
}

/*
 * The game's own predicate, copied rather than linked because the file it
 * lives in is the whole car orientation module. The clipping cross product is
 * the emulated GTE's, and the arguments are recorded so the digest pins the
 * hull the caller built and not just the answer.
 */
s32 IsPointInQuad(s32 p0, s32 p1, s32 p2, s32 p3, s32 pt) {
    s32 ret = 0;

    RECORD("quadtest", p0, p1, p2, p3, pt);
    if (NormalClip(p0, p1, pt) >= 0) {
        if (NormalClip(p1, p3, pt) >= 0) {
            if (NormalClip(p3, p2, pt) >= 0) {
                ret = NormalClip(p2, p0, pt) >= 0;
            }
        }
    }
    return ret;
}

static int s_knockbacks;

void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode) {
    RECORD("knockback", (s32)(car - g_Cars), x, z, mode);
    s_knockbacks++;
}

int main(int argc, char **argv) {
    /*
     * What the collision did before it was taken apart. Run the test with a
     * file name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 1138725312UL;
    static const s32 indices[] = {0, 5, 9};
    /* Either side of the two hundred units of track the check looks over, at
     * both ends of the lap. */
    static const s32 progressDeltas[] = {0, 199, 200, -201, -200, -1};
    /* Either side of the hundred units across the track. */
    static const s32 lateralDeltas[] = {0, 99, 100, -99, -100};
    static const s32 yaws[] = {0, 0x400, 0x800};
    /* Overlaps from head-on to barely touching, so that the centre sample is
     * sometimes the only point of the other car inside the hull. */
    static const s32 nudges[] = {0, 40, -40, 90, 150, 200};
    /* The shove divides the speed difference by 32 rounding toward zero, and
     * -63 is a difference that lands on the rounding itself. */
    static const s32 velocityDeltas[] = {0, 700, -700, -1, -63};
    int ii, pd, ld, active, vertical, cy, oy, nudge, vel;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    /* A car two units long and one wide, in the units the hull uses. */
    g_CarCollisionCorners[0].x = -0x80;
    g_CarCollisionCorners[0].z = 0x100;
    g_CarCollisionCorners[1].x = 0x80;
    g_CarCollisionCorners[1].z = 0x100;
    g_CarCollisionCorners[2].x = -0x80;
    g_CarCollisionCorners[2].z = -0x100;
    g_CarCollisionCorners[3].x = 0x80;
    g_CarCollisionCorners[3].z = -0x100;

    for (ii = 0; ii < 3; ii++)
    for (pd = 0; pd < 6; pd++)
    for (ld = 0; ld < 5; ld++)
    for (active = 0; active < 2; active++)
    for (vertical = 0; vertical < 2; vertical++)
    for (cy = 0; cy < 3; cy++)
    for (oy = 0; oy < 3; oy++)
    for (nudge = 0; nudge < 6; nudge++)
    for (vel = 0; vel < 5; vel++) {
        char label[192];
        GameCarRuntime *car;
        GameCarRuntime *other;
        s32 index = indices[ii];
        s32 result;
        int i;

        memset(g_Cars, 0, sizeof(g_Cars));
        g_TrackLength = 0x8000;
        s_knockbacks = 0;

        /* Every car but the two of interest is out of the race. */
        for (i = 0; i < 11; i++) {
            g_Cars[i].activeFlag = -1;
        }

        car = &g_Cars[index];
        other = &g_Cars[index + 1];

        car->activeFlag = 0;
        car->verticalMotionState = 1;
        car->trackProgress = 0x1000;
        car->trackLateralOffset = 0;
        car->x = 0x2000;
        car->z = 0x3000;
        car->bodyYaw = yaws[cy];
        car->bodyPitch = 0x100;
        car->bodyRoll = -0x100;
        car->worldVelocityX = 500;
        car->worldVelocityZ = -500;
        car->acceleration = 100000;

        other->activeFlag = (s16)(active ? -1 : 0);
        other->verticalMotionState = (s16)(vertical ? 1 : 2);
        other->trackProgress = car->trackProgress + progressDeltas[pd];
        other->trackLateralOffset = lateralDeltas[ld];
        other->x = car->x + nudges[nudge];
        other->z = car->z + nudges[(nudge + 1) % 6];
        other->bodyYaw = yaws[oy];
        other->bodyPitch = -0x100;
        other->bodyRoll = 0x100;
        other->worldVelocityX = 500 + velocityDeltas[vel];
        other->worldVelocityZ = -500 - velocityDeltas[vel];
        other->acceleration = 200000;

        sprintf(label,
                "== index%d/progress%d/lateral%d/active%d/vertical%d/caryaw%d/"
                "otheryaw%d/nudge%d/vel%d",
                index, progressDeltas[pd], lateralDeltas[ld], active, vertical,
                yaws[cy], yaws[oy], nudges[nudge], velocityDeltas[vel]);
        Record(label, NULL, 0);

        result = CollideRivalCars(car, index);

        RECORD("result", result, s_knockbacks, car->collisionFlag,
               car->acceleration, other->collisionFlag, other->acceleration);
        steps++;
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL rival collisions behave differently: %d states making %d "
               "calls digest to %lu, expected %lu\n", steps, s_calls, s_digest,
               expected);
        return 1;
    }
    printf("rival collisions take the same %d states they always did\n", steps);
    return 0;
}
