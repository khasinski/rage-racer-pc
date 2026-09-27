/* Pure multiplayer data operations: no socket or host-clock dependency. */
#include "mp_client.h"
#include "runtime_parse.h"
#include "game/race_sim.h"
#include "client_race.h"

#include <limits.h>
#include <string.h>

_Static_assert(CAR_MODEL_VARIANT_COUNT <= 32, "availability mask capacity");
int MpEncodeAvailability(uint32_t mask, uint8_t *wire, size_t size) {
    if (!wire || size != MP_AVAILABILITY_WIRE_SIZE || (uint64_t)mask >> CAR_MODEL_VARIANT_COUNT) return 0;
    wire[0] = 0x89; wire[1] = 1;
    for (unsigned byte = 0; byte < 4; ++byte) wire[byte + 2] = (uint8_t)(mask >> (byte * 8));
    return 1;
}
int MpDecodeAvailability(const uint8_t *wire, size_t size, uint32_t *mask) {
    if (!wire || !mask || size != MP_AVAILABILITY_WIRE_SIZE || wire[0] != 0x89 || wire[1] != 1) return 0;
    uint32_t decoded = 0;
    for (unsigned byte = 0; byte < 4; ++byte) decoded |= (uint32_t)wire[byte + 2] << (byte * 8);
    if ((uint64_t)decoded >> CAR_MODEL_VARIANT_COUNT) return 0;
    *mask = decoded;
    return 1;
}

int MpEncodeConfig(const MpCarConfig *config, uint8_t *wire, size_t size) {
    if (!config || !wire || size != MP_CONFIG_WIRE_SIZE) return 0;
    uint8_t packet[MP_CONFIG_WIRE_SIZE] = {0x88, 1};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        if (config->variant[seat] >= CAR_MODEL_VARIANT_COUNT) return 0;
        size_t offset = 2 + seat * (1 + CAR_SPEC_WIRE_SIZE);
        packet[offset] = config->variant[seat];
        if (!EncodeCarSpec(&config->specs[seat], packet + offset + 1, CAR_SPEC_WIRE_SIZE)) return 0;
    }
    memcpy(wire, packet, sizeof(packet));
    return 1;
}

int MpDecodeConfig(const uint8_t *wire, size_t size, MpCarConfig *config) {
    if (!wire || !config || size != MP_CONFIG_WIRE_SIZE || wire[0] != 0x88 || wire[1] != 1) return 0;
    MpCarConfig decoded = {0};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        size_t offset = 2 + seat * (1 + CAR_SPEC_WIRE_SIZE);
        decoded.variant[seat] = wire[offset];
        if (decoded.variant[seat] >= CAR_MODEL_VARIANT_COUNT ||
            !DecodeCarSpec(wire + offset + 1, CAR_SPEC_WIRE_SIZE, &decoded.specs[seat])) return 0;
    }
    *config = decoded;
    return 1;
}

static int ValidAcknowledgements(const RaceSim *race, const uint32_t *acknowledged) {
    if (!race || !acknowledged) return 0;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        if (driver->rival || (acknowledged[seat] &&
            (driver->status == SIM_EMPTY || !driver->inputTick))) return 0;
    }
    return 1;
}

int MpEncodeCorrection(const RaceSim *race, const uint32_t acknowledged[MP_SEAT_LIMIT],
                       uint8_t *wire, size_t size) {
    if (!wire || size != MP_CORRECTION_WIRE_SIZE ||
        !ValidAcknowledgements(race, acknowledged)) return 0;
    /* Encode the fallible part before publishing any envelope bytes. */
    uint8_t packet[MP_CORRECTION_WIRE_SIZE];
    if (!EncodeRaceFrame(race, packet + 2 + MP_SEAT_LIMIT * 4, RACE_FRAME_WIRE_SIZE)) return 0;
    packet[0] = MP_S2C_CORRECTION;
    packet[1] = MP_CORRECTION_VERSION;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
        for (int byte = 0; byte < 4; ++byte)
            packet[2 + seat * 4 + byte] = (uint8_t)(acknowledged[seat] >> (byte * 8));
    memcpy(wire, packet, sizeof(packet));
    return 1;
}

int MpDecodeCorrection(const RaceSim *race, const uint8_t *wire, size_t size,
                       MpCorrection *out) {
    if (!race || !wire || !out || size != MP_CORRECTION_WIRE_SIZE ||
        wire[0] != MP_S2C_CORRECTION || wire[1] != MP_CORRECTION_VERSION) return 0;
    MpCorrection decoded = {0};
    if (!DecodeRaceFrame(race, wire + 2 + MP_SEAT_LIMIT * 4,
                         RACE_FRAME_WIRE_SIZE, &decoded.frame)) return 0;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        for (int byte = 0; byte < 4; ++byte)
            decoded.acknowledged[seat] |= (uint32_t)wire[2 + seat * 4 + byte] << (byte * 8);
        const DriverFrame *driver = &decoded.frame.drivers[seat];
        if (driver->rival || (decoded.acknowledged[seat] &&
            (driver->status == SIM_EMPTY || !driver->inputTick))) return 0;
    }
    *out = decoded;
    return 1;
}

static int ValidCommands(const MpCommands *commands) {
    return commands && commands->head < MP_COMMAND_CAPACITY &&
        commands->count <= MP_COMMAND_CAPACITY && commands->acknowledged <= commands->sent &&
        commands->sent - commands->acknowledged == commands->count;
}

