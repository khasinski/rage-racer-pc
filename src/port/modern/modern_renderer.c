#include "modern_renderer.h"

#include <psyz/overlay_sdl3_gpu.h>
#include <psyz/present_sdl3_gpu.h>

#include <stdio.h>

static int s_enabled;
static SDL_Window *s_window;
static SDL_GPUDevice *s_device;
static PsyzOverlayInitCB_SDL3GPU s_prev_overlay_init;
static PsyzPresentSourceCB_SDL3GPU s_prev_present_source;

static void ModernOverlayInit(SDL_Window *window, SDL_GPUDevice *device) {
    s_window = window;
    s_device = device;
    if (s_prev_overlay_init) {
        s_prev_overlay_init(window, device);
    }
}

static void ModernPresentSource(PsyzPresentSourceInfo *info) {
    /* R0: the modern renderer has no image of its own yet; leaving
     * info->texture NULL presents the compat PS1 VRAM as usual. */
    if (s_prev_present_source) {
        s_prev_present_source(info);
    }
}

int RageModernInit(const RagePortConfig *config) {
    if (config->renderer != RAGE_RENDERER_MODERN) {
        return 1;
    }
    if (s_enabled) {
        return 1;
    }
    s_prev_overlay_init = Psyz_OverlayInit_SDL3GPU(ModernOverlayInit);
    s_prev_present_source = Psyz_PresentSource_SDL3GPU(ModernPresentSource);
    s_enabled = 1;
    fprintf(stderr, "rage-port: modern renderer enabled (presenting compat image until R2)\n");
    return 1;
}

void RageModernShutdown(void) {
    if (!s_enabled) {
        return;
    }
    Psyz_PresentSource_SDL3GPU(s_prev_present_source);
    Psyz_OverlayInit_SDL3GPU(s_prev_overlay_init);
    s_prev_present_source = NULL;
    s_prev_overlay_init = NULL;
    s_window = NULL;
    s_device = NULL;
    s_enabled = 0;
}

int RageModernIsEnabled(void) {
    return s_enabled;
}
