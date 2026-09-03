#include "port_config.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime_config.h"

static char *Trim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

void PortConfigDefaults(RagePortConfig *config) {
    if (config == NULL) return;
    config->renderer = RAGE_RENDERER_CLASSIC;
    config->modernInternalScale = 2.0f;
    config->modernAspect = RAGE_MODERN_ASPECT_AUTO;
    config->modernFps = RAGE_MODERN_FPS_LOGIC;
    config->modernDrawDistance = 1.0f;
    config->modernTextureFilterLinear = 0;
    config->modernPost = RAGE_MODERN_POST_NONE;
    config->modernGrading = 0;
}

static int ParseFloat(const char *value, float *out, float min, float max) {
    char *end;
    float parsed = strtof(value, &end);
    if (end == value || *Trim(end) != '\0') return 0;
    if (!isfinite(parsed) || parsed < min || parsed > max) return 0;
    *out = parsed;
    return 1;
}

static int ApplySetting(RagePortConfig *config, const char *name,
                        const char *value) {
    if (strcmp(name, "renderer") == 0) {
        if (strcmp(value, "compat") == 0 || strcmp(value, "classic") == 0) {
            config->renderer = RAGE_RENDERER_CLASSIC;
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
        int parsed;

        if (strcmp(value, "logic") == 0) {
            config->modernFps = RAGE_MODERN_FPS_LOGIC;
            return 1;
        }
        if (strcmp(value, "vsync") == 0) {
            config->modernFps = RAGE_MODERN_FPS_VSYNC;
            return 1;
        }
        if (!RuntimeParseInt(value, 10, 1, 1000, &parsed)) return 0;
        config->modernFps = parsed;
        return 1;
    }
    if (strcmp(name, "modern.draw_distance") == 0) {
        return ParseFloat(value, &config->modernDrawDistance, 0.0f, 16.0f);
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
        /* Accepted as a no-op so old configuration files keep loading. The
         * effect was removed; a future bloom will have a new implementation. */
        (void)value;
        return 1;
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

int PortConfigApplyRuntime(RagePortConfig *config) {
    static const struct { const char *runtimeKey, *legacyKey; } keys[] = {
        {"video.renderer", "renderer"},
        {"video.internal_scale", "modern.internal_scale"},
        {"video.aspect", "modern.aspect"},
        {"video.fps", "modern.fps"},
        {"video.draw_distance", "modern.draw_distance"},
        {"video.texture_filter", "modern.texture_filter"},
        {"video.post", "modern.post"},
        {"video.bloom", "modern.bloom"},
        {"video.grading", "modern.grading"}
    };
    int index, applied = 0;

    if (config == NULL) return 0;
    for (index = 0; index < (int)(sizeof(keys) / sizeof(keys[0])); index++) {
        const char *value = RuntimeConfigGet(keys[index].runtimeKey);
        if (value) applied += ApplySetting(config, keys[index].legacyKey, value);
    }
    return applied;
}

static RagePortConfig active_config = {
    .renderer = RAGE_RENDERER_CLASSIC,
    .modernInternalScale = 2.0f,
    .modernAspect = RAGE_MODERN_ASPECT_AUTO,
    .modernFps = RAGE_MODERN_FPS_LOGIC,
    .modernDrawDistance = 1.0f,
    .modernTextureFilterLinear = 0,
    .modernPost = RAGE_MODERN_POST_NONE,
    .modernGrading = 0,
};

void PortConfigSetActive(const RagePortConfig *config) {
    if (config == NULL) return;
    active_config = *config;
}

const RagePortConfig *PortActiveConfig(void) {
    return &active_config;
}

/* The rear-view mirror scans a third of the main view's depth: retail passes
 * 0x6000 to BuildVisibleCells where the road ahead gets 0x14000, which is why
 * traffic drops out of the mirror while it is still plainly behind the car.
 * Scale that reach without touching the retail default. */
int PortMirrorFarDepth(int retailFar) {
    static float multiplier = -1.0f;
    double scaled;
    if (multiplier < 0.0f) {
        const char *text = RuntimeConfigGet("modern.mirror_distance");
        multiplier = 1.0f;
        if (text != NULL && text[0] != '\0') {
            float parsed = 1.0f;
            if (ParseFloat(text, &parsed, 0.25f, 8.0f)) {
                multiplier = parsed;
            } else {
                fprintf(stderr,
                        "rage-port: ignoring invalid modern.mirror_distance=%s (expected 0.25..8)\n",
                        text);
            }
        }
        if (multiplier != 1.0f)
            fprintf(stderr, "rage-port: mirror draw distance x%.2f\n", multiplier);
    }
    scaled = (double)retailFar * multiplier;
    if (scaled < 0x1000) scaled = 0x1000;
    if (scaled > 0x14000) scaled = 0x14000;
    return (int)scaled;
}