int MpDecodePublicationSnapshot(const uint8_t *wire, size_t size, MpSnapshot *snapshot) {
    if (!wire || !snapshot || size != MP_PUBLICATION_WIRE_SIZE ||
        wire[0] != MP_S2C_CORRECTION || wire[1] != MP_CORRECTION_VERSION ||
        wire[10] != RACE_FRAME_WIRE_VERSION || wire[MP_CORRECTION_WIRE_SIZE] != MP_S2C_SNAPSHOT) return 0;
    MpSnapshot poses;
    if (!MpDecodeSnapshot(wire + MP_CORRECTION_WIRE_SIZE + 1, MP_SNAPSHOT_BODY_SIZE, &poses)) return 0;
    const uint32_t clocks[] = {poses.tick, poses.elapsed, poses.phase};
    const size_t offsets[] = {10 + 13, 10 + 21, 10 + 25};
    for (int field = 0; field < 3; ++field)
        for (int byte = 0; byte < 4; ++byte)
            if (wire[offsets[field] + byte] != (uint8_t)(clocks[field] >> (8 * byte))) return 0;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
        for (int byte = 0; byte < 4; ++byte)
            if (wire[2 + seat * 4 + byte] != (uint8_t)(poses.acknowledged[seat] >> (8 * byte))) return 0;
    *snapshot = poses;
    return 1;
}

int MpDecodePublication(const RaceSim *race, const uint8_t *wire, size_t size,
                        MpCorrection *correction, MpSnapshot *snapshot) {
    if (!wire || !correction || !snapshot || size != MP_PUBLICATION_WIRE_SIZE ||
        wire[MP_CORRECTION_WIRE_SIZE] != MP_S2C_SNAPSHOT) return 0;
    MpCorrection state;
    MpSnapshot poses;
    if (!MpDecodeCorrection(race, wire, MP_CORRECTION_WIRE_SIZE, &state) ||
        !MpDecodePublicationSnapshot(wire, size, &poses) ||
        state.frame.tick != poses.tick || state.frame.elapsed != poses.elapsed ||
        state.frame.phase != poses.phase ||
        memcmp(state.acknowledged, poses.acknowledged, sizeof(state.acknowledged))) return 0;
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) {
        const DriverFrame *driver = &state.frame.drivers[seat];
        const PlayerCarRuntime *car = &driver->car;
        const MpCarPose *pose = &poses.seats[seat];
        int status = driver->status == SIM_DRIVING ? MP_DRIVING :
                     driver->status == SIM_DRIVER_FINISHED ? MP_FINISHED : MP_RETIRED;
        if (pose->status != status) return 0;
        const GameCarRuntime *rival = AsConstRivalCar(car);
        if (pose->x != car->x || pose->y != car->y || pose->z != car->z ||
            pose->yaw != car->bodyYaw || pose->pitch != car->bodyPitch || pose->roll != car->bodyRoll ||
            pose->steering != car->steeringAngle || pose->wheels != car->wheelRotation ||
            pose->brake != car->drive.brakeInput || pose->progress != car->trackProgress ||
            pose->rpm != (driver->rival ? rival->engineRpm : car->drive.engineRpm) ||
            pose->throttle != car->drive.acceleratorInput.value ||
            pose->clutch != (driver->rival ? 0 : car->drive.clutch) ||
            pose->gear != (driver->rival ? 0 : car->drive.gear) ||
            pose->ground != car->modelY || pose->rollSpeed != car->bodyRollVelocity ||
            pose->speed != car->speed || pose->lap != car->lap ||
            (status == MP_FINISHED && pose->place != driver->place)) return 0;
    }
    *correction = state;
    *snapshot = poses;
    return 1;
}

int MpRememberCommand(MpCommands *commands, uint32_t sequence, uint32_t tick, const DriverInput *input) {
    if (!ValidCommands(commands) || !ValidDriverInput(input) ||
        commands->count == MP_COMMAND_CAPACITY || commands->sent == UINT32_MAX ||
        sequence != commands->sent + 1 || tick < commands->lastTick) return 0;
    unsigned tail = (commands->head + commands->count) % MP_COMMAND_CAPACITY;
    commands->entries[tail] = (MpCommand){sequence, tick, *input};
    commands->count++;
    commands->sent = sequence;
    commands->lastTick = tick;
    return 1;
}

int MpAcknowledgeCommands(MpCommands *commands, uint32_t sequence) {
    if (!ValidCommands(commands) || sequence < commands->acknowledged || sequence > commands->sent) return 0;
    unsigned removed = sequence - commands->acknowledged;
    commands->head = (commands->head + removed) % MP_COMMAND_CAPACITY;
    commands->count -= removed;
    commands->acknowledged = sequence;
    return 1;
}

const MpCommand *MpCommandAt(const MpCommands *commands, unsigned index) {
    if (!ValidCommands(commands) || index >= commands->count) return NULL;
    return &commands->entries[(commands->head + index) % MP_COMMAND_CAPACITY];
}

