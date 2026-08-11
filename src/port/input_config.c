#include "input_config.h"

#include <ctype.h>
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

void RageInputConfigDefaults(RageInputConfig *config) {
    int i;
    for (i = 0; i < RAGE_INPUT_BUTTON_COUNT; i++) {
        snprintf(config->keys[i], sizeof(config->keys[i]), "%s", default_keys[i]);
    }
}

int RageInputButtonIndex(const char *name) {
    int i;
    for (i = 0; i < RAGE_INPUT_BUTTON_COUNT; i++) {
        if (strcmp(name, button_names[i]) == 0) return i;
    }
    return -1;
}

int RageInputConfigLoad(RageInputConfig *config, const char *path) {
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
        index = RageInputButtonIndex(name);
        if (index < 0 || *key == '\0' || strlen(key) > RAGE_INPUT_KEY_NAME_MAX) continue;
        snprintf(config->keys[index], sizeof(config->keys[index]), "%s", key);
        loaded++;
    }
    fclose(file);
    return loaded;
}
