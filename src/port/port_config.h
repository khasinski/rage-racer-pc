#ifndef RAGE_PORT_CONFIG_H
#define RAGE_PORT_CONFIG_H

/* Host-side port configuration, read from rage-port.cfg at startup.
 * Selects the presented renderer and the modern renderer's options.
 * The compat renderer always runs; these settings never affect game
 * behaviour, only presentation. */

typedef enum RageRendererKind {
    RAGE_RENDERER_COMPAT = 0,
    RAGE_RENDERER_MODERN = 1
} RageRendererKind;

typedef enum RageModernAspect {
    RAGE_MODERN_ASPECT_AUTO = 0,
    RAGE_MODERN_ASPECT_4_3 = 1,
    RAGE_MODERN_ASPECT_16_9 = 2
} RageModernAspect;

/* modernFps: RAGE_MODERN_FPS_LOGIC renders one frame per logic frame,
 * RAGE_MODERN_FPS_VSYNC follows the display, a positive value is an
 * explicit target. */
#define RAGE_MODERN_FPS_LOGIC 0
#define RAGE_MODERN_FPS_VSYNC (-1)

typedef struct RagePortConfig {
    RageRendererKind renderer;
    float modernInternalScale;  /* multiplier of 320x240, default 2 */
    RageModernAspect modernAspect;
    int modernFps;
    float modernDrawDistance;   /* multiplier, default 1 */
    int modernTextureFilterLinear; /* 0 nearest, 1 linear */
} RagePortConfig;

void RagePortConfigDefaults(RagePortConfig *config);
/* Returns the number of recognized settings applied; 0 when the file is
 * missing or contains nothing usable. Unknown keys are ignored. */
int RagePortConfigLoad(RagePortConfig *config, const char *path);

/* Publish/read the process-wide active configuration. */
void RagePortConfigSetActive(const RagePortConfig *config);
const RagePortConfig *RagePortActiveConfig(void);

#endif
