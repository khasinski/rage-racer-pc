/*
 * Where the AI steering puts a car's heading.
 *
 * The function aims the car at a point on the racing line two track points
 * ahead of it, offset sideways by however far off the line the car is meant
 * to run, and turns the heading part of the way towards it. It had no test,
 * and it is written in the shape it was recovered in, which is why one is
 * needed before the shape can be changed: the arithmetic truncates in several
 * places and the truncation is part of the angle.
 *
 * The track around the car is supplied here rather than loaded, so the sweep
 * is repeatable: a circle of points, with the interpolation and the smoothed
 * angle answered from the index. What matters is not that this is a real
 * course but that every case reaches the same code with the same numbers.
 */

#include "common.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/render.h"
#include "game/track.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

void SteerCarToTrackLine(PlayerCarRuntime *car);

s32 g_TrackPointCount;
GameCarSpec *g_CarSpec;

/*
 * A ring of points a thousand units across, so an index maps to a position
 * and an angle without a course being loaded.
 */
void InterpolateTrackPoint(s32 pointIndex, s32 *out, s32 weight) {
    s32 angle = (pointIndex * 4096) / (g_TrackPointCount > 0 ? g_TrackPointCount : 1);
    angle = (angle + weight / 16) & 0xFFF;
    out[0] = (rsin(angle) * 1000) >> 12;
    out[1] = 0;
    out[2] = (rcos(angle) * 1000) >> 12;
}

s32 SmoothTrackAngle(s32 pointIndex, s32 weight) {
    s32 angle = (pointIndex * 4096) / (g_TrackPointCount > 0 ? g_TrackPointCount : 1);
    return (angle + weight / 16) & 0xFFF;
}

/*
 * The steering shares a translation unit with the jump handlers, which reach
 * for the render state, the camera and the sound effects. None of that runs in
 * this sweep, so answer it rather than link half the game in. Random15 is
 * answered with a fixed sequence: a sweep whose results depend on chance
 * cannot be folded into one number.
 */
GameRenderState g_RenderState;

void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z, int32_t pitch,
                              int32_t yaw, int32_t roll) {
    (void)x; (void)y; (void)z; (void)pitch; (void)yaw; (void)roll;
}

MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2) {
    (void)m0;
    (void)m1;
    return m2;
}

s32 Random15(void) {
    static s32 state;
    state = (state * 1103515245 + 12345) & 0x7FFF;
    return state;
}

void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume) {
    (void)index;
    (void)phase;
    (void)volume;
}

static unsigned long s_digest = 2166136261UL;

static void Fold(FILE *out, const char *label, const PlayerCarRuntime *car) {
    char line[256];
    const char *p;

    snprintf(line, sizeof(line), "%s -> heading=%d\n", label,
             car->headingAngle);
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (out != NULL) fputs(line, out);
}

int main(int argc, char **argv) {
    /*
     * What the function did before it was touched. Run it with a file name to
     * write the sweep out and diff two runs to see which cases moved.
     */
    static const unsigned long expected = 1996295519UL;
    static const s32 lateralOffsets[] = {0, 500, -500, 4096, -4096};
    static const s32 pointIndices[] = {0, 1, 7, 15};
    static const s32 headings[] = {0, 0x400, 0x800, 0xC00};
    static const s32 fractions[] = {0, 2048, 4095};
    /* The last one is past a signed 16-bit turn, which the divisor truncates
     * to; the sweep carries it so the truncation stays on the record. */
    static const u16 steerResponses[] = {1, 10, 30, 40000};
    static const s32 launchDirections[] = {0, 1};
    static const s16 verticalStates[] = {0, 1};
    GameCarSpec spec;
    FILE *out = NULL;
    size_t l, p, h, f, s, d, v;
    int cases = 0;

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            fprintf(stderr, "cannot write %s\n", argv[1]);
            return 2;
        }
    }

    g_TrackPointCount = 16;
    memset(&spec, 0, sizeof(spec));
    g_CarSpec = &spec;

    for (l = 0; l < sizeof(lateralOffsets) / sizeof(lateralOffsets[0]); l++)
    for (p = 0; p < sizeof(pointIndices) / sizeof(pointIndices[0]); p++)
    for (h = 0; h < sizeof(headings) / sizeof(headings[0]); h++)
    for (f = 0; f < sizeof(fractions) / sizeof(fractions[0]); f++)
    for (s = 0; s < sizeof(steerResponses) / sizeof(steerResponses[0]); s++)
    for (d = 0; d < sizeof(launchDirections) / sizeof(launchDirections[0]); d++)
    for (v = 0; v < sizeof(verticalStates) / sizeof(verticalStates[0]); v++) {
        PlayerCarRuntime car;
        char label[160];

        memset(&car, 0, sizeof(car));
        spec.steerResponse = steerResponses[s];
        car.trackLateralOffset = lateralOffsets[l];
        car.trackPointIndex = pointIndices[p];
        car.segmentFraction = fractions[f];
        car.headingAngle = (s16)headings[h];
        car.verticalMotionState = verticalStates[v];
        car.drive.launchDirection = (s16)launchDirections[d];
        car.x = 100;
        car.z = 900;

        snprintf(label, sizeof(label),
                 "lat=%d point=%d heading=%d frac=%d steer=%u dir=%d vert=%d",
                 lateralOffsets[l], pointIndices[p], headings[h], fractions[f],
                 (unsigned)steerResponses[s], launchDirections[d],
                 verticalStates[v]);
        SteerCarToTrackLine(&car);
        Fold(out, label, &car);
        cases++;
    }

    if (out != NULL) fclose(out);
    if (s_digest != expected) {
        printf("steer_to_track_line: %d cases folded to %lu, expected %lu\n",
               cases, s_digest, expected);
        return 1;
    }
    printf("steer_to_track_line: %d cases unchanged\n", cases);
    return 0;
}
