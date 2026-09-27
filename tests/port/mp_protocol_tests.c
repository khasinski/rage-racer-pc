#include "port/mp_client.h"
#include "game/race_sim.h"
#include "port/client_race.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int TestReplayInput(void) {
    MpCommands commands = {.head = MP_COMMAND_CAPACITY - 2};
    const DriverInput samples[] = {
        {.throttle = 128, .shiftUp = 1}, {.throttle = 256},
        {.brake = 64, .shiftDown = 1},
        {.steering = {.mode = STEERING_ANALOG, .angle = 100}, .throttle = 200}
    };
    const uint32_t ticks[] = {98, 101, 101, 103};
    for (unsigned index = 0; index < 4; ++index)
        CHECK(MpRememberCommand(&commands, index + 1, ticks[index], &samples[index]));
    const MpCommands unchanged = commands;
    uint32_t sequence = 0;
    DriverInput controls = {.throttle = 32, .shiftDown = 1};
    CHECK(MpReplayInput(&commands, 100, &sequence, &controls));
    CHECK(sequence == 1 && controls.throttle == 128 && controls.shiftUp && controls.shiftDown);
    CHECK(MpReplayInput(&commands, 100, &sequence, &controls));
    CHECK(sequence == 1 && controls.shiftUp && controls.shiftDown); /* Not yet consumed by physics. */
    controls.shiftUp = controls.shiftDown = 0;
    CHECK(MpReplayInput(&commands, 101, &sequence, &controls));
    CHECK(sequence == 3 && controls.brake == 64 && !controls.throttle && !controls.shiftUp && controls.shiftDown);
    controls.shiftDown = 0;
    CHECK(MpReplayInput(&commands, 101, &sequence, &controls));
    CHECK(sequence == 3 && !controls.shiftDown);
    CHECK(MpReplayInput(&commands, 102, &sequence, &controls));
    CHECK(sequence == 3 && controls.brake == 64);
    CHECK(MpReplayInput(&commands, 103, &sequence, &controls));
    CHECK(sequence == 4 && controls.throttle == 200 && controls.steering.angle == 100);
    CHECK(memcmp(&commands, &unchanged, sizeof(commands)) == 0);
    sequence = 0;
    const DriverInput held = controls;
    for (int bad = 0; bad < 4; ++bad) {
        unsigned last = (commands.head + 3) % MP_COMMAND_CAPACITY;
        if (bad == 0) commands.entries[last].input.throttle = 257;
        if (bad == 1) commands.entries[last].tick = 97;
        if (bad == 2) commands.entries[last].sequence = 9;
        if (bad == 3) commands.lastTick++;
        CHECK(!MpReplayInput(&commands, 100, &sequence, &controls));
        CHECK(sequence == 0 && memcmp(&controls, &held, sizeof(controls)) == 0);
        commands = unchanged;
    }
    CHECK(MpAcknowledgeCommands(&commands, 2));
    CHECK(!MpReplayInput(&commands, 102, &sequence, &controls));
    sequence = 2;
    CHECK(MpReplayInput(&commands, 102, &sequence, &controls) && sequence == 3);
    CHECK(MpAcknowledgeCommands(&commands, 4));
    sequence = 4;
    CHECK(MpReplayInput(&commands, 200, &sequence, &controls) && sequence == 4);
    sequence = 5;
    CHECK(!MpReplayInput(&commands, 200, &sequence, &controls) && sequence == 5);
    CHECK(!MpReplayInput(NULL, 0, &sequence, &controls));
    CHECK(!MpReplayInput(&commands, 0, NULL, &controls));
    CHECK(!MpReplayInput(&commands, 0, &sequence, NULL));
    return 0;
}

static int TestPredictionClock(void) {
    MpClock clock = {0};
    uint32_t target = 0xA5A5;
    CHECK(!MpClockTarget(&clock, 0, &target, NULL) && target == 0xA5A5);
    CHECK(MpClockObserve(&clock, 100, 10));
    CHECK(MpClockTarget(&clock, 20000009, &target, NULL) && target == 100);
    CHECK(MpClockTarget(&clock, 20000010, &target, NULL) && target == 101);
    CHECK(MpClockObserve(&clock, 101, 25000010));
    CHECK(clock.origin == 10 && clock.originTick == 100);
    CHECK(MpClockTarget(&clock, 120000010, &target, NULL) && target == 106);
    CHECK(MpClockTarget(&clock, 400000010, &target, NULL) && target == 111);
    CHECK(MpClockObserve(&clock, 120, 401000010));
    CHECK(clock.origin == 401000010 && clock.originTick == 120);
    const MpClock unchanged = clock;
    CHECK(!MpClockObserve(&clock, 120, 402000010));
    CHECK(!MpClockObserve(&clock, 119, 402000010));
    CHECK(!MpClockObserve(&clock, 121, 401000009));
    CHECK(memcmp(&clock, &unchanged, sizeof(clock)) == 0);
    target = 0xA5A5;
    CHECK(!MpClockTarget(&clock, 401000009, &target, NULL) && target == 0xA5A5);
    const unsigned rates[] = {24, 30, 50, 60, 144, 240};
    for (size_t rate = 0; rate < sizeof(rates) / sizeof(rates[0]); ++rate) {
        clock = (MpClock){0};
        CHECK(MpClockObserve(&clock, 100, 0));
        for (unsigned frame = 0; frame <= rates[rate]; ++frame) {
            uint64_t now = (uint64_t)frame * 100000000 / rates[rate];
            CHECK(MpClockTarget(&clock, now, &target, NULL));
            CHECK(target == 100 + now / 20000000);
        }
        CHECK(target == 105);
        CHECK(MpClockTarget(&clock, UINT64_MAX, &target, NULL) && target == 110);
    }
    clock = (MpClock){0};
    CHECK(MpClockObserve(&clock, UINT32_MAX - 2, UINT64_MAX - 1000000000));
    CHECK(MpClockTarget(&clock, UINT64_MAX, &target, NULL) && target == UINT32_MAX);
    CHECK(MpClockObserve(&clock, UINT32_MAX, UINT64_MAX));
    CHECK(!MpClockObserve(&clock, 0, UINT64_MAX));
    clock.originTick = UINT32_MAX; clock.latestTick = 1;
    CHECK(!MpClockTarget(&clock, UINT64_MAX, &target, NULL));
    CHECK(!MpClockObserve(NULL, 1, 0));
    MpClock presentation = {0};
    uint32_t fraction = 0xA5A5;
    CHECK(!MpClockTarget(&presentation, 0, &target, &fraction) && fraction == 0xA5A5);
    CHECK(MpClockObserve(&presentation, 100, 10));
    for (unsigned quarter = 0; quarter < 4; ++quarter) {
        CHECK(MpClockTarget(&presentation, 10 + quarter * UINT64_C(5000000), &target, &fraction));
        CHECK(target == 100 && fraction == quarter * 16384);
    }
    CHECK(MpClockTarget(&presentation, 20000009, &target, &fraction) && target == 100 && fraction == 65535);
    CHECK(MpClockTarget(&presentation, 20000010, &target, &fraction) && target == 101 && fraction == 0);
    CHECK(MpClockTarget(&presentation, 210000010, &target, &fraction) && target == 110 && fraction == 0);
    CHECK(MpClockTarget(&presentation, UINT64_MAX, &target, &fraction) && target == 110 && fraction == 0);
    CHECK(MpClockObserve(&presentation, 120, 210000010));
    CHECK(MpClockTarget(&presentation, 210000010, &target, &fraction) && target == 120 && fraction == 0);
    target = fraction = 123;
    CHECK(!MpClockTarget(&presentation, 210000009, &target, &fraction) && target == 123 && fraction == 123);
    CHECK(!MpClockTarget(&presentation, 210000010, &target, &target) && target == 123);
    CHECK(!MpClockTarget(NULL, 0, &target, NULL));
    CHECK(!MpClockTarget(&unchanged, unchanged.origin, NULL, NULL));
    return 0;
}

