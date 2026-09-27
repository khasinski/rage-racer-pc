#ifndef WEB_RULES_H
#define WEB_RULES_H
/* Race rules shared by the browser client and the race server, so both
 * accept exactly the same rooms, cars and fields. */
#include <stdint.h>

#include "game/car_control.h"
#include "game/race_data.h"
#include "game/race_grid.h"

enum {
    WEB_CLASS_COUNT = 6,
    WEB_COURSE_COUNT = 4,
    WEB_MAX_LAPS = 6,
    WEB_MAX_PLAYERS = 8, /* two lanes, four rows */
    WEB_COUNTDOWN_TICKS = 3 * SIM_TICK_RATE,
};

/* Retail custom-race rule (custom_race.c): a class offers every model its
 * progression has unlocked, each at the grade that class buys. Returns the
 * variant, or -1 when the model is not available in the class. */
int WebClassCar(int classIndex, int model);
/* Whether `variant` is the class's grade of an available model. */
int WebCarAllowed(int classIndex, int variant);
int WebCarModel(int variant);
int WebCarGrade(int variant);
const char *WebCarName(int model);
const char *WebCourseName(int course);
/* The Extreme Oval exists from the third class up (custom_race.c). */
int WebCourseAllowed(int classIndex, int course);

/* Seat description exchanged between server and clients. */
typedef struct WebSeat {
    int kind;    /* RaceSeatKind */
    int variant; /* human car variant */
    int manual;  /* human transmission */
} WebSeat;

/* Builds the field: humans in seats 0..humanCount-1 on the players' grid
 * (the retail player start, then two lanes behind it, all before the line)
 * and, when `rivals` is set, the retail AI on their authored starts in the
 * remaining seats (the final class races only its contenders). Returns the
 * entrant count, or 0 when the setup is invalid. */
int WebBuildField(const RaceData *archive, int classIndex, int course, int reverse,
                  const WebSeat *humans, int humanCount, int rivals,
                  RaceEntrant entrants[DRIVER_SEAT_LIMIT]);
/* Most players a course takes (WEB_MAX_PLAYERS), 0 when it is not raced. */
int WebMaxHumans(const RaceData *archive, int classIndex, int course, int reverse);

/* Driver commands on the wire: mode, left, right, angle, throttle, brake,
 * shiftUp, shiftDown. Returns 1 only for a command the simulation accepts. */
enum { WEB_INPUT_WORDS = 8 };
int WebDecodeInput(const int32_t words[WEB_INPUT_WORDS], DriverInput *out);
void WebEncodeInput(const DriverInput *input, int32_t words[WEB_INPUT_WORDS]);

/* FNV-1a over the imported archive: server and clients must race the same data. */
uint64_t WebArchiveFingerprint(const RaceData *archive);
#endif
