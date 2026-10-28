#include "graphics_overlay.h"

#include <SDL3/SDL.h>
#include <psyz/overlay_sdl3.h>
#include <psyz/overlay_sdl3_gpu.h>
#include <psyz/present_sdl3_gpu.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "modern/modern_renderer.h"
#include "modern/shaders/overlay_frag_msl.h"
#include "modern/shaders/overlay_frag_spv.h"
#include "modern/shaders/overlay_vert_msl.h"
#include "modern/shaders/overlay_vert_spv.h"
#include "overlay_model.h"
#include "overlay_raster.h"
#include "touch_model.h"
#include "port_config.h"
#include "runtime_config.h"
#include "session_options.h"

/* The SDL side of the settings panel: events become model actions, and the
 * painted panel is uploaded and drawn over the frame. The rules live in
 * overlay_model.c and the pixels in overlay_raster.c. */

static int s_installed;
static OverlayModel s_model;
static OverlaySurface s_surface;
static OverlayLayout s_layout;
static SDL_Window *s_window;
static int s_windowW, s_windowH;
static PsyzOverlayEventCB_SDL3 s_prevEvent;
static PsyzOverlayRenderCB_SDL3GPU s_prevRender;

int PortGraphicsOverlayActive(void) { return s_model.open; }

int PortGraphicsOverlayTakeClosed(void) {
    return OverlayModelTakeClosed(&s_model);
}

static unsigned NowMs(void) { return (unsigned)SDL_GetTicks(); }

/* ---- input ---- */

static void OpenPanel(void) {
    const RagePortConfig *active = PortActiveConfig();
    PortSessionOptions session;
    SDL_GPUDevice *device = NULL;
    int w = 0, h = 0;

    s_window = NULL;
    if (active == NULL ||
        !Psyz_VideoGetPresentationDevice_SDL3GPU(&s_window, &device)) {
        return;
    }
    PortSessionOptionsRead(&session);
    OverlayModelOpen(&s_model, active, &session);
    if (s_window != NULL) {
        SDL_GetWindowSizeInPixels(s_window, &w, &h);
        if (w > 0 && h > 0) {
            s_windowW = w;
            s_windowH = h;
            OverlayComputeLayout(OverlayModelRowCount(&s_model), w, h,
                                 &s_layout);
        }
    }
}

/* Keys that change a value or move between pages act once per press: each
 * step can rebuild the presentation, so holding one must not run through
 * every value. Moving the selection may repeat. */
static OverlayAction KeyAction(SDL_Scancode key, int repeat) {
    switch (key) {
    case SDL_SCANCODE_UP: return OVERLAY_UP;
    case SDL_SCANCODE_DOWN: return OVERLAY_DOWN;
    case SDL_SCANCODE_LEFT: return repeat ? OVERLAY_NONE : OVERLAY_LEFT;
    case SDL_SCANCODE_RIGHT: return repeat ? OVERLAY_NONE : OVERLAY_RIGHT;
    case SDL_SCANCODE_PAGEUP: return repeat ? OVERLAY_NONE : OVERLAY_PAGE_PREV;
    case SDL_SCANCODE_PAGEDOWN: return repeat ? OVERLAY_NONE : OVERLAY_PAGE_NEXT;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
    case SDL_SCANCODE_SPACE: return repeat ? OVERLAY_NONE : OVERLAY_ACTIVATE;
    default: return OVERLAY_NONE;
    }
}

static OverlayAction ButtonAction(SDL_GamepadButton button) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return OVERLAY_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return OVERLAY_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return OVERLAY_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return OVERLAY_RIGHT;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return OVERLAY_PAGE_PREV;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return OVERLAY_PAGE_NEXT;
    case SDL_GAMEPAD_BUTTON_SOUTH: return OVERLAY_ACTIVATE;
    case SDL_GAMEPAD_BUTTON_EAST:
    case SDL_GAMEPAD_BUTTON_START: return OVERLAY_CLOSE;
    default: return OVERLAY_NONE;
    }
}