int MpReplayInput(const MpCommands *commands, uint32_t tick,
                   uint32_t *sequence, DriverInput *input) {
    if (!ValidCommands(commands) || !sequence || !ValidDriverInput(input) ||
        *sequence < commands->acknowledged || *sequence > commands->sent) return 0;
    uint32_t consumed = *sequence, previousTick = 0;
    DriverInput controls = *input;
    for (unsigned index = 0; index < commands->count; ++index) {
        const MpCommand *command = MpCommandAt(commands, index);
        if (command->sequence != commands->acknowledged + index + 1 ||
            command->tick < previousTick || !ValidDriverInput(&command->input)) return 0;
        previousTick = command->tick;
        if (command->sequence <= consumed || command->tick > tick) continue;
        int up = controls.shiftUp || command->input.shiftUp;
        int down = controls.shiftDown || command->input.shiftDown;
        controls = command->input;
        controls.shiftUp = up; controls.shiftDown = down;
        consumed = command->sequence;
    }
    if (commands->count && previousTick != commands->lastTick) return 0;
    *sequence = consumed;
    *input = controls;
    return 1;
}

static int ValidClock(const MpClock *clock) {
    return clock && clock->started == 1 && clock->origin <= clock->observedAt &&
        clock->originTick <= clock->latestTick;
}

int MpClockTarget(const MpClock *clock, uint64_t now, uint32_t *tick, uint32_t *fraction) {
    if (!tick || fraction == tick || !ValidClock(clock) || now < clock->observedAt) return 0;
    const uint64_t interval = 1000000000 / SIM_TICK_RATE;
    uint64_t elapsed = (now - clock->origin) / interval;
    uint64_t limit = (uint64_t)clock->latestTick + MP_PREDICTION_LEAD;
    if (limit > UINT32_MAX) limit = UINT32_MAX;
    /* Clamp before adding so even UINT64_MAX timestamps cannot overflow. */
    if (elapsed > limit - clock->originTick) elapsed = limit - clock->originTick;
    uint32_t target = clock->originTick + (uint32_t)elapsed;
    uint32_t phase = target >= clock->latestTick && target < limit
        ? (uint32_t)(((now - clock->origin) % interval) * 65536 / interval) : 0;
    *tick = target < clock->latestTick ? clock->latestTick : target;
    if (fraction) *fraction = phase;
    return 1;
}

int MpMotionTarget(const RaceSim *prediction, uint32_t authorityTick, int seat,
                    uint32_t clockFraction, uint32_t *tick, uint32_t *fraction) {
    if (!prediction || !tick || !fraction || tick == fraction || clockFraction >= 65536 ||
        (unsigned)seat >= MP_SEAT_LIMIT || prediction->drivers[seat].rival ||
        prediction->drivers[seat].status == SIM_EMPTY ||
        (unsigned)prediction->drivers[seat].status > SIM_RETIRED ||
        prediction->phase < SIM_COUNTDOWN || prediction->phase > SIM_FINISHED ||
        prediction->elapsed > prediction->tick || prediction->tick < authorityTick ||
        (uint64_t)prediction->tick > (uint64_t)authorityTick + MP_PREDICTION_LEAD) return 0;
    uint32_t target = prediction->tick, blend = 0;
    if (prediction->drivers[seat].status == SIM_DRIVING && prediction->phase != SIM_FINISHED) {
        uint32_t odd = prediction->phase == SIM_RACING ? prediction->elapsed % SIM_PHYSICS_INTERVAL : 0;
        uint32_t interval = prediction->phase == SIM_RACING ? SIM_PHYSICS_INTERVAL : 1;
        uint32_t ahead = interval - odd;
        if ((uint64_t)target + ahead <= UINT32_MAX &&
            (uint64_t)target + ahead <= (uint64_t)authorityTick + MP_PREDICTION_LEAD) {
            target += ahead;
            blend = (odd * 65536 + clockFraction) / interval;
        }
    }
    *tick = target;
    *fraction = blend;
    return 1;
}

int MpClockObserve(MpClock *clock, uint32_t tick, uint64_t now) {
    if (!clock || (clock->started && (!ValidClock(clock) ||
        tick <= clock->latestTick || now < clock->observedAt))) return 0;
    MpClock next = *clock;
    if (!next.started) {
        next = (MpClock){.origin = now, .observedAt = now,
            .originTick = tick, .latestTick = tick, .started = 1};
    } else {
        uint32_t projected;
        if (!MpClockTarget(clock, now, &projected, NULL)) return 0;
        if (tick > projected) { next.origin = now; next.originTick = tick; }
        next.latestTick = tick;
        next.observedAt = now;
    }
    *clock = next;
    return 1;
}

int MpApplyCorrection(RaceSim *race, MpCommands *commands, int seat,
                      const MpCorrection *correction) {
    if (!correction || (unsigned)seat >= MP_SEAT_LIMIT || !ValidCommands(commands) ||
        !ValidRaceFrame(race, &correction->frame)) return 0;
    uint32_t sequence = correction->acknowledged[seat];
    if (sequence < commands->acknowledged || sequence > commands->sent) return 0;
    for (int human = 0; human < MP_SEAT_LIMIT; ++human) {
        const DriverFrame *driver = &correction->frame.drivers[human];
        if (driver->rival || (correction->acknowledged[human] &&
            (driver->status == SIM_EMPTY || !driver->inputTick))) return 0;
    }
    /* All preconditions are checked before either owned destination changes.
     * These pure operations have no concurrent writer or fallible allocation. */
    RestoreRaceFrame(race, &correction->frame);
    MpAcknowledgeCommands(commands, sequence);
    return 1;
}

