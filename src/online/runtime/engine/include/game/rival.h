#ifndef GAME_RIVAL_H
#define GAME_RIVAL_H
#include "game/car.h"
#include "game/track.h"

typedef struct TrafficCar {
    const GameCarRuntime *car;
    int human;
} TrafficCar;

/* Read a field of humans and AI; only the avoiding rival is changed. */
void AvoidRivalTraffic(GameCarRuntime *car, s32 slot, s32 trackLength,
                       const TrafficCar *field, s32 count);

/* Select the authored configuration by logical model ID, not grid position.
 * configs contains TRACK_RIVAL_COUNT entries; invalid IDs use entry zero. */
void ConfigureRival(GameCarRuntime *car, const TrackRivalAiConfig *configs, s32 model,
                      s32 trackLength, s32 gridPosition);
void SeedRivalSpeedKey(GameCarRuntime *car,
                         const TrackAiSpeedKey table[TRACK_AI_SPEED_KEY_COUNT]);
void StepRivalTargetSpeed(GameCarRuntime *car, s32 carIndex,
                            const TrackAiSpeedKey table[TRACK_AI_SPEED_KEY_COUNT], int reverse);
void StepRivalAcceleration(GameCarRuntime *car, int racing);
void StepRivalLine(GameCarRuntime *car, s32 carIndex,
                     const TrackRacingLineHint hints[TRACK_RACING_LINE_HINT_COUNT]);
void MoveRival(GameCarRuntime *car, s32 slot);
void FinishRival(GameCarRuntime *car, const TrackEventData *events,
                    s32 trackLength, int reverse);
/* Called after the field has updated every car's lap progress. */
void PlaceRival(GameCarRuntime *car, const TrackRoute *route, int reverse);
void SteerRival(GameCarRuntime *car, const TrackRoute *route, int reverse);
void ClampRivalLine(GameCarRuntime *car, s32 slot, const TrackRoute *route);
int InitRival(GameCarRuntime *car, const TrackRoute *route,
                 const TrackRivalStart *start, s32 walkStart, int reverse, u16 model);
/* Authored AI motion before field collision response and FinishRival.
 * Traffic is an optional immutable pre-movement field (excluding this car).
 * The field owner supplies collision flags separately. */
int AdvanceRival(GameCarRuntime *car, const TrackRoute *route,
                    const TrackEventData *events, s32 slot, int reverse,
                    const TrafficCar *traffic, s32 count);
#endif
