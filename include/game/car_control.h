#ifndef GAME_CAR_CONTROL_H
#define GAME_CAR_CONTROL_H

#include "game/car.h"

typedef enum SteeringMode {
    STEERING_CENTER,
    STEERING_DIGITAL,
    STEERING_ANALOG,
    STEERING_AUTOMATIC,
} SteeringMode;

typedef struct SteeringInput {
    SteeringMode mode;
    int left;
    int right;
    /* Calibrated analog request in the simulation's steering units. */
    s32 angle;
} SteeringInput;

/* One driver's commands for one simulation step. Pedals use 0..256; shift
 * commands are edges. Device calibration and race-phase policy live outside
 * the simulation. This native struct is not a network wire format. */
typedef struct DriverInput {
    SteeringInput steering;
    s16 throttle;
    s16 brake;
    int shiftUp;
    int shiftDown;
} DriverInput;

void ApplyDriverInput(PlayerCarRuntime *car, const GameCarSpec *spec,
                       const DriverInput *input);
DriverInput ReadDriverInput(void);
/* Calibrated, bounded device controls, independent of single-player race phase
 * and auto-steer policy. Suitable for SetRaceInput on a selected human seat. */
DriverInput ReadCarControls(void);

void UpdateCarSteering(PlayerCarRuntime *car, const SteeringInput *input);

void UpdatePlayerSteeringTarget(PlayerCarRuntime *car);
void UpdateCarWheelRotation(GameCarRuntime *car);
/* Analog steering retains the retail release/hold behaviour. No device is
 * consulted here: the caller supplies the driver's control mode. */
void UpdateCarControlFeedback(PlayerCarRuntime *car, int analogSteering);

#endif