int MpDecodeLobby(const uint8_t *wire, size_t size, MpLobby *lobby) {
    if (!wire || !lobby || size != 53 || wire[0] != MP_S2C_LOBBY || wire[1] != MP_PROTOCOL_VERSION) return 0;
    uint8_t directory[18] = {MP_S2C_LIST, MP_PROTOCOL_VERSION, 1};
    memcpy(directory + 3, wire + 2, 15);
    MpRoomInfo rooms[MP_ROOM_LIMIT];
    size_t count;
    if (!MpDecodeRoomList(directory, sizeof(directory), rooms, &count)) return 0;
    MpLobby decoded = {.room = rooms[0]};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const uint8_t *entry = wire + 17 + seat * 18;
        MpLobbySeat *player = &decoded.seats[seat];
        if (entry[0] >= CAR_MODEL_VARIANT_COUNT || entry[1] > 1 || entry[2] > MP_NAME_CAPACITY) return 0;
        for (int byte = entry[2]; byte < MP_NAME_CAPACITY; ++byte) if (entry[3 + byte]) return 0;
        if (!(decoded.room.occupied & (1 << seat))) {
            for (int byte = 0; byte < 18; ++byte) if (entry[byte]) return 0;
        }
        player->variant = entry[0]; player->manual = entry[1]; player->length = entry[2];
        memcpy(player->name, entry + 3, player->length);
    }
    *lobby = decoded;
    return 1;
}

int MpDecodeRoomList(const uint8_t *wire, size_t size,
                     MpRoomInfo rooms[MP_ROOM_LIMIT], size_t *count) {
    if (!wire || !rooms || !count || size < 3 || wire[0] != MP_S2C_LIST ||
        wire[1] != MP_PROTOCOL_VERSION || wire[2] > MP_ROOM_LIMIT ||
        size != 3 + (size_t)wire[2] * 15) return 0;
    MpRoomInfo decoded[MP_ROOM_LIMIT] = {0};
    for (size_t i = 0; i < wire[2]; ++i) {
        const uint8_t *entry = wire + 3 + i * 15;
        MpRoomInfo *room = &decoded[i];
        for (int byte = 0; byte < 8; ++byte) room->code |= (uint64_t)entry[byte] << (8 * byte);
        room->options = (MpRaceOptions){entry[8], entry[9], entry[10], entry[11]};
        room->occupied = entry[12];
        room->ready = entry[13];
        room->state = entry[14];
        if (!room->code || room->code > INT64_MAX || !MpValidRaceOptions(&room->options) ||
            (room->occupied & ~3) || (room->ready & ~room->occupied) || room->state > 2) return 0;
        for (size_t earlier = 0; earlier < i; ++earlier)
            if (decoded[earlier].code == room->code) return 0;
    }
    memcpy(rooms, decoded, sizeof(decoded));
    *count = wire[2];
    return 1;
}

int MpDecodeWelcome(const uint8_t wire[11], int *seat, uint64_t *room) {
    if (!wire || !seat || !room || wire[0] != MP_S2C_WELCOME ||
        wire[1] != MP_PROTOCOL_VERSION || wire[2] >= MP_SEAT_LIMIT) return 0;
    uint64_t code = 0;
    for (int i = 0; i < 8; ++i) code |= (uint64_t)wire[i + 3] << (8 * i);
    if (!code || code > INT64_MAX) return 0;
    *seat = wire[2];
    *room = code;
    return 1;
}

int MpParseRoom(const char *value, uint64_t *code) {
    if (!code) return 0;
    if (!value || strcmp(value, "auto") == 0) { *code = UINT64_MAX; return 1; }
    if (strcmp(value, "create") == 0) { *code = 0; return 1; }
    if (!*value) return 0;
    uint64_t parsed = 0;
    for (const char *digit = value; *digit; ++digit) {
        if (*digit < '0' || *digit > '9' || parsed > ((uint64_t)INT64_MAX - (*digit - '0')) / 10) return 0;
        parsed = parsed * 10 + (unsigned)(*digit - '0');
    }
    *code = parsed;
    return 1;
}

int MpValidRaceOptions(const MpRaceOptions *options) {
    return options && options->classIndex < 6 && options->course < 4 &&
        options->laps >= 1 && options->laps <= PLAYER_LAP_TIME_CAPACITY && options->reverse <= 1;
}

int MpChangeRace(MpRaceOptions *options, int field, int direction) {
    if (!MpValidRaceOptions(options) || (unsigned)field >= 4 ||
        (direction != -1 && direction != 1)) return 0;
    switch (field) {
    case 0: options->classIndex = (uint8_t)((options->classIndex + 6 + direction) % 6); break;
    case 1: options->course = (uint8_t)((options->course + 4 + direction) % 4); break;
    case 2: options->laps = (uint8_t)((options->laps - 1 + PLAYER_LAP_TIME_CAPACITY + direction) %
                                    PLAYER_LAP_TIME_CAPACITY + 1); break;
    case 3: options->reverse ^= 1; break;
    }
    return 1;
}

