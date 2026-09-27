#include "port/mp_client.h"
#include "port/mp_race_client.h"
#include "game/race.h"
#include "game/race_data.h"
#include "game/state.h"
#include "game/screens.h"
#include "game/race_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(test) do { if (!(test)) { fprintf(stderr, "line %d: %s\n", __LINE__, #test); exit(1); } } while (0)

u16 g_PadPressed, g_PadPressedRepeat, g_PadHeld;
const char *g_NativeCarNames[GAME_CAR_COUNT] = {[0] = "ERRISO", [12] = "SQUALDON"};
typedef struct Action { u16 pressed, repeat; } Action;
static const Action *actions;
static unsigned frames, actionCount, specReads, timeFormats;
static int badSpec;
static char text[8][4][64];
static int readyValues[8], readyCalls, startCalls, startResult = 1, sendResult = 1;
static MpRoomInfo directory[2];
static size_t directoryCount;
static int listCalls, listResult;
static int hasLobby;
static MpLobby visibleLobby;
static int pickCalls, pickValues[8][2];
static int startVariant, startManual;
int MpClientPollPick(MpClient *client, int variant, int manual) {
    CHECK(client && pickCalls < 8);
    pickValues[pickCalls][0] = variant; pickValues[pickCalls++][1] = manual;
    return sendResult;
}
int MpClientPollList(MpClient *client, MpRoomInfo rooms[MP_ROOM_LIMIT], size_t *count) {
    CHECK(client && rooms && count);
    listCalls++;
    if (listResult == 1) {
        memcpy(rooms, directory, directoryCount * sizeof(*rooms));
        *count = directoryCount;
    }
    return listResult;
}

uint64_t MpClientRoom(const MpClient *client) { return client ? 42 : 0; }
const MpLobby *MpClientLobby(const MpClient *client) { (void)client; return hasLobby ? &visibleLobby : NULL; }
int MpClientPollReady(MpClient *client, int ready) {
    CHECK(client && readyCalls < 8);
    readyValues[readyCalls++] = ready;
    return sendResult;
}
int MpClientPollStart(MpClient *client, MpStart *start) {
    CHECK(client && start);
    startCalls++;
    if (frames < actionCount) return 3;
    if (startResult == 1) {
        start->laps = 2;
        for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
            start->seats[seat] = (MpSeat){.model = startVariant, .manual = startManual};
    }
    return startResult;
}

void DrawHostMenuFrame(const char *title, const char *choice, const char *controls, const char *status) {
    CHECK(frames < actionCount && frames < 8);
    const char *lines[] = {title, choice, controls, status};
    for (unsigned i = 0; i < 4; ++i) snprintf(text[frames][i], 64, "%s", lines[i]);
    g_PadPressed = actions[frames].pressed;
    g_PadPressedRepeat = actions[frames].repeat;
    frames++;
}

int ReadRaceCar(const RaceData *archive, s32 variant, GameCarSpec *spec) {
    CHECK(archive && variant >= 0 && variant < CAR_MODEL_VARIANT_COUNT);
    specReads++;
    if (variant == badSpec) return 0;
    memset(spec, 0, sizeof(*spec));
    spec->topGear = variant == 31 ? 6 : 5;
    return 1;
}

void FormatLapTime(char dst[LAP_TIME_TEXT_CAPACITY], s32 milliseconds) {
    CHECK(milliseconds >= 0);
    timeFormats++;
    snprintf(dst, LAP_TIME_TEXT_CAPACITY, "TIME%d", milliseconds);
}

static void Script(const Action *script, unsigned count) {
    actions = script;
    actionCount = count;
    frames = specReads = timeFormats = 0;
    badSpec = -1;
    memset(text, 0, sizeof(text));
    readyCalls = startCalls = 0;
    startResult = sendResult = 1;
    listCalls = 0;
    listResult = 1;
    hasLobby = 0;
    pickCalls = 0;
    startVariant = startManual = 0;
    directoryCount = 2;
    directory[0] = (MpRoomInfo){.code = 7, .options = {0, 0, 3, 0}, .occupied = 3, .state = 2};
    directory[1] = (MpRoomInfo){.code = 8, .options = {5, 3, 6, 1}, .occupied = 1};
}

