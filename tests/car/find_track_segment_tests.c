/*
 * Which segment of the track a car is standing on.
 *
 * The search walks outwards from a guess, alternating sides, and asks each
 * segment whether the car is inside the quad its two centreline points span.
 * A ring of points with varying widths is built here rather than loaded, so
 * the sweep is repeatable and the answers do not depend on a course.
 */

#include "common.h"
#include "game/car.h"
#include "game/render.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 FindTrackSegment(GameCarRuntime *car, s32 idx);

enum { TRACK_POINTS = 24, TRACK_RADIUS = 20000 };

static GameTrackPoint s_points[TRACK_POINTS];

static unsigned long s_digest = 2166136261UL;

static void Fold(FILE *out, const char *label, s32 result,
                 const GameCarRuntime *car) {
    char line[192];
    const char *p;

    snprintf(line, sizeof(line), "%s -> segment=%d pos=(%d,%d)\n", label,
             result, car->x, car->z);
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (out != NULL) fputs(line, out);
}

/* A closed ring, so every index has a segment either side of it and the
 * outward search wraps the way it does on a real course. */
static void BuildRing(void) {
    s32 i;

    memset(s_points, 0, sizeof(s_points));
    for (i = 0; i < TRACK_POINTS; i++) {
        s32 angle = (i * 4096) / TRACK_POINTS;
        s_points[i].x = (rsin(angle) * TRACK_RADIUS) >> 12;
        s_points[i].z = (rcos(angle) * TRACK_RADIUS) >> 12;
        s_points[i].angle = (s16)angle;
        /* Widths vary so the quads are not all the same shape. One point is
         * wide enough that doubling it leaves a signed sixteen-bit word,
         * which the search truncates: a real course has no such point, but
         * the truncation is in the code and this reaches it. */
        s_points[i].leftHalfWidth = (s16)(600 + (i % 5) * 120);
        s_points[i].rightHalfWidth = (s16)(600 + ((i + 2) % 5) * 120);
        if (i == 9) {
            s_points[i].leftHalfWidth = 20000;
        }
    }
    g_TrackPoints = s_points;
    g_TrackPointCount = TRACK_POINTS;
}

int main(int argc, char **argv) {
    /*
     * What the search answered before it was touched. Run with a file name to
     * write the sweep out and diff two runs to see which cases moved.
     */
    static const unsigned long expected = 3072612995UL;
    static const s32 radii[] = {0, 19000, 19700, 20000, 20400, 21000, 40000};
    static const s32 guesses[] = {0, 5, 12, 23};
    FILE *out = NULL;
    GameCarRuntime normalized;
    GameCarRuntime oversized;
    GameCarRuntime emptyTrackCar;
    s32 normalizedResult;
    s32 oversizedResult;
    s32 step;
    size_t r, g;
    int cases = 0;

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            fprintf(stderr, "cannot write %s\n", argv[1]);
            return 2;
        }
    }
    BuildRing();

    /* Positions all the way round the ring, at several distances from its
     * centre: on the line, inside it, outside it, and far away. */
    for (step = 0; step < 24; step++) {
        for (r = 0; r < sizeof(radii) / sizeof(radii[0]); r++) {
            for (g = 0; g < sizeof(guesses) / sizeof(guesses[0]); g++) {
                GameCarRuntime car;
                char label[160];
                s32 angle = (step * 4096) / 24 + 60;
                s32 result;

                memset(&car, 0, sizeof(car));
                car.x = (rsin(angle) * radii[r]) >> 12;
                car.z = (rcos(angle) * radii[r]) >> 12;

                snprintf(label, sizeof(label), "angle=%d radius=%d guess=%d",
                         angle, radii[r], guesses[g]);
                result = FindTrackSegment(&car, guesses[g]);
                Fold(out, label, result, &car);
                cases++;
            }
        }
    }

    /*
     * A car standing exactly on a centreline point sits on the boundary
     * between two segments. One of the four edge tests is strict so that it
     * belongs to one of them rather than both, and only a case placed exactly
     * there can tell whether it still is.
     */
    for (step = 0; step < TRACK_POINTS; step++) {
        GameCarRuntime car;
        char label[160];
        s32 result;

        memset(&car, 0, sizeof(car));
        car.x = s_points[step].x;
        car.z = s_points[step].z;
        snprintf(label, sizeof(label), "exactly on point %d", step);
        result = FindTrackSegment(&car, step);
        Fold(out, label, result, &car);
        cases++;
    }

    memset(&normalized, 0, sizeof(normalized));
    normalized.x = s_points[5].x;
    normalized.z = s_points[5].z;
    oversized = normalized;
    normalizedResult = FindTrackSegment(&normalized, 5);
    oversizedResult = FindTrackSegment(&oversized, TRACK_POINTS + 5);
    if (oversizedResult != normalizedResult ||
        oversized.x != normalized.x || oversized.z != normalized.z) {
        fprintf(stderr, "oversized starting index was not normalized\n");
        return 1;
    }

    memset(&emptyTrackCar, 0, sizeof(emptyTrackCar));
    emptyTrackCar.x = 123;
    emptyTrackCar.z = 456;
    g_TrackPointCount = 0;
    if (FindTrackSegment(&emptyTrackCar, 0) != -1 ||
        emptyTrackCar.x != 123 || emptyTrackCar.z != 456) {
        fprintf(stderr, "empty track search changed the car\n");
        return 1;
    }
    g_TrackPointCount = TRACK_POINTS;
    g_TrackPoints = NULL;
    if (FindTrackSegment(&emptyTrackCar, 0) != -1 ||
        emptyTrackCar.x != 123 || emptyTrackCar.z != 456) {
        fprintf(stderr, "missing track search changed the car\n");
        return 1;
    }

    g_TrackPoints = s_points;
    {
        GameCarRuntime lowWords;
        GameCarRuntime signedExtreme;
        s32 lowWordResult;
        s32 extremeResult;

        memset(&lowWords, 0, sizeof(lowWords));
        memset(&signedExtreme, 0, sizeof(signedExtreme));
        signedExtreme.x = INT_MIN;
        signedExtreme.z = INT_MIN;
        lowWordResult = FindTrackSegment(&lowWords, 0);
        extremeResult = FindTrackSegment(&signedExtreme, 0);
        if (extremeResult != lowWordResult) {
            fprintf(stderr,
                    "signed coordinate limits changed their low-word segment\n");
            return 1;
        }
    }

    if (out != NULL) {
        fclose(out);
    }
    if (s_digest != expected) {
        printf("find_track_segment: %d cases folded to %lu, expected %lu\n",
               cases, s_digest, expected);
        return 1;
    }
    printf("find_track_segment: %d cases unchanged\n", cases);
    return 0;
}