int MpChangeCar(MpSettings *choice, int direction, int toggle, uint32_t automaticCars) {
    if (!choice || (unsigned)choice->car >= CAR_MODEL_VARIANT_COUNT ||
        (unsigned)choice->manual > 1 || direction < -1 || direction > 1 ||
        (unsigned)toggle > 1) return 0;
    int car = (choice->car + CAR_MODEL_VARIANT_COUNT + direction) % CAR_MODEL_VARIANT_COUNT;
    int manual = automaticCars & (UINT32_C(1) << car) ? choice->manual ^ toggle : 1;
    choice->car = car;
    choice->manual = manual;
    return 1;
}

static void PutLE16(uint8_t *out, int16_t v) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)((uint16_t)v >> 8);
}
static uint32_t GetLE32(const uint8_t *in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}
static int32_t GetLE32Signed(const uint8_t *in) { return (int32_t)GetLE32(in); }

void MpEncodeCommand(const DriverInput *input, uint32_t sequence, uint8_t out[MP_COMMAND_WIRE_SIZE]) {
    uint8_t body[MP_INPUT_WIRE_SIZE];
    MpEncodeInput(input, body);
    out[0] = MP_C2S_COMMAND;
    for (int byte = 0; byte < 4; ++byte) out[byte + 1] = (uint8_t)(sequence >> (8 * byte));
    memcpy(out + 5, body + 1, 12);
}

void MpEncodeInput(const DriverInput *input, uint8_t out[MP_INPUT_WIRE_SIZE]) {
    out[0] = MP_C2S_INPUT;
    out[1] = (uint8_t)input->steering.mode;
    out[2] = input->steering.left ? 1 : 0;
    out[3] = input->steering.right ? 1 : 0;
    /* Steering angle can exceed +/-32767 in principle; the retail range
     * (+/-13*512, see SetRaceInput) fits comfortably in 16 bits. */
    PutLE16(out + 4, (int16_t)input->steering.angle);
    PutLE16(out + 6, input->throttle);
    PutLE16(out + 8, input->brake);
    out[10] = input->shiftUp ? 1 : 0;
    out[11] = input->shiftDown ? 1 : 0;
    out[12] = 0; /* Reserved byte in the server's 12-byte input body. */
}

static int ValidStart(const MpStart *start) {
    if (!start || start->course >= 4 || start->classIndex >= 6 ||
        start->laps < 1 || start->laps > PLAYER_LAP_TIME_CAPACITY || start->reverse > 1 ||
        !memchr(start->boot, 0, sizeof(start->boot))) return 0;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
        if (start->seats[seat].model >= CAR_MODEL_VARIANT_COUNT ||
            start->seats[seat].manual > 1) return 0;
    unsigned slots = 0;
    for (int i = 0; i < MP_AI_LIMIT; ++i) {
        const MpRival *rival = &start->rivals[i];
        if (rival->active > 1) return 0;
        if (!rival->active) {
            if (rival->model || rival->slot || rival->seed) return 0;
            continue;
        }
        if (rival->model >= RACE_CAR_SLOT_COUNT || rival->slot >= RACE_CAR_SLOT_COUNT ||
            (slots & (1u << rival->slot))) return 0;
        slots |= 1u << rival->slot;
    }
    return 1;
}

int MpParseSettings(const char *port, const char *car, const char *manual, MpSettings *out) {
    MpSettings settings = {7243, 0, 0};
    if (!out || (port && !RuntimeParseInt(port, 0, 1, 65535, &settings.port)) ||
        (car && !RuntimeParseInt(car, 0, 0, CAR_MODEL_VARIANT_COUNT - 1, &settings.car)) ||
        (manual && !RuntimeParseInt(manual, 0, 0, 1, &settings.manual))) return 0;
    *out = settings;
    return 1;
}

int MpDecodeStart(const uint8_t *body, size_t size, MpStart *out) {
    if (!body || !out || size != MP_START_BODY_SIZE ||
        body[0] != MP_PROTOCOL_VERSION || body[9] != MP_SEAT_LIMIT) return 0;
    MpStart result = {.course = body[1], .classIndex = body[2],
        .laps = body[3], .reverse = body[4], .countdown = GetLE32(body + 5)};
    memcpy(result.boot, body + 10, sizeof(result.boot));
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const uint8_t *entry = body + 26 + seat * 6;
        result.seats[seat] = (MpSeat){entry[0], entry[1], GetLE32(entry + 2)};
    }
    const uint8_t *hash = body + 26 + MP_SEAT_LIMIT * 6;
    result.fingerprint = (uint64_t)GetLE32(hash) | ((uint64_t)GetLE32(hash + 4) << 32);
    result.executable = (uint64_t)GetLE32(hash + 8) | ((uint64_t)GetLE32(hash + 12) << 32);
    for (int i = 0; i < MP_AI_LIMIT; ++i) {
        const uint8_t *entry = hash + 16 + i * 7;
        result.rivals[i] = (MpRival){entry[0], entry[1], entry[2], GetLE32(entry + 3)};
    }
    if (!ValidStart(&result)) return 0;
    *out = result;
    return 1;
}

int MpMatchesChoice(const MpStart *start, int seat, const MpSettings *choice) {
    return ValidStart(start) && choice && seat >= 0 && seat < MP_SEAT_LIMIT &&
        start->seats[seat].model == choice->car &&
        start->seats[seat].manual == choice->manual;
}

