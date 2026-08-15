#include "port_config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *Trim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

void RagePortConfigDefaults(RagePortConfig *config) {
    config->renderer = RAGE_RENDERER_COMPAT;
    config->modernInternalScale = 2.0f;
    config->modernAspect = RAGE_MODERN_ASPECT_AUTO;
    config->modernFps = RAGE_MODERN_FPS_LOGIC;
    config->modernDrawDistance = 1.0f;
    config->modernTextureFilterLinear = 0;
    config->modernPost = RAGE_MODERN_POST_NONE;
    config->modernBloom = 0.0f;
    config->modernGrading = 0;
}

static int ParseFloat(const char *value, float *out, float min, float max) {
    char *end;
    float parsed = strtof(value, &end);
    if (end == value || *Trim(end) != '\0') return 0;
    if (parsed < min || parsed > max) return 0;
    *out = parsed;
    return 1;
}

static int ApplySetting(RagePortConfig *config, const char *name,
                        const char *value) {
    if (strcmp(name, "renderer") == 0) {
        if (strcmp(value, "compat") == 0) {
            config->renderer = RAGE_RENDERER_COMPAT;
            return 1;
        }
        if (strcmp(value, "modern") == 0) {
            config->renderer = RAGE_RENDERER_MODERN;
            return 1;
        }
        return 0;
    }
    if (strcmp(name, "modern.internal_scale") == 0) {
        return ParseFloat(value, &config->modernInternalScale, 0.5f, 16.0f);
    }
    if (strcmp(name, "modern.aspect") == 0) {
        if (strcmp(value, "auto") == 0) {
            config->modernAspect = RAGE_MODERN_ASPECT_AUTO;
            return 1;
        }
        if (strcmp(value, "4:3") == 0) {
            config->modernAspect = RAGE_MODERN_ASPECT_4_3;
            return 1;
        }
        if (strcmp(value, "16:9") == 0) {
            config->modernAspect = RAGE_MODERN_ASPECT_16_9;
            return 1;
        }
        return 0;
    }
    if (strcmp(name, "modern.fps") == 0) {
        char *end;
        long parsed;
        if (strcmp(value, "logic") == 0) {
            config->modernFps = RAGE_MODERN_FPS_LOGIC;
            return 1;
        }
        if (strcmp(value, "vsync") == 0) {
            config->modernFps = RAGE_MODERN_FPS_VSYNC;
            return 1;
        }
        parsed = strtol(value, &end, 10);
        if (end == value || *end != '\0' || parsed < 1 || parsed > 1000) {
            return 0;
        }
        config->modernFps = (int)parsed;
        return 1;
    }
    if (strcmp(name, "modern.draw_distance") == 0) {
        return ParseFloat(value, &config->modernDrawDistance, 0.5f, 16.0f);
    }
    if (strcmp(name, "modern.post") == 0) {
        if (strcmp(value, "none") == 0) {
            config->modernPost = RAGE_MODERN_POST_NONE;
            return 1;
        }
        if (strcmp(value, "fxaa") == 0) {
            config->modernPost = RAGE_MODERN_POST_FXAA;
            return 1;
        }
        return 0;
    }
    if (strcmp(name, "modern.bloom") == 0) {
        if (strcmp(value, "off") == 0) {
            config->modernBloom = 0.0f;
            return 1;
        }
        if (strcmp(value, "on") == 0) {
            config->modernBloom = 0.6f;
            return 1;
        }
        return ParseFloat(value, &config->modernBloom, 0.0f, 2.0f);
    }
    if (strcmp(name, "modern.grading") == 0) {
        if (strcmp(value, "off") == 0) {
            config->modernGrading = 0;
            return 1;
        }
        if (strcmp(value, "vibrant") == 0) {
            config->modernGrading = 1;
            return 1;
        }
        return 0;
    }
    if (strcmp(name, "modern.texture_filter") == 0) {
        if (strcmp(value, "nearest") == 0) {
            config->modernTextureFilterLinear = 0;
            return 1;
        }
        if (strcmp(value, "linear") == 0) {
            config->modernTextureFilterLinear = 1;
            return 1;
        }
        return 0;
    }
    return 0;
}

int RagePortConfigLoad(RagePortConfig *config, const char *path) {
    FILE *file = fopen(path, "r");
    char line[256];
    int loaded = 0;
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        char *equals;
        char *name = Trim(line);
        char *value;
        if (*name == '\0' || *name == '#' || *name == ';') continue;
        equals = strchr(name, '=');
        if (!equals) continue;
        *equals = '\0';
        value = Trim(equals + 1);
        name = Trim(name);
        if (*name == '\0' || *value == '\0') continue;
        loaded += ApplySetting(config, name, value);
    }
    fclose(file);
    return loaded;
}

static RagePortConfig active_config = {
    RAGE_RENDERER_COMPAT, 2.0f, RAGE_MODERN_ASPECT_AUTO,
    RAGE_MODERN_FPS_LOGIC, 1.0f, 0, RAGE_MODERN_POST_NONE,
    0.0f, 0
};

void RagePortConfigSetActive(const RagePortConfig *config) {
    active_config = *config;
}

const RagePortConfig *RagePortActiveConfig(void) {
    return &active_config;
}
