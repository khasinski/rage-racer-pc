
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform_paths.h"

enum { RAGE_RUNTIME_MAX_VALUES = 192, RAGE_RUNTIME_KEY_MAX = 95,
       RAGE_RUNTIME_VALUE_MAX = 1023 };

typedef struct RageRuntimeValue {
    char key[RAGE_RUNTIME_KEY_MAX + 1];
    char value[RAGE_RUNTIME_VALUE_MAX + 1];
} RageRuntimeValue;

static RageRuntimeValue s_values[RAGE_RUNTIME_MAX_VALUES];
static int s_valueCount;

static char *Trim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

static int Store(const char *key, const char *value) {
    int index;
    if (!key || !*key || !value || strlen(key) > RAGE_RUNTIME_KEY_MAX ||
        strlen(value) > RAGE_RUNTIME_VALUE_MAX) return 0;
    for (index = 0; index < s_valueCount; index++) {
        if (strcmp(s_values[index].key, key) == 0) break;
    }
    if (index == s_valueCount) {
        if (s_valueCount == RAGE_RUNTIME_MAX_VALUES) return 0;
        strcpy(s_values[index].key, key);
        s_valueCount++;
    }
    strcpy(s_values[index].value, value);
    return 1;
}

static int LoadIni(const char *path) {
    FILE *file = fopen(path, "r");
    char line[1400], section[64] = "";
    int applied = 0, lineNumber = 0;
    if (!file) {
        fprintf(stderr, "rage-port: cannot open configuration %s\n", path);
        return -1;
    }
    while (fgets(line, sizeof(line), file)) {
        int complete = strchr(line, '\n') != NULL || feof(file);
        char key[128], *text, *equals, *end;
        lineNumber++;
        if (!complete) {
            int character;
            while ((character = fgetc(file)) != '\n' && character != EOF) {}
            fprintf(stderr, "rage-port: %s:%d: line too long\n",
                    path, lineNumber);
            continue;
        }
        text = Trim(line);
        if (!*text || *text == '#' || *text == ';') continue;
        if (*text == '[' && (end = strchr(text + 1, ']')) != NULL) {
            *end = '\0';
            text = Trim(text + 1);
            if (strlen(text) < sizeof(section)) strcpy(section, text);
            else {
                section[0] = '\0';
                fprintf(stderr, "rage-port: %s:%d: section name too long\n",
                        path, lineNumber);
            }
            continue;
        }
        equals = strchr(text, '=');
        if (!equals) {
            fprintf(stderr, "rage-port: %s:%d: expected key=value\n",
                    path, lineNumber);
            continue;
        }
        *equals = '\0';
        text = Trim(text);
        equals = Trim(equals + 1);
        if (*section) snprintf(key, sizeof(key), "%s.%s", section, text);
        else snprintf(key, sizeof(key), "%s", text);
        if (!Store(key, equals))
            fprintf(stderr, "rage-port: %s:%d: setting is too long or capacity is exhausted\n",
                    path, lineNumber);
        else applied++;
    }
    fclose(file);
    return applied;
}

int RuntimeConfigInit(int argc, char **argv) {
    int index, valid = 1;
    const char *configPath = NULL, *scenarioPath = NULL;
    char defaultPath[RAGE_RUNTIME_VALUE_MAX + 1];
    s_valueCount = 0;
    for (index = 1; index < argc; index++) {
        if (!strcmp(argv[index], "--config") ||
            !strcmp(argv[index], "--scenario")) {
            if (index + 1 >= argc) {
                fprintf(stderr, "rage-port: %s requires a path\n", argv[index]);
                valid = 0;
            } else if (!strcmp(argv[index], "--config"))
                configPath = argv[++index];
            else scenarioPath = argv[++index];
        }
    }
    if (!configPath) configPath = getenv("RAGE_CONFIG");
    if (!scenarioPath) scenarioPath = getenv("RAGE_TEST_SCENARIO");
    if (configPath && *configPath) {
        if (LoadIni(configPath) < 0) valid = 0;
    }
    else if (PlatformFindConfigFile(argc > 0 ? argv[0] : NULL,
                                        "rage-port.ini", defaultPath,
                                        sizeof(defaultPath)))
        LoadIni(defaultPath);
    if (scenarioPath && *scenarioPath) {
        if (LoadIni(scenarioPath) < 0) valid = 0;
        Store("race.enabled", "true");
    }
    for (index = 1; index < argc; index++) {
        if (!strcmp(argv[index], "--set") && index + 1 < argc) {
            char copy[1200], *equals;
            snprintf(copy, sizeof(copy), "%s", argv[++index]);
            equals = strchr(copy, '=');
            if (equals) {
                *equals = '\0';
                Store(Trim(copy), Trim(equals + 1));
            } else {
                fprintf(stderr, "rage-port: --set expects key=value\n");
                valid = 0;
            }
        } else if (!strcmp(argv[index], "--set")) {
            fprintf(stderr, "rage-port: --set requires key=value\n");
            valid = 0;
        }
    }
    return valid;
}