static int TouchEnabled(void);
static void OnTouchEvent(const SDL_Event *event);

static void OnMouse(float windowX, float windowY, int click) {
    float density;
    float x, y;

    if (!s_model.open || s_layout.scale <= 0.0f) return;
    density = s_window != NULL ? SDL_GetWindowPixelDensity(s_window) : 1.0f;
    x = (windowX * density - (float)s_layout.x) / s_layout.scale;
    y = (windowY * density - (float)s_layout.y) / s_layout.scale;

    OverlayModelPointer(&s_model, x, y, click, NowMs());
}

static void OnEvent(const SDL_Event *event) {
    if (s_prevEvent != NULL) s_prevEvent(event);
    if (event->type >= SDL_EVENT_FINGER_DOWN && event->type <= SDL_EVENT_FINGER_CANCELED &&
        TouchEnabled()) {
        OnTouchEvent(event);
        return;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        SDL_Scancode key = event->key.scancode;

        if (key == SDL_SCANCODE_ESCAPE) {
            if (event->key.repeat) return;
            if (s_model.open) OverlayModelClose(&s_model);
            else OpenPanel();
        } else if (s_model.open) {
            OverlayModelDo(&s_model, KeyAction(key, event->key.repeat != 0),
                           NowMs());
        }
    } else if (!s_model.open) {
        return;
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        if (TouchEnabled() && event->motion.which == SDL_TOUCH_MOUSEID) return;
        OnMouse(event->motion.x, event->motion.y, 0);
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
               event->button.button == SDL_BUTTON_LEFT) {
        if (TouchEnabled() && event->button.which == SDL_TOUCH_MOUSEID) return;
        OnMouse(event->button.x, event->button.y, 1);
    } else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        OverlayModelDo(&s_model, ButtonAction(event->gbutton.button), NowMs());
    }
}

/* ---- GPU ---- */

typedef struct OverlayQuad {
    float rect[4];
    float uv[4];
    float tint[4];
} OverlayQuad;

static SDL_GPUDevice *s_device;
static SDL_GPUGraphicsPipeline *s_pipeline;
static SDL_GPUSampler *s_sampler;
static SDL_GPUTexture *s_texture;
static int s_textureW, s_textureH;

static TouchState s_touch;
static TouchLayout s_touchLayout;
static OverlaySurface s_atlas;
static SDL_GPUTexture *s_atlasTexture;
static int s_atlasW, s_atlasH;
static int s_touchW, s_touchH;
static SDL_Rect s_touchSafe;
static int s_touchEnabled = -1;

void PortGraphicsOverlayReleaseGpu(void) {
    if (s_device != NULL) {
        if (s_texture != NULL) SDL_ReleaseGPUTexture(s_device, s_texture);
        if (s_atlasTexture != NULL) SDL_ReleaseGPUTexture(s_device, s_atlasTexture);
        if (s_sampler != NULL) SDL_ReleaseGPUSampler(s_device, s_sampler);
        if (s_pipeline != NULL) SDL_ReleaseGPUGraphicsPipeline(s_device, s_pipeline);
    }
    s_atlasTexture = NULL;
    s_touchW = s_touchH = 0;
    s_texture = NULL;
    s_sampler = NULL;
    s_pipeline = NULL;
    s_device = NULL;
    s_textureW = s_textureH = 0;
    s_model.dirty = 1;
}

static SDL_GPUShader *CreateShader(SDL_GPUDevice *device, SDL_GPUShaderStage stage,
                                   const unsigned char *spirv, size_t spirvSize,
                                   const unsigned char *msl, size_t mslSize,
                                   const char *mslEntry, Uint32 samplers,
                                   Uint32 uniforms) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderCreateInfo info;

    SDL_zero(info);
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniforms;
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.code = spirv;
        info.code_size = spirvSize;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.code = msl;
        info.code_size = mslSize;
        info.entrypoint = mslEntry;
        info.format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        return NULL;
    }
    return SDL_CreateGPUShader(device, &info);
}

