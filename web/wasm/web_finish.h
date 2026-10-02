/* A finished car's run-out (web_finish.c). Past the line the simulation
 * leaves the car where it crossed. Presentation keeps it on the road for
 * FINISH_COAST_STEPS physics steps (2.5 s), braking to a stop, then removes
 * the whole car. A partial vertex alpha is not used: the shell is drawn
 * blended with depth writes off, which shows the cockpit. */
#ifndef WEB_FINISH_H
#define WEB_FINISH_H
#include "game/car.h"
#include "game/track.h"

enum { FINISH_COAST_STEPS = 62 };

/* A server correction still easing out of a car's drawn pose (rage_web.c). */
typedef struct WebSmooth { float x, y, z, yaw; } WebSmooth;

typedef struct FinishRun {
    int steps;          /* physics steps since the finish, 0 while racing */
    float motion[3];    /* its last step's motion while driving */
    s32 raw[3];         /* its last simulated position (no smoothing) */
    int placed;         /* route snapshot taken */
    s32 pointIndex;
    s32 fraction;       /* 0..0x400 along the current segment */
    s32 lateral;        /* lane offset at the line, world units */
    s32 yawSlip;        /* body yaw relative to the route heading */
    s32 progress;       /* trackProgress, advanced with the coast */
    float along;        /* world units per step toward increasing point index */
    float errX, errY, errZ;
    float pitchErr, rollErr;
} FinishRun;

/* Forgets a run-out, for a car that is racing again. */
void FinishRunReset(FinishRun *run);
/* One physics step of a finished car: `pose` (its simulated pose) becomes
 * the drawn one, following the road from where it crossed the line. */
void FinishRunCoast(FinishRun *run, const TrackRoute *route, int reverse,
                    const WebSmooth *smooth, PlayerCarRuntime *pose);
/* The run-out is over and the car is no longer drawn. */
static inline int FinishRunGone(const FinishRun *run) { return run->steps > FINISH_COAST_STEPS; }
#endif
