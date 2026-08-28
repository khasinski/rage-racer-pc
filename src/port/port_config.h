#ifndef RAGE_PORT_CONFIG_H
#define RAGE_PORT_CONFIG_H

/* Host-side presentation configuration. There are exactly two renderers:
 * classic presents the PS1-compatible output, while modern presents native
 * RenderWorld 3D with the remaining captured 2D overlays. Modern never falls
 * back to captured PS1 3D. These settings affect presentation only. */

typedef enum RageRendererKind {
    RAGE_RENDERER_CLASSIC = 0,
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

typedef enum RageModernPost {
    RAGE_MODERN_POST_NONE = 0,
    RAGE_MODERN_POST_FXAA = 1
} RageModernPost;

typedef struct RagePortConfig {
    RageRendererKind renderer;
    float modernInternalScale;  /* multiplier of 320x240, default 2 */
    RageModernAspect modernAspect;
    int modernFps;
    float modernDrawDistance;   /* multiplier, default 1 */
    int modernTextureFilterLinear; /* 0 nearest, 1 linear */
    RageModernPost modernPost;
    int modernGrading;          /* 0 off, 1 vibrant */
} RagePortConfig;

void PortConfigDefaults(RagePortConfig *config);
/* Applies [video] values loaded by runtime_config. */
int PortConfigApplyRuntime(RagePortConfig *config);

/* Publish/read the process-wide active configuration. */
void PortConfigSetActive(const RagePortConfig *config);
const RagePortConfig *PortActiveConfig(void);

#endif