static int EnsurePipeline(SDL_GPUDevice *device, SDL_GPUTextureFormat format) {
    SDL_GPUShader *vertex, *fragment;
    SDL_GPUColorTargetDescription color;
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUSamplerCreateInfo sampler;

    if (s_device != NULL && s_device != device) PortGraphicsOverlayReleaseGpu();
    if (s_pipeline != NULL && s_sampler != NULL) return 1;
    s_device = device;
    vertex = CreateShader(device, SDL_GPU_SHADERSTAGE_VERTEX, overlay_vert_spv,
                          overlay_vert_spv_len, overlay_vert_msl,
                          overlay_vert_msl_len, "vs_overlay", 0, 1);
    fragment = CreateShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, overlay_frag_spv,
                            overlay_frag_spv_len, overlay_frag_msl,
                            overlay_frag_msl_len, "fs_overlay", 1, 0);
    if (vertex == NULL || fragment == NULL) {
        if (vertex != NULL) SDL_ReleaseGPUShader(device, vertex);
        if (fragment != NULL) SDL_ReleaseGPUShader(device, fragment);
        fprintf(stderr, "rage-port: graphics overlay shaders failed: %s\n",
                SDL_GetError());
        return 0;
    }
    SDL_zero(color);
    color.format = format;
    color.blend_state.enable_blend = true;
    color.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    color.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    SDL_zero(info);
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.target_info.color_target_descriptions = &color;
    info.target_info.num_color_targets = 1;
    s_pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    SDL_zero(sampler);
    sampler.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    s_sampler = SDL_CreateGPUSampler(device, &sampler);
    if (s_pipeline == NULL || s_sampler == NULL) {
        fprintf(stderr, "rage-port: graphics overlay pipeline failed: %s\n",
                SDL_GetError());
        PortGraphicsOverlayReleaseGpu();
        return 0;
    }
    return 1;
}

