#include "mp_race_client.h"
#include "mp_client.h"
#include "game/boot_internal.h"
#include "game/car_catalog.h"
#include "game/race.h"
#include "game/race_data.h"
#include "game/race_sim.h"
#include "game/state.h"
#include "game/screens.h"
#include "keyboard_text.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* Arcade-style cursor editor shared by name and server entry. Two ways to
 * edit the same fixed-width cell grid: LEFT-RIGHT moves the cursor and
 * UP-DOWN cycles the character under it through charset (pad-friendly);
 * typing a character in charset drops it in directly and advances the
 * cursor, and backspace steps back and clears (keyboard-friendly). CONFIRM
 * accepts (trimming trailing padding; empty input keeps editing). text
 * holds the initial value on entry and, on a 1 return, the result. */
static int EditText(const char *title, const char *charset, char *text, size_t capacity) {
    if (!title || !charset || !text || capacity < 2 || capacity > 32) return 0;
    size_t width = capacity - 1;
    size_t charsetLength = strlen(charset);
    if (!charsetLength) return 0;
    char working[32];
    size_t length = strnlen(text, width);
    memset(working, ' ', width);
    memcpy(working, text, length);
    working[width] = '\0';
    int cursor = 0;
    for (;;) {
        char pointer[32];
        for (size_t i = 0; i < width; ++i) pointer[i] = (i == (size_t)cursor) ? '^' : ' ';
        pointer[width] = '\0';
        DrawHostMenuFrame(title, working, pointer,
                         "TYPE DIRECTLY, OR LEFT-RIGHT/UP-DOWN  CONFIRM: ACCEPT");
        char typed = PortConsumeTypedChar();
        if (g_PadPressed & PAD_CANCEL) return -1;
        if (g_PadPressed & PAD_CONFIRM) {
            char result[32];
            size_t resultLength;
            snprintf(result, sizeof(result), "%s", working);
            resultLength = strlen(result);
            while (resultLength > 0 && result[resultLength - 1] == ' ') result[--resultLength] = '\0';
            if (resultLength == 0) continue;
            snprintf(text, capacity, "%s", result);
            return 1;
        }
        if (typed == '\b') {
            cursor = (int)((cursor + width - 1) % width);
            working[cursor] = ' ';
            continue;
        }
        if (typed && memchr(charset, typed, charsetLength)) {
            working[cursor] = typed;
            cursor = (int)((cursor + 1) % width);
            continue;
        }
        if (g_PadPressedRepeat & PAD_LEFT) cursor = (int)((cursor + width - 1) % width);
        else if (g_PadPressedRepeat & PAD_RIGHT) cursor = (int)((cursor + 1) % width);
        int direction = g_PadPressedRepeat & PAD_UP ? -1 : g_PadPressedRepeat & PAD_DOWN ? 1 : 0;
        if (direction) {
            const char *at = memchr(charset, working[cursor], charsetLength);
            size_t index = at ? (size_t)(at - charset) : 0;
            index = (index + charsetLength + (size_t)direction) % charsetLength;
            working[cursor] = charset[index];
        }
    }
}

/* Pure editing only; no file access here so this stays headless-testable.
 * The caller (mp_race_client.c, the full client only) pre-fills the buffer
 * with the last saved value or a default, and persists a 1 return. */
int MpEnterName(char *name, size_t capacity) {
    static const char charset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (!name || capacity < 2) return 0;
    return EditText("MULTIPLAYER / DRIVER NAME", charset, name, capacity);
}

int MpEnterServer(char *hostPort, size_t capacity) {
    static const char charset[] = " .:0123456789";
    if (!hostPort || capacity < 2) return 0;
    return EditText("MULTIPLAYER / SERVER ADDRESS", charset, hostPort, capacity);
}

int MpHudText(const RaceSim *race, int seat, int speed, const char *units, char rows[2][64]) {
    if (!race || !rows || !units || (unsigned)seat >= MP_SEAT_LIMIT ||
        race->laps < 1 || race->laps > PLAYER_LAP_TIME_CAPACITY || speed < 0) return 0;
    const SimDriver *driver = &race->drivers[seat];
    if (driver->status != SIM_DRIVING && driver->status != SIM_DRIVER_FINISHED) return 0;
    int lap = driver->car.lap > race->laps ? race->laps : driver->car.lap;
    char time[LAP_TIME_TEXT_CAPACITY];
    uint64_t ms = (uint64_t)race->elapsed * 1000 / SIM_TICK_RATE;
    FormatLapTime(time, ms > INT32_MAX ? INT32_MAX : (s32)ms);
    int entrants = MP_SEAT_LIMIT;
    for (int i = MP_SEAT_LIMIT; i < MP_FIELD_LIMIT; ++i)
        entrants += race->drivers[i].status != SIM_EMPTY;
    snprintf(rows[0], 64, "POS %d/%d LAP %d/%d", driver->place, entrants, lap, race->laps);
    snprintf(rows[1], 64, "%d %s G%d %s", speed, units, driver->car.drive.gear, time);
    return 1;
}

