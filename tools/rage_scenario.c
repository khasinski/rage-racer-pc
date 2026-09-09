/* Small native convenience wrapper for a reproducible race scenario.
 * Scenario interpretation remains in runtime_config/scenario_control; this
 * program validates its ergonomic options and forwards them as --set values. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

enum { TEXT = 1024, GRID = 256 };
enum {
    OVERRIDE_MODE = 1u << 0,
    OVERRIDE_SERIES = 1u << 1,
    OVERRIDE_AFTER_FINISH = 1u << 2,
    OVERRIDE_GRID = 1u << 3,
    OVERRIDE_RIVAL_POINTS = 1u << 4,
    OVERRIDE_CLASS = 1u << 5,
    OVERRIDE_COURSE = 1u << 6,
    OVERRIDE_CAR = 1u << 7,
    OVERRIDE_START_POINT = 1u << 8,
};

typedef struct ScenarioOptions {
    char mode[32], series[32], afterFinish[32], grid[GRID];
    int classIndex, course, car;
    int startPoint;
    char rivalPoints[GRID];
} ScenarioOptions;

static void Usage(const char *program) {
    fprintf(stderr,
        "usage: %s [scenario.ini] [--binary GAME] [--dry-run]\n"
        "       [--mode grand-prix|time-attack] [--series grand-prix|extra-gp]\n"
        "       [--class 0..5] [--course 0..3] [--car 0..12]\n"
        "       [--after-finish menu|repeat|exit] [--grid default|11 IDs]\n"
        "       [--start-point N] [--rival-points IDs|-]\n", program);
}

static char *Trim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) *--end = '\0';
    return text;
}

static int CopyText(char *destination, size_t size, const char *source) {
    size_t length;
    if (!source) return 0;
    length = strlen(source);
    if (length >= size) return 0;
    memcpy(destination, source, length + 1);
    return 1;
}

static int ParseInt(const char *text, int low, int high, int *out) {
    char *end = NULL;
    long value;
    errno = 0;
    value = strtol(text, &end, 0);
    if (errno || end == text || *end != '\0' || value < low || value > high)
        return 0;
    *out = (int)value;
    return 1;
}

static int ReadScenario(const char *path, ScenarioOptions *options) {
    size_t byteCount;
    void *bytes = SDL_LoadFile(path, &byteCount);
    char *text;
    char *cursor;
    int race = 0;
    if (!bytes || byteCount >= SIZE_MAX - 1) {
        SDL_free(bytes);
        fprintf(stderr, "%s: cannot read scenario\n", path);
        return 0;
    }
    text = SDL_malloc(byteCount + 1);
    if (!text) {
        SDL_free(bytes);
        return 0;
    }
    memcpy(text, bytes, byteCount);
    text[byteCount] = '\0';
    SDL_free(bytes);
    cursor = text;
    while (cursor < text + byteCount) {
        char *key, *value, *equals;
        char *line = cursor;
        char *newline = strchr(cursor, '\n');
        if (newline) {
            *newline = '\0';
            cursor = newline + 1;
        } else {
            cursor = text + byteCount;
        }
        line = Trim(line);
        if (*line == '\0' || *line == '#' || *line == ';') continue;
        if (*line == '[') {
            race = strcmp(line, "[race]") == 0;
            continue;
        }
        if (!race || (equals = strchr(line, '=')) == NULL) continue;
        *equals = '\0';
        key = Trim(line);
        value = Trim(equals + 1);
        if (strcmp(key, "mode") == 0 && !CopyText(options->mode, sizeof(options->mode), value))
            goto invalid;
        if (strcmp(key, "series") == 0 && !CopyText(options->series, sizeof(options->series), value))
            goto invalid;
        if (strcmp(key, "after_finish") == 0 && !CopyText(options->afterFinish, sizeof(options->afterFinish), value))
            goto invalid;
        if (strcmp(key, "grid") == 0 && !CopyText(options->grid, sizeof(options->grid), value))
            goto invalid;
        if (strcmp(key, "class") == 0 && !ParseInt(value, 0, 5, &options->classIndex)) goto invalid;
        if (strcmp(key, "course") == 0 && !ParseInt(value, 0, 3, &options->course)) goto invalid;
        if (strcmp(key, "car") == 0 && !ParseInt(value, 0, 12, &options->car)) goto invalid;
    }
    SDL_free(text);
    return 1;
invalid:
    SDL_free(text);
    fprintf(stderr, "%s: invalid [race] value\n", path);
    return 0;
}

static int IsOneOf(const char *value, const char *a, const char *b,
                   const char *c) {
    return strcmp(value, a) == 0 || (b && strcmp(value, b) == 0) ||
           (c && strcmp(value, c) == 0);
}

static int ValidateList(const char *text, int minimum, int maximum,
                        int exactCount, int allowDash) {
    char copy[GRID];
    char *cursor, *entry, *comma;
    int count = 0;
    if (!CopyText(copy, sizeof(copy), text)) return 0;
    cursor = copy;
    while (cursor != NULL) {
        int ignored;
        entry = cursor;
        comma = strchr(cursor, ',');
        if (comma != NULL) {
            *comma = '\0';
            cursor = comma + 1;
        } else {
            cursor = NULL;
        }
        entry = Trim(entry);
        if (allowDash && strcmp(entry, "-") == 0) {
            count++;
            continue;
        }
        if (!ParseInt(entry, minimum, maximum, &ignored)) return 0;
        count++;
    }
    return exactCount ? count == exactCount : count >= 1 && count <= 11;
}

static int Launch(char *const command[]) {
#ifdef _WIN32
    intptr_t result = _spawnv(_P_WAIT, command[0], (const char *const *)command);
    return result >= 0 && result == 0;
#else
    pid_t child = fork();
    int status;
    if (child < 0) return 0;
    if (child == 0) {
        execv(command[0], command);
        _exit(127);
    }
    return waitpid(child, &status, 0) == child && WIFEXITED(status) &&
           WEXITSTATUS(status) == 0;
#endif
}

static int FileExists(const char *path) {
    SDL_PathInfo info;
    return SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_FILE;
}

int main(int argc, char **argv) {
    ScenarioOptions options = {
        .mode = "grand-prix", .series = "grand-prix", .afterFinish = "menu",
        .grid = "default", .classIndex = 0, .course = 0, .car = 3,
        .startPoint = -1,
    };
    const char *scenario = "race-scenario.ini";
    const char *binary = "build/release/rage-racer";
    int dryRun = 0;
    int positional = 0;
    unsigned overrides = 0;
    int index;
    char assignments[9][TEXT + GRID];
    char *command[24];
    int argument = 0;

    for (index = 1; index < argc; ++index) {
        const char *value;
        if (strcmp(argv[index], "--help") == 0) { Usage(argv[0]); return 0; }
        if (strcmp(argv[index], "--dry-run") == 0) { dryRun = 1; continue; }
        if (argv[index][0] != '-') {
            if (positional++) { Usage(argv[0]); return 2; }
            scenario = argv[index];
            continue;
        }
        if (++index == argc) { Usage(argv[0]); return 2; }
        value = argv[index];
        if (strcmp(argv[index - 1], "--binary") == 0) binary = value;
        else if (strcmp(argv[index - 1], "--mode") == 0 && CopyText(options.mode, sizeof(options.mode), value)) overrides |= OVERRIDE_MODE;
        else if (strcmp(argv[index - 1], "--series") == 0 && CopyText(options.series, sizeof(options.series), value)) overrides |= OVERRIDE_SERIES;
        else if (strcmp(argv[index - 1], "--after-finish") == 0 && CopyText(options.afterFinish, sizeof(options.afterFinish), value)) overrides |= OVERRIDE_AFTER_FINISH;
        else if (strcmp(argv[index - 1], "--grid") == 0 && CopyText(options.grid, sizeof(options.grid), value)) overrides |= OVERRIDE_GRID;
        else if (strcmp(argv[index - 1], "--rival-points") == 0 && CopyText(options.rivalPoints, sizeof(options.rivalPoints), value)) overrides |= OVERRIDE_RIVAL_POINTS;
        else if (strcmp(argv[index - 1], "--class") == 0 && ParseInt(value, 0, 5, &options.classIndex)) overrides |= OVERRIDE_CLASS;
        else if (strcmp(argv[index - 1], "--course") == 0 && ParseInt(value, 0, 3, &options.course)) overrides |= OVERRIDE_COURSE;
        else if (strcmp(argv[index - 1], "--car") == 0 && ParseInt(value, 0, 12, &options.car)) overrides |= OVERRIDE_CAR;
        else if (strcmp(argv[index - 1], "--start-point") == 0 && ParseInt(value, 0, INT_MAX, &options.startPoint)) overrides |= OVERRIDE_START_POINT;
        else { Usage(argv[0]); return 2; }
    }
    {
        ScenarioOptions commandLine = options;
        if (!ReadScenario(scenario, &options)) return 2;
        if (overrides & OVERRIDE_MODE) CopyText(options.mode, sizeof(options.mode), commandLine.mode);
        if (overrides & OVERRIDE_SERIES) CopyText(options.series, sizeof(options.series), commandLine.series);
        if (overrides & OVERRIDE_AFTER_FINISH) CopyText(options.afterFinish, sizeof(options.afterFinish), commandLine.afterFinish);
        if (overrides & OVERRIDE_GRID) CopyText(options.grid, sizeof(options.grid), commandLine.grid);
        if (overrides & OVERRIDE_RIVAL_POINTS) CopyText(options.rivalPoints, sizeof(options.rivalPoints), commandLine.rivalPoints);
        if (overrides & OVERRIDE_CLASS) options.classIndex = commandLine.classIndex;
        if (overrides & OVERRIDE_COURSE) options.course = commandLine.course;
        if (overrides & OVERRIDE_CAR) options.car = commandLine.car;
        if (overrides & OVERRIDE_START_POINT) options.startPoint = commandLine.startPoint;
    }
    if (
        !IsOneOf(options.mode, "grand-prix", "time-attack", NULL) ||
        !IsOneOf(options.series, "grand-prix", "extra-gp", NULL) ||
        !IsOneOf(options.afterFinish, "menu", "repeat", "exit") ||
        (strcmp(options.grid, "default") != 0 &&
         !ValidateList(options.grid, -1, 12, 11, 0)) ||
        (options.rivalPoints[0] &&
         !ValidateList(options.rivalPoints, 0, 1000000, 0, 1)) ||
        (strcmp(options.mode, "time-attack") == 0 &&
         strcmp(options.series, "extra-gp") == 0)) {
        fprintf(stderr, "invalid scenario options\n");
        return 2;
    }
    if (!FileExists(binary)) {
        fprintf(stderr, "binary does not exist: %s\n", binary);
        return 2;
    }
    if (snprintf(assignments[0], sizeof(assignments[0]), "race.mode=%s", options.mode) >= (int)sizeof(assignments[0]) ||
        snprintf(assignments[1], sizeof(assignments[1]), "race.series=%s", options.series) >= (int)sizeof(assignments[1]) ||
        snprintf(assignments[2], sizeof(assignments[2]), "race.class=%d", options.classIndex) >= (int)sizeof(assignments[2]) ||
        snprintf(assignments[3], sizeof(assignments[3]), "race.course=%d", options.course) >= (int)sizeof(assignments[3]) ||
        snprintf(assignments[4], sizeof(assignments[4]), "race.car=%d", options.car) >= (int)sizeof(assignments[4]) ||
        snprintf(assignments[5], sizeof(assignments[5]), "race.after_finish=%s", options.afterFinish) >= (int)sizeof(assignments[5]) ||
        snprintf(assignments[6], sizeof(assignments[6]), "race.grid=%s", options.grid) >= (int)sizeof(assignments[6])) {
        return 2;
    }
    command[argument++] = (char *)binary;
    command[argument++] = "--scenario";
    command[argument++] = (char *)scenario;
    for (index = 0; index < 7; ++index) {
        command[argument++] = "--set";
        command[argument++] = assignments[index];
    }
    if (options.startPoint >= 0) {
        snprintf(assignments[7], sizeof(assignments[7]), "start.player_track_point=%d", options.startPoint);
        command[argument++] = "--set";
        command[argument++] = assignments[7];
    }
    if (options.rivalPoints[0]) {
        snprintf(assignments[8], sizeof(assignments[8]),
                 "start.rival_track_points=%s", options.rivalPoints);
        command[argument++] = "--set";
        command[argument++] = assignments[8];
    }
    command[argument] = NULL;
    printf("Rage Racer scenario: mode=%s series=%s class=%d course=%d car=%d grid=%s after_finish=%s\n",
           options.mode, options.series, options.classIndex, options.course,
           options.car, options.grid, options.afterFinish);
    if (dryRun) return 0;
    if (!Launch(command)) {
        fprintf(stderr, "scenario launch failed: %s\n", binary);
        return 1;
    }
    return 0;
}
