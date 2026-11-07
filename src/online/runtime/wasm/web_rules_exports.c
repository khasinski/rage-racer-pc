/* The shared race rules, exported identically by the browser module
 * (rage_web.c) and the race server module (rage_server.c), so the lobby UI
 * and the server validate rooms and cars with the same compiled code. */
#include <emscripten/emscripten.h>
#include <inttypes.h>
#include <stdio.h>

#include "game/car.h"
#include "web_rules.h"

/* The archive the module imported, or NULL (defined by each bridge). */
const RaceData *WebLoadedArchive(void);

EMSCRIPTEN_KEEPALIVE int rw_car_models(void) { return GAME_CAR_COUNT; }
EMSCRIPTEN_KEEPALIVE int rw_class_car(int classIndex, int model) { return WebClassCar(classIndex, model); }
EMSCRIPTEN_KEEPALIVE int rw_car_allowed(int classIndex, int variant) { return WebCarAllowed(classIndex, variant); }
EMSCRIPTEN_KEEPALIVE int rw_car_model(int variant) { return WebCarModel(variant); }
EMSCRIPTEN_KEEPALIVE int rw_car_grade(int variant) { return WebCarGrade(variant); }
EMSCRIPTEN_KEEPALIVE const char *rw_car_name(int model) { return WebCarName(model); }
EMSCRIPTEN_KEEPALIVE const char *rw_course_name(int course) { return WebCourseName(course); }
EMSCRIPTEN_KEEPALIVE int rw_course_allowed(int classIndex, int course) {
    return WebCourseAllowed(classIndex, course);
}

/* Human seats the course offers, or 0 without a disc. */
EMSCRIPTEN_KEEPALIVE int rw_max_humans(int classIndex, int course, int reverse) {
    return WebMaxHumans(WebLoadedArchive(), classIndex, course, reverse);
}

/* 1 when the disc offers this variant with the automatic gearbox. */
EMSCRIPTEN_KEEPALIVE int rw_car_automatic(int variant) {
    int automatic = 0;
    return WebLoadedArchive() && ReadRaceCarTransmission(WebLoadedArchive(), variant, &automatic) &&
           automatic;
}

/* Boot serial and archive fingerprint; server and players must match. */
EMSCRIPTEN_KEEPALIVE const char *rw_disc_id(void) {
    static char id[48];
    const RaceData *archive = WebLoadedArchive();
    if (!archive) return "";
    snprintf(id, sizeof(id), "%.15s-%016" PRIx64, archive->boot[0] ? archive->boot : "RAGE",
             WebArchiveFingerprint(archive));
    return id;
}
