#ifndef GAME_CAR_AUDIO_H
#define GAME_CAR_AUDIO_H

/* The player car's sound calls from the desktop's game/car_internal.h, whose
 * other declarations need the retail renderer headers the browser lacks. */
#include "common.h"
#include "game/car.h"
#include "game/car_drive.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"

void PlayCarDrivingVoice(const PlayerCarRuntime *car, const GameCarSpec *spec);
void PlayCarLaunchVoice(const PlayerCarRuntime *car);
void PlayCarAirborneVoice(const PlayerCarRuntime *car);
void PlayCarStandingStartVoice(const PlayerCarRuntime *car);
void PlayPlayerLandingCue(s32 landingFrames, int audible);
void PlayPlayerContactCue(const PlayerCarRuntime *car, s32 skid, s32 slip, int audible);

#endif
