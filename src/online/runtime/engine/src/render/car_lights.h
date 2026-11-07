#ifndef CAR_LIGHTS_H
#define CAR_LIGHTS_H

/* Owned by each car. Advance once per simulation tick, never per view. */
typedef struct CarLights {
    float headlights;
    float tail;
    float stop;
    int automatic;
} CarLights;

/* daylight and shelter are linear luminances in [0, 1]. shelter is the
 * fraction of outdoor light reaching the car (1 outdoors, 0 fully enclosed).
 * braking is a driving intent, not a measured drop in speed. */
void UpdateCarLights(CarLights *lights, float daylight, float shelter,
                     int braking, float seconds);

#endif
