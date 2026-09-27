#include "game/driver.h"
#include "game/rival.h"
#include <stddef.h>

static int Active(const DriverSeat *seat) {
    return seat->car != NULL && seat->car->activeFlag != -1;
}

int StepDriverField(DriverSeat *seats, s32 count) {
    const TrackRoute *route = NULL;
    int reverse = 0;
    if (seats == NULL || count < 1 || count > DRIVER_SEAT_LIMIT) return 0;
    for (s32 i = 0; i < count; i++) {
        if (!Active(&seats[i])) continue;
        const DriverContext *context = seats[i].context;
        if (context == NULL ||
            context->route == NULL || context->route->points == NULL ||
            context->route->count <= 0 || context->route->length <= 0 ||
            (context->reverse != 0 && context->reverse != 1) ||
            seats[i].hull.points == NULL || seats[i].hull.corners == NULL) return 0;
        if (seats[i].rival) {
            if (context->events == NULL || seats[i].rivalCorners == NULL ||
                (u32)seats[i].rivalSlot >= RACE_CAR_SLOT_COUNT) return 0;
        } else if (context->spec == NULL || context->performance == NULL ||
                   context->launchThreshold == NULL || context->corners == NULL) return 0;
        if (route != NULL && (route != context->route || reverse != context->reverse)) return 0;
        route = context->route;
        reverse = context->reverse;
        for (s32 j = 0; j < i; j++) {
            if (seats[i].car == seats[j].car) return 0;
            if (Active(&seats[j]) && seats[i].rival && seats[j].rival &&
                seats[i].rivalSlot == seats[j].rivalSlot) return 0;
        }
    }
    CarCollider field[DRIVER_SEAT_LIMIT] = {0};
    GameCarRuntime previous[DRIVER_SEAT_LIMIT] = {0};
    TrafficCar traffic[DRIVER_SEAT_LIMIT] = {0};
    for (s32 i = 0; i < count; i++) {
        if (!Active(&seats[i])) continue;
        previous[i] = *AsRivalCar(seats[i].car);
        traffic[i] = (TrafficCar){&previous[i], !seats[i].rival};
    }
    for (s32 i = 0; i < count; i++) {
        seats[i].step = (DriverStep){.skidAngle = -1};
        seats[i].crashed = 0;
        if (!Active(&seats[i])) continue;
        seats[i].car->collisionFlag = 0;
        if (seats[i].rival) {
            /* All AI scan the same pre-movement field. Exclude their own
             * snapshot; its address differs from the live car's address. */
            traffic[i].car = NULL;
            AdvanceRival(AsRivalCar(seats[i].car), route, seats[i].context->events,
                         seats[i].rivalSlot, reverse, traffic, count);
            traffic[i].car = &previous[i];
        } else {
            seats[i].step = MoveDriver(seats[i].car, &seats[i].input,
                                        seats[i].context, &seats[i].random);
        }
        field[i] = (CarCollider){.car = AsRivalCar(seats[i].car),
                                .corners = seats[i].hull.corners};
    }
    DriverContact contacts[DRIVER_SEAT_LIMIT][DRIVER_SEAT_LIMIT] = {0};
    for (s32 i = 0; i < count; i++) {
        if (!Active(&seats[i])) continue;
        /* Slipstream sampled after movement affects the next engine step. */
        if (!seats[i].rival) {
            seats[i].car->drive.dragScale = 1000;
            FindCarContact(seats[i].car, seats[i].hull.points, field, count, route->length);
        }
        const DriverHull firstHull = seats[i].hull;
        for (s32 j = i + 1; j < count; j++) {
            if (!Active(&seats[j])) continue;
            const DriverHull secondHull = seats[j].hull;
            if (seats[i].rival && seats[j].rival) {
                contacts[i][j].firstRegion = FindRivalContact(AsRivalCar(seats[i].car),
                    seats[i].rivalCorners, AsRivalCar(seats[j].car), seats[j].rivalCorners, route->length);
            } else if (!seats[i].rival && !seats[j].rival) {
                contacts[i][j] = FindDriverContact(seats[i].car, &firstHull,
                    seats[j].car, &secondHull, route->length);
            } else {
                const s32 human = seats[i].rival ? j : i;
                const s32 rival = seats[i].rival ? i : j;
                PlayerCarRuntime copy = *seats[human].car;
                const CarContact hit = FindCarContact(&copy, seats[human].hull.points,
                    &field[rival], 1, route->length);
                if (human == i) contacts[i][j].firstRegion = hit.region;
                else contacts[i][j].secondRegion = hit.region;
            }
        }
    }
    for (s32 i = 0; i < count; i++) {
        for (s32 j = i + 1; j < count; j++) {
            const DriverContact hit = contacts[i][j];
            if (hit.firstRegion == 0 && hit.secondRegion == 0) continue;
            if (seats[i].rival && seats[j].rival) {
                ApplyRivalCollision(AsRivalCar(seats[i].car), AsRivalCar(seats[j].car), hit.firstRegion);
            } else if (!seats[i].rival && !seats[j].rival) {
                ApplyDriverCollision(seats[i].car, seats[j].car,
                    hit.firstRegion, hit.secondRegion, reverse,
                    seats[i].wrongWayFrames, seats[j].wrongWayFrames);
            } else {
                const s32 human = seats[i].rival ? j : i;
                const s32 rival = seats[i].rival ? i : j;
                ApplyCarCollision(seats[human].car, AsRivalCar(seats[rival].car),
                    human == i ? hit.firstRegion : hit.secondRegion, reverse, seats[human].wrongWayFrames);
                seats[human].car->collisionFlag = 1;
            }
            seats[i].crashed = seats[j].crashed = 1;
        }
    }
    for (s32 i = 0; i < count; i++) {
        if (!Active(&seats[i])) continue;
        if (seats[i].rival) {
            FinishRival(AsRivalCar(seats[i].car), seats[i].context->events, route->length, reverse);
        } else {
            FinishDriver(seats[i].car, seats[i].context, &seats[i].random,
                           seats[i].crashed, &seats[i].step);
        }
    }
    return 1;
}