static int TestCorrection(void) {
    const GameTrackPoint points[3] = {0}, otherPoints[3] = {0};
    RaceSim source = {.route = {.points = points, .count = 3, .length = 3000},
        .laps = 1, .phase = SIM_RACING, .tick = 100, .elapsed = 80};
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) source.drivers[seat].variant = -1;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        source.drivers[seat].status = SIM_DRIVING;
        source.drivers[seat].inputTick = 98;
        source.drivers[seat].car.drive.gear = 1;
    }
    uint32_t acknowledged[2] = {0x12345678, 42};
    uint8_t packet[MP_CORRECTION_WIRE_SIZE];
    CHECK(MpEncodeCorrection(&source, acknowledged, packet, sizeof(packet)));
    const uint8_t header[] = {0x87, 1, 0x78, 0x56, 0x34, 0x12, 42, 0, 0, 0};
    CHECK(memcmp(packet, header, sizeof(header)) == 0);
    RaceSim receiver = source;
    receiver.route.points = otherPoints;
    receiver.drivers[0].inputTick = 0; /* Validate received state, not stale local state. */
    MpCorrection decoded;
    CHECK(MpDecodeCorrection(&receiver, packet, sizeof(packet), &decoded));
    CHECK(decoded.frame.track == otherPoints && decoded.frame.tick == 100);
    CHECK(memcmp(decoded.acknowledged, acknowledged, sizeof(acknowledged)) == 0);
    uint8_t publication[MP_PUBLICATION_WIRE_SIZE] = {0};
    memcpy(publication, packet, sizeof(packet));
    uint8_t *body = publication + MP_CORRECTION_WIRE_SIZE + 1;
    publication[MP_CORRECTION_WIRE_SIZE] = MP_S2C_SNAPSHOT;
    body[0] = 100; body[4] = 80; body[8] = SIM_RACING;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        body[MP_SNAPSHOT_HEADER_SIZE + seat * MP_SNAPSHOT_SEAT_SIZE] = MP_DRIVING;
        body[MP_SNAPSHOT_HEADER_SIZE + seat * MP_SNAPSHOT_SEAT_SIZE + 53] = 1;
    }
    memcpy(body + MP_SNAPSHOT_BODY_SIZE - 8, packet + 2, 8);
    MpCorrection matched;
    MpSnapshot poses;
    CHECK(MpDecodePublication(&receiver, publication, sizeof(publication), &matched, &poses));
    const MpCorrection matchedState = matched;
    const MpSnapshot matchedPoses = poses;
    for (size_t size = 0; size < sizeof(publication); ++size) {
        CHECK(!MpDecodePublication(&receiver, publication, size, &matched, &poses));
        CHECK(memcmp(&matched, &matchedState, sizeof(matched)) == 0);
        CHECK(memcmp(&poses, &matchedPoses, sizeof(poses)) == 0);
    }
    /* Independently valid snapshot fields must still agree with the checkpoint. */
    const size_t inconsistent[] = {0, 4, MP_SNAPSHOT_BODY_SIZE - 8,
        MP_SNAPSHOT_HEADER_SIZE + MP_SNAPSHOT_SEAT_SIZE,
        MP_SNAPSHOT_HEADER_SIZE + MP_SNAPSHOT_SEAT_SIZE + 1,
        MP_SNAPSHOT_HEADER_SIZE + MP_SNAPSHOT_SEAT_SIZE + 53};
    for (size_t i = 0; i < sizeof(inconsistent) / sizeof(inconsistent[0]); ++i) {
        body[inconsistent[i]] ^= 1;
        CHECK(!MpDecodePublication(&receiver, publication, sizeof(publication), &matched, &poses));
        CHECK(memcmp(&matched, &matchedState, sizeof(matched)) == 0);
        CHECK(memcmp(&poses, &matchedPoses, sizeof(poses)) == 0);
        body[inconsistent[i]] ^= 1;
    }
    MpCommands commands = {0};
    const DriverInput input = {.throttle = 128};
    for (uint32_t sequence = 1; sequence <= 3; ++sequence)
        CHECK(MpRememberCommand(&commands, sequence, 0, &input));
    MpCorrection correction = decoded;
    correction.acknowledged[0] = 2;
    receiver.drivers[0].car.x = 1234;
    CHECK(MpApplyCorrection(&receiver, &commands, 0, &correction));
    CHECK(receiver.drivers[0].car.x == source.drivers[0].car.x);
    CHECK(commands.acknowledged == 2 && commands.count == 1);
    CHECK(MpCommandAt(&commands, 0)->sequence == 3);
    const RaceSim unchangedRace = receiver;
    const MpCommands unchangedCommands = commands;
    for (int bad = 0; bad < 7; ++bad) {
        correction = decoded;
        correction.acknowledged[0] = 2;
        switch (bad) {
        case 0: correction.acknowledged[0] = 1; break;
        case 1: correction.acknowledged[0] = 4; break;
        case 2: correction.frame.drivers[1].car.drive.gear = 7; break;
        case 3: correction.frame.drivers[1].inputTick = 0; break;
        case 4: correction.frame.track = points; break;
        case 5: commands.count++; break;
        default: break;
        }
        const MpCommands before = commands;
        CHECK(!MpApplyCorrection(&receiver, &commands, bad == 6 ? 2 : 0, &correction));
        CHECK(memcmp(&receiver, &unchangedRace, sizeof(receiver)) == 0);
        CHECK(memcmp(&commands, &before, sizeof(commands)) == 0);
        commands = unchangedCommands;
    }
    correction = decoded;
    correction.acknowledged[0] = 2;
    CHECK(MpApplyCorrection(&receiver, &commands, 0, &correction));
    CHECK(memcmp(&commands, &unchangedCommands, sizeof(commands)) == 0);
    correction.acknowledged[1] = 3;
    CHECK(MpApplyCorrection(&receiver, &commands, 1, &correction));
    CHECK(commands.count == 0 && commands.acknowledged == 3);
    CHECK(!MpApplyCorrection(NULL, &commands, 0, &correction));
    CHECK(!MpApplyCorrection(&receiver, NULL, 0, &correction));
    CHECK(!MpApplyCorrection(&receiver, &commands, -1, &correction));
    CHECK(!MpApplyCorrection(&receiver, &commands, 0, NULL));
    CHECK(RestoreRaceFrame(&receiver, &decoded.frame));
    uint8_t repeated[MP_CORRECTION_WIRE_SIZE];
    CHECK(MpEncodeCorrection(&receiver, decoded.acknowledged, repeated, sizeof(repeated)));
    CHECK(memcmp(packet, repeated, sizeof(packet)) == 0);
    memset(&decoded, 0xA5, sizeof(decoded));
    const MpCorrection untouched = decoded;
    for (size_t size = 0; size < sizeof(packet); ++size) {
        CHECK(!MpDecodeCorrection(&receiver, packet, size, &decoded));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    }
    CHECK(!MpDecodeCorrection(&receiver, packet, sizeof(packet) + 1, &decoded));
    const size_t invalidBytes[] = {0, 1, 10};
    for (size_t i = 0; i < sizeof(invalidBytes) / sizeof(invalidBytes[0]); ++i) {
        size_t byte = invalidBytes[i]; uint8_t saved = packet[byte];
        packet[byte] = 0xFF;
        CHECK(!MpDecodeCorrection(&receiver, packet, sizeof(packet), &decoded));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
        packet[byte] = saved;
    }
    source.drivers[1].inputTick = 0;
    memset(repeated, 0xA5, sizeof(repeated));
    CHECK(!MpEncodeCorrection(&source, acknowledged, repeated, sizeof(repeated)));
    for (size_t i = 0; i < sizeof(repeated); ++i) CHECK(repeated[i] == 0xA5);
    acknowledged[1] = 0;
    CHECK(MpEncodeCorrection(&source, acknowledged, packet, sizeof(packet)));
    packet[6] = 1; /* Late acknowledgement cannot precede consumption. */
    CHECK(!MpDecodeCorrection(&receiver, packet, sizeof(packet), &decoded));
    CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    CHECK(!MpEncodeCorrection(NULL, acknowledged, packet, sizeof(packet)));
    CHECK(!MpEncodeCorrection(&source, NULL, packet, sizeof(packet)));
    CHECK(!MpDecodeCorrection(NULL, packet, sizeof(packet), &decoded));
    CHECK(!MpDecodeCorrection(&receiver, NULL, sizeof(packet), &decoded));
    CHECK(!MpDecodeCorrection(&receiver, packet, sizeof(packet), NULL));
    return 0;
}

static int TestFullField(void) {
    uint8_t body[MP_START_BODY_SIZE] = {MP_PROTOCOL_VERSION, 0, 0, 1};
    body[9] = MP_SEAT_LIMIT;
    const size_t aiOffset = 26 + MP_SEAT_LIMIT * 6 + 16;
    const size_t last = aiOffset + (MP_AI_LIMIT - 1) * 7;
    body[last] = 1;
    body[last + 1] = 10;
    body[last + 2] = 10;
    body[last + 3] = 42;
    MpStart start;
    CHECK(MpDecodeStart(body, sizeof(body), &start));
    RaceSetup setup;
    CHECK(MpBuildSetup(&start, &setup));
    CHECK(setup.entrants[MP_FIELD_LIMIT - 1].kind == RACE_SEAT_AI);
    CHECK(setup.entrants[MP_FIELD_LIMIT - 1].model == 10);
    CHECK(setup.entrants[MP_FIELD_LIMIT - 1].rivalSlot == 10);
    CHECK(setup.entrants[MP_FIELD_LIMIT - 1].seed == 42);
    CHECK(setup.entrants[2].kind == RACE_SEAT_EMPTY);
    MpStart saved = start;
    body[aiOffset] = 1;
    body[aiOffset + 2] = 10; /* Duplicate behavior slot. */
    CHECK(!MpDecodeStart(body, sizeof(body), &start));
    CHECK(memcmp(&start, &saved, sizeof(start)) == 0);
    body[aiOffset] = 0; /* Inactive fields must be canonical zero. */
    CHECK(!MpDecodeStart(body, sizeof(body), &start));

    RaceSim race = {.phase = SIM_RACING, .laps = 1};
    SimDriver *ai = &race.drivers[MP_FIELD_LIMIT - 1];
    ai->status = SIM_DRIVING;
    ai->rival = 1;
    AsRivalCar(&ai->car)->targetSpeed = 700;
    ai->car.drive.gear = 5; /* Overlaps AI state, must remain untouched. */
    MpSnapshot update = {.tick = 1, .elapsed = 1, .phase = SIM_RACING};
    MpCarPose *pose = &update.seats[MP_FIELD_LIMIT - 1];
    *pose = (MpCarPose){.status = MP_DRIVING, .x = 123, .rpm = 4200,
                       .throttle = 256, .lap = 1, .place = 12};
    CHECK(MpApplySnapshot(&race, &update));
    CHECK(ai->car.x == 123 && ai->place == 12);
    CHECK(AsRivalCar(&ai->car)->engineRpm == 4200);
    CHECK(AsRivalCar(&ai->car)->targetSpeed == 700 && ai->car.drive.gear == 5);
    CHECK(race.drivers[2].status == SIM_EMPTY);
    const RaceSim before = race;
    update.tick++;
    pose->gear = 1; /* AI cannot receive player gearbox data. */
    CHECK(!MpApplySnapshot(&race, &update));
    CHECK(memcmp(&race, &before, sizeof(race)) == 0);
    return 0;
}