static int UploadTo(SDL_GPUDevice *device, SDL_GPUTexture **texture,
                    int *textureW, int *textureH,
                    const OverlaySurface *surface) {
    SDL_GPUTransferBufferCreateInfo transferInfo;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *command;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureTransferInfo source;
    SDL_GPUTextureRegion destination;
    Uint32 bytes = (Uint32)((size_t)surface->w * (size_t)surface->h * 4u);
    void *mapped;

    if (*texture == NULL || *textureW != surface->w ||
        *textureH != surface->h) {
        SDL_GPUTextureCreateInfo info;

        if (*texture != NULL) SDL_ReleaseGPUTexture(device, *texture);
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)surface->w;
        info.height = (Uint32)surface->h;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        *texture = SDL_CreateGPUTexture(device, &info);
        *textureW = surface->w;
        *textureH = surface->h;
        if (*texture == NULL) return 0;
    }
    SDL_zero(transferInfo);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bytes;
    transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (transfer == NULL) return 0;
    mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped == NULL) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    memcpy(mapped, surface->pixels, bytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    command = SDL_AcquireGPUCommandBuffer(device);
    if (command == NULL) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    copy = SDL_BeginGPUCopyPass(command);
    SDL_zero(source);
    source.transfer_buffer = transfer;
    source.pixels_per_row = (Uint32)surface->w;
    source.rows_per_layer = (Uint32)surface->h;
    SDL_zero(destination);
    destination.texture = *texture;
    destination.w = (Uint32)surface->w;
    destination.h = (Uint32)surface->h;
    destination.d = 1;
    SDL_UploadToGPUTexture(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(command);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return 1;
}

static int UploadSurface(SDL_GPUDevice *device) {
    return UploadTo(device, &s_texture, &s_textureW, &s_textureH, &s_surface);
}

/* Repaints and uploads the panel when the model or the window changed. */
static void RefreshSurface(SDL_GPUDevice *device, SDL_Window *window) {
    int w = 0, h = 0;
    OverlayLayout layout;

    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (w <= 0 || h <= 0 || !OverlayFontReady()) return;
    OverlayComputeLayout(OverlayModelRowCount(&s_model), w, h, &layout);
    if (w != s_windowW || h != s_windowH || layout.w != s_layout.w ||
        layout.h != s_layout.h) {
        s_model.dirty = 1;
    }
    s_windowW = w;
    s_windowH = h;
    s_layout = layout;
    if (!s_model.dirty) return;
    if (!OverlaySurfaceResize(&s_surface, layout.w, layout.h)) return;
    OverlayPaint(&s_surface, &s_model, &layout);
    if (EnsurePipeline(device, SDL_GetGPUSwapchainTextureFormat(device, window))) {
        UploadSurface(device);
    }
    {
        const char *dump = RuntimeConfigGet("diagnostics.overlay_dump");
        if (dump != NULL && dump[0] != '\0') OverlaySurfaceDump(&s_surface, dump);
    }
    s_model.dirty = 0;
}

/* ---- touch pad ---- */

/* The phone build always has it; elsewhere it is for trying the pad out. */
static int TouchEnabled(void) {
    if (s_touchEnabled < 0) {
#ifdef __ANDROID__
        s_touchEnabled = 1;
#else
        s_touchEnabled = RuntimeConfigEnabled("controls.touch");
#endif
    }
    return s_touchEnabled;
}

uint32_t PortTouchHeldControls(void) {
    return TouchEnabled() && !s_model.open ? TouchHeldMask(&s_touch) : 0u;
}

static void OnTouchPressed(uint32_t pressed) {
    if (pressed & (1u << TOUCH_SETTINGS)) {
        TouchReleaseAll(&s_touch);
        OpenPanel();
    }
}

static void OnTouchEvent(const SDL_Event *event) {
    const float x = event->tfinger.x * (float)s_touchW;
    const float y = event->tfinger.y * (float)s_touchH;
    const int64_t id = (int64_t)event->tfinger.fingerID;

    if (s_model.open) {
        TouchReleaseAll(&s_touch);
        if ((event->type == SDL_EVENT_FINGER_DOWN ||
             event->type == SDL_EVENT_FINGER_MOTION) &&
            s_windowW > 0 && s_windowH > 0 && s_layout.scale > 0.0f) {
            float px = event->tfinger.x * (float)s_windowW;
            float py = event->tfinger.y * (float)s_windowH;
            OverlayModelPointer(&s_model,
                                (px - (float)s_layout.x) / s_layout.scale,
                                (py - (float)s_layout.y) / s_layout.scale,
                                event->type == SDL_EVENT_FINGER_DOWN, NowMs());
        }
        return;
    }
    if (s_touchW <= 0) {
        TouchReleaseAll(&s_touch);
        return;
    }
    switch (event->type) {
    case SDL_EVENT_FINGER_DOWN:
        OnTouchPressed(TouchFingerDown(&s_touch, &s_touchLayout, id, x, y));
        break;
    case SDL_EVENT_FINGER_MOTION:
        OnTouchPressed(TouchFingerMove(&s_touch, &s_touchLayout, id, x, y));
        break;
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        TouchFingerUp(&s_touch, id);
        break;
    default:
        break;
    }
}

/* Lays the pad out for the window and repaints its atlas when that changed. */
static void RefreshTouch(SDL_GPUDevice *device, SDL_Window *window) {
    int w = 0, h = 0, lw = 0, lh = 0, atlasW, atlasH;
    SDL_Rect safe = {0};

    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (w <= 0 || h <= 0 || !OverlayFontReady()) return;
    SDL_GetWindowSize(window, &lw, &lh);
    SDL_GetWindowSafeArea(window, &safe);
    if (w != s_touchW || h != s_touchH) {
        fprintf(stderr, "rage-port: touch pad window=%dx%d logical=%dx%d safe=%d,%d %dx%d\n",
                w, h, lw, lh, safe.x, safe.y, safe.w, safe.h);
    }
    if (safe.w > 0 && safe.h > 0 && lw > 0 && lh > 0 &&
        (w != lw || h != lh)) {
        float sx = (float)w / (float)lw;
        float sy = (float)h / (float)lh;
        safe.x = (int)((float)safe.x * sx + 0.5f);
        safe.y = (int)((float)safe.y * sy + 0.5f);
        safe.w = (int)((float)safe.w * sx + 0.5f);
        safe.h = (int)((float)safe.h * sy + 0.5f);
    }
    if (safe.w <= 0 || safe.h <= 0) safe = (SDL_Rect){0, 0, w, h};
    if (w == s_touchW && h == s_touchH && s_atlasTexture != NULL &&
        memcmp(&safe, &s_touchSafe, sizeof(safe)) == 0) return;
    s_touchSafe = safe;
    s_touchW = w;
    s_touchH = h;
    TouchComputeLayout(safe.x, safe.y, safe.w, safe.h, &s_touchLayout);
    TouchAtlasSize(&s_touchLayout, &atlasW, &atlasH);
    if (!OverlaySurfaceResize(&s_atlas, atlasW, atlasH)) return;
    OverlayPaintTouchAtlas(&s_atlas, &s_touchLayout);
    if (EnsurePipeline(device, SDL_GetGPUSwapchainTextureFormat(device, window))) {
        UploadTo(device, &s_atlasTexture, &s_atlasW, &s_atlasH, &s_atlas);
    }
}

static void DrawTouch(SDL_GPUCommandBuffer *command, SDL_GPURenderPass *pass) {
    const uint32_t held = TouchHeldMask(&s_touch);
    SDL_GPUTextureSamplerBinding binding;
    SDL_GPUViewport viewport;
    SDL_Rect scissor;
    int control;

    if (s_pipeline == NULL || s_atlasTexture == NULL || s_touchW <= 0) return;
    viewport = (SDL_GPUViewport){0.0f, 0.0f, (float)s_touchW, (float)s_touchH,
                                 0.0f, 1.0f};
    scissor = (SDL_Rect){0, 0, s_touchW, s_touchH};
    SDL_SetGPUViewport(pass, &viewport);
    SDL_SetGPUScissor(pass, &scissor);
    SDL_BindGPUGraphicsPipeline(pass, s_pipeline);
    binding.texture = s_atlasTexture;
    binding.sampler = s_sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    for (control = 0; control < TOUCH_CONTROL_COUNT; control++) {
        const TouchRect *rect = &s_touchLayout.rect[control];
        OverlayQuad quad;
        int ox, oy;

        TouchAtlasOrigin(&s_touchLayout, control, (held >> control) & 1u, &ox,
                         &oy);
        quad.rect[0] = rect->x / (float)s_touchW * 2.0f - 1.0f;
        quad.rect[1] = 1.0f - rect->y / (float)s_touchH * 2.0f;
        quad.rect[2] = (rect->x + rect->w) / (float)s_touchW * 2.0f - 1.0f;
        quad.rect[3] = 1.0f - (rect->y + rect->h) / (float)s_touchH * 2.0f;
        quad.uv[0] = (float)ox / (float)s_atlasW;
        quad.uv[1] = (float)oy / (float)s_atlasH;
        quad.uv[2] = ((float)ox + rect->w) / (float)s_atlasW;
        quad.uv[3] = ((float)oy + rect->h) / (float)s_atlasH;
        quad.tint[0] = quad.tint[1] = quad.tint[2] = quad.tint[3] = 1.0f;
        SDL_PushGPUVertexUniformData(command, 0, &quad, sizeof(quad));
        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    }
}

void PortGraphicsOverlayTick(void) {
    SDL_GPUDevice *device = NULL;
    SDL_Window *window = NULL;
    OverlayPending due = OverlayModelTakePending(&s_model, NowMs());

    if (due.config) {
        ModernApplyConfig(&s_model.values.config);
        OverlayModelAdopt(&s_model, PortActiveConfig());
    }
    if (due.session) PortSessionOptionsApply(&s_model.values.session);
    if (OverlayModelTakeQuit(&s_model)) {
        SDL_Event quit;

        SDL_zero(quit);
        quit.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit);
    }
    if (!s_model.open && !TouchEnabled()) return;
    if (!Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device)) return;
    if (!s_model.open) {
        RefreshTouch(device, window);
        return;
    }
    s_window = window;
    RefreshSurface(device, window);
}

