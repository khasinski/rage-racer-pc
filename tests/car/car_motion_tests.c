/*
 * The three pieces of car motion nothing was watching.
 *
 * These motion functions were recovered beside the AI table walkers even
 * though they operate independently. They now live in focused modules; this
 * characterisation sweep protects their arithmetic while that structure and
 * the recovered expressions are cleaned up.
 *
 * They are arithmetic over one car, so this walks the states that decide which
 * branch runs and folds everything the call could have written into one
 * number. The sine table is the real one, because all three lean on it and a
 * stub that answers zero flattens exactly the arithmetic worth checking.
 */

#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[11];
TrackEventData *g_TrackEventData;
s32 g_RaceSeries;
s32 g_TrackLength;
CarCollisionPoint g_CarCollisionCorners[4];

void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode) {
    (void)car; (void)x; (void)z; (void)mode;
}
void TransformCollisionVector(const s16 *input, s32 *output) {
    (void)input; (void)output;
}
MATRIX *RotMatrix(SVECTOR *rotation, MATRIX *matrix) {
    (void)rotation;
    return matrix;
}
void SetRotMatrix(MATRIX *matrix) { (void)matrix; }

/* The console's quarter-turn sine table, mirrored into the other three. */
s32 rsin(s32 angle) {
    static const s32 quarter[17] = {0,     0x18F,  0x31F,  0x4AD, 0x63A, 0x7C4,
                                    0x94C, 0xACF,  0xC4E,  0xDC7, 0xF3A, 0x10A6,
                                    0x120A, 0x1365, 0x14B7, 0x15FF, 0x173C};
    s32 index = ((angle & 0xFFF) * 16) / 0x400;
    s32 sign = 1;

    if (index >= 32) {
        index -= 32;
        sign = -1;
    }
    if (index >= 16) {
        index = 32 - index;
    }
    return sign * quarter[index];
}
s32 rcos(s32 angle) { return rsin(angle + 0x400); }

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

/* Everything the three of them can move. */
static void RecordCar(const char *label, GameCarRuntime *car) {
    GameCarAiBlock *ai = GetCarAiBlock(car);
    s32 state[16];

    state[0] = car->motionMode;
    state[1] = car->motionModeTimer;
    state[2] = car->motionValue.value;
    state[3] = car->bodyKickOffset;
    state[4] = car->verticalPitch;
    state[5] = car->verticalRoll;
    state[6] = car->bodyPitch;
    state[7] = car->bodyRoll;
    state[8] = car->verticalMotionState;
    state[9] = car->verticalMotionRate;
    state[10] = car->verticalMotionTimer;
    state[11] = car->verticalTargetY;
    state[12] = car->y;
    state[13] = car->slideInput.value;
    state[14] = car->yawRate;
    state[15] = ai->yawRate;
    Record(label, state, 16);
}

static TrackEventData s_events;
static GameCarRuntime s_car;

/* A crest table the car can be walked over end to end. */
static void BuildEvents(void) {
    int i;

    TrackCrestEvent *row = s_events.crestEvents[0];

    memset(&s_events, 0, sizeof(s_events));
    /* Seven crests along the lap, alternating the way they throw the car,
     * and the sentinel that ends the list. */
    for (i = 0; i < 7; i++) {
        row[i].progress = i * 0x400;
        row[i].motionValue = ((i % 3) - 1) * 11;
    }
    row[7].motionValue = -1;
    g_TrackEventData = &s_events;
    g_TrackLength = 0x4000;
}

