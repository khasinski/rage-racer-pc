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

    s_window = NULL;
    if (active == NULL ||
        !Psyz_VideoGetPresentationDevice_SDL3GPU(&s_window, &device)) {
        return;
    }
    PortSessionOptionsRead(&session);
    OverlayModelOpen(&s_model, active, &session);
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

static void OnMouse(float windowX, float windowY, int click) {
    float density = s_window != NULL ? SDL_GetWindowPixelDensity(s_window) : 1.0f;
    float x = (windowX * density - (float)s_layout.x) / s_layout.scale;
    float y = (windowY * density - (float)s_layout.y) / s_layout.scale;

    OverlayModelPointer(&s_model, x, y, click, NowMs());
}

static void OnEvent(const SDL_Event *event) {
    if (s_prevEvent != NULL) s_prevEvent(event);
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
        OnMouse(event->motion.x, event->motion.y, 0);
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
               event->button.button == SDL_BUTTON_LEFT) {
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

void PortGraphicsOverlayReleaseGpu(void) {
    if (s_device != NULL) {
        if (s_texture != NULL) SDL_ReleaseGPUTexture(s_device, s_texture);
        if (s_sampler != NULL) SDL_ReleaseGPUSampler(s_device, s_sampler);
        if (s_pipeline != NULL) SDL_ReleaseGPUGraphicsPipeline(s_device, s_pipeline);
    }
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

static int UploadSurface(SDL_GPUDevice *device) {
    SDL_GPUTransferBufferCreateInfo transferInfo;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *command;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureTransferInfo source;
    SDL_GPUTextureRegion destination;
    Uint32 bytes = (Uint32)((size_t)s_surface.w * (size_t)s_surface.h * 4u);
    void *mapped;

    if (s_texture == NULL || s_textureW != s_surface.w ||
        s_textureH != s_surface.h) {
        SDL_GPUTextureCreateInfo info;

        if (s_texture != NULL) SDL_ReleaseGPUTexture(device, s_texture);
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s_surface.w;
        info.height = (Uint32)s_surface.h;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        s_texture = SDL_CreateGPUTexture(device, &info);
        s_textureW = s_surface.w;
        s_textureH = s_surface.h;
        if (s_texture == NULL) return 0;
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
    memcpy(mapped, s_surface.pixels, bytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    command = SDL_AcquireGPUCommandBuffer(device);
    if (command == NULL) {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return 0;
    }
    copy = SDL_BeginGPUCopyPass(command);
    SDL_zero(source);
    source.transfer_buffer = transfer;
    source.pixels_per_row = (Uint32)s_surface.w;
    source.rows_per_layer = (Uint32)s_surface.h;
    SDL_zero(destination);
    destination.texture = s_texture;
    destination.w = (Uint32)s_surface.w;
    destination.h = (Uint32)s_surface.h;
    destination.d = 1;
    SDL_UploadToGPUTexture(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(command);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return 1;
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
    if (!s_model.open) return;
    if (!Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device)) return;
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
}

void PortGraphicsOverlayInstall(void) {
    if (s_installed) return;
    s_installed = 1;
    s_prevEvent = Psyz_OverlayEvent_SDL3(OnEvent);
    s_prevRender = Psyz_OverlayRender_SDL3GPU(OnRender);
}