int MpWaitRoom(MpClient *client, int seat, MpStart *start, MpSettings *settings, uint32_t automaticCars) {
    uint64_t room = MpClientRoom(client);
    if (!room || (unsigned)seat >= MP_SEAT_LIMIT || !start || !settings ||
        (unsigned)settings->car >= CAR_MODEL_VARIANT_COUNT || (unsigned)settings->manual > 1 ||
        (uint64_t)automaticCars >> CAR_MODEL_VARIANT_COUNT) return 0;
    if (!settings->manual && !(automaticCars & (UINT32_C(1) << settings->car))) return 0;
    uint64_t requested = UINT64_C(1) << (settings->car * 2 + settings->manual);
    MpSettings choice = *settings;
    char label[48];
    /* Pick already leaves the server seat not ready. */
    int ready = 0, pending = 0, target = 0;
    for (;;) {
        snprintf(label, sizeof(label), "%s %llu / CAR %d %s", seat == 0 ? "HOST" : "GUEST",
                 (unsigned long long)room, settings->car + 1, settings->manual ? "MT" : "AT");
        char peer[64];
        const MpLobby *lobby = MpClientLobby(client);
        if (lobby) {
            int other = 1 - seat;
            const MpLobbySeat *player = &lobby->seats[other];
            char name[16];
            for (int i = 0; i < 16; ++i) name[i] = i < player->length ?
                (player->name[i] >= 32 && player->name[i] <= 126 ? player->name[i] : '?') : '\0';
            if (lobby->room.occupied & (1 << other))
                snprintf(peer, sizeof(peer), "%.15s CAR %d %s / %s", name[0] ? name : "PLAYER",
                         player->variant + 1, player->manual ? "MT" : "AT",
                         lobby->room.ready & (1 << other) ? "READY" : "NOT READY");
            else snprintf(peer, sizeof(peer), "WAITING FOR OTHER PLAYER");
        } else snprintf(peer, sizeof(peer), "%s", ready ? "READY / WAITING FOR OTHER PLAYER" : "NOT READY");
        DrawHostMenuFrame(ready ? "MULTIPLAYER / YOU READY" : "MULTIPLAYER / YOU NOT READY", label, peer,
                         ready ? "CONFIRM: UNREADY / CANCEL: LEAVE" : "LEFT-RIGHT: CAR / UP: AT-MT / CONFIRM: READY");
        if (g_PadPressed & PAD_CANCEL) return -1;
        /* Finish pending Ready before accepting Start and reporting Loaded.
         * A completed Start wins over a new button edge in the same frame. */
        if (!pending) {
            int received = MpClientPollStart(client, start);
            if (received == 1) {
                const MpSeat *selected = &start->seats[seat];
                if (selected->model >= CAR_MODEL_VARIANT_COUNT || selected->manual > 1) return 0;
                uint64_t accepted = UINT64_C(1) << (selected->model * 2 + selected->manual);
                if (!(requested & accepted)) return 0;
                /* Start wins when a valid earlier choice froze before Pick. */
                settings->car = selected->model; settings->manual = selected->manual;
                return 1;
            }
            if (received != 2 && received != 3) return received;
            if (g_PadPressed & PAD_CONFIRM) { target = !ready; pending = 1; }
            else if (!ready) {
                int direction = g_PadPressedRepeat & PAD_LEFT ? -1 :
                                g_PadPressedRepeat & PAD_RIGHT ? 1 : 0;
                int toggle = (g_PadPressed & PAD_UP) != 0;
                if (direction || toggle) {
                    choice = *settings;
                    if (!MpChangeCar(&choice, direction, toggle, automaticCars)) return 0;
                    pending = 2;
                }
            }
        }
        if (pending) {
            int sent = pending == 1 ? MpClientPollReady(client, target) :
                       MpClientPollPick(client, choice.car, choice.manual);
            if (!sent) return 0;
            if (sent == 1) {
                if (pending == 1) ready = target;
                else {
                    *settings = choice;
                    requested |= UINT64_C(1) << (choice.car * 2 + choice.manual);
                    ready = 0;
                }
                pending = 0;
            }
        }
    }
}