/*
 * Every setting can also come from the environment, which is how the tests and
 * the release scripts drive the game. The name is derived from the key:
 * race.class is RAGE_PORT_RACE_CLASS. These settings were given their
 * environment names before that rule existed, so they keep the names that are
 * documented and scripted against; this table is the only place that knows.
 *
 * A new setting needs no entry. It gets the derived name for free.
 */
typedef struct EnvironmentAlias {
    const char *key;
    const char *name;
} EnvironmentAlias;

static const EnvironmentAlias s_environmentAliases[] = {
    {"capture.all_phases", "RAGE_PORT_SMOKE_CAPTURE_ALL_PHASES"},
    {"capture.directory", "RAGE_PORT_SMOKE_CAPTURE_DIR"},
    {"capture.scene", "RAGE_PORT_SMOKE_CAPTURE_SCENE"},
    {"capture.timer_max", "RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX"},
    {"capture.timer_min", "RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN"},
    {"capture.timer_stride", "RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE"},
    {"capture.visible_cells", "RAGE_PORT_SMOKE_VISIBLE_CELLS"},
    {"checks.complete_save_load", "RAGE_PORT_SMOKE_COMPLETE_SAVE_LOAD"},
    {"checks.save_roundtrip", "RAGE_PORT_SMOKE_SAVE_ROUNDTRIP"},
    {"diagnostics.car.collision_trace", "RAGE_PORT_CAR_COLLISION_TRACE"},
    {"diagnostics.car.collision_trace_timer", "RAGE_PORT_CAR_COLLISION_TRACE_TIMER"},
    {"diagnostics.car.knockback_trace", "RAGE_PORT_CAR_KNOCKBACK_TRACE"},
    {"diagnostics.car.knockback_trace_timer", "RAGE_PORT_CAR_KNOCKBACK_TRACE_TIMER"},
    {"diagnostics.car.knockback_trace_timer_max", "RAGE_PORT_CAR_KNOCKBACK_TRACE_TIMER_MAX"},
    {"diagnostics.car.knockback_trace_timer_min", "RAGE_PORT_CAR_KNOCKBACK_TRACE_TIMER_MIN"},
    {"diagnostics.car.motion_trace", "RAGE_PORT_CAR_MOTION_TRACE"},
    {"diagnostics.car.motion_trace_timer", "RAGE_PORT_CAR_MOTION_TRACE_TIMER"},
    {"diagnostics.car.state_trace", "RAGE_PORT_CAR_STATE_TRACE"},
    {"diagnostics.car.state_trace_timer_max", "RAGE_PORT_CAR_STATE_TRACE_TIMER_MAX"},
    {"diagnostics.car.state_trace_timer_min", "RAGE_PORT_CAR_STATE_TRACE_TIMER_MIN"},
    {"diagnostics.car.track_trace", "RAGE_PORT_CAR_TRACK_TRACE"},
    {"diagnostics.car.track_trace_timer", "RAGE_PORT_CAR_TRACK_TRACE_TIMER"},
    {"diagnostics.car.track_trace_timer_max", "RAGE_PORT_CAR_TRACK_TRACE_TIMER_MAX"},
    {"diagnostics.car.track_trace_timer_min", "RAGE_PORT_CAR_TRACK_TRACE_TIMER_MIN"},
    {"diagnostics.course_trace_clut", "RAGE_PORT_COURSE_TRACE_CLUT"},
    {"diagnostics.course_trace_timer", "RAGE_PORT_COURSE_TRACE_TIMER"},
    {"diagnostics.course_trace_tpage", "RAGE_PORT_COURSE_TRACE_TPAGE"},
    {"diagnostics.fmv_trace", "RAGE_PORT_FMV_TRACE"},
    {"diagnostics.marker_capture", "RAGE_PORT_MARKER_CAPTURE"},
    {"diagnostics.marker_frame", "RAGE_PORT_MARKER_FRAME"},
    {"diagnostics.modern_asset_trace", "RAGE_PORT_MODERN_ASSET_TRACE"},
    {"diagnostics.input.debug", "RAGE_PORT_INPUT_DEBUG"},
    {"diagnostics.log", "RAGE_PORT_LOG_PATH"},
    {"diagnostics.model_trace", "RAGE_PORT_MODEL_TRACE"},
    {"diagnostics.model_trace_timer", "RAGE_PORT_MODEL_TRACE_TIMER"},
    {"diagnostics.modern_dump", "RAGE_PORT_MODERN_DUMP"},
    {"diagnostics.modern_dump_every", "RAGE_PORT_MODERN_DUMP_EVERY"},
    {"diagnostics.modern_dump_frame", "RAGE_PORT_MODERN_DUMP_FRAME"},
    {"diagnostics.modern_dump_scene", "RAGE_PORT_MODERN_DUMP_SCENE"},
    {"diagnostics.modern_span_trace", "RAGE_PORT_MODERN_SPAN_TRACE"},
    {"diagnostics.random.trace", "RAGE_PORT_RANDOM_TRACE"},
    {"diagnostics.render.car_draw_trace", "RAGE_PORT_CAR_DRAW_TRACE"},
    {"diagnostics.render.car_draw_trace_timer", "RAGE_PORT_CAR_DRAW_TRACE_TIMER"},
    {"diagnostics.render.tachometer_trace", "RAGE_PORT_TACHO_TRACE"},
    {"diagnostics.scene_trace", "RAGE_PORT_SCENE_TRACE"},
    {"diagnostics.scene_trace_verbose", "RAGE_PORT_SCENE_TRACE_VERBOSE"},
    {"diagnostics.terrain_decision_limit", "RAGE_PORT_TERRAIN_DECISION_LIMIT"},
    {"diagnostics.terrain_decision_timer", "RAGE_PORT_TERRAIN_DECISION_TIMER"},
    {"diagnostics.terrain_decision_trace", "RAGE_PORT_TERRAIN_DECISION_TRACE"},
    {"diagnostics.terrain_trace_clut", "RAGE_PORT_TERRAIN_TRACE_CLUT"},
    {"diagnostics.terrain_trace_timer", "RAGE_PORT_TERRAIN_TRACE_TIMER"},
    {"diagnostics.terrain_trace_tpage", "RAGE_PORT_TERRAIN_TRACE_TPAGE"},
    {"diagnostics.test_log", "RAGE_PORT_TEST_LOG"},
    {"disc.choose", "RAGE_PORT_CHOOSE_DISC"},
    {"hooks.auto_confirm_frame", "RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME"},
    {"hooks.finish_frame", "RAGE_PORT_SMOKE_FINISH_FRAME"},
    {"hooks.menu_sweep", "RAGE_PORT_SMOKE_MENU_SWEEP"},
    {"hooks.mirror_track", "RAGE_PORT_SMOKE_MIRROR_TRACK"},
    {"hooks.option_sweep", "RAGE_PORT_SMOKE_OPTION_SWEEP"},
    {"hooks.play_fmv", "RAGE_PORT_SMOKE_PLAY_FMV"},
    {"hooks.retire", "RAGE_PORT_SMOKE_RETIRE"},
    {"input.disable_host", "RAGE_PORT_DISABLE_HOST_INPUT"},
    {"input.raw_script", "RAGE_PORT_RAW_INPUT_SCRIPT"},
    {"input.state_script", "RAGE_PORT_STATE_INPUT_SCRIPT"},
    {"modern.mirror_distance", "RAGE_PORT_MIRROR_DISTANCE"},
    {"race.car", "RAGE_PORT_SCENARIO_CAR"},
    {"race.class", "RAGE_PORT_SCENARIO_CLASS"},
    {"race.course", "RAGE_PORT_SCENARIO_COURSE"},
    {"race.mode", "RAGE_PORT_SCENARIO_MODE"},
    {"race.series", "RAGE_PORT_SCENARIO_SERIES"},
    {"diagnostics.sound_cue_trace", "RAGE_PORT_SOUND_CUE_TRACE"},
    {"race.enabled", "RAGE_PORT_SCENARIO"},
    {"race.grid", "RAGE_PORT_SCENARIO_GRID"},
    {"report.audio_metrics", "RAGE_PORT_SMOKE_AUDIO_METRICS"},
    {"report.camera_state", "RAGE_PORT_SMOKE_CAMERA_STATE"},
    {"report.initial_state", "RAGE_PORT_SMOKE_INITIAL_STATE"},
    {"report.window_size", "RAGE_PORT_SMOKE_WINDOW_SIZE"},
    {"run.frames", "RAGE_PORT_SMOKE_FRAMES"},
    {"runtime.test_mode", "RAGE_PORT_TEST_MODE"},
    {"stop.scene", "RAGE_PORT_SMOKE_STOP_SCENE"},
    {"stop.timer", "RAGE_PORT_SMOKE_STOP_SCENE_TIMER"},
    {"trace.spu", "RAGE_PORT_SPU_TRACE"},
    {"video.modern", "RAGE_PORT_MODERN"},
};