static void DrawPanel(SDL_GPUCommandBuffer *command, SDL_GPURenderPass *pass) {
    OverlayQuad quad;
    SDL_GPUTextureSamplerBinding binding;
    SDL_GPUViewport viewport;
    SDL_Rect scissor;
    float w, h;

    if (s_pipeline == NULL || s_texture == NULL || s_windowW <= 0) {
        return;
    }
    w = (float)s_windowW;
    h = (float)s_windowH;
    viewport = (SDL_GPUViewport){0.0f, 0.0f, w, h, 0.0f, 1.0f};
    scissor = (SDL_Rect){0, 0, s_windowW, s_windowH};
    SDL_SetGPUViewport(pass, &viewport);
    SDL_SetGPUScissor(pass, &scissor);
    SDL_BindGPUGraphicsPipeline(pass, s_pipeline);
    binding.texture = s_texture;
    binding.sampler = s_sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);

    /* Backdrop: the solid texel, dimmed. */
    quad.rect[0] = -1.0f; quad.rect[1] = 1.0f;
    quad.rect[2] = 1.0f;  quad.rect[3] = -1.0f;
    quad.uv[0] = quad.uv[2] = 0.5f / (float)s_textureW;
    quad.uv[1] = quad.uv[3] = ((float)s_textureH - 0.5f) / (float)s_textureH;
    quad.tint[0] = quad.tint[1] = quad.tint[2] = 1.0f;
    quad.tint[3] = 0.62f;
    SDL_PushGPUVertexUniformData(command, 0, &quad, sizeof(quad));
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);

    /* The panel at one texel per pixel. */
    quad.rect[0] = (float)s_layout.x / w * 2.0f - 1.0f;
    quad.rect[1] = 1.0f - (float)s_layout.y / h * 2.0f;
    quad.rect[2] = (float)(s_layout.x + s_layout.w) / w * 2.0f - 1.0f;
    quad.rect[3] = 1.0f - (float)(s_layout.y + s_layout.h) / h * 2.0f;
    quad.uv[0] = 0.0f;
    quad.uv[1] = 0.0f;
    quad.uv[2] = 1.0f;
    quad.uv[3] = (float)(s_textureH - 1) / (float)s_textureH;
    quad.tint[3] = 1.0f;
    SDL_PushGPUVertexUniformData(command, 0, &quad, sizeof(quad));
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
}

static void OnRender(SDL_GPUCommandBuffer *command, SDL_GPURenderPass *pass) {
    if (s_prevRender != NULL) s_prevRender(command, pass);
    if (s_model.open) DrawPanel(command, pass);
    else if (TouchEnabled()) DrawTouch(command, pass);
}

void PortGraphicsOverlayInstall(void) {
    if (s_installed) return;
    s_installed = 1;
    s_prevEvent = Psyz_OverlayEvent_SDL3(OnEvent);
    s_prevRender = Psyz_OverlayRender_SDL3GPU(OnRender);
}