int MpBrowseRooms(MpClient *client, uint64_t *code) {
    if (!client || !code) return 0;
    MpRoomInfo rooms[MP_ROOM_LIMIT] = {0};
    size_t count = 0, selected = 0;
    uint64_t previous = 0;
    int loading = 1;
    for (;;) {
        char choice[64], details[64];
        if (count) {
            const MpRoomInfo *room = &rooms[selected];
            snprintf(choice, sizeof(choice), "%zu/%zu ROOM %llu / %s", selected + 1, count,
                     (unsigned long long)room->code, room->state == 0 ?
                     (room->occupied == 3 ? "FULL" : "OPEN") : room->state == 1 ? "LOADING" : "RACING");
            snprintf(details, sizeof(details), "CLASS %d COURSE %d %s / %d LAPS", room->options.classIndex + 1,
                     room->options.course + 1, room->options.reverse ? "REV" : "FWD", room->options.laps);
        } else {
            snprintf(choice, sizeof(choice), "%s", loading ? "READING ROOMS..." : "NO ROOMS");
            details[0] = '\0';
        }
        DrawHostMenuFrame("MULTIPLAYER / ROOMS", choice, details,
                         loading ? "REFRESHING... / CANCEL" : "UP-DOWN / CONFIRM: JOIN / RIGHT: REFRESH");
        if (g_PadPressed & PAD_CANCEL) return -1;
        if (loading) {
            int received = MpClientPollList(client, rooms, &count);
            if (!received) return 0;
            if (received == 1) {
                selected = 0;
                for (size_t i = 0; i < count; ++i) if (rooms[i].code == previous) selected = i;
                loading = 0;
            }
            continue;
        }
        if (g_PadPressed & PAD_RIGHT) {
            previous = count ? rooms[selected].code : 0;
            loading = 1;
        } else if (count) {
            if (g_PadPressedRepeat & PAD_UP) selected = (selected + count - 1) % count;
            else if (g_PadPressedRepeat & PAD_DOWN) selected = (selected + 1) % count;
            if ((g_PadPressed & PAD_CONFIRM) && rooms[selected].state == 0 && rooms[selected].occupied != 3) {
                *code = rooms[selected].code;
                return 1;
            }
        }
    }
}

int MpSelectRoom(uint64_t *code) {
    if (!code || (*code > INT64_MAX && *code != UINT64_MAX)) return 0;
    int mode = *code == UINT64_MAX ? 0 : *code == 0 ? 1 : 2;
    const char *labels[] = {"AUTO JOIN", "CREATE ROOM", "JOIN BY CODE", "BROWSE ROOMS"};
    char digits[20];
    snprintf(digits, sizeof(digits), "%019llu",
             (unsigned long long)(mode == 2 ? *code : 1));
    int editing = 0, cursor = 18, invalid = 0;
    for (;;) {
        char choice[48], controls[64];
        if (editing) {
            snprintf(choice, sizeof(choice), "CODE %s / DIGIT %d", digits, cursor + 1);
            snprintf(controls, sizeof(controls), "LEFT-RIGHT: DIGIT / UP-DOWN: VALUE");
        } else {
            snprintf(choice, sizeof(choice), "%s", labels[mode]);
            snprintf(controls, sizeof(controls), "UP-DOWN: CHOOSE / CONFIRM / CANCEL");
        }
        DrawHostMenuFrame("MULTIPLAYER / ROOM", choice, controls,
                         invalid ? "INVALID CODE: USE 1..9223372036854775807" :
                         "CONFIRM: CONTINUE / CANCEL: BACK");
        if (g_PadPressed & PAD_CANCEL) {
            if (!editing) return -1;
            editing = invalid = 0;
            continue;
        }
        if (g_PadPressed & PAD_CONFIRM) {
            if (!editing && mode == 2) { editing = 1; continue; }
            uint64_t selected = mode == 0 ? UINT64_MAX : mode == 3 ? MP_BROWSE_ROOM : 0;
            if (editing && (!MpParseRoom(digits, &selected) || !selected)) {
                invalid = 1;
                continue;
            }
            *code = selected;
            return 1;
        }
        int vertical = g_PadPressedRepeat & PAD_UP ? -1 :
                       g_PadPressedRepeat & PAD_DOWN ? 1 : 0;
        if (!editing) mode = (mode + 4 + vertical) % 4;
        else {
            if (g_PadPressedRepeat & PAD_LEFT) cursor = (cursor + 18) % 19;
            else if (g_PadPressedRepeat & PAD_RIGHT) cursor = (cursor + 1) % 19;
            if (vertical) {
                digits[cursor] = (char)('0' + (digits[cursor] - '0' + 10 - vertical) % 10);
                invalid = 0;
            }
        }
    }
}