static int TestStart(void) {
    uint8_t body[MP_START_BODY_SIZE + 1] = {
        MP_PROTOCOL_VERSION, 0, 0, 3, 0, 150, 0, 0, 0, 2,
        'S', 'C', 'E', 'S', '_', '0', '0', '6', '.', '9', '6', 0, 0, 0, 0, 0,
        0, 0, 0x70, 0x56, 0x34, 0x12, 0, 0, 0x71, 0x56, 0x34, 0x12,
        0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01,
        8, 7, 6, 5, 4, 3, 2, 1
    };
    MpStart start;
    CHECK(MpDecodeStart(body, MP_START_BODY_SIZE, &start));
    CHECK(start.course == 0 && start.classIndex == 0 && start.laps == 3);
    CHECK(start.countdown == 150 && start.reverse == 0);
    CHECK(start.fingerprint == UINT64_C(0x0123456789ABCDEF));
    CHECK(start.executable == UINT64_C(0x0102030405060708));
    CHECK(strcmp(start.boot, "SCES_006.96") == 0);
    CHECK(start.seats[0].seed == 0x12345670 && start.seats[1].seed == 0x12345671);
    MpStart before = start;
    for (size_t size = 0; size < MP_START_BODY_SIZE; ++size) {
        CHECK(!MpDecodeStart(body, size, &start));
        CHECK(memcmp(&start, &before, sizeof(start)) == 0);
    }
    CHECK(!MpDecodeStart(body, sizeof(body), &start));
    CHECK(memcmp(&start, &before, sizeof(start)) == 0);
    CHECK(!MpDecodeStart(NULL, MP_START_BODY_SIZE, &start));
    CHECK(!MpDecodeStart(body, MP_START_BODY_SIZE, NULL));
    const struct { size_t offset; uint8_t value; } invalid[] = {
        {0, 0}, {0, 1}, {0, 15}, {0, 16}, {0, 17}, {0, 18}, {0, 19}, {0, 20}, {1, 4}, {2, 6}, {3, 0},
        {3, PLAYER_LAP_TIME_CAPACITY + 1}, {4, 2}, {9, 1}, {9, 3},
        {26, 32}, {27, 2}, {32, 32}, {33, 2},
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        uint8_t original = body[invalid[i].offset];
        body[invalid[i].offset] = invalid[i].value;
        CHECK(!MpDecodeStart(body, MP_START_BODY_SIZE, &start));
        CHECK(memcmp(&start, &before, sizeof(start)) == 0);
        body[invalid[i].offset] = original;
    }
    memset(body + 10, 'X', 16);
    CHECK(!MpDecodeStart(body, MP_START_BODY_SIZE, &start));
    CHECK(memcmp(&start, &before, sizeof(start)) == 0);
    memset(body + 10, 0, 16);
    body[1] = 3; body[2] = 5; body[3] = PLAYER_LAP_TIME_CAPACITY;
    body[4] = 1; body[32] = 31; body[33] = 1;
    CHECK(MpDecodeStart(body, MP_START_BODY_SIZE, &start));
    CHECK(start.course == 3 && start.classIndex == 5 && start.reverse == 1);
    CHECK(start.seats[1].model == 31 && start.seats[1].manual == 1);
    unsigned char source[] = {0, 1, 2, 255};
    RaceData archive = {.data = source, .size = sizeof(source), .executable = start.executable};
    memcpy(archive.boot, "SCES_006.96", 12);
    memcpy(start.boot, archive.boot, sizeof(start.boot));
    start.fingerprint = ArchiveFingerprint(source, sizeof(source));
    CHECK(MpMatchesArchive(&start, &archive));
    archive.executable ^= 1;
    CHECK(!MpMatchesArchive(&start, &archive));
    archive.executable ^= 1;
    uint64_t code = start.executable;
    start.executable = 0;
    CHECK(!MpMatchesArchive(&start, &archive));
    start.executable = code;
    source[3] ^= 1;
    CHECK(!MpMatchesArchive(&start, &archive));
    source[3] ^= 1;
    archive.size--;
    CHECK(!MpMatchesArchive(&start, &archive));
    archive.size++;
    archive.boot[0] = 'X';
    CHECK(!MpMatchesArchive(&start, &archive));
    archive.boot[0] = 'S';
    CHECK(!MpMatchesArchive(NULL, &archive));
    CHECK(!MpMatchesArchive(&start, NULL));
    memset(archive.boot, 'X', sizeof(archive.boot));
    CHECK(!MpMatchesArchive(&start, &archive));

    RaceSetup setup;
    memset(&setup, 0xA5, sizeof(setup));
    CHECK(MpBuildSetup(&start, &setup));
    CHECK(setup.classIndex == 5 && setup.courseIndex == 3 && setup.laps == PLAYER_LAP_TIME_CAPACITY);
    CHECK(setup.reverse == 1);
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        CHECK(setup.entrants[seat].kind == RACE_SEAT_HUMAN);
        CHECK(setup.entrants[seat].grid == seat);
        CHECK(setup.entrants[seat].model == start.seats[seat].model);
        CHECK(setup.entrants[seat].manual == start.seats[seat].manual);
        CHECK(setup.entrants[seat].seed == start.seats[seat].seed);
        CHECK(setup.looks[seat].variant == start.seats[seat].model);
        CHECK(!setup.looks[seat].hasPaint);
    }
    RaceSetup empty = {0};
    CHECK(memcmp(setup.entrants + MP_SEAT_LIMIT, empty.entrants + MP_SEAT_LIMIT,
                 sizeof(RaceEntrant) * (DRIVER_SEAT_LIMIT - MP_SEAT_LIMIT)) == 0);
    CHECK(memcmp(setup.looks + MP_SEAT_LIMIT, empty.looks + MP_SEAT_LIMIT,
                 sizeof(RaceCarLook) * (DRIVER_SEAT_LIMIT - MP_SEAT_LIMIT)) == 0);
    RaceSetup beforeSetup = setup;
    CHECK(!MpBuildSetup(NULL, &setup));
    CHECK(!MpBuildSetup(&start, NULL));
    start.seats[1].model = CAR_MODEL_VARIANT_COUNT;
    CHECK(!MpBuildSetup(&start, &setup));
    CHECK(memcmp(&setup, &beforeSetup, sizeof(setup)) == 0);
    return 0;
}

static int TestChoice(void) {
    MpStart start = {.laps = 1};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        for (int car = 0; car < CAR_MODEL_VARIANT_COUNT; ++car) {
            for (int manual = 0; manual <= 1; ++manual) {
                MpSettings choice = {.car = car, .manual = manual};
                start.seats[seat] = (MpSeat){.model = (uint8_t)car, .manual = (uint8_t)manual};
                CHECK(MpMatchesChoice(&start, seat, &choice));
                choice.car = (car + 1) % CAR_MODEL_VARIANT_COUNT;
                CHECK(!MpMatchesChoice(&start, seat, &choice));
                choice.car = car;
                choice.manual = !manual;
                CHECK(!MpMatchesChoice(&start, seat, &choice));
            }
        }
    }
    MpSettings choice = {.car = 31, .manual = 1};
    CHECK(!MpMatchesChoice(NULL, 1, &choice));
    CHECK(!MpMatchesChoice(&start, 1, NULL));
    CHECK(!MpMatchesChoice(&start, -1, &choice));
    CHECK(!MpMatchesChoice(&start, MP_SEAT_LIMIT, &choice));
    start.seats[0].manual = 2;
    CHECK(!MpMatchesChoice(&start, 1, &choice));
    return 0;
}

static int TestCarPresentation(void) {
    PlayerCarRuntime car = {.activeFlag = 7, .x = 99};
    car.drive.brakeInput = 0; // A newer authoritative sample has released the brake.
    MpCarPose before = {.status = MP_DRIVING, .x = 10, .gear = 2, .lap = 1};
    MpCarPose after = {.status = MP_DRIVING, .x = 30, .brake = 256,
                       .throttle = 200, .gear = 3, .rpm = 8000, .lap = 2};
    MpCarPose pose;
    CHECK(MpBlendPose(&before, &after, 32768, &pose));
    CHECK(MpApplyPose(&car, &pose, 0));
    CHECK(car.x == 20 && car.drive.brakeInput == 256 && car.drive.gear == 3);
    CHECK(car.activeFlag == 7);
    CHECK(car.lap == 2); /* Lap is discrete, never interpolated. */
    CHECK(MpBlendPose(&before, &after, 65536, &pose));
    CHECK(MpApplyPose(&car, &pose, 0));
    CHECK(car.x == 30 && car.drive.brakeInput == 256 && car.drive.gear == 3);
    CHECK(car.drive.acceleratorInput.value == 200 && car.drive.engineRpm == 8000);
    PlayerCarRuntime saved;
    memcpy(&saved, &car, sizeof(saved));
    pose.x = -1;
    pose.brake = 257;
    CHECK(!MpApplyPose(&car, &pose, 0));
    CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
    CHECK(!MpApplyPose(NULL, &after, 0));
    CHECK(!MpApplyPose(&car, NULL, 0));
    return 0;
}

