#include "user_settings.h"
#include <SDL3/SDL.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int UserSettingsSave(const char *path, const RagePortConfig *c,
                     const PortSessionOptions *s) {
    char contents[4096], parent[4096], temporary[4200], fps[32];
    char *slash, *backslash;
    int length;
    if (!path || !*path || !c || !s || strlen(path) >= sizeof(parent)) return 0;
    SDL_strlcpy(parent, path, sizeof(parent));
    slash = strrchr(parent, '/');
    backslash = strrchr(parent, '\\');
    if (backslash && (!slash || backslash > slash)) slash = backslash;
    if (slash && slash != parent) {
        *slash = '\0';
        if (!SDL_CreateDirectory(parent)) return 0;
    }
    if (c->modernFps > 0) snprintf(fps, sizeof(fps), "%d", c->modernFps);
    else SDL_strlcpy(fps, c->modernFps == RAGE_MODERN_FPS_VSYNC ? "vsync" : "logic", sizeof(fps));
    length = snprintf(contents, sizeof(contents),
        "# Esc settings, format 1. Explicit launch options take precedence.\n"
        "[video]\nrenderer=%s\ninternal_scale=%.6g\naspect=%s\nfps=%s\n"
        "draw_distance=%.6g\ntexture_filter=%s\npost=%s\ngrading=%s\n"
        "brightness=%.6g\nssao=%s\nshadows=%s\ncar_lights=%s\nfullscreen=%s\n"
        "[input]\nanalog=%s\nwheel=%s\nsteering_linearity=%.6g\n"
        "force_feedback=%s\nffb_gain=%.6g\n"
        "[game]\ntick_rate=%s\n"
        "[hud]\nanchor=%s\nshow_lap_times=%s\nshow_time_limit=%s\n"
        "[camera]\nchase_distance=%.6g\nchase_height=%.6g\nchase_turn_lookahead=%.6g\n",
        c->renderer == RAGE_RENDERER_MODERN ? "modern" : "classic",
        c->modernInternalScale,
        c->modernAspect == RAGE_MODERN_ASPECT_16_9 ? "16:9" :
        c->modernAspect == RAGE_MODERN_ASPECT_4_3 ? "4:3" : "auto",
        fps,
        c->modernDrawDistance, c->modernTextureFilterLinear ? "linear" : "nearest",
        c->modernPost == RAGE_MODERN_POST_FXAA ? "fxaa" : "none",
        c->modernGrading ? "vibrant" : "off", c->modernBrightness,
        c->modernSsao ? "on" : "off",
        c->modernShadows == 2 ? "high" : c->modernShadows ? "standard" : "off",
        c->carLights ? "on" : "off", s->fullscreen ? "true" : "false",
        s->analog ? "true" : "false", s->wheel ? "true" : "false",
        s->steeringLinearity, s->forceFeedback ? "true" : "false",
        s->forceFeedbackGain / 100.0, s->tickMultiplier >= 2 ? "double" : "retail",
        s->hudEdges ? "edges" : "center", s->showLapTimes ? "true" : "false",
        s->showTimeLimit ? "true" : "false", s->chaseDistance, s->chaseHeight,
        s->chaseLookahead);
    if (length < 0 || (size_t)length >= sizeof(contents)) return 0;
    snprintf(temporary, sizeof(temporary), "%s.tmp.%" PRIu64, path, SDL_GetTicksNS());
    if (!SDL_SaveFile(temporary, contents, (size_t)length) ||
        !SDL_RenamePath(temporary, path)) {
        SDL_RemovePath(temporary);
        return 0;
    }
    return 1;
}
