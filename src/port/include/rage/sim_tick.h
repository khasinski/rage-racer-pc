#ifndef RAGE_SIM_TICK_H
#define RAGE_SIM_TICK_H

#include <stddef.h>
#include <stdint.h>

#include "game/camera_types.h"
#include "game/car.h"
#include "game/player_car_internal.h"

/*
 * Experimental double-rate race simulation.
 *
 * Retail steps the race once per two video fields (30 Hz NTSC, 25 Hz PAL),
 * with every coefficient written per step. With game.tick_rate = double the
 * live part of the race advances once per field instead. The retail step is
 * kept as the single source of truth for the dynamics and is applied as half
 * a step: the state moves by half of what one retail step would change. A
 * decay such as speed * 94 / 100 becomes roughly 97 / 100 twice, an
 * integration such as x += motion advances by motion / 2, and the fixed points
 * (top speed, steady-state steering) stay where retail put them.
 *
 * Everything that counts frames (race clock, laps, countdown, cues, replay
 * recording) still runs once per retail step, on the first field of each pair.
 */

typedef enum SimFieldKind {
    SIM_FIELD_S32,
    SIM_FIELD_S16,
    SIM_FIELD_COUNTDOWN_S16, /* loaded outright when rising; half-stepped while counting down */
    SIM_FIELD_ANGLE32 /* 12-bit angle held in an s32; delta takes the short way */
} SimFieldKind;

typedef struct SimField {
    size_t offset;
    SimFieldKind kind;
} SimField;

/* Half of the change from `before` to `after`, rounded so that two equal
 * consecutive deltas add up exactly. `parity` is 0 or 1 and alternates per
 * tick. */
int32_t SimHalfDelta(int32_t delta, unsigned parity);

/* Replaces each listed field of `current` (the state after a retail step)
 * with `before` plus half the step. Fields that are not listed keep the
 * value the retail step gave them. */
void SimBlendFields(void *current, const void *before, const SimField *fields,
                    size_t count, unsigned parity);

/* The configured race tick rate: 1 for retail, 2 for double. */
int SimTickMultiplier(void);
/* Forces the multiplier regardless of configuration (tests). */
void SimTickSetMultiplier(int multiplier);

/* Called once at the top of every race scene frame. `live` is non-zero while
 * the race is in the phase that runs at double rate. */
void SimRaceFrameBegin(int live);
/* True when this frame is a double-rate tick (live and multiplier 2). */
int SimRaceScaled(void);
/* True on the second field of a retail pair: logic that counts frames must
 * not run. */
int SimRaceSubstep(void);
/* 0 or 1, alternating per scaled tick. */
unsigned SimRaceParity(void);
/* The frame-sync threshold the race scene should wait for this frame. */
int SimRaceFrameSyncThreshold(int retailThreshold);

/* Pad edges seen on a substep are handed to the next full step. */
void SimRaceCarryPadEdges(unsigned short *pressed);

/* Retail step when not scaled; half step otherwise. */
void SimUpdatePlayerCar(PlayerCarRuntime *car);
void SimUpdateRaceCars(void);
void SimUpdateCamera(Camera *camera, CameraViewMode mode, GameCarRuntime *car);

/* RAGE_PORT_RACE_TRACE=1 prints one line per live tick on stderr. */
void SimRaceTrace(void);

#endif