static int TestSnapshotApplication(void) {
    RaceSim race = {.phase = SIM_COUNTDOWN, .tick = 10};
    race.drivers[0].status = race.drivers[1].status = SIM_DRIVING;
    race.drivers[0].random = 42;
    race.drivers[1].random = 43;
    race.drivers[2].car.x = 123;
    MpSnapshot snapshot = {.tick = 20, .phase = SIM_RACING, .seats = {
        {.status = 1, .x = 100, .y = -200, .z = 300, .yaw = 1024, .pitch = -23, .roll = 12, .steering = -80, .wheels = 4096, .brake = 256, .progress = 99, .rpm = 8000, .throttle = 256, .clutch = 2, .gear = 3, .ground = -600, .rollSpeed = -7},
        {.status = 0},
    }};
    RaceSim expected = race;
    snapshot.seats[0].speed = 400;
    expected.drivers[0].car.speed = 400;
    expected.tick = 20;
    expected.phase = SIM_RACING;
    expected.drivers[0].car.x = 100;
    expected.drivers[0].car.y = -200;
    expected.drivers[0].car.z = 300;
    expected.drivers[0].car.bodyYaw = 1024;
    expected.drivers[0].car.bodyPitch = -23;
    expected.drivers[0].car.bodyRoll = 12;
    expected.drivers[0].car.steeringAngle = -80;
    expected.drivers[0].car.wheelRotation = 4096;
    expected.drivers[0].car.drive.brakeInput = 256;
    expected.drivers[0].car.trackProgress = 99;
    expected.drivers[0].car.drive.engineRpm = 8000;
    expected.drivers[0].car.drive.acceleratorInput.value = 256;
    expected.drivers[0].car.drive.clutch = 2;
    expected.drivers[0].car.drive.gear = 3;
    expected.drivers[0].car.modelY = -600;
    expected.drivers[0].car.bodyRollVelocity = -7;
    expected.drivers[1].car.activeFlag = -1;
    expected.drivers[1].status = SIM_RETIRED;
    CHECK(MpApplySnapshot(&race, &snapshot));
    CHECK(memcmp(&race, &expected, sizeof(race)) == 0);
    snapshot.seats[0].x = 999; /* Detect partial writes before a late-seat error. */
    CHECK(!MpApplySnapshot(&race, &snapshot)); /* Duplicate. */
    snapshot.tick = 19;
    CHECK(!MpApplySnapshot(&race, &snapshot));
    snapshot.tick = 21;
    snapshot.phase = SIM_COUNTDOWN;
    CHECK(!MpApplySnapshot(&race, &snapshot));
    snapshot.phase = 255;
    CHECK(!MpApplySnapshot(&race, &snapshot));
    snapshot.phase = SIM_RACING;
    snapshot.seats[1].status = 1; /* Retired seats cannot reappear. */
    CHECK(!MpApplySnapshot(&race, &snapshot));
    snapshot.seats[1].status = 2;
    CHECK(!MpApplySnapshot(&race, &snapshot));
    CHECK(!MpApplySnapshot(NULL, &snapshot));
    CHECK(!MpApplySnapshot(&race, NULL));
    CHECK(memcmp(&race, &expected, sizeof(race)) == 0);
    snapshot.seats[1].status = 0;
    snapshot.tick = 40; /* Newest-only delivery may skip ticks. */
    CHECK(MpApplySnapshot(&race, &snapshot));
    CHECK(race.tick == 40);
    snapshot.tick = 41;
    snapshot.seats[0].status = MP_FINISHED;
    CHECK(MpApplySnapshot(&race, &snapshot));
    CHECK(race.drivers[0].status == SIM_DRIVER_FINISHED);
    expected = race;
    snapshot.tick = 42;
    for (int status = MP_RETIRED; status <= MP_DRIVING; ++status) {
        snapshot.seats[0].status = status;
        CHECK(!MpApplySnapshot(&race, &snapshot));
        CHECK(memcmp(&race, &expected, sizeof(race)) == 0);
    }
    snapshot.seats[0].status = MP_FINISHED;
    CHECK(MpApplySnapshot(&race, &snapshot));
    expected.tick = 42;
    CHECK(memcmp(&race, &expected, sizeof(race)) == 0);
    return 0;
}

static int TestCountdown(void) {
    RaceSim race = {.phase = SIM_COUNTDOWN, .countdown = 150};
    race.drivers[0].status = race.drivers[1].status = SIM_DRIVING;
    MpSnapshot snapshot = {.tick = 60, .phase = SIM_COUNTDOWN,
        .seats = {{.status = MP_DRIVING}, {.status = MP_DRIVING}}};
    CHECK(MpApplySnapshot(&race, &snapshot));
    CHECK(race.countdown == 90 && race.elapsed == 0);
    snapshot.tick = 70;
    CHECK(MpApplySnapshot(&race, &snapshot));
    CHECK(race.countdown == 80);
    RaceSim unchanged = race;
    snapshot.tick = 150;
    CHECK(!MpApplySnapshot(&race, &snapshot)); /* Must already be racing. */
    CHECK(memcmp(&race, &unchanged, sizeof(race)) == 0);
    snapshot.phase = SIM_RACING;
    CHECK(MpApplySnapshot(&race, &snapshot));
    CHECK(race.countdown == 0 && race.elapsed == 0);
    snapshot.tick = 157;
    snapshot.elapsed = 7;
    CHECK(MpApplySnapshot(&race, &snapshot));
    CHECK(race.countdown == 0 && race.elapsed == 7);
    return 0;
}

static int TestCommands(void) {
    MpCommands commands = {0};
    CHECK(!MpCommandAt(&commands, 0) && !MpCommandAt(NULL, 0));
    CHECK(MpAcknowledgeCommands(&commands, 0));
    DriverInput input = {.throttle = 256, .shiftUp = 1};
    CHECK(!MpRememberCommand(NULL, 1, 0, &input));
    CHECK(!MpRememberCommand(&commands, 1, 0, NULL));
    CHECK(!MpAcknowledgeCommands(NULL, 0));
    for (int round = 0; round < 3; ++round) {
        while (commands.count < MP_COMMAND_CAPACITY) {
            uint32_t sequence = commands.sent + 1;
            input.throttle = sequence % 257;
            CHECK(MpRememberCommand(&commands, sequence, 0, &input));
        }
        for (unsigned index = 0; index < commands.count; ++index) {
            const MpCommand *command = MpCommandAt(&commands, index);
            CHECK(command && command->sequence == commands.acknowledged + index + 1);
            CHECK(command->input.throttle == command->sequence % 257 && command->input.shiftUp);
        }
        MpCommands unchanged = commands;
        CHECK(!MpRememberCommand(&commands, commands.sent + 1, 0, &input));
        CHECK(!MpAcknowledgeCommands(&commands, commands.sent + 1));
        CHECK(!MpCommandAt(&commands, commands.count));
        CHECK(memcmp(&commands, &unchanged, sizeof(commands)) == 0);
        CHECK(MpAcknowledgeCommands(&commands, commands.acknowledged + MP_COMMAND_CAPACITY / 2));
        CHECK(commands.count == MP_COMMAND_CAPACITY / 2);
        unchanged = commands;
        CHECK(!MpAcknowledgeCommands(&commands, commands.acknowledged - 1));
        CHECK(!MpRememberCommand(&commands, commands.sent, 0, &input));
        input.shiftDown = 2;
        CHECK(!MpRememberCommand(&commands, commands.sent + 1, 0, &input));
        input.shiftDown = 0;
        CHECK(memcmp(&commands, &unchanged, sizeof(commands)) == 0);
    }
    CHECK(MpAcknowledgeCommands(&commands, commands.sent));
    CHECK(!commands.count && !MpCommandAt(&commands, 0));
    commands = (MpCommands){.sent = UINT32_MAX - 1, .acknowledged = UINT32_MAX - 1};
    CHECK(MpRememberCommand(&commands, UINT32_MAX, 0, &input));
    CHECK(!MpRememberCommand(&commands, 0, 0, &input));
    CHECK(MpAcknowledgeCommands(&commands, UINT32_MAX));
    CHECK(!commands.count);
    const MpCommands unchanged = commands;
    commands.head = MP_COMMAND_CAPACITY;
    CHECK(!MpCommandAt(&commands, 0) && !MpAcknowledgeCommands(&commands, UINT32_MAX));
    commands = unchanged;
    CHECK(!MpRememberCommand(&commands, 0, 0, &input));
    CHECK(memcmp(&commands, &unchanged, sizeof(commands)) == 0);
    commands = (MpCommands){0};
    CHECK(MpRememberCommand(&commands, 1, 100, &input));
    CHECK(MpRememberCommand(&commands, 2, 100, &input));
    CHECK(MpCommandAt(&commands, 0)->tick == 100);
    const MpCommands timed = commands;
    CHECK(!MpRememberCommand(&commands, 3, 99, &input));
    CHECK(memcmp(&commands, &timed, sizeof(timed)) == 0);
    CHECK(MpAcknowledgeCommands(&commands, 2));
    CHECK(commands.count == 0 && commands.lastTick == 100);
    CHECK(!MpRememberCommand(&commands, 3, 99, &input));
    CHECK(MpRememberCommand(&commands, 3, 101, &input));
    CHECK(MpCommandAt(&commands, 0)->tick == 101);
    return 0;
}

