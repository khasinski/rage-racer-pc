#include "runtime_config.h"

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

int RageRuntimeConfigInit(int argc, char **argv) {
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
    else if (RagePlatformFindConfigFile(argc > 0 ? argv[0] : NULL,
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

const char *RageRuntimeConfigGet(const char *key) {
    int index;
    for (index = s_valueCount - 1; index >= 0; index--)
        if (!strcmp(s_values[index].key, key)) return s_values[index].value;
    return NULL;
}

const char *RageRuntimeConfigGetLegacy(const char *key, const char *legacyEnv) {
    const char *value = RageRuntimeConfigGet(key);
    return value ? value : (legacyEnv ? getenv(legacyEnv) : NULL);
}

const char *RageRuntimeConfigGetOverride(const char *key,
                                         const char *overrideEnv) {
    const char *value = overrideEnv ? getenv(overrideEnv) : NULL;
    return value ? value : RageRuntimeConfigGet(key);
}

int RageRuntimeConfigEnabled(const char *key, const char *legacyEnv) {
    const char *value = RageRuntimeConfigGetLegacy(key, legacyEnv);
    if (!value) return 0;
    char normalized[16];
    size_t index, length = strlen(value);
    if (length >= sizeof(normalized)) return 1;
    for (index = 0; index <= length; index++)
        normalized[index] = (char)tolower((unsigned char)value[index]);
    return strcmp(normalized, "0") && strcmp(normalized, "false") &&
           strcmp(normalized, "off") && strcmp(normalized, "no");
}
