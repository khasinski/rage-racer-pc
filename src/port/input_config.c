#include "input_config.h"

#include <ctype.h>
#include "runtime_config.h"
#include <stdio.h>
#include <string.h>

static const char *const button_names[RAGE_INPUT_BUTTON_COUNT] = {
    "L2", "R2", "L1", "R1", "TRIANGLE", "CIRCLE", "CROSS", "SQUARE",
    "SELECT", "L3", "R3", "START", "UP", "RIGHT", "DOWN", "LEFT"
};

static const char *const default_keys[RAGE_INPUT_BUTTON_COUNT] = {
    "W", "E", "Q", "R", "S", "D", "X", "Z",
    "Backspace", "1", "2", "Return", "Up", "Right", "Down", "Left"
};

static char *Trim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

void InputConfigDefaults(RageInputConfig *config) {
    int i;
    for (i = 0; i < RAGE_INPUT_BUTTON_COUNT; i++) {
        snprintf(config->keys[i], sizeof(config->keys[i]), "%s", default_keys[i]);
    }
}

int InputButtonIndex(const char *name) {
    int i;
    for (i = 0; i < RAGE_INPUT_BUTTON_COUNT; i++) {
        if (strcmp(name, button_names[i]) == 0) return i;
    }
    return -1;
}

int InputConfigLoad(RageInputConfig *config, const char *path) {
    FILE *file = fopen(path, "r");
    char line[256];
    int loaded = 0;
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        char *equals;
        char *name = Trim(line);
        char *key;
        int index;
        if (*name == '\0' || *name == '#' || *name == ';') continue;
        equals = strchr(name, '=');
        if (!equals) continue;
        *equals = '\0';
        key = Trim(equals + 1);
        name = Trim(name);
        index = InputButtonIndex(name);
        if (index < 0 || *key == '\0' || strlen(key) > RAGE_INPUT_KEY_NAME_MAX) continue;
        snprintf(config->keys[index], sizeof(config->keys[index]), "%s", key);
        loaded++;
    }
    fclose(file);
    return loaded;
}

/* Key bindings live in the [input] section of the runtime configuration, one
 * entry per PlayStation button under its lower-case name. Applied over
 * whatever came before, so the runtime configuration is the last word. */
int InputConfigApplyRuntime(RageInputConfig *config) {
    char key[32];
    int index, applied = 0;
    for (index = 0; index < RAGE_INPUT_BUTTON_COUNT; index++) {
        const char *value;
        size_t at;
        snprintf(key, sizeof(key), "input.%s", button_names[index]);
        for (at = 6; key[at] != '\0'; at++)
            key[at] = (char)tolower((unsigned char)key[at]);
        value = RuntimeConfigGet(key);
        if (value == NULL || *value == '\0' ||
            strlen(value) > RAGE_INPUT_KEY_NAME_MAX)
            continue;
        snprintf(config->keys[index], sizeof(config->keys[index]), "%s", value);
        applied++;
    }
    return applied;
}