static int TestRaceClock(void) {
    uint8_t body[MP_SNAPSHOT_BODY_SIZE] = {10};
    body[4] = 7;
    body[8] = SIM_RACING;
    MpSnapshot decoded;
    CHECK(MpDecodeSnapshot(body, sizeof(body), &decoded));
    CHECK(decoded.tick == 10 && decoded.elapsed == 7);
    const size_t ack = MP_SNAPSHOT_BODY_SIZE - MP_SEAT_LIMIT * 4;
    body[ack] = 4; body[ack + 1] = 3; body[ack + 2] = 2; body[ack + 3] = 1;
    memset(body + ack + 4, 255, 4);
    CHECK(MpDecodeSnapshot(body, sizeof(body), &decoded));
    CHECK(decoded.acknowledged[0] == UINT32_C(0x01020304));
    CHECK(decoded.acknowledged[1] == UINT32_MAX);
    MpSnapshot unchanged = decoded;
    body[4] = 11;
    CHECK(!MpDecodeSnapshot(body, sizeof(body), &decoded));
    CHECK(memcmp(&decoded, &unchanged, sizeof(decoded)) == 0);
    body[4] = 7;
    body[8] = SIM_COUNTDOWN;
    CHECK(!MpDecodeSnapshot(body, sizeof(body), &decoded));
    CHECK(memcmp(&decoded, &unchanged, sizeof(decoded)) == 0);
    MpHistory history = {0};
    CHECK(MpHistoryPush(&history, &decoded, 0));
    MpHistory saved = history;
    decoded.tick++;
    decoded.acknowledged[1]--;
    CHECK(!MpHistoryPush(&history, &decoded, 0));
    CHECK(memcmp(&history, &saved, sizeof(history)) == 0);
    decoded.tick--;
    decoded.acknowledged[1]++;
    decoded.tick++;
    decoded.elapsed--;
    CHECK(!MpHistoryPush(&history, &decoded, 0));
    CHECK(memcmp(&history, &saved, sizeof(history)) == 0);
    decoded.elapsed = 8;
    RaceSim race = {.phase = SIM_RACING};
    race.drivers[0].status = race.drivers[1].status = SIM_DRIVING;
    CHECK(MpApplySnapshot(&race, &decoded));
    CHECK(race.elapsed == 8 && race.tick == 11);
    RaceSim before = race;
    decoded.tick++;
    decoded.elapsed--;
    CHECK(!MpApplySnapshot(&race, &decoded));
    CHECK(memcmp(&race, &before, sizeof(race)) == 0);
    return 0;
}

static int TestFinalSnapshot(void) {
    uint8_t body[MP_SNAPSHOT_BODY_SIZE] = {1};
    body[8] = SIM_FINISHED;
    MpSnapshot decoded;
    CHECK(MpDecodeSnapshot(body, sizeof(body), &decoded));
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        body[MP_SNAPSHOT_HEADER_SIZE + seat * MP_SNAPSHOT_SEAT_SIZE] = MP_DRIVING;
        MpSnapshot unchanged = decoded;
        CHECK(!MpDecodeSnapshot(body, sizeof(body), &decoded));
        CHECK(memcmp(&decoded, &unchanged, sizeof(decoded)) == 0);
        MpSnapshot invalid = unchanged;
        invalid.seats[seat].status = MP_DRIVING;
        MpHistory history = {0}, saved = history;
        CHECK(!MpHistoryPush(&history, &invalid, 0));
        CHECK(memcmp(&history, &saved, sizeof(history)) == 0);
        RaceSim race = {.phase = SIM_RACING};
        race.drivers[0].status = race.drivers[1].status = SIM_DRIVING;
        RaceSim before = race;
        CHECK(!MpApplySnapshot(&race, &invalid));
        CHECK(memcmp(&race, &before, sizeof(race)) == 0);
        body[MP_SNAPSHOT_HEADER_SIZE + seat * MP_SNAPSHOT_SEAT_SIZE] = MP_FINISHED;
        CHECK(MpDecodeSnapshot(body, sizeof(body), &decoded));
        CHECK(MpApplySnapshot(&race, &decoded));
        CHECK(MpHistoryPush(&history, &decoded, 0));
    }
    return 0;
}