int main(int argc, char **argv) {
    /*
     * What these three did before anything moved them. Run the test with a
     * file name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 3499363269UL;
    static const s32 modes[] = {0, 1, 2, 3};
    static const s32 timers[] = {1, 2, 30};
    static const s32 amounts[] = {0, 0x100, -0x100};
    static const s32 hopStates[] = {0, 1, 2, 3};
    static const s32 hopTimers[] = {0, 1, 12, 300};
    static const s32 speeds[] = {0, 0x3C0, 0x3C1, 0x1000};
    static const s32 slides[] = {0, 1, -1, 0x20, -0x20};
    static const s32 rates[] = {0, 0x2BB, 0x2BC, -0x2BB, -0x2BC};
    static const s32 indices[] = {0, 1, 7};
    int mi, ti, ai_, hi, si, sl, ri, ii, series;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }
    BuildEvents();

    /* The body kick: a wave whose amplitude falls with the timer. */
    for (mi = 0; mi < 4; mi++)
    for (ti = 0; ti < 3; ti++)
    for (ai_ = 0; ai_ < 3; ai_++) {
        char label[96];

        memset(&s_car, 0, sizeof(s_car));
        s_car.motionMode = (s16)modes[mi];
        s_car.motionModeTimer = (s16)timers[ti];
        s_car.motionValue.value = amounts[ai_];
        s_car.bodyKickOffset = 0x40;
        sprintf(label, "kick mode%d timer%d amount%d", modes[mi], timers[ti],
                amounts[ai_]);
        Record(label, NULL, 0);
        UpdateCarBodyKick(&s_car);
        RecordCar("kicked", &s_car);
        steps++;
    }

    /* The hop: what a crest does to a car passing over it. */
    for (hi = 0; hi < 4; hi++)
    for (si = 0; si < 4; si++)
    for (ti = 0; ti < 4; ti++)
    for (ii = 0; ii < 8; ii++) {
        char label[128];

        memset(&s_car, 0, sizeof(s_car));
        s_car.verticalMotionState = (s16)hopStates[hi];
        /* A hop already under way squares its timer, so a zero one hides the
         * arithmetic that follows. */
        s_car.verticalMotionTimer = (s16)hopTimers[ti];
        s_car.verticalMotionRate = 3;
        s_car.verticalTargetY = 0x400;
        s_car.speed = speeds[si];
        s_car.trackProgress = ii * 0x400;
        /* Coming from behind the first crest as well as onto it, so the scan
         * of the list is entered at its first entry and not only later ones. */
        s_car.previousTrackProgress =
            (ii == 0) ? (g_TrackLength - 0x40) : (s_car.trackProgress - 0x40);
        s_car.bodyPitch = 0x100;
        s_car.bodyRoll = -0x100;
        s_car.verticalPitch = 0x20;
        s_car.verticalRoll = -0x20;
        s_car.y = 0x800;
        sprintf(label, "hop state%d speed%d timer%d at%d", hopStates[hi],
                speeds[si], hopTimers[ti], ii * 0x400);
        Record(label, NULL, 0);
        UpdateCarCrestHop(&s_car);
        RecordCar("hopped", &s_car);
        steps++;
    }

    /* The slide: started by a car off the line, then decayed away. */
    for (sl = 0; sl < 5; sl++)
    for (ri = 0; ri < 5; ri++)
    for (si = 0; si < 4; si++)
    for (ii = 0; ii < 3; ii++)
    for (series = 0; series < 2; series++) {
        char label[112];

        memset(&s_car, 0, sizeof(s_car));
        s_car.slideInput.value = slides[sl];
        s_car.yawRate = (s16)rates[ri];
        s_car.speed = speeds[si];
        GetCarAiBlock(&s_car)->slideInput.value = slides[sl];
        GetCarAiBlock(&s_car)->yawRate = (s16)rates[ri];
        g_RaceSeries = series;
        sprintf(label, "slide in%d rate%d speed%d car%d series%d", slides[sl],
                rates[ri], speeds[si], indices[ii], series);
        Record(label, NULL, 0);
        UpdateCarSlideAngle(&s_car, indices[ii]);
        RecordCar("slid", &s_car);
        steps++;
    }

    /*
     * A course that fills all eight crest slots, so the last one is a crest
     * and not the sentinel. A scan that stops one entry early still answers
     * correctly on a track that ends its list early; only a full one tells
     * the two apart.
     */
    {
        TrackCrestEvent *row = s_events.crestEvents[0];
        int slot;

        /* The crest list is per series, and the sweep above left the reverse
         * one selected. */
        g_RaceSeries = 0;
        for (slot = 0; slot < 8; slot++) {
            row[slot].progress = slot * 0x400;
            row[slot].motionValue = (slot + 1) * 7;
        }
        for (ii = 0; ii < 8; ii++) {
            char label[96];

            memset(&s_car, 0, sizeof(s_car));
            s_car.speed = 0x800;
            s_car.trackProgress = ii * 0x400;
            s_car.previousTrackProgress = s_car.trackProgress - 0x40;
            sprintf(label, "full table at%d", ii * 0x400);
            Record(label, NULL, 0);
            UpdateCarCrestHop(&s_car);
            RecordCar("hopped", &s_car);
            steps++;
        }
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL car motion behaves differently: %d states making %d "
               "records digest to %lu, expected %lu\n", steps, s_calls,
               s_digest, expected);
        return 1;
    }
    printf("car motion takes the same %d states it always did\n", steps);
    return 0;
}
