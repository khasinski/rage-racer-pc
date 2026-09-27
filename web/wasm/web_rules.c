#include "web_rules.h"

#include <string.h>

#include "game/car.h"

/* First retail variant of each model (catalog_parse.c kFirstVariant) and the
 * class each model's first grade is bought in (host_state_car_catalog.c
 * g_CarModelUnlockBase). */
static const unsigned char kFirstVariant[GAME_CAR_COUNT] = {
    0, 4, 7, 9, 14, 18, 21, 23, 26, 28, 29, 30, 31
};
static const unsigned char kUnlockBase[GAME_CAR_COUNT] = {
    1, 2, 3, 0, 1, 2, 3, 2, 3, 4, 5, 5, 5
};
/* native_game_state.c g_NativeCarNames / g_CourseNames. */
static const char *const kCarNames[GAME_CAR_COUNT] = {
    "ERRISO", "ABEILLE", "PEGASE", "ESPERANZA", "ACCERON", "BAYONET", "HIJACK",
    "FATALITA", "ISTANTE", "GHEPARDO", "VAINQURE", "BULSHADE", "SQUALDON"
};
static const char *const kCourseNames[WEB_COURSE_COUNT] = {
    "MYTHICAL COAST", "OVER PASS CITY", "LAKESIDE GATE", "THE EXTREME OVAL"
};
enum { OVAL_COURSE = 3, OVAL_MINIMUM_CLASS = 2 };

static int GradeCount(int model) {
    const int end = model + 1 < GAME_CAR_COUNT ? kFirstVariant[model + 1] : CAR_MODEL_VARIANT_COUNT;
    return end - kFirstVariant[model];
}

int WebClassCar(int classIndex, int model) {
    if ((unsigned)classIndex >= WEB_CLASS_COUNT || (unsigned)model >= GAME_CAR_COUNT ||
        kUnlockBase[model] > classIndex) return -1;
    int grade = classIndex - kUnlockBase[model];
    if (grade >= GradeCount(model)) grade = GradeCount(model) - 1;
    return kFirstVariant[model] + grade;
}

int WebCarModel(int variant) {
    if ((unsigned)variant >= CAR_MODEL_VARIANT_COUNT) return -1;
    int model = GAME_CAR_COUNT - 1;
    while (kFirstVariant[model] > variant) --model;
    return model;
}

int WebCarGrade(int variant) {
    const int model = WebCarModel(variant);
    return model < 0 ? -1 : variant - kFirstVariant[model];
}

int WebCarAllowed(int classIndex, int variant) {
    const int model = WebCarModel(variant);
    return model >= 0 && WebClassCar(classIndex, model) == variant;
}

const char *WebCarName(int model) {
    return (unsigned)model < GAME_CAR_COUNT ? kCarNames[model] : "";
}

const char *WebCourseName(int course) {
    return (unsigned)course < WEB_COURSE_COUNT ? kCourseNames[course] : "";
}

int WebCourseAllowed(int classIndex, int course) {
    return (unsigned)classIndex < WEB_CLASS_COUNT && (unsigned)course < WEB_COURSE_COUNT &&
           (course != OVAL_COURSE || classIndex >= OVAL_MINIMUM_CLASS);
}

/* Grid slots in field order: the player start, then every authored active
 * AI start (the final class races only its contenders). */
static int GridOrder(const TrackData *track, int classIndex, int reverse, s32 grids[DRIVER_SEAT_LIMIT]) {
    int count = 0;
    grids[count++] = 0;
    for (s32 grid = 1; grid < DRIVER_SEAT_LIMIT; ++grid) {
        if ((classIndex != WEB_CLASS_COUNT - 1 || grid <= RIVAL_CONTENDER_COUNT) &&
            track->events->rivalStarts[reverse][grid].activeFlag != -1)
            grids[count++] = grid;
    }
    return count;
}

int WebMaxHumans(const RaceData *archive, int classIndex, int course, int reverse) {
    s32 grids[DRIVER_SEAT_LIMIT];
    if (!archive || !WebCourseAllowed(classIndex, course)) return 0;
    TrackData *track = CopyRaceTrack(archive, classIndex, course);
    if (!track) return 0;
    const int count = GridOrder(track, classIndex, reverse ? 1 : 0, grids);
    FreeTrackData(track);
    return count;
}

int WebBuildField(const RaceData *archive, int classIndex, int course, int reverse,
                  const WebSeat *humans, int humanCount, int rivals,
                  RaceEntrant entrants[DRIVER_SEAT_LIMIT]) {
    s32 grids[DRIVER_SEAT_LIMIT];
    if (!archive || !entrants || !humans || humanCount < 1 ||
        !WebCourseAllowed(classIndex, course)) return 0;
    reverse = reverse ? 1 : 0;
    TrackData *track = CopyRaceTrack(archive, classIndex, course);
    if (!track) return 0;
    const int slots = GridOrder(track, classIndex, reverse, grids);
    FreeTrackData(track);
    if (humanCount > slots) return 0;
    RaceEntrant field[DRIVER_SEAT_LIMIT];
    memset(field, 0, sizeof(field));
    int count = 0;
    for (int seat = 0; seat < humanCount; ++seat) {
        const WebSeat *human = &humans[seat];
        if (human->kind != RACE_SEAT_HUMAN || !WebCarAllowed(classIndex, human->variant)) return 0;
        int automatic = 0;
        if (!human->manual && (!ReadRaceCarTransmission(archive, human->variant, &automatic) ||
                               !automatic)) return 0;
        field[seat] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = grids[seat],
                                    .model = human->variant, .manual = human->manual ? 1 : 0,
                                    .seed = 0x5eedu + (u32)seat};
        ++count;
    }
    /* AI keep the retail behaviour slot and model of the start they take. */
    for (int seat = humanCount; rivals && seat < slots; ++seat) {
        field[seat] = (RaceEntrant){.kind = RACE_SEAT_AI, .grid = grids[seat],
                                    .model = grids[seat] - 1, .rivalSlot = grids[seat] - 1};
        ++count;
    }
    memcpy(entrants, field, sizeof(field));
    return count;
}

uint64_t WebArchiveFingerprint(const RaceData *archive) {
    uint64_t hash = 14695981039346656037ull;
    if (!archive || !archive->data) return 0;
    for (size_t i = 0; i < archive->size; ++i) {
        hash ^= archive->data[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

int WebDecodeInput(const int32_t words[WEB_INPUT_WORDS], DriverInput *out) {
    DriverInput input;
    if (!words || !out || words[4] < INT16_MIN || words[4] > INT16_MAX ||
        words[5] < INT16_MIN || words[5] > INT16_MAX) return 0;
    memset(&input, 0, sizeof(input));
    input.steering.mode = (SteeringMode)words[0];
    input.steering.left = words[1];
    input.steering.right = words[2];
    input.steering.angle = words[3];
    input.throttle = (s16)words[4];
    input.brake = (s16)words[5];
    input.shiftUp = words[6];
    input.shiftDown = words[7];
    if (!ValidDriverInput(&input)) return 0;
    *out = input;
    return 1;
}

void WebEncodeInput(const DriverInput *input, int32_t words[WEB_INPUT_WORDS]) {
    words[0] = (int32_t)input->steering.mode;
    words[1] = input->steering.left;
    words[2] = input->steering.right;
    words[3] = input->steering.angle;
    words[4] = input->throttle;
    words[5] = input->brake;
    words[6] = input->shiftUp;
    words[7] = input->shiftDown;
}
