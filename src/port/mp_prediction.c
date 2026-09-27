#include "mp_client.h"

int MpApplyConfig(RaceSim *race, const MpCarConfig *config) {
    if (!race || !config || race->phase != SIM_SETUP) return 0;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        if (config->variant[seat] >= CAR_MODEL_VARIANT_COUNT ||
            driver->variant != config->variant[seat] || driver->rival || driver->status != SIM_DRIVING) return 0;
    }
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
        if (!ConfigureRaceDriver(race, seat, &config->specs[seat])) return 0;
    return 1;
}

int MpPredictRace(const RaceSim *authority, const MpCommands *commands,
                   const MpCommand pending[2], unsigned count, int seat,
                   uint32_t target, RaceSim *out) {
    if (!authority || !out || out == authority || !commands || count > 2 ||
        (count && !pending) || (unsigned)seat >= MP_SEAT_LIMIT ||
        target < authority->tick || (uint64_t)target > (uint64_t)authority->tick + MP_PREDICTION_LEAD ||
        authority->drivers[seat].rival || authority->drivers[seat].status == SIM_EMPTY) return 0;
    RaceFrame frame;
    if (!SaveRaceFrame(authority, &frame)) return 0;
    uint32_t previousTick = commands->lastTick;
    for (unsigned index = 0; index < count; ++index) {
        if (!ValidDriverInput(&pending[index].input) || pending[index].tick < previousTick ||
            (pending[index].sequence && (index || commands->sent == UINT32_MAX ||
                pending[index].sequence != commands->sent + 1)) ||
            (!pending[index].sequence && index + 1 != count)) return 0;
        previousTick = pending[index].tick;
    }
    uint32_t sequence = commands->acknowledged;
    DriverInput controls = authority->drivers[seat].input;
    /* Validate the complete command ring even when no tick needs advancing. */
    if (!MpReplayInput(commands, authority->tick, &sequence, &controls)) return 0;
    sequence = commands->acknowledged;
    *out = *authority;
    /* A completed/retired local driver waits for authoritative results. There
     * is no local motion to predict, even while the other entrants still race. */
    if (out->drivers[seat].status != SIM_DRIVING) return 1;
    unsigned nextPending = 0;
    while (out->tick < target && out->phase != SIM_FINISHED &&
           out->drivers[seat].status == SIM_DRIVING) {
        controls = out->drivers[seat].input;
        if (!MpReplayInput(commands, out->tick + 1, &sequence, &controls)) return 0;
        while (nextPending < count && pending[nextPending].tick <= out->tick + 1) {
            const DriverInput *sample = &pending[nextPending++].input;
            int up = controls.shiftUp || sample->shiftUp;
            int down = controls.shiftDown || sample->shiftDown;
            controls = *sample;
            controls.shiftUp = up; controls.shiftDown = down;
        }
        if (out->drivers[seat].status == SIM_DRIVING && !SetRaceInput(out, seat, &controls)) return 0;
        if (!StepRaceSim(out)) return 0;
    }
    return 1;
}