static int TestResult(void) {
    uint8_t body[MP_RESULT_BODY_SIZE + 1] = {1, 1, 0xE8, 3, 0, 0, 0, 0, 255, 255, 255, 255};
    MpResult result;
    CHECK(MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
    CHECK(result.seats[0].finished == 1 && result.seats[0].place == 1 && result.seats[0].milliseconds == 1000);
    CHECK(result.seats[1].finished == 0 && result.seats[1].place == 0 && result.seats[1].milliseconds == -1);
    MpSnapshot final = {.tick = 50, .elapsed = 50, .phase = SIM_FINISHED,
        .seats = {{.status = MP_FINISHED}, {.status = MP_RETIRED}}};
    CHECK(MpMatchesResult(&final, &result));
    result.seats[0].milliseconds = 1001;
    CHECK(!MpMatchesResult(&final, &result));
    result.seats[0].milliseconds = -1;
    CHECK(!MpMatchesResult(&final, &result));
    result.seats[0].milliseconds = INT32_MAX;
    final.tick = final.elapsed = UINT32_MAX;
    CHECK(MpMatchesResult(&final, &result)); /* Retail clock saturates to i32. */
    result.seats[0].milliseconds = 1000;
    final.tick = final.elapsed = 50;
    final.elapsed = 49;
    CHECK(!MpMatchesResult(&final, &result));
    final.elapsed = 50;
    CHECK(!MpMatchesResult(NULL, &result));
    CHECK(!MpMatchesResult(&final, NULL));
    final.phase = SIM_RACING;
    CHECK(!MpMatchesResult(&final, &result));
    final.phase = SIM_FINISHED;
    final.seats[0].status = MP_DRIVING;
    CHECK(!MpMatchesResult(&final, &result));
    final.seats[0].status = MP_RETIRED;
    CHECK(!MpMatchesResult(&final, &result));
    final.seats[0].status = MP_FINISHED;
    final.seats[1].status = MP_FINISHED;
    CHECK(!MpMatchesResult(&final, &result));
    final.seats[1].status = MP_RETIRED;
    CHECK(MpMatchesResult(&final, &result));
    MpResult before = result;
    for (size_t size = 0; size < MP_RESULT_BODY_SIZE; ++size) {
        CHECK(!MpDecodeResult(body, size, &result));
        CHECK(memcmp(&result, &before, sizeof(result)) == 0);
    }
    CHECK(!MpDecodeResult(body, sizeof(body), &result));
    CHECK(!MpDecodeResult(NULL, MP_RESULT_BODY_SIZE, &result));
    CHECK(!MpDecodeResult(body, MP_RESULT_BODY_SIZE, NULL));
    const struct { size_t offset; uint8_t value; } invalid[] = {
        {0, 2}, {1, 0}, {1, DRIVER_SEAT_LIMIT + 1}, {5, 128}, {7, 1}, {8, 0},
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        uint8_t old = body[invalid[i].offset];
        body[invalid[i].offset] = invalid[i].value;
        CHECK(!MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
        CHECK(memcmp(&result, &before, sizeof(result)) == 0);
        body[invalid[i].offset] = old;
    }
    memcpy(body + 6, body, 6);
    CHECK(!MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result)); /* Duplicate place. */
    CHECK(memcmp(&result, &before, sizeof(result)) == 0);
    body[7] = 2;
    CHECK(MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
    CHECK(result.seats[1].finished && result.seats[1].place == 2);
    before = result;
    body[8] = 0xE7; /* Second place cannot finish before first place. */
    CHECK(!MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
    CHECK(memcmp(&result, &before, sizeof(result)) == 0);
    body[8] = 0xE9;
    CHECK(MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
    body[1] = 2;
    body[7] = 1; /* Same contradiction when the winner is seat one. */
    before = result;
    CHECK(!MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
    CHECK(memcmp(&result, &before, sizeof(result)) == 0);
    body[8] = 0xE8; /* Equal times can have different places within one tick. */
    CHECK(MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
    return 0;
}

static int TestPoseBlend(void) {
    MpCarPose before = {.status = MP_DRIVING, .x = INT32_MIN,
        .yaw = 4090, .pitch = 6, .wheels = 4090};
    MpCarPose after = {.status = MP_DRIVING, .x = INT32_MAX,
        .yaw = 6, .pitch = 4090, .wheels = 6, .gear = 3, .brake = 256};
    MpCarPose pose;
    CHECK(MpBlendPose(&before, &after, 32768, &pose));
    CHECK(pose.x == -1 && pose.yaw == 0 && pose.pitch == 0 && pose.wheels == 0);
    CHECK(pose.gear == 3 && pose.brake == 256);
    after.wheels |= CAR_WHEEL_BLUR_FLAG;
    CHECK(MpBlendPose(&before, &after, 32768, &pose));
    CHECK(pose.wheels == CAR_WHEEL_BLUR_FLAG);
    CHECK(MpBlendPose(&before, &after, 0, &pose) && pose.x == INT32_MIN);
    CHECK(MpBlendPose(&before, &after, 65536, &pose) && pose.x == INT32_MAX);
    MpCarPose saved = pose;
    CHECK(!MpBlendPose(&before, &after, 65537, &pose));
    CHECK(memcmp(&saved, &pose, sizeof(pose)) == 0);
    after.status = MP_FINISHED;
    CHECK(MpBlendPose(&before, &after, 0, &pose));
    CHECK(memcmp(&pose, &after, sizeof(pose)) == 0);
    after.status = MP_RETIRED;
    CHECK(MpBlendPose(&before, &after, 32768, &pose));
    CHECK(memcmp(&pose, &after, sizeof(pose)) == 0);
    CHECK(!MpBlendPose(NULL, &after, 0, &pose));
    CHECK(!MpBlendPose(&before, NULL, 0, &pose));
    CHECK(!MpBlendPose(&before, &after, 0, NULL));
    after.brake = 257;
    saved = pose;
    CHECK(!MpBlendPose(&before, &after, 32768, &pose));
    CHECK(memcmp(&saved, &pose, sizeof(pose)) == 0);
    return 0;
}

static int TestHistory(void) {
    const uint64_t origin = 1000000000;
    MpHistory history = {0};
    MpCarPose pose = {.x = 123}, saved = pose;
    CHECK(!MpHistoryPose(&history, 0, origin, 0, &pose));
    CHECK(memcmp(&pose, &saved, sizeof(pose)) == 0);
    MpSnapshot sample = {.tick = 100, .phase = SIM_RACING};
    sample.seats[0].status = sample.seats[1].status = MP_DRIVING;
    CHECK(MpHistoryPush(&history, &sample, origin));
    sample.tick = 102;
    sample.seats[1].x = 200;
    CHECK(MpHistoryPush(&history, &sample, origin + 50000000));
    sample.tick = 104;
    sample.seats[1].x = 400;
    CHECK(MpHistoryPush(&history, &sample, origin + 90000000));
    CHECK(MpHistoryPose(&history, 1, origin + 130000000, 100000000, &pose));
    CHECK(pose.x == 150); /* Tick 101.5, despite irregular arrivals. */
    sample.tick = 106;
    sample.seats[1].x = 600;
    CHECK(MpHistoryPush(&history, &sample, origin + 130000000));
    CHECK(MpHistoryPose(&history, 1, origin + 130000000, 100000000, &pose));
    CHECK(pose.x == 150); /* Arrival does not restart playback. */
    CHECK(MpHistoryPose(&history, 1, UINT64_MAX, 0, &pose) && pose.x == 600);
    CHECK(MpHistoryPose(&history, 1, origin, UINT64_MAX, &pose) && pose.x == 0);
    MpHistory unchanged = history;
    CHECK(!MpHistoryPush(&history, &sample, origin + 140000000));
    sample.tick++;
    CHECK(!MpHistoryPush(&history, &sample, origin - 1));
    sample.seats[1].brake = 257;
    CHECK(!MpHistoryPush(&history, &sample, origin + 150000000));
    CHECK(memcmp(&history, &unchanged, sizeof(history)) == 0);
    sample.seats[1].brake = 0;
    for (unsigned i = 0; i < MP_HISTORY_CAPACITY + 2; ++i) {
        sample.tick = 108 + i;
        sample.seats[1].x = (int32_t)i;
        CHECK(MpHistoryPush(&history, &sample, origin + 160000000 + i * 20000000));
    }
    CHECK(history.count == MP_HISTORY_CAPACITY && history.samples[0].tick == 110);
    CHECK(MpHistoryPose(&history, 1, origin, 0, &pose) && pose.x == 2);
    saved = pose;
    CHECK(!MpHistoryPose(&history, MP_FIELD_LIMIT, origin, 0, &pose));
    CHECK(!MpHistoryPose(&history, 0, origin - 1, 0, &pose));
    CHECK(memcmp(&pose, &saved, sizeof(pose)) == 0);
    sample.tick++;
    sample.seats[1].status = MP_FINISHED;
    CHECK(MpHistoryPush(&history, &sample, origin + 500000000));
    CHECK(MpHistoryPose(&history, 1, UINT64_MAX, 0, &pose) && pose.status == MP_FINISHED);
    unchanged = history;
    sample.tick++;
    sample.seats[1].status = MP_DRIVING;
    CHECK(!MpHistoryPush(&history, &sample, origin + 520000000));
    sample.seats[1].status = MP_FINISHED;
    sample.phase = SIM_COUNTDOWN;
    CHECK(!MpHistoryPush(&history, &sample, origin + 520000000));
    CHECK(memcmp(&history, &unchanged, sizeof(history)) == 0);
    return 0;
}

static int TestLapWire(void) {
    uint8_t body[MP_SNAPSHOT_BODY_SIZE] = {10};
    body[4] = 5;
    body[8] = SIM_RACING;
    body[MP_SNAPSHOT_HEADER_SIZE] = MP_DRIVING;
    const size_t lap = MP_SNAPSHOT_HEADER_SIZE + 69;
    body[lap] = 2;
    body[lap + 4] = 2;
    MpSnapshot snapshot;
    CHECK(MpDecodeSnapshot(body, sizeof(body), &snapshot));
    CHECK(snapshot.seats[0].lap == 2 && snapshot.seats[1].lap == 0);
    CHECK(snapshot.seats[0].place == 2 && snapshot.seats[1].place == 0);
    const MpSnapshot before = snapshot;
    body[lap] = PLAYER_LAP_TIME_CAPACITY + 2;
    CHECK(!MpDecodeSnapshot(body, sizeof(body), &snapshot));
    CHECK(memcmp(&snapshot, &before, sizeof(snapshot)) == 0);
    body[lap] = 2;
    memset(body + lap + 1, 0, 3);
    body[lap + 4] = DRIVER_SEAT_LIMIT + 1;
    CHECK(!MpDecodeSnapshot(body, sizeof(body), &snapshot));
    CHECK(memcmp(&snapshot, &before, sizeof(snapshot)) == 0);
    body[lap + 4] = 2;
    memset(body + lap, 255, 4);
    CHECK(!MpDecodeSnapshot(body, sizeof(body), &snapshot));
    CHECK(memcmp(&snapshot, &before, sizeof(snapshot)) == 0);
    PlayerCarRuntime car = {0};
    MpCarPose pose = before.seats[0];
    CHECK(MpApplyPose(&car, &pose, 0) && car.lap == 2);
    const PlayerCarRuntime saved = car;
    pose.lap = -1;
    CHECK(!MpApplyPose(&car, &pose, 0));
    CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
    RaceSim race = {.laps = 3, .tick = 1, .phase = SIM_RACING};
    MpSnapshot update = {.tick = 2, .elapsed = 1, .phase = SIM_RACING};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        race.drivers[seat].status = SIM_DRIVING;
        race.drivers[seat].car.lap = 1;
        update.seats[seat].status = MP_DRIVING;
        update.seats[seat].lap = 2;
        update.seats[seat].place = seat + 1;
    }
    CHECK(MpApplySnapshot(&race, &update));
    CHECK(race.drivers[0].car.lap == 2 && race.drivers[1].car.lap == 2);
    CHECK(race.drivers[0].place == 1 && race.drivers[1].place == 2);
    const RaceSim current = race;
    update.tick++;
    update.seats[0].x = 999;
    update.seats[1].lap = 1;
    CHECK(!MpApplySnapshot(&race, &update)); /* A late-seat regression is atomic. */
    CHECK(memcmp(&race, &current, sizeof(race)) == 0);
    update.seats[1].lap = 5;
    CHECK(!MpApplySnapshot(&race, &update)); /* More than the configured laps. */
    CHECK(memcmp(&race, &current, sizeof(race)) == 0);
    update.seats[1].lap = 2;
    update.seats[1].status = MP_RETIRED;
    update.seats[1].place = 0;
    CHECK(MpApplySnapshot(&race, &update));
    CHECK(race.drivers[1].status == SIM_RETIRED && race.drivers[1].place == 0);
    return 0;
}

static int TestCarPicker(void) {
    const uint32_t automatic = UINT32_C(1) | (UINT32_C(1) << 31);
    MpSettings choice = {7243, 0, 0};
    CHECK(MpChangeCar(&choice, -1, 0, automatic));
    CHECK(choice.car == 31 && choice.manual == 0);
    CHECK(MpChangeCar(&choice, 1, 1, automatic));
    CHECK(choice.car == 0 && choice.manual == 1);
    CHECK(MpChangeCar(&choice, 0, 1, automatic));
    CHECK(choice.manual == 0);
    CHECK(MpChangeCar(&choice, 1, 1, automatic));
    CHECK(choice.car == 1 && choice.manual == 1); /* MT-only, even with toggle. */
    CHECK(MpChangeCar(&choice, 0, 1, automatic));
    CHECK(choice.manual == 1);
    uint32_t visited = 0;
    for (int i = 0; i < 32; ++i) {
        CHECK(MpChangeCar(&choice, 1, 0, automatic));
        CHECK(choice.car >= 0 && choice.car < 32 && choice.port == 7243);
        CHECK(!(visited & (UINT32_C(1) << choice.car)));
        visited |= UINT32_C(1) << choice.car;
    }
    CHECK(visited == UINT32_MAX && choice.car == 1);
    const MpSettings before = choice;
    CHECK(!MpChangeCar(&choice, 2, 0, automatic));
    CHECK(!MpChangeCar(&choice, 0, 2, automatic));
    CHECK(memcmp(&choice, &before, sizeof(choice)) == 0);
    CHECK(!MpChangeCar(NULL, 0, 0, automatic));
    return 0;
}

int main(void) {
    uint8_t lobbyWire[53] = {MP_S2C_LOBBY, MP_PROTOCOL_VERSION, 42};
    lobbyWire[12] = 3; lobbyWire[14] = 2; lobbyWire[15] = 2;
    lobbyWire[35] = 31; lobbyWire[36] = 1; lobbyWire[37] = 15;
    memcpy(lobbyWire + 38, "123456789012345", 15);
    MpLobby lobby;
    CHECK(MpDecodeLobby(lobbyWire, sizeof(lobbyWire), &lobby));
    CHECK(lobby.room.code == 42 && lobby.seats[1].variant == 31 && lobby.seats[1].manual == 1);
    CHECK(strcmp(lobby.seats[1].name, "123456789012345") == 0);
    const MpLobby savedLobby = lobby;
    for (size_t size = 0; size < sizeof(lobbyWire); ++size) {
        CHECK(!MpDecodeLobby(lobbyWire, size, &lobby));
        CHECK(memcmp(&lobby, &savedLobby, sizeof(lobby)) == 0);
    }
    const struct { int offset, value; } badLobby[] = {{0, 0}, {1, MP_PROTOCOL_VERSION - 1},
        {14, 0}, {15, 1}, {35, 32}, {36, 2}, {37, 16}, {17, 1}, {20, 1}};
    for (size_t i = 0; i < sizeof(badLobby) / sizeof(badLobby[0]); ++i) {
        uint8_t before = lobbyWire[badLobby[i].offset];
        lobbyWire[badLobby[i].offset] = badLobby[i].value;
        CHECK(!MpDecodeLobby(lobbyWire, sizeof(lobbyWire), &lobby));
        CHECK(memcmp(&lobby, &savedLobby, sizeof(lobby)) == 0);
        lobbyWire[badLobby[i].offset] = before;
    }
    CHECK(!MpDecodeLobby(NULL, 53, &lobby) && !MpDecodeLobby(lobbyWire, 53, NULL));
    uint8_t directory[18] = {MP_S2C_LIST, MP_PROTOCOL_VERSION, 1,
        8, 7, 6, 5, 4, 3, 2, 1, 5, 3, 6, 1, 3, 2, 1};
    MpRoomInfo rooms[MP_ROOM_LIMIT];
    size_t count = 99;
    memset(rooms, 0xA5, sizeof(rooms));
    MpRoomInfo original[MP_ROOM_LIMIT];
    memcpy(original, rooms, sizeof(rooms));
    for (size_t size = 0; size < sizeof(directory); ++size) {
        CHECK(!MpDecodeRoomList(directory, size, rooms, &count));
        CHECK(count == 99 && memcmp(rooms, original, sizeof(rooms)) == 0);
    }
    CHECK(MpDecodeRoomList(directory, sizeof(directory), rooms, &count));
    CHECK(count == 1 && rooms[0].code == UINT64_C(0x0102030405060708));
    CHECK(rooms[0].options.classIndex == 5 && rooms[0].options.course == 3 &&
          rooms[0].options.laps == 6 && rooms[0].options.reverse == 1);
    CHECK(rooms[0].occupied == 3 && rooms[0].ready == 2 && rooms[0].state == 1);
    memcpy(original, rooms, sizeof(rooms));
    directory[15] = 1; /* Ready seat must be present. */
    CHECK(!MpDecodeRoomList(directory, sizeof(directory), rooms, &count));
    CHECK(count == 1 && memcmp(rooms, original, sizeof(rooms)) == 0);
    directory[15] = 3;
    uint8_t duplicate[33];
    memcpy(duplicate, directory, 18);
    duplicate[2] = 2;
    memcpy(duplicate + 18, directory + 3, 15);
    CHECK(!MpDecodeRoomList(duplicate, sizeof(duplicate), rooms, &count));
    const uint8_t emptyList[] = {MP_S2C_LIST, MP_PROTOCOL_VERSION, 0};
    CHECK(MpDecodeRoomList(emptyList, sizeof(emptyList), rooms, &count) && count == 0);
    CHECK(!MpDecodeRoomList(NULL, 3, rooms, &count));
    CHECK(!MpDecodeRoomList(emptyList, 3, NULL, &count));
    CHECK(!MpDecodeRoomList(emptyList, 3, rooms, NULL));
    uint8_t welcome[11] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION, 1, 8, 7, 6, 5, 4, 3, 2, 1};
    int seat = -1;
    uint64_t assigned = 0;
    CHECK(MpDecodeWelcome(welcome, &seat, &assigned));
    CHECK(seat == 1 && assigned == UINT64_C(0x0102030405060708));
    welcome[10] = 128;
    CHECK(!MpDecodeWelcome(welcome, &seat, &assigned));
    CHECK(seat == 1 && assigned == UINT64_C(0x0102030405060708));
    memset(welcome + 3, 0, 8);
    CHECK(!MpDecodeWelcome(welcome, &seat, &assigned));
    CHECK(!MpDecodeWelcome(NULL, &seat, &assigned));
    uint64_t room = 42;
    CHECK(MpParseRoom(NULL, &room) && room == UINT64_MAX);
    CHECK(MpParseRoom("auto", &room) && room == UINT64_MAX);
    CHECK(MpParseRoom("create", &room) && room == 0);
    CHECK(MpParseRoom("0", &room) && room == 0);
    CHECK(MpParseRoom("12345", &room) && room == 12345);
    CHECK(MpParseRoom("9223372036854775807", &room) && room == INT64_MAX);
    const char *invalidRooms[] = {"", "-1", "+1", " 1", "1 ", "0x10",
        "9223372036854775808", "18446744073709551615", "999999999999999999999"};
    for (size_t i = 0; i < sizeof(invalidRooms) / sizeof(invalidRooms[0]); ++i) {
        CHECK(!MpParseRoom(invalidRooms[i], &room) && room == INT64_MAX);
    }
    CHECK(!MpParseRoom("1", NULL));
    MpRaceOptions options = {.laps = 1};
    CHECK(MpValidRaceOptions(&options));
    for (int field = 0; field < 4; ++field) CHECK(MpChangeRace(&options, field, -1));
    CHECK(options.classIndex == 5 && options.course == 3 && options.laps == 6 && options.reverse == 1);
    for (int field = 0; field < 4; ++field) CHECK(MpChangeRace(&options, field, 1));
    CHECK(options.classIndex == 0 && options.course == 0 && options.laps == 1 && options.reverse == 0);
    const MpRaceOptions unchangedOptions = options;
    CHECK(!MpChangeRace(&options, 4, 1) && !MpChangeRace(&options, 0, 0));
    CHECK(memcmp(&options, &unchangedOptions, sizeof(options)) == 0);
    CHECK(!MpValidRaceOptions(NULL) && !MpChangeRace(NULL, 0, 1));
    options.laps = 0;
    CHECK(!MpValidRaceOptions(&options) && !MpChangeRace(&options, 0, 1));
    CHECK(TestFullField() == 0);
    CHECK(TestCorrection() == 0);
    CHECK(TestPredictionClock() == 0);
    CHECK(TestReplayInput() == 0);
    CHECK(TestCarPicker() == 0);
    CHECK(TestLapWire() == 0);
    CHECK(TestCarPresentation() == 0);
    MpSettings settings = {0};
    CHECK(MpParseSettings(NULL, NULL, NULL, &settings));
    CHECK(settings.port == 7243 && settings.car == 0 && settings.manual == 0);
    CHECK(MpParseSettings("65535", "31", "1", &settings));
    CHECK(settings.port == 65535 && settings.car == 31 && settings.manual == 1);
    MpSettings unchanged = settings;
    const char *invalid[][3] = {
        {"0", "31", "1"}, {"65536", "31", "1"},
        {"7243", "-1", "1"}, {"7243", "32", "1"},
        {"7243", "31", "2"}, {"7243", "31", "-1"},
        {"", NULL, NULL}, {NULL, "", NULL}, {NULL, NULL, ""},
        {"7243", "31tail", "1"}, {"7243", "31", "true"},
        {"999999999999999999999999", "0", "0"},
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        CHECK(!MpParseSettings(invalid[i][0], invalid[i][1], invalid[i][2], &settings));
        CHECK(memcmp(&settings, &unchanged, sizeof(settings)) == 0);
    }
    CHECK(!MpParseSettings(NULL, NULL, NULL, NULL));
    CHECK(TestHistory() == 0);
    const uint64_t origin = UINT64_C(123456789);
    CHECK(!MpSnapshotExpired(origin + UINT64_C(5000000000) - 1, origin, 1));
    CHECK(MpSnapshotExpired(origin + UINT64_C(5000000000), origin, 1));
    CHECK(!MpSnapshotExpired(origin + UINT64_C(65000000000) - 1, origin, 0));
    CHECK(MpSnapshotExpired(origin + UINT64_C(65000000000), origin, 0));
    CHECK(!MpSnapshotExpired(origin - 1, origin, 1));
    CHECK(!MpSnapshotExpired(UINT64_MAX, UINT64_MAX - 1, 1));
    CHECK(TestPoseBlend() == 0);
    MpCarPose slow = {.status = MP_DRIVING, .speed = 100};
    MpCarPose fast = {.status = MP_DRIVING, .speed = 300}, halfway;
    CHECK(MpBlendPose(&slow, &fast, 32768, &halfway) && halfway.speed == 200);
    CHECK(TestResult() == 0);
    CHECK(TestFinalSnapshot() == 0);
    CHECK(TestCommands() == 0);
    CHECK(TestRaceClock() == 0);
    CHECK(TestCountdown() == 0);
    CHECK(TestSnapshotApplication() == 0);
    CHECK(TestStart() == 0);
    CHECK(TestChoice() == 0);
    /* MpEncodeInput must exactly match what server/src/main.rs's client
     * reader decodes (see its C2S_INPUT handling): 1 type byte, then
     * mode/left/right, angle/throttle/brake as little-endian i16, then the
     * two shift flags. */
    DriverInput input;
    input.steering.mode = STEERING_ANALOG;
    input.steering.left = 0;
    input.steering.right = 1;
    input.steering.angle = -1234;
    input.throttle = 200;
    input.brake = 5;
    input.shiftUp = 1;
    input.shiftDown = 0;

    uint8_t wire[MP_INPUT_WIRE_SIZE];
    memset(wire, 0xA5, sizeof(wire));
    MpEncodeInput(&input, wire);
    uint8_t command[MP_COMMAND_WIRE_SIZE];
    memset(command, 0xA5, sizeof(command));
    MpEncodeCommand(&input, UINT32_C(0x01020304), command);
    const uint8_t commandHeader[] = {MP_C2S_COMMAND, 4, 3, 2, 1};
    CHECK(memcmp(command, commandHeader, sizeof(commandHeader)) == 0);
    CHECK(memcmp(command + 5, wire + 1, 12) == 0);
    const uint8_t expected[MP_INPUT_WIRE_SIZE] = {
        MP_C2S_INPUT, STEERING_ANALOG, 0, 1,
        (uint8_t)(-1234), (uint8_t)((uint16_t)-1234 >> 8),
        200, 0,
        5, 0,
        1, 0
    };
    CHECK(memcmp(wire, expected, sizeof(wire)) == 0);

    /* A known little-endian position plus the Rust presentation-pose fixture. */
    uint8_t snapshot[MP_SNAPSHOT_BODY_SIZE] = {0};
    snapshot[0] = 1; /* tick = 1 (LE u32) */
    snapshot[8] = 1; /* phase = SIM_COUNTDOWN */
    size_t seat0 = MP_SNAPSHOT_HEADER_SIZE;
    snapshot[seat0] = 1; /* active */
    snapshot[seat0 + 1] = 0x1D; snapshot[seat0 + 2] = 0xB7; /* x = 46877 (LE) */
    size_t seat1 = seat0 + MP_SNAPSHOT_SEAT_SIZE;
    snapshot[seat1] = 0; /* inactive */

    /* Match the Rust presentation-pose byte fixture. */
    memset(snapshot + seat0 + 17, 0xFF, 4);
    snapshot[seat0 + 21] = 1;
    memset(snapshot + seat0 + 25, 0xFF, 4);
    snapshot[seat0 + 25] = 0xB0;
    snapshot[seat0 + 30] = 16;
    snapshot[seat0 + 34] = 1;
    snapshot[seat0 + 37] = 99;
    snapshot[seat0 + 41] = 0x40; snapshot[seat0 + 42] = 0x1F;
    snapshot[seat0 + 46] = 1; snapshot[seat0 + 49] = 2; snapshot[seat0 + 53] = 3;
    const uint8_t contact[] = {0xA8, 0xFD, 0xFF, 0xFF, 0xF9, 0xFF, 0xFF, 0xFF};
    memcpy(snapshot + seat0 + 57, contact, sizeof(contact));
    const uint8_t speed[] = {0x2E, 0xFB, 0xFF, 0xFF}; /* -1234, matching Rust fixture. */
    memcpy(snapshot + seat0 + 65, speed, sizeof(speed));
    MpSnapshot decoded;
    memset(&decoded, 0xAA, sizeof(decoded));
    CHECK(MpDecodeSnapshot(snapshot, sizeof(snapshot), &decoded));
    CHECK(decoded.tick == 1);
    CHECK(decoded.phase == 1);
    CHECK(decoded.seats[0].status == 1);
    CHECK(decoded.seats[0].x == 46877);
    CHECK(decoded.seats[0].y == 0);
    CHECK(decoded.seats[0].pitch == -1 && decoded.seats[0].roll == 1);
    CHECK(decoded.seats[0].steering == -80 && decoded.seats[0].wheels == 4096);
    CHECK(decoded.seats[0].brake == 256 && decoded.seats[0].progress == 99);
    CHECK(decoded.seats[0].rpm == 8000 && decoded.seats[0].throttle == 256);
    CHECK(decoded.seats[0].clutch == 2 && decoded.seats[0].gear == 3);
    CHECK(decoded.seats[0].ground == -600 && decoded.seats[0].rollSpeed == -7);
    CHECK(decoded.seats[0].speed == -1234);
    CHECK(decoded.seats[1].status == 0);

    /* A short buffer is rejected without touching *out. */
    MpSnapshot untouched = decoded;
    CHECK(!MpDecodeSnapshot(snapshot, sizeof(snapshot) - 1, &decoded));
    CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    CHECK(!MpDecodeSnapshot(NULL, sizeof(snapshot), &decoded));
    CHECK(!MpDecodeSnapshot(snapshot, sizeof(snapshot), NULL));

    /* Every incomplete body and extra data must preserve the destination. */
    for (size_t size = 0; size < sizeof(snapshot); ++size) {
        CHECK(!MpDecodeSnapshot(snapshot, size, &decoded));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    }
    uint8_t oversized[sizeof(snapshot) + 1];
    memcpy(oversized, snapshot, sizeof(snapshot));
    oversized[sizeof(snapshot)] = 0;
    CHECK(!MpDecodeSnapshot(oversized, sizeof(oversized), &decoded));
    CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);

    for (unsigned phase = 4; phase <= 255; ++phase) {
        snapshot[8] = (uint8_t)phase;
        CHECK(!MpDecodeSnapshot(snapshot, sizeof(snapshot), &decoded));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    }
    snapshot[8] = 1;
    for (size_t seat = seat0; seat <= seat1; seat += MP_SNAPSHOT_SEAT_SIZE) {
        uint8_t original = snapshot[seat];
        for (unsigned flag = 3; flag <= 255; ++flag) {
            snapshot[seat] = (uint8_t)flag;
            CHECK(!MpDecodeSnapshot(snapshot, sizeof(snapshot), &decoded));
            CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
        }
        snapshot[seat] = original;
    }
    for (unsigned phase = 0; phase <= 3; ++phase) {
        snapshot[8] = (uint8_t)phase;
        snapshot[seat0] = phase == SIM_FINISHED ? MP_FINISHED : MP_DRIVING;
        CHECK(MpDecodeSnapshot(snapshot, sizeof(snapshot), &decoded));
        CHECK(decoded.phase == phase);
    }

    untouched = decoded;
    snapshot[seat1 + 34] = 2; /* Late-seat brake 512 must reject atomically. */
    CHECK(!MpDecodeSnapshot(snapshot, sizeof(snapshot), &decoded));
    CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    snapshot[seat1 + 34] = 0;
const struct { size_t offset; uint8_t byte; } badDrive[] = {{46, 2}, {50, 128}, {53, 7}};
for (size_t i = 0; i < sizeof(badDrive) / sizeof(badDrive[0]); ++i) {
    snapshot[seat1 + badDrive[i].offset] = badDrive[i].byte;
    CHECK(!MpDecodeSnapshot(snapshot, sizeof(snapshot), &decoded));
    CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    snapshot[seat1 + badDrive[i].offset] = 0;
}

    MpCarConfig config = {.variant = {1, 9}}, decodedConfig;
    config.specs[0].torqueCurve[0] = 0x12345678;
    config.specs[1].shiftPoints[5].upshiftSpeed = 2345;
    uint8_t configWire[MP_CONFIG_WIRE_SIZE];
    CHECK(MpEncodeConfig(&config, configWire, sizeof(configWire)));
    CHECK(configWire[0] == 0x88 && configWire[1] == 1 && configWire[2] == 1);
    CHECK(configWire[3] == 0x78 && configWire[2 + 1 + CAR_SPEC_WIRE_SIZE] == 9);
    CHECK(MpDecodeConfig(configWire, sizeof(configWire), &decodedConfig));
    CHECK(decodedConfig.specs[0].torqueCurve[0] == 0x12345678);
    CHECK(decodedConfig.specs[1].shiftPoints[5].upshiftSpeed == 2345);
    MpCarConfig preserved = decodedConfig;
    for (size_t size = 0; size < sizeof(configWire); ++size) {
        CHECK(!MpDecodeConfig(configWire, size, &decodedConfig));
        CHECK(memcmp(&decodedConfig, &preserved, sizeof(preserved)) == 0);
    }
    configWire[2 + 1 + CAR_SPEC_WIRE_SIZE] = CAR_MODEL_VARIANT_COUNT;
    CHECK(!MpDecodeConfig(configWire, sizeof(configWire), &decodedConfig));
    CHECK(memcmp(&decodedConfig, &preserved, sizeof(preserved)) == 0);
    memset(configWire, 0xA5, sizeof(configWire));
    config.variant[1] = CAR_MODEL_VARIANT_COUNT;
    CHECK(!MpEncodeConfig(&config, configWire, sizeof(configWire)));
    for (size_t i = 0; i < sizeof(configWire); ++i) CHECK(configWire[i] == 0xA5);
    uint8_t availability[MP_AVAILABILITY_WIRE_SIZE];
    uint32_t allowed = 0;
    CHECK(MpEncodeAvailability(UINT32_C(0x80000001), availability, sizeof(availability)));
    CHECK(memcmp(availability, (uint8_t[]){0x89, 1, 1, 0, 0, 0x80}, sizeof(availability)) == 0);
    CHECK(MpDecodeAvailability(availability, sizeof(availability), &allowed));
    CHECK(allowed == UINT32_C(0x80000001));
    for (size_t size = 0; size < sizeof(availability); ++size) {
        CHECK(!MpDecodeAvailability(availability, size, &allowed));
        CHECK(allowed == UINT32_C(0x80000001));
    }
    availability[1] = 2;
    CHECK(!MpDecodeAvailability(availability, sizeof(availability), &allowed));
    CHECK(allowed == UINT32_C(0x80000001));
    CHECK(!MpDecodeAvailability(NULL, sizeof(availability), &allowed));
    CHECK(!MpDecodeAvailability(availability, sizeof(availability), NULL));
    CHECK(!MpEncodeAvailability(0, NULL, sizeof(availability)));
    puts("mp_protocol: pure packet, setup and presentation operations pass");
    return 0;
}
