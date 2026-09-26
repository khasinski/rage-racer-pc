#ifndef DRIVER_FIXTURE_H
#define DRIVER_FIXTURE_H

#include "game/driver.h"

static void PrepareDriver(PlayerCarRuntime *car, GameCarSpec *spec, CarPerformance *engine) {
    *car = (PlayerCarRuntime){0};
    *spec = (GameCarSpec){.topGear = 6, .revLimit = 15000, .redline = 8000,
        .automaticAccelerationScale = 1000, .baseSteeringGrip = 100,
        .steeringGripResponse = 1000, .speedDragDivisor = 1000, .steerResponse = 20};
    for (int i = 0; i < CAR_TORQUE_CURVE_SAMPLE_COUNT; i++) {
        spec->torqueCurve[i] = (i + 1) * 20000;
        spec->torqueBand.values[i] = i * 1000;
    }
    for (int i = 1; i <= CAR_FORWARD_GEAR_COUNT; i++) {
        spec->gearRatio[i] = 100;
        spec->torqueScale[i - 1] = 100;
    }
    PrepareCarPerformance(&car->drive, spec, engine);
    car->x = 200;
    car->speed = 500;
    car->bodyYaw = car->headingAngle = car->drive.targetHeading = 1024;
    car->drive.gear = car->drive.gearDisp = 1;
    car->drive.manual = car->drive.drivetrainCoupled = 1;
    car->drive.engineRpm = 1500;
    car->drive.drivetrainTorque = GetCarGearLoad(spec, 1) * 1500;
    car->drive.dragScale = 1000;
}

#endif