int MpSelectCar(MpSettings *settings, const RaceData *archive, uint32_t automaticCars) {
    if (!settings || !archive || (unsigned)settings->car >= CAR_MODEL_VARIANT_COUNT ||
        (unsigned)settings->manual > 1) return 0;
    if ((uint64_t)automaticCars >> CAR_MODEL_VARIANT_COUNT) return 0;
    /* Snapshot cosmetic catalog labels; never use single-player selections,
     * progression, prices or overridden physics for this network race. */
    char names[GAME_CAR_COUNT][RAGE_CAR_CATALOG_TEXT_CAPACITY];
    for (int model = 0; model < GAME_CAR_COUNT; ++model)
        snprintf(names[model], sizeof(names[model]), "%s", g_CarNames[model] ? g_CarNames[model] : "CAR");
    if (!(automaticCars & (UINT32_C(1) << settings->car))) settings->manual = 1;
    GameCarSpec specification;
    int loaded = -1;
    for (;;) {
        if (loaded != settings->car) {
            if (!ReadRaceCar(archive, settings->car, &specification)) return 0;
            loaded = settings->car;
        }
        int model = 0;
        while (model + 1 < GAME_CAR_COUNT &&
               settings->car >= CarCatalogVariant(model + 1, 0)) model++;
        int grade = settings->car - CarCatalogVariant(model, 0) + 1;
        char choice[48];
        snprintf(choice, sizeof(choice), "%02d/%02d %.16s G%d %s", settings->car + 1,
                 CAR_MODEL_VARIANT_COUNT, names[model], grade, settings->manual ? "MT" : "AT");
        char title[40];
        snprintf(title, sizeof(title), "MULTIPLAYER / %d GEARS", specification.topGear);
        DrawHostMenuFrame(title, choice, "LEFT/RIGHT: CAR  UP/DOWN: AT/MT",
                         "CONFIRM: JOIN ROOM / CANCEL");
        if (g_PadPressed & PAD_CANCEL) return -1;
        if (g_PadPressed & PAD_CONFIRM) return 1;
        int direction = g_PadPressedRepeat & PAD_LEFT ? -1 :
                        g_PadPressedRepeat & PAD_RIGHT ? 1 : 0;
        int toggle = (g_PadPressed & (PAD_UP | PAD_DOWN)) != 0;
        if ((direction || toggle) && !MpChangeCar(settings, direction, toggle, automaticCars)) return 0;
    }
}

int MpSelectRace(MpRaceOptions *options) {
    if (!MpValidRaceOptions(options)) return 0;
    int field = 0;
    const char *labels[] = {"CLASS", "COURSE", "LAPS", "DIRECTION"};
    MpRaceOptions choice = *options;
    for (;;) {
        char values[64], editing[48];
        snprintf(values, sizeof(values), "CLASS %d COURSE %d LAPS %d %s",
                 choice.classIndex + 1, choice.course + 1, choice.laps,
                 choice.reverse ? "REVERSE" : "FORWARD");
        snprintf(editing, sizeof(editing), "EDIT %s / LEFT-RIGHT: CHANGE", labels[field]);
        DrawHostMenuFrame("ROOM SETTINGS / HOST", values, editing,
                         "UP-DOWN: FIELD / CONFIRM / CANCEL");
        if (g_PadPressed & PAD_CANCEL) return -1;
        if (g_PadPressed & PAD_CONFIRM) { *options = choice; return 1; }
        if (g_PadPressedRepeat & PAD_UP) field = (field + 3) % 4;
        else if (g_PadPressedRepeat & PAD_DOWN) field = (field + 1) % 4;
        int direction = g_PadPressedRepeat & PAD_LEFT ? -1 :
                        g_PadPressedRepeat & PAD_RIGHT ? 1 : 0;
        if (direction && !MpChangeRace(&choice, field, direction)) return 0;
    }
}

static void Acknowledge(const char *title, const char *first, const char *second) {
    do {
        DrawHostMenuFrame(title, first, second, "CONFIRM / CANCEL: RETURN TO MENU");
    } while (!(g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)));
}

void MpShowError(const char *message) {
    Acknowledge("MULTIPLAYER ERROR", message, "");
}

void MpShowResults(const MpResult *result, int localSeat) {
    char rows[MP_SEAT_LIMIT][64];
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const MpFinish *finish = &result->seats[seat];
        const char *label = seat == localSeat ? "YOU" : "OTHER DRIVER";
        if (finish->finished) {
            char time[LAP_TIME_TEXT_CAPACITY];
            FormatLapTime(time, finish->milliseconds);
            snprintf(rows[seat], sizeof(rows[seat]), "%s: PLACE %d / %s", label, finish->place, time);
        } else {
            snprintf(rows[seat], sizeof(rows[seat]), "%s: RETIRED", label);
        }
    }
    Acknowledge("RACE RESULTS", rows[0], rows[1]);
}
