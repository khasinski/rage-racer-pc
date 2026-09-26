#include "race_view.h"
#include "car_parts.h"
#include "game/asset_index.h"
#include "render/track_lighting.h"
#include "render/car_paint.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static int Visible(const SimDriver *driver) {
    return driver->status == SIM_DRIVER_FINISHED ||
           (driver->status == SIM_DRIVING && driver->car.activeFlag != -1);
}

void FreeRaceView(RaceView *view) {
    if (view == NULL) return;
    for (unsigned variant = 0; variant < CAR_MODEL_VARIANT_COUNT; variant++)
        FreeCarModelData(view->models[variant]);
    free(view);
}

RaceView *LoadRaceView(const RaceData *archive, const RaceSim *race,
                       const RaceCarLook looks[DRIVER_SEAT_LIMIT]) {
    if (archive == NULL || race == NULL || looks == NULL) return NULL;
    RaceView *view = calloc(1, sizeof(*view));
    if (view == NULL) return NULL;
    memcpy(view->looks, looks, sizeof(view->looks));
    for (unsigned seat = 0; seat < DRIVER_SEAT_LIMIT; seat++) {
        const SimDriver *driver = &race->drivers[seat];
        if (!Visible(driver)) continue;
        view->cars[seat] = view->previousCars[seat] = *AsConstRivalCar(&driver->car);
        view->carSeen[seat] = 1;
        if (driver->rival) {
            if ((u32)driver->car.modelIndex >= RACE_CAR_SLOT_COUNT) {
                FreeRaceView(view);
                return NULL;
            }
            continue;
        }
        s32 variant = looks[seat].variant;
        if ((u32)variant >= CAR_MODEL_VARIANT_COUNT ||
            (driver->variant >= 0 && driver->variant != variant)) {
            FreeRaceView(view);
            return NULL;
        }
        if (view->models[variant] == NULL) {
            view->models[variant] = CopyRaceCarModel(archive, variant);
            if (view->models[variant] == NULL) {
                FreeRaceView(view);
                return NULL;
            }
        }
    }
    return view;
}

static int ValidField(const RaceSim *race, const RaceView *view) {
    if (race == NULL || view == NULL) return 0;
    for (unsigned seat = 0; seat < DRIVER_SEAT_LIMIT; seat++) {
        const SimDriver *driver = &race->drivers[seat];
        if (!Visible(driver)) continue;
        if (driver->rival) {
            if ((u32)driver->car.modelIndex >= RACE_CAR_SLOT_COUNT) return 0;
        } else if ((u32)view->looks[seat].variant >= CAR_MODEL_VARIANT_COUNT ||
                   (driver->variant >= 0 && driver->variant != view->looks[seat].variant) ||
                   view->models[view->looks[seat].variant] == NULL) return 0;
    }
    return 1;
}

int TickRaceView(RaceView *view, const RaceSim *race, float daylight) {
    if (!isfinite(daylight) || daylight < 0 || daylight > 1 || !ValidField(race, view) || (view->tickSeen && race->tick <= view->tick)) return 0;
    for (unsigned seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        if (!Visible(driver)) { view->carSeen[seat] = 0; continue; }
        const GameCarRuntime *car = AsConstRivalCar(&driver->car);
        view->previousCars[seat] = view->carSeen[seat] ? view->cars[seat] : *car;
        view->cars[seat] = *car;
        view->carSeen[seat] = 1;
        const TrackZoneEffect zone = ReadTrackZoneEffect(race->events, driver->car.trackProgress,
                                                         race->route.length, race->reverse);
        const float shelter = 1.0f - (float)zone.blend / 256.0f * 0.75f;
        UpdateCarLights(&view->lamps[seat], daylight, shelter,
                        AsConstRivalCar(&driver->car)->brakeInput > 0, 1.0f / SIM_TICK_RATE);
        if (driver->rival) continue;
        StepEngineSound(&view->engines[seat], &driver->car.drive,
                        &driver->spec, race->tick, driver->random);
    }
    view->tick = race->tick;
    view->tickSeen = 1;
    return 1;
}

int SubmitRaceView(const RaceSim *race, const RaceView *view,
                   const RivalLook *rivals, u32 trackAsset, u8 textureVariant, RenderWorld *world) {
    if (!ValidField(race, view) || textureVariant >= CAR_MODEL_VARIANT_COUNT) return 0;
    RenderMeshInstance field[DRIVER_SEAT_LIMIT * CAR_PART_COUNT];
    u32 count = 0;
    for (unsigned seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        if (!Visible(driver)) continue;
        const GameCarRuntime *car = AsConstRivalCar(&driver->car);
        const TrackZoneEffect zone = ReadTrackZoneEffect(race->events, car->trackProgress,
                                                        race->route.length, race->reverse);
        float light[3];
        TrackZoneLightColor(zone.blend, zone.code, light);
        RenderMeshInstance body = {.entity = seat, .assetSource = RENDER_ASSET_OWNED,
            .flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING | RAGE_RENDER_INSTANCE_ENABLE_FOG,
            .environmentLight = {light[0], light[1], light[2]}, .lamps = view->lamps[seat]};
        const CarShape *shape;
        u32 rear, front;
        if (driver->rival) {
            if (!rivals || rivals[car->modelIndex].palette > 2) return 0;
            const RivalLook *look = &rivals[car->modelIndex];
            body.assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
            body.assetKey = trackAsset;
            body.mesh = look->bodyMesh;
            body.materialVariant = (u8)(textureVariant * 3 + look->palette);
            rear = body.mesh + 3; front = body.mesh + 2;
            shape = &look->shape;
        } else {
            const RaceCarLook *look = &view->looks[seat];
            body.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
            body.assetKey = (u32)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, look->variant);
            if (look->hasPaint && look->paint.paintColor1 < RAGE_CAR_PAINT_COLOR_COUNT &&
                look->paint.paintColor2 < RAGE_CAR_PAINT_COLOR_COUNT) {
                body.hasCarPaint = 1;
                body.carPaintColor1 = look->paint.paintColor1;
                body.carPaintColor2 = look->paint.paintColor2;
            }
            u32 wheel = (u32)car->renderDepth * 2;
            if (car->wheelRotation & 0x1000) wheel += 10;
            if (wheel + 3 >= 22) wheel = 0;
            rear = wheel + 3; front = wheel + 2;
            shape = &view->models[look->variant]->shape;
        }
        if (!BuildCarInstances(car, &view->previousCars[seat], shape, &body,
                               rear, front, &field[count])) return 0;
        count += CAR_PART_COUNT;
    }
    if (!RenderWorldCarFieldFits(world, DRIVER_SEAT_LIMIT, count) ||
        !RenderWorldBeginCarField(world, DRIVER_SEAT_LIMIT)) return 0;
    for (u32 part = 0; part < count; ++part) RenderWorldSubmitMesh(world, &field[part]);
    return 1;
}
