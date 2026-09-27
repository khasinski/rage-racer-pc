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

static int TestStart(void) {
    uint8_t body[MP_START_BODY_SIZE + 1] = {
        9, 0, 0, 3, 0, 150, 0, 0, 0, 2,
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
        {0, 0}, {0, 1}, {1, 4}, {2, 6}, {3, 0},
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

static int TestResult(void) {
    uint8_t body[MP_RESULT_BODY_SIZE + 1] = {1, 1, 0xE8, 3, 0, 0, 0, 0, 255, 255, 255, 255};
    MpResult result;
    CHECK(MpDecodeResult(body, MP_RESULT_BODY_SIZE, &result));
    CHECK(result.seats[0].finished == 1 && result.seats[0].place == 1 && result.seats[0].milliseconds == 1000);
    CHECK(result.seats[1].finished == 0 && result.seats[1].place == 0 && result.seats[1].milliseconds == -1);
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

int main(void) {
    CHECK(TestPoseBlend() == 0);
    CHECK(TestResult() == 0);
    CHECK(TestSnapshotApplication() == 0);
    CHECK(TestStart() == 0);
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
    const uint8_t expected[MP_INPUT_WIRE_SIZE] = {
        MP_C2S_INPUT, STEERING_ANALOG, 0, 1,
        (uint8_t)(-1234), (uint8_t)((uint16_t)-1234 >> 8),
        200, 0,
        5, 0,
        1, 0
    };
    CHECK(memcmp(wire, expected, sizeof(wire)) == 0);

    /* A known little-endian position plus the Rust presentation-pose fixture. */
    uint8_t snapshot[MP_SNAPSHOT_HEADER_SIZE + MP_SEAT_LIMIT * MP_SNAPSHOT_SEAT_SIZE] = {0};
    snapshot[0] = 1; /* tick = 1 (LE u32) */
    snapshot[4] = 1; /* phase = SIM_COUNTDOWN */
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
        snapshot[4] = (uint8_t)phase;
        CHECK(!MpDecodeSnapshot(snapshot, sizeof(snapshot), &decoded));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    }
    snapshot[4] = 1;
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
        snapshot[4] = (uint8_t)phase;
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

    /* A connection nobody is listening on fails cleanly rather than hanging
     * or crashing; this is the one behavior narrow-testable without a real
     * running server. */
    CHECK(MpClientConnect(NULL, 7878) == NULL);
    CHECK(MpClientConnect("127.0.0.1", 0) == NULL);
    /* Invalid numeric addresses also exercise socket/startup cleanup. */
    for (int attempt = 0; attempt < 3; ++attempt) {
        CHECK(MpClientConnect("999.1.1.1", 7878) == NULL);
        CHECK(MpClientConnect("", 7878) == NULL);
    }
    MpClient *client = MpClientConnect("127.0.0.1", 1);
    CHECK(client == NULL);
    MpClientClose(NULL);

    puts("mp_protocol: encode/decode match the server wire format, connect failure is clean");
    return 0;
}