int MpMatchesArchive(const MpStart *start, const RaceData *archive) {
    return ValidStart(start) && archive && archive->data && archive->size &&
        start->boot[0] && archive->boot[0] &&
        memchr(archive->boot, 0, sizeof(archive->boot)) &&
        strcmp(start->boot, archive->boot) == 0 &&
        start->executable && start->executable == archive->executable &&
        start->fingerprint == ArchiveFingerprint(archive->data, archive->size);
}

int MpBuildSetup(const MpStart *start, RaceSetup *out) {
    if (!out || !ValidStart(start)) return 0;
    RaceSetup setup = {.classIndex = start->classIndex, .courseIndex = start->course,
                       .laps = start->laps, .reverse = start->reverse};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const MpSeat *entry = &start->seats[seat];
        setup.entrants[seat] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = seat,
            .model = entry->model, .manual = entry->manual, .seed = entry->seed};
        setup.looks[seat].variant = entry->model;
    }
    for (int i = 0; i < MP_AI_LIMIT; ++i) {
        const MpRival *rival = &start->rivals[i];
        if (!rival->active) continue;
        const int seat = MP_SEAT_LIMIT + i;
        setup.entrants[seat] = (RaceEntrant){.kind = RACE_SEAT_AI, .grid = seat,
            .model = rival->model, .rivalSlot = rival->slot, .seed = rival->seed};
    }
    *out = setup;
    return 1;
}

int MpDecodeResult(const uint8_t *body, size_t size, MpResult *out) {
    if (!body || !out || size != MP_RESULT_BODY_SIZE) return 0;
    MpResult result = {0};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const uint8_t *entry = body + seat * 6;
        MpFinish finish = {entry[0], entry[1], GetLE32Signed(entry + 2)};
        if (finish.finished > 1 || (finish.finished
                ? finish.place < 1 || finish.place > DRIVER_SEAT_LIMIT || finish.milliseconds < 0
                : finish.place != 0 || finish.milliseconds != -1)) return 0;
        for (int earlier = 0; earlier < seat; ++earlier) {
            const MpFinish *previous = &result.seats[earlier];
            if (!finish.finished || !previous->finished) continue;
            if (finish.place == previous->place ||
                (finish.place < previous->place && finish.milliseconds > previous->milliseconds) ||
                (finish.place > previous->place && finish.milliseconds < previous->milliseconds)) return 0;
        }
        result.seats[seat] = finish;
    }
    *out = result;
    return 1;
}

int MpMatchesResult(const MpSnapshot *snapshot, const MpResult *result) {
    if (!snapshot || !result || snapshot->phase != SIM_FINISHED) return 0;
    const uint64_t elapsedMs = (uint64_t)snapshot->elapsed * 1000 / SIM_TICK_RATE;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const MpFinish *finish = &result->seats[seat];
        if (finish->finished && (finish->milliseconds < 0 ||
            (uint64_t)finish->milliseconds > elapsedMs)) return 0;
        if (result->seats[seat].finished < 0 || result->seats[seat].finished > 1 ||
            snapshot->seats[seat].status !=
                (result->seats[seat].finished ? MP_FINISHED : MP_RETIRED)) return 0;
    }
    return 1;
}

static int ValidPose(const MpCarPose *pose) {
    return pose->status >= 0 && pose->status <= MP_FINISHED && pose->brake >= 0 && pose->brake <= 256 &&
        pose->throttle >= 0 && pose->throttle <= 256 &&
        pose->clutch >= INT16_MIN && pose->clutch <= INT16_MAX &&
        pose->gear >= 0 && pose->gear <= CAR_FORWARD_GEAR_COUNT &&
        pose->lap >= 0 && pose->lap <= PLAYER_LAP_TIME_CAPACITY + 1 &&
        pose->place >= 0 && pose->place <= DRIVER_SEAT_LIMIT &&
        (pose->status != MP_RETIRED || pose->place == 0);
}

static int ValidSnapshot(const MpSnapshot *snapshot) {
    if (!snapshot || snapshot->phase > SIM_FINISHED || snapshot->elapsed > snapshot->tick ||
        (snapshot->phase <= SIM_COUNTDOWN && snapshot->elapsed != 0)) return 0;
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) {
        const MpCarPose *pose = &snapshot->seats[seat];
        if (!ValidPose(pose) ||
            (snapshot->phase == SIM_FINISHED && pose->status == MP_DRIVING)) return 0;
    }
    return 1;
}

int MpSnapshotExpired(uint64_t now, uint64_t receivedAt, int started) {
    uint64_t limit = (started ? UINT64_C(5) : UINT64_C(65)) * UINT64_C(1000000000);
    return now >= receivedAt && now - receivedAt >= limit;
}

static int32_t BlendValue(int32_t before, int32_t after, uint32_t fraction) {
    return (int32_t)(before + ((int64_t)after - before) * fraction / 65536);
}

static int32_t BlendPoseAngle(int32_t before, int32_t after, uint32_t fraction) {
    int64_t delta = ((int64_t)after - before) % 4096;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    return (int32_t)(((int64_t)before + delta * fraction / 65536) & 4095);
}

