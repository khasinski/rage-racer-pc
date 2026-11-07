#include "web_rules.h"

#include <math.h>
#include <string.h>

#include "game/car.h"
#include "game/car_catalog.h"
#include "game/car_track_internal.h"

/* The class each model's first grade is bought in (host_state_car_catalog.c
 * g_CarModelUnlockBase); its variants are the car catalog's. */
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

int WebClassCar(int classIndex, int model) {
    if ((unsigned)classIndex >= WEB_CLASS_COUNT || (unsigned)model >= GAME_CAR_COUNT ||
        kUnlockBase[model] > classIndex) return -1;
    /* The class's grade, or the model's best when it has fewer grades. */
    for (int grade = classIndex - kUnlockBase[model]; grade >= 0; --grade) {
        const int variant = CarCatalogVariant(model, grade);
        if (variant >= 0) return variant;
    }
    return -1;
}

int WebCarModel(int variant) {
    if ((unsigned)variant >= CAR_MODEL_VARIANT_COUNT) return -1;
    int model = GAME_CAR_COUNT - 1;
    while (CarCatalogVariant(model, 0) > variant) --model;
    return model;
}

int WebCarGrade(int variant) {
    const int model = WebCarModel(variant);
    return model < 0 ? -1 : variant - CarCatalogVariant(model, 0);
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

/* Players' starting grid: two lanes of rows reaching back from the retail
 * player start, so the players start together at the back of the field,
 * before the line. Rivals keep their authored starts, which retail spreads
 * along the course ahead. Rows are as tight as the collision box allows
 * (g_CarCollisionCorners: 192 x 640), leaving a short gap between cars. */
enum { GRID_ROW_SPACING = 760, GRID_LANE_OFFSET = 225, GRID_EDGE_MARGIN = 100 };

typedef struct GridPlace {
    double x, z;
    s32 index; /* nearest route point, the search hint for the car */
} GridPlace;

static s32 SeededProgress(const TrackData *track, int reverse, s32 index, int selector) {
    GameCarRuntime car;
    memset(&car, 0, sizeof(car));
    car.trackPointIndex = RouteIndex(&track->route, index);
    SeedCarTrackProgress(&car, &track->route, track->events->trackWalkStart, selector, reverse);
    return car.progressA;
}

/* Seeding selector (the start's activeFlag) whose progress lies nearest to
 * the expected value: past the lap walk the progress is seeded the other way
 * round, exactly as for the rivals' authored starts. */
static int NearestSelector(const TrackData *track, int reverse, s32 index, int64_t expected) {
    const int64_t first = SeededProgress(track, reverse, index, 0) - expected;
    const int64_t second = SeededProgress(track, reverse, index, 1) - expected;
    return (second < 0 ? -second : second) < (first < 0 ? -first : first) ? 1 : 0;
}

static int64_t Progress(const TrackData *track, int reverse, s32 index, int64_t expected) {
    return SeededProgress(track, reverse, index, NearestSelector(track, reverse, index, expected));
}

static const GameTrackPoint *Point(const TrackRoute *route, s32 index) {
    return RoutePoint(route, RouteIndex(route, index));
}

/* The route point order that leads away from the line (progress decreases). */
static int BackwardStep(const TrackData *track, int reverse, s32 pole, int64_t poleProgress) {
    return Progress(track, reverse, pole + 1, poleProgress) < Progress(track, reverse, pole - 1, poleProgress)
        ? 1 : -1;
}

/* Where the retail player start really puts the car (driver_init.c
 * PlaceDriver): its authored position on its route segment, or the route
 * point itself when that position is not on the route. */
static void PlacePole(const TrackRoute *route, const TrackRivalStart *pole, double *x, double *z, s32 *segment) {
    GameCarRuntime car;
    memset(&car, 0, sizeof(car));
    car.x = pole->x;
    car.z = pole->z;
    const s32 hint = RouteIndex(route, pole->trackPointIndex);
    const s32 found = FindCarTrackSegment(&car, route, hint);
    *segment = found >= 0 ? found : hint;
    *x = found >= 0 ? pole->x : Point(route, hint)->x;
    *z = found >= 0 ? pole->z : Point(route, hint)->z;
}

/* Walks `distance` back along the route polyline from the player start. */
static GridPlace WalkBack(const TrackRoute *route, const TrackRivalStart *pole, int step, double distance) {
    double x, z;
    s32 segment;
    PlacePole(route, pole, &x, &z, &segment);
    /* First route point behind the start. */
    const GameTrackPoint *from = Point(route, segment), *to = Point(route, segment + step);
    const double bx = (double)to->x - from->x, bz = (double)to->z - from->z;
    s32 vertex = segment;
    for (int guard = 0; guard < 4 && ((double)Point(route, vertex)->x - x) * bx +
                                        ((double)Point(route, vertex)->z - z) * bz <= 0.0; ++guard)
        vertex = RouteIndex(route, vertex + step);
    for (int guard = 0; guard < route->count; ++guard) {
        const GameTrackPoint *target = Point(route, vertex);
        const double dx = target->x - x, dz = target->z - z, length = sqrt(dx * dx + dz * dz);
        if (length >= distance && length > 0.0) {
            return (GridPlace){x + dx / length * distance, z + dz / length * distance, vertex};
        }
        distance -= length;
        x = target->x;
        z = target->z;
        vertex = RouteIndex(route, vertex + step);
    }
    return (GridPlace){x, z, vertex};
}

static TrackRivalStart GridStart(const TrackData *track, int reverse, int place) {
    const TrackRoute *route = &track->route;
    const TrackRivalStart *pole = &track->events->rivalStarts[reverse][0];
    double poleX, poleZ;
    s32 poleIndex;
    PlacePole(route, pole, &poleX, &poleZ, &poleIndex);
    const int64_t poleProgress = SeededProgress(track, reverse, poleIndex, pole->activeFlag == 1 ? 1 : 0);
    const int step = BackwardStep(track, reverse, poleIndex, poleProgress);
    const double row = (double)(place / 2) * GRID_ROW_SPACING;
    const GridPlace centre = WalkBack(route, pole, step, row);
    /* Lane direction: across the route where the car stands. */
    const GridPlace ahead = WalkBack(route, pole, step, row > 200.0 ? row - 200.0 : 0.0);
    const GridPlace rear = WalkBack(route, pole, step, row + 200.0);
    double tx = ahead.x - rear.x, tz = ahead.z - rear.z;
    const double length = sqrt(tx * tx + tz * tz);
    if (length > 0.0) { tx /= length; tz /= length; } else { tx = 0.0; tz = 1.0; }
    const GameTrackPoint *point = Point(route, centre.index);
    double lane = GRID_LANE_OFFSET;
    const double half = (point->leftHalfWidth < point->rightHalfWidth ? point->leftHalfWidth
                                                                       : point->rightHalfWidth) - GRID_EDGE_MARGIN;
    if (half > 0.0 && lane > half) lane = half;
    if (place % 2) lane = -lane;
    TrackRivalStart start = {.x = (s32)lround(centre.x - tz * lane), .z = (s32)lround(centre.z + tx * lane),
                             .trackPointIndex = (s16)centre.index};
    start.activeFlag = (s16)NearestSelector(track, reverse, centre.index, poleProgress - (int64_t)row);
    return start;
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

int WebReadSeats(const int32_t *words, int humanCount, WebSeat humans[DRIVER_SEAT_LIMIT]) {
    if (!words || humanCount < 1 || humanCount > DRIVER_SEAT_LIMIT) return 0;
    for (int seat = 0; seat < humanCount; ++seat) {
        const int tire = words[seat * 3 + 2];
        if (tire < 0 || tire >= CAR_LAUNCH_THRESHOLD_COUNT) return 0;
        humans[seat] = (WebSeat){words[seat * 3], words[seat * 3 + 1], tire};
    }
    return 1;
}

int WebMaxHumans(const RaceData *archive, int classIndex, int course, int reverse) {
    (void)reverse;
    return archive && WebCourseAllowed(classIndex, course) ? WEB_MAX_PLAYERS : 0;
}

int WebBuildField(const RaceData *archive, int classIndex, int course, int reverse,
                  const WebSeat *humans, int humanCount, int rivals,
                  RaceEntrant entrants[DRIVER_SEAT_LIMIT]) {
    s32 grids[DRIVER_SEAT_LIMIT];
    RaceEntrant field[DRIVER_SEAT_LIMIT];
    if (!archive || !entrants || !humans || humanCount < 1 || humanCount > WEB_MAX_PLAYERS ||
        !WebCourseAllowed(classIndex, course)) return 0;
    reverse = reverse ? 1 : 0;
    for (int seat = 0; seat < humanCount; ++seat) {
        const WebSeat *human = &humans[seat];
        int automatic = 0;
        if (!WebCarAllowed(classIndex, human->variant) ||
            human->tire < 0 || human->tire >= CAR_LAUNCH_THRESHOLD_COUNT ||
            (!human->manual && (!ReadRaceCarTransmission(archive, human->variant, &automatic) ||
                                !automatic))) return 0;
    }
    TrackData *track = CopyRaceTrack(archive, classIndex, course);
    if (!track) return 0;
    const int slots = GridOrder(track, classIndex, reverse, grids);
    memset(field, 0, sizeof(field));
    /* Rivals take the authored starts after the first humanCount - 1, so the
     * field stays within the twelve seats. */
    u32 usedGrid = 1u; /* the player start, grid 0 */
    int count = humanCount;
    for (int list = humanCount; rivals && list < slots && count < DRIVER_SEAT_LIMIT; ++list) {
        field[count++] = (RaceEntrant){.kind = RACE_SEAT_AI, .grid = grids[list],
                                       .model = grids[list] - 1, .rivalSlot = grids[list] - 1};
        usedGrid |= 1u << grids[list];
    }
    /* Humans: seat order, the player start first, then the players' grid.
     * Their grid numbers only need to be unique. */
    for (int seat = 0; seat < humanCount; ++seat) {
        s32 grid = 0;
        if (seat > 0) {
            while (usedGrid & (1u << grid)) ++grid;
            usedGrid |= 1u << grid;
        }
        field[seat] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = grid, .model = humans[seat].variant,
                                    .manual = humans[seat].manual ? 1 : 0, .tire = humans[seat].tire,
                                    .seed = 0x5eedu + (u32)seat};
        /* Alone, the player keeps the retail start; together, the grid. */
        if (humanCount > 1) {
            field[seat].hasStart = 1;
            field[seat].start = GridStart(track, reverse, seat);
        }
    }
    FreeTrackData(track);
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
