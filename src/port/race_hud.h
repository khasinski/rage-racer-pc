#ifndef PORT_RACE_HUD_H
#define PORT_RACE_HUD_H
#include "game/race_sim.h"
#include "game/engine_sound.h"

/* Draw a human seat from an explicit race and per-car presentation state.
 * Invalid/absent/retired/AI seats draw nothing. No GP time-limit policy. */
int DrawSimHud(const RaceSim *race, s32 seat, const EngineSound *engine,
                TachometerLightingMode lighting, s32 blendAmount);
#endif