/* RAGE_PORT_ plus the key, dots to underscores, upper case. */
static const char *DerivedName(const char *key, char *buffer, size_t size) {
    size_t at = sizeof("RAGE_PORT_") - 1;
    if (size < sizeof("RAGE_PORT_")) return NULL;
    memcpy(buffer, "RAGE_PORT_", at);
    for (; *key != '\0' && at + 1 < size; key++)
        buffer[at++] = *key == '.' ? '_' : (char)toupper((unsigned char)*key);
    buffer[at] = '\0';
    return buffer;
}

/* An aliased setting answers to both names: the documented one that existing
 * scripts pass, and the one the rule gives, which is the one somebody reading
 * the key would guess. */
static const char *EnvironmentValue(const char *key) {
    char derived[192];
    const char *name;
    size_t index;
    for (index = 0; index < sizeof(s_environmentAliases) / sizeof(s_environmentAliases[0]); index++) {
        if (!strcmp(s_environmentAliases[index].key, key)) {
            const char *value = getenv(s_environmentAliases[index].name);
            if (value != NULL) return value;
            break;
        }
    }
    name = DerivedName(key, derived, sizeof(derived));
    return name != NULL ? getenv(name) : NULL;
}

static const char *ConfiguredValue(const char *key) {
    int index;
    for (index = s_valueCount - 1; index >= 0; index--)
        if (!strcmp(s_values[index].key, key)) return s_values[index].value;
    return NULL;
}

/* A file or --set wins over the environment, which is the ambient default. */
const char *RuntimeConfigGet(const char *key) {
    const char *value = ConfiguredValue(key);
    return value ? value : EnvironmentValue(key);
}

/* The other way round, for the few settings an outer harness has to be able to
 * force regardless of the shipped file: the log path and the disc image. */
const char *RuntimeConfigGetForced(const char *key) {
    const char *value = EnvironmentValue(key);
    return value ? value : ConfiguredValue(key);
}

int RuntimeConfigEnabled(const char *key) {
    const char *value = RuntimeConfigGet(key);
    if (!value) return 0;
    char normalized[16];
    size_t index, length = strlen(value);
    if (length >= sizeof(normalized)) return 1;
    for (index = 0; index <= length; index++)
        normalized[index] = (char)tolower((unsigned char)value[index]);
    return strcmp(normalized, "0") && strcmp(normalized, "false") &&
           strcmp(normalized, "off") && strcmp(normalized, "no");
}