int MpBlendPose(const MpCarPose *before, const MpCarPose *after,
                uint32_t fraction, MpCarPose *out) {
    if (!before || !after || !out || fraction > 65536 ||
        !ValidPose(before) || !ValidPose(after)) return 0;
    MpCarPose pose = *after;
    if (before->status == MP_DRIVING && after->status == MP_DRIVING) {
        pose.x = BlendValue(before->x, after->x, fraction);
        pose.y = BlendValue(before->y, after->y, fraction);
        pose.z = BlendValue(before->z, after->z, fraction);
        pose.ground = BlendValue(before->ground, after->ground, fraction);
        pose.steering = BlendValue(before->steering, after->steering, fraction);
        pose.rollSpeed = BlendValue(before->rollSpeed, after->rollSpeed, fraction);
        pose.speed = BlendValue(before->speed, after->speed, fraction);
        pose.yaw = BlendPoseAngle(before->yaw, after->yaw, fraction);
        pose.pitch = BlendPoseAngle(before->pitch, after->pitch, fraction);
        pose.roll = BlendPoseAngle(before->roll, after->roll, fraction);
        pose.wheels = BlendPoseAngle(before->wheels, after->wheels, fraction) |
                      (after->wheels & CAR_WHEEL_BLUR_FLAG);
    }
    *out = pose;
    return 1;
}

int MpBlendDriver(const SimDriver *before, const SimDriver *after,
                  uint32_t fraction, PlayerCarRuntime *out) {
    if (!before || !after || !out || before->rival || after->rival ||
        before->status == SIM_EMPTY || after->status == SIM_EMPTY ||
        (unsigned)before->status > SIM_RETIRED || (unsigned)after->status > SIM_RETIRED ||
        before->variant != after->variant) return 0;
    const SimDriver *drivers[2] = {before, after};
    MpCarPose poses[2];
    for (int index = 0; index < 2; ++index) {
        const SimDriver *driver = drivers[index];
        const PlayerCarRuntime *car = &driver->car;
        poses[index] = (MpCarPose){
            .status = driver->status == SIM_DRIVING ? MP_DRIVING :
                      driver->status == SIM_DRIVER_FINISHED ? MP_FINISHED : MP_RETIRED,
            .x = car->x, .y = car->y, .z = car->z, .yaw = car->bodyYaw,
            .pitch = car->bodyPitch, .roll = car->bodyRoll, .steering = car->steeringAngle,
            .wheels = car->wheelRotation, .brake = car->drive.brakeInput,
            .progress = car->trackProgress, .rpm = car->drive.engineRpm,
            .throttle = car->drive.acceleratorInput.value, .clutch = car->drive.clutch,
            .gear = car->drive.gear, .ground = car->modelY, .rollSpeed = car->bodyRollVelocity,
            .speed = car->speed, .lap = car->lap, .place = driver->place};
    }
    MpCarPose blended;
    PlayerCarRuntime car = after->car;
    if (!MpBlendPose(&poses[0], &poses[1], fraction, &blended) || !MpApplyPose(&car, &blended, 0)) return 0;
    *out = car;
    return 1;
}

int MpDecodeSnapshot(const uint8_t *body, size_t size, MpSnapshot *out) {
    if (!body || !out) return 0;
    size_t expect = MP_SNAPSHOT_BODY_SIZE;
    if (size != expect) return 0;
    MpSnapshot result = {0};
    result.tick = GetLE32(body);
    result.elapsed = GetLE32(body + 4);
    result.phase = body[8];
    const uint8_t *cursor = body + MP_SNAPSHOT_HEADER_SIZE;
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) {
        if (cursor[0] > MP_FINISHED) return 0;
        result.seats[seat].status = cursor[0];
        result.seats[seat].x = GetLE32Signed(cursor + 1);
        result.seats[seat].y = GetLE32Signed(cursor + 5);
        result.seats[seat].z = GetLE32Signed(cursor + 9);
        result.seats[seat].yaw = GetLE32Signed(cursor + 13);
        result.seats[seat].pitch = GetLE32Signed(cursor + 17);
        result.seats[seat].roll = GetLE32Signed(cursor + 21);
        result.seats[seat].steering = GetLE32Signed(cursor + 25);
        result.seats[seat].wheels = GetLE32Signed(cursor + 29);
        result.seats[seat].brake = GetLE32Signed(cursor + 33);
        result.seats[seat].progress = GetLE32Signed(cursor + 37);
        result.seats[seat].rpm = GetLE32Signed(cursor + 41);
        result.seats[seat].throttle = GetLE32Signed(cursor + 45);
        result.seats[seat].clutch = GetLE32Signed(cursor + 49);
        result.seats[seat].gear = GetLE32Signed(cursor + 53);
        result.seats[seat].ground = GetLE32Signed(cursor + 57);
        result.seats[seat].rollSpeed = GetLE32Signed(cursor + 61);
        result.seats[seat].speed = GetLE32Signed(cursor + 65);
        result.seats[seat].lap = GetLE32Signed(cursor + 69);
        result.seats[seat].place = GetLE32Signed(cursor + 73);
        cursor += MP_SNAPSHOT_SEAT_SIZE;
    }
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        result.acknowledged[seat] = GetLE32(cursor);
        cursor += 4;
    }
    if (!ValidSnapshot(&result)) return 0;
    *out = result;
    return 1;
}