int main(void) {
    MpClient *client = (MpClient *)&frames; /* Opaque token for transport fixture. */
    MpStart start = {.laps = 5};
    MpSettings settings = {7243, 0, 0};
    RaceData archive = {0};
    uint64_t chosenRoom = 42;
    const Action browseActions[] = {{0, 0}, {PAD_CONFIRM, 0}, {0, PAD_DOWN}, {PAD_CONFIRM, 0}};
    Script(browseActions, 4);
    CHECK(MpBrowseRooms(client, &chosenRoom) == 1 && chosenRoom == 8 && listCalls == 1);
    CHECK(strcmp(text[1][1], "1/2 ROOM 7 / RACING") == 0);
    CHECK(strcmp(text[3][2], "CLASS 6 COURSE 4 REV / 6 LAPS") == 0);
    const Action cancelBrowse[] = {{PAD_CANCEL, 0}};
    Script(cancelBrowse, 1);
    CHECK(MpBrowseRooms(client, &chosenRoom) == -1 && chosenRoom == 8 && listCalls == 0);
    Script(cancelBrowse, 1);
    CHECK(MpBrowseRooms(NULL, &chosenRoom) == 0 && MpBrowseRooms(client, NULL) == 0);
    const Action emptyBrowse[] = {{0, 0}, {PAD_CONFIRM, 0}, {PAD_CANCEL, 0}};
    Script(emptyBrowse, 3);
    directoryCount = 0;
    CHECK(MpBrowseRooms(client, &chosenRoom) == -1 && chosenRoom == 8);
    CHECK(strcmp(text[1][1], "NO ROOMS") == 0);
    const Action refreshBrowse[] = {{0, 0}, {0, PAD_DOWN}, {PAD_RIGHT, 0}, {0, 0}, {PAD_CONFIRM, 0}};
    Script(refreshBrowse, 5);
    CHECK(MpBrowseRooms(client, &chosenRoom) == 1 && chosenRoom == 8 && listCalls == 2);
    Script(emptyBrowse, 3);
    listResult = 0;
    CHECK(MpBrowseRooms(client, &chosenRoom) == 0 && frames == 1 && chosenRoom == 8);
    const Action readyActions[] = {{PAD_CONFIRM, 0}, {PAD_CONFIRM, 0}, {0, 0},
        {0, 0}, {PAD_CONFIRM, 0}};
    Script(readyActions, 5);
    CHECK(MpWaitRoom(client, 1, &start, &settings, UINT32_C(0x80000001)) == 1 && start.laps == 2);
    CHECK(readyCalls == 2 && readyValues[0] == 1 && readyValues[1] == 0);
    CHECK(strcmp(text[0][1], "GUEST 42 / CAR 1 AT") == 0);
    CHECK(strcmp(text[1][2], "READY / WAITING FOR OTHER PLAYER") == 0);
    CHECK(strcmp(text[3][2], "NOT READY") == 0);
    const Action cancelWaiting[] = {{0, 0}, {PAD_CANCEL, 0}};
    Script(cancelWaiting, 2);
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == -1 && startCalls == 1);
    CHECK(strcmp(text[0][1], "HOST 42 / CAR 1 AT") == 0);
    const Action blockedReady[] = {{PAD_CONFIRM, 0}, {PAD_CONFIRM, 0}, {PAD_CANCEL, 0}};
    Script(blockedReady, 3);
    sendResult = 3;
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == -1);
    CHECK(readyCalls == 2 && readyValues[0] == 1 && readyValues[1] == 1 && startCalls == 1);
    Script(readyActions, 5);
    sendResult = 0;
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 0 && frames == 1);
    const Action disconnected[] = {{0, 0}, {PAD_CONFIRM, 0}};
    Script(disconnected, 2);
    startResult = 0;
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 0 && readyCalls == 0);
    CHECK(MpWaitRoom(NULL, 0, &start, &settings, UINT32_C(0x80000001)) == 0);
    CHECK(MpWaitRoom(client, 2, &start, &settings, UINT32_C(0x80000001)) == 0);
    CHECK(MpWaitRoom(client, 0, NULL, &settings, UINT32_C(0x80000001)) == 0);
    Script(disconnected, 2);
    hasLobby = 1;
    visibleLobby = (MpLobby){.room = {.code = 42, .occupied = 3, .ready = 2}};
    visibleLobby.seats[1] = (MpLobbySeat){.variant = 31, .manual = 1, .length = 5};
    memcpy(visibleLobby.seats[1].name, "Guest", 5);
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 1);
    CHECK(strcmp(text[0][2], "Guest CAR 32 MT / READY") == 0);
    const Action changeCar[] = {{0, PAD_LEFT}, {PAD_UP, 0}, {PAD_CONFIRM, 0}, {0, 0}};
    Script(changeCar, 4);
    startVariant = 31; startManual = 1;
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 1);
    CHECK(pickCalls == 2 && pickValues[0][0] == 31 && pickValues[0][1] == 0 &&
          pickValues[1][0] == 31 && pickValues[1][1] == 1);
    CHECK(settings.car == 31 && settings.manual == 1 && readyCalls == 1);
    CHECK(strcmp(text[2][1], "HOST 42 / CAR 32 MT") == 0);
    settings.car = settings.manual = 0;
    Script(changeCar, 4); /* Earlier valid choice won the server freeze. */
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 1);
    CHECK(settings.car == 0 && settings.manual == 0);
    Script(changeCar, 4);
    startVariant = 7; /* Never requested: preserve rejection of changed choices. */
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 0);
    settings.car = settings.manual = 0;
    const Action lockedCar[] = {{PAD_CONFIRM, 0}, {PAD_UP, PAD_LEFT}, {0, 0}};
    Script(lockedCar, 3);
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 1 && pickCalls == 0);
    Script(disconnected, 2);
    settings.car = 1; settings.manual = 0; /* Retail MT-only metadata. */
    CHECK(MpWaitRoom(client, 0, &start, &settings, UINT32_C(0x80000001)) == 0 && frames == 0);
    settings.car = 0;
    uint64_t room = UINT64_MAX;
    const Action createRoom[] = {{0, PAD_DOWN}, {PAD_CONFIRM, 0}};
    Script(createRoom, 2);
    CHECK(MpSelectRoom(&room) == 1 && room == 0);
    CHECK(strcmp(text[1][1], "CREATE ROOM") == 0);
    room = UINT64_MAX;
    const Action joinRoom[] = {{0, PAD_DOWN}, {0, PAD_DOWN}, {PAD_CONFIRM, 0}, {0, PAD_UP}, {PAD_CONFIRM, 0}};
    Script(joinRoom, 5);
    CHECK(MpSelectRoom(&room) == 1 && room == 2);
    CHECK(strstr(text[4][1], "0000000000000000002"));
    const Action cancelRoom[] = {{PAD_CONFIRM, 0}, {0, PAD_UP}, {PAD_CANCEL, 0}, {PAD_CANCEL, 0}};
    Script(cancelRoom, 4);
    CHECK(MpSelectRoom(&room) == -1 && room == 2);
    CHECK(MpSelectRoom(NULL) == 0);
    room = (uint64_t)INT64_MAX + 1;
    CHECK(MpSelectRoom(&room) == 0);
    room = 1;
    const Action zeroRoom[] = {{PAD_CONFIRM, 0}, {0, PAD_DOWN}, {PAD_CONFIRM, 0},
        {0, PAD_UP}, {PAD_CONFIRM, 0}};
    Script(zeroRoom, 5);
    CHECK(MpSelectRoom(&room) == 1 && room == 1);
    CHECK(strstr(text[3][3], "INVALID CODE"));
    room = INT64_MAX;
    const Action overflowRoom[] = {{PAD_CONFIRM, 0}, {0, PAD_UP}, {PAD_CONFIRM, 0},
        {0, PAD_DOWN}, {PAD_CONFIRM, 0}};
    Script(overflowRoom, 5);
    CHECK(MpSelectRoom(&room) == 1 && room == INT64_MAX);
    CHECK(strstr(text[3][3], "INVALID CODE"));
    room = 7;
    const Action autoRoom[] = {{0, PAD_DOWN}, {0, PAD_DOWN}, {PAD_CONFIRM, 0}};
    Script(autoRoom, 3);
    CHECK(MpSelectRoom(&room) == 1 && room == UINT64_MAX);
    const Action browseRoom[] = {{0, PAD_UP}, {PAD_CONFIRM, 0}};
    Script(browseRoom, 2);
    CHECK(MpSelectRoom(&room) == 1 && room == MP_BROWSE_ROOM);
    room = 1;
    const Action cursorRoom[] = {{PAD_CONFIRM, 0}, {0, PAD_RIGHT}, {0, PAD_DOWN},
        {0, PAD_LEFT}, {PAD_CONFIRM, 0}};
    Script(cursorRoom, 5);
    CHECK(MpSelectRoom(&room) == 1 && room == UINT64_C(9000000000000000001));
    CHECK(strstr(text[2][1], "DIGIT 1"));
    const Action raceActions[] = {{0, PAD_LEFT}, {0, PAD_DOWN}, {0, PAD_LEFT},
        {0, PAD_DOWN}, {0, PAD_LEFT}, {0, PAD_DOWN}, {0, PAD_RIGHT}, {PAD_CONFIRM, 0}};
    MpRaceOptions options = {.laps = 3};
    Script(raceActions, 8);
    CHECK(MpSelectRace(&options) == 1);
    CHECK(options.classIndex == 5 && options.course == 3 && options.laps == 2 && options.reverse == 1);
    CHECK(strcmp(text[7][1], "CLASS 6 COURSE 4 LAPS 2 REVERSE") == 0);
    const MpRaceOptions before = options;
    const Action cancelRace[] = {{0, PAD_RIGHT}, {PAD_CANCEL, 0}};
    Script(cancelRace, 2);
    CHECK(MpSelectRace(&options) == -1 && memcmp(&options, &before, sizeof(options)) == 0);
    CHECK(MpSelectRace(NULL) == 0);
    /* Screens borrow specification readers; transmission policy is injected. */
    const Action choose[] = {{0, PAD_LEFT}, {PAD_UP, 0}, {0, PAD_RIGHT}, {PAD_DOWN, 0}, {PAD_CONFIRM, 0}};
    Script(choose, 5);
    MpSettings choice = {7243, 0, 0};
    CHECK(MpSelectCar(&choice, &archive, UINT32_C(0x80000001)) == 1);
    CHECK(choice.car == 0 && choice.manual == 0 && choice.port == 7243);
    CHECK(frames == 5 && specReads == 3);
    CHECK(strcmp(text[0][1], "01/32 ERRISO G1 AT") == 0);
    CHECK(strcmp(text[1][1], "32/32 SQUALDON G1 AT") == 0);
    CHECK(strcmp(text[2][1], "32/32 SQUALDON G1 MT") == 0);
    CHECK(strcmp(text[1][0], "MULTIPLAYER / 6 GEARS") == 0);

    const Action confirm[] = {{PAD_CONFIRM, 0}};
    Script(confirm, 1);
    choice.car = 1;
    CHECK(MpSelectCar(&choice, &archive, UINT32_C(0x80000001)) == 1);
    CHECK(choice.manual == 1);
    CHECK(strcmp(text[0][1], "02/32 ERRISO G2 MT") == 0);
    Script(confirm, 1);
    choice.manual = 0;
    CHECK(MpSelectCar(&choice, &archive, UINT32_C(1) << 1) == 1);
    CHECK(choice.car == 1 && choice.manual == 0); /* Server enables AT. */
    Script(confirm, 1);
    choice.car = 0;
    CHECK(MpSelectCar(&choice, &archive, 0) == 1 && choice.manual == 1);

    const Action cancel[] = {{PAD_CANCEL, 0}};
    Script(cancel, 1);
    CHECK(MpSelectCar(&choice, &archive, UINT32_C(0x80000001)) == -1 && frames == 1);
    Script(confirm, 1);
    badSpec = choice.car;
    CHECK(MpSelectCar(&choice, &archive, UINT32_C(0x80000001)) == 0 && frames == 0);
    choice.car = -1;
    CHECK(MpSelectCar(&choice, &archive, UINT32_C(0x80000001)) == 0);
    CHECK(MpSelectCar(NULL, &archive, 0) == 0 && MpSelectCar(&choice, NULL, 0) == 0);

    const Action acknowledge[] = {{0, 0}, {PAD_CONFIRM, 0}};
    Script(acknowledge, 2);
    g_PadHeld = PAD_CONFIRM; /* A held racing button is not a new confirmation. */
    const MpResult result = {.seats = {{0, 0, -1}, {1, 1, 1000}}};
    MpShowResults(&result, 1);
    CHECK(frames == 2 && timeFormats == 1);
    CHECK(strcmp(text[0][0], "RACE RESULTS") == 0);
    CHECK(strcmp(text[0][1], "OTHER DRIVER: RETIRED") == 0);
    CHECK(strcmp(text[0][2], "YOU: PLACE 1 / TIME1000") == 0);
    Script(cancel, 1);
    MpShowResults(&result, 0);
    CHECK(frames == 1 && strcmp(text[0][1], "YOU: RETIRED") == 0);
    Script(acknowledge, 2);
    MpShowError("SERVER AND CLIENT DISCS DIFFER");
    CHECK(frames == 2 && timeFormats == 0);
    CHECK(strcmp(text[0][0], "MULTIPLAYER ERROR") == 0);
    CHECK(strcmp(text[0][1], "SERVER AND CLIENT DISCS DIFFER") == 0);
    CHECK(text[0][2][0] == '\0');
    RaceSim race = {.laps = 3, .elapsed = 50};
    race.drivers[1].status = SIM_DRIVING;
    race.drivers[1].place = 2;
    race.drivers[1].car.lap = 2;
    race.drivers[1].car.drive.gear = 4;
    char hud[2][64];
    CHECK(MpHudText(&race, 1, 120, "MPH", hud));
    CHECK(strcmp(hud[0], "POS 2/2 LAP 2/3") == 0);
    race.drivers[MP_FIELD_LIMIT - 1].status = SIM_RETIRED;
    CHECK(MpHudText(&race, 1, 120, "MPH", hud));
    CHECK(strcmp(hud[0], "POS 2/3 LAP 2/3") == 0);
    race.drivers[MP_FIELD_LIMIT - 1].status = SIM_EMPTY;
    CHECK(strcmp(hud[1], "120 MPH G4 TIME1000") == 0);
    race.drivers[1].car.lap = 4;
    race.drivers[1].status = SIM_DRIVER_FINISHED;
    CHECK(MpHudText(&race, 1, 0, "KPH", hud));
    CHECK(strcmp(hud[0], "POS 2/2 LAP 3/3") == 0);
    race.drivers[1].status = SIM_RETIRED;
    CHECK(!MpHudText(&race, 1, 0, "KPH", hud));
    CHECK(!MpHudText(&race, MP_SEAT_LIMIT, 0, "KPH", hud));
    CHECK(!MpHudText(NULL, 0, 0, "KPH", hud));

    /* MpEnterName/MpEnterServer: arcade cursor text entry. */
    char name[4] = "";
    const Action nameCursorWrap[] = {{0, PAD_LEFT}, {0, PAD_DOWN}, {PAD_CONFIRM, 0}};
    Script(nameCursorWrap, 3);
    CHECK(MpEnterName(name, sizeof(name)) == 1 && frames == 3);
    CHECK(strcmp(text[0][2], "^  ") == 0 && strcmp(text[1][2], "  ^") == 0);
    CHECK(strcmp(name, "  A") == 0);

    char name2[4] = "";
    const Action nameEmptyThenFixed[] = {{PAD_CONFIRM, 0}, {0, PAD_DOWN}, {PAD_CONFIRM, 0}};
    Script(nameEmptyThenFixed, 3);
    CHECK(MpEnterName(name2, sizeof(name2)) == 1 && frames == 3);
    CHECK(strcmp(name2, "A") == 0);

    char name3[8] = "HELLO";
    const Action nameCancel[] = {{0, PAD_DOWN}, {PAD_CANCEL, 0}};
    Script(nameCancel, 2);
    CHECK(MpEnterName(name3, sizeof(name3)) == -1);
    CHECK(strcmp(name3, "HELLO") == 0);

    char name4[4] = "";
    const Action nameWrapUp[] = {{0, PAD_UP}, {PAD_CONFIRM, 0}};
    Script(nameWrapUp, 2);
    CHECK(MpEnterName(name4, sizeof(name4)) == 1);
    CHECK(strcmp(name4, "9") == 0);

    CHECK(MpEnterName(NULL, 8) == 0);
    char tiny[1];
    CHECK(MpEnterName(tiny, 1) == 0);
    CHECK(MpEnterServer(NULL, 8) == 0);

    char server[6] = "";
    const Action serverEntry[] = {{0, PAD_DOWN}, {0, PAD_RIGHT}, {PAD_CONFIRM, 0}};
    Script(serverEntry, 3);
    CHECK(MpEnterServer(server, sizeof(server)) == 1);
    CHECK(strcmp(server, ".") == 0);

    puts("mp_menu: selection, metadata failures, result acknowledgement and text entry pass");
    return 0;
}