int MpHistoryPush(MpHistory *history, const MpSnapshot *snapshot, uint64_t now) {
    if (!history || history->count > MP_HISTORY_CAPACITY || !ValidSnapshot(snapshot)) return 0;
    if (history->count) {
        const MpSnapshot *last = &history->samples[history->count - 1];
        if (now < history->receivedAt || snapshot->tick <= last->tick ||
            snapshot->phase < last->phase || snapshot->elapsed < last->elapsed) return 0;
        for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
            if (snapshot->acknowledged[seat] < last->acknowledged[seat]) return 0;
        for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat)
            if (last->seats[seat].status != MP_DRIVING &&
                snapshot->seats[seat].status != last->seats[seat].status) return 0;
    } else {
        history->origin = now;
        history->firstTick = snapshot->tick;
    }
    if (history->count == MP_HISTORY_CAPACITY) {
        memmove(history->samples, history->samples + 1,
                (MP_HISTORY_CAPACITY - 1) * sizeof(*history->samples));
        history->count--;
    }
    history->samples[history->count++] = *snapshot;
    history->receivedAt = now;
    return 1;
}

int MpHistoryPose(const MpHistory *history, int seat, uint64_t now,
                  uint64_t delay, MpCarPose *out) {
    if (!history || !out || (unsigned)seat >= MP_FIELD_LIMIT ||
        !history->count || history->count > MP_HISTORY_CAPACITY || now < history->origin) return 0;
    const uint64_t interval = 1000000000 / SIM_TICK_RATE;
    uint64_t elapsed = now - history->origin;
    elapsed = elapsed > delay ? elapsed - delay : 0;
    uint64_t target = (uint64_t)history->firstTick * 65536 +
                      elapsed / interval * 65536 + elapsed % interval * 65536 / interval;
    const MpSnapshot *before = &history->samples[0];
    if (target <= (uint64_t)before->tick * 65536) {
        return MpBlendPose(&before->seats[seat], &before->seats[seat], 0, out);
    }
    for (unsigned i = 1; i < history->count; ++i) {
        const MpSnapshot *after = &history->samples[i];
        if (after->tick <= before->tick) return 0;
        uint64_t end = (uint64_t)after->tick * 65536;
        if (target <= end) {
            uint32_t fraction = (uint32_t)((target - (uint64_t)before->tick * 65536) /
                                         (after->tick - before->tick));
            return MpBlendPose(&before->seats[seat], &after->seats[seat], fraction, out);
        }
        before = after;
    }
    return MpBlendPose(&before->seats[seat], &before->seats[seat], 0, out);
}

int MpApplyPose(PlayerCarRuntime *car, const MpCarPose *pose, int rival) {
    if (!car || !pose || !ValidPose(pose) || (unsigned)rival > 1 ||
        (rival && (pose->gear || pose->clutch))) return 0;
    car->x = pose->x;
    car->y = pose->y;
    car->z = pose->z;
    car->bodyYaw = pose->yaw;
    car->bodyPitch = pose->pitch;
    car->bodyRoll = pose->roll;
    car->modelY = pose->ground;
    car->bodyRollVelocity = pose->rollSpeed;
    car->speed = pose->speed;
    car->lap = (s16)pose->lap;
    car->steeringAngle = pose->steering;
    car->wheelRotation = pose->wheels;
    car->drive.brakeInput = (s16)pose->brake;
    car->trackProgress = pose->progress;
    if (rival) AsRivalCar(car)->engineRpm = pose->rpm;
    else car->drive.engineRpm = pose->rpm;
    car->drive.acceleratorInput.value = (s16)pose->throttle;
    if (!rival) {
        car->drive.clutch = (s16)pose->clutch;
        car->drive.gear = (s16)pose->gear;
    }
    return 1;
}

int MpApplySnapshot(RaceSim *race, const MpSnapshot *snapshot) {
    if (!race || !ValidSnapshot(snapshot) ||
        snapshot->phase < race->phase || snapshot->tick <= race->tick ||
        snapshot->elapsed < race->elapsed) return 0;
    const uint32_t advance = snapshot->tick - race->tick;
    if (snapshot->phase == SIM_COUNTDOWN && advance >= race->countdown) return 0;
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        const MpCarPose *pose = &snapshot->seats[seat];
        if (driver->status == SIM_EMPTY) {
            if (pose->status != MP_RETIRED) return 0;
            continue;
        }
        if ((driver->rival && (pose->gear || pose->clutch)) ||
            pose->lap < driver->car.lap || pose->lap > race->laps + 1 ||
            (pose->status != MP_RETIRED && driver->status == SIM_RETIRED) ||
            (driver->status == SIM_DRIVER_FINISHED && pose->status != MP_FINISHED)) return 0;
    }
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) {
        SimDriver *driver = &race->drivers[seat];
        const MpCarPose *pose = &snapshot->seats[seat];
        if (driver->status == SIM_EMPTY) continue;
        driver->place = pose->place;
        if (!pose->status) {
            driver->car.activeFlag = -1;
            driver->status = SIM_RETIRED;
            continue;
        }
        driver->status = pose->status == MP_FINISHED ? SIM_DRIVER_FINISHED : SIM_DRIVING;
        MpApplyPose(&driver->car, pose, driver->rival);
    }
    race->countdown = snapshot->phase == SIM_COUNTDOWN ? race->countdown - advance : 0;
    race->tick = snapshot->tick;
    race->elapsed = snapshot->elapsed;
    race->phase = (SimRacePhase)snapshot->phase;
    return 1;
}
