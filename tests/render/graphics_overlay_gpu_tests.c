/* Draws the graphics panel into an offscreen target through the same code the
 * game uses on the swapchain, then reads the image back: the backdrop must
 * dim what is behind the panel, and the panel itself must land centred, at
 * one texel per pixel, with its own text and rows drawn in. */
#include <SDL3/SDL.h>
#include "session_options.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The module under test is compiled into this file so its private state can
 * be driven directly; the host services it calls are replaced below. */
#include "graphics_overlay.c"

static RagePortConfig s_active;

PsyzOverlayEventCB_SDL3 Psyz_OverlayEvent_SDL3(PsyzOverlayEventCB_SDL3 cb) {
    (void)cb;
    return NULL;
}
PsyzOverlayRenderCB_SDL3GPU Psyz_OverlayRender_SDL3GPU(
    PsyzOverlayRenderCB_SDL3GPU cb) {
    (void)cb;
    return NULL;
}
static int s_presentationAvailable;

bool Psyz_VideoGetPresentationDevice_SDL3GPU(SDL_Window **window,
                                             SDL_GPUDevice **device) {
    *window = NULL;
    *device = NULL;
    return s_presentationAvailable != 0;
}
int ModernApplyConfig(const RagePortConfig *config) {
    s_active = *config;
    return 1;
}
const RagePortConfig *PortActiveConfig(void) { return &s_active; }
const char *RuntimeConfigGet(const char *key) {
    (void)key;
    return NULL;
}
static PortSessionOptions s_storedSession;
static int s_haveSession;

void PortSessionOptionsRead(PortSessionOptions *options) {
    if (!s_haveSession) {
        memset(&s_storedSession, 0, sizeof(s_storedSession));
        s_storedSession.analog = 1;
        s_storedSession.wheel = 1;
        s_storedSession.steeringLinearity = 0.5f;
        s_storedSession.forceFeedback = 1;
        s_storedSession.forceFeedbackGain = 55;
        s_storedSession.music = 15;
        s_storedSession.effects = 15;
        s_storedSession.hudEdges = 1;
        s_storedSession.showLapTimes = 1;
        s_storedSession.showTimeLimit = 1;
        s_storedSession.chaseDistance = 1.0f;
        s_storedSession.chaseHeight = 1.0f;
        s_haveSession = 1;
    }
    *options = s_storedSession;
}

void PortSessionOptionsApply(const PortSessionOptions *options) {
    s_storedSession = *options;
    s_haveSession = 1;
}

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s: %s\n", \
    __LINE__, #x, SDL_GetError()); return 1; } } while (0)

enum { TARGET_W = 1280, TARGET_H = 820 };

static void Press(SDL_Scancode key) {
    SDL_Event event;

    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = key;
    OnEvent(&event);
}

static void Repeat(SDL_Scancode key) {
    SDL_Event event;

    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = key;
    event.key.repeat = true;
    OnEvent(&event);
}

static int ScaleIndex(void) {
    return OverlayPageSettings(OVERLAY_PAGE_GRAPHICS, &(int){0})[1].get(
        &s_model.values);
}

/* The adapter turns events into model actions and applies what the model says
 * is due. The rules themselves are tested without SDL in overlay_model_tests. */
static int TestPanelControls(void) {
    int scale;
    int scaleAfter;

    s_active.renderer = RAGE_RENDERER_CLASSIC;
    s_active.modernInternalScale = 4.0f;
    s_presentationAvailable = 1;

    CHECK(!PortGraphicsOverlayActive());
    Press(SDL_SCANCODE_DOWN);
    CHECK(!PortGraphicsOverlayActive() && s_model.selected == 0);
    Press(SDL_SCANCODE_ESCAPE);
    CHECK(PortGraphicsOverlayActive());
    CHECK(s_model.values.config.renderer == RAGE_RENDERER_CLASSIC);

    Press(SDL_SCANCODE_RIGHT);
    CHECK(s_model.values.config.renderer == RAGE_RENDERER_MODERN &&
          s_model.configPending);
    Press(SDL_SCANCODE_LEFT);
    CHECK(s_model.values.config.renderer == RAGE_RENDERER_CLASSIC);
    Press(SDL_SCANCODE_UP);
    CHECK(s_model.selected == OverlayModelRowCount(&s_model) - 1);
    Press(SDL_SCANCODE_DOWN);
    Press(SDL_SCANCODE_DOWN);
    CHECK(s_model.selected == 1);
    scale = ScaleIndex();
    Press(SDL_SCANCODE_RIGHT);
    scaleAfter = ScaleIndex();
    CHECK(scaleAfter == (scale + 1) % 6);

    /* A held key steps once: each value can rebuild the presentation. */
    Repeat(SDL_SCANCODE_RIGHT);
    CHECK(ScaleIndex() == scaleAfter);

    /* Changes reach the renderer from the main loop once the player has
     * stopped stepping, not from the event and not at once. */
    s_active.modernInternalScale = 0.0f;
    PortGraphicsOverlayTick();
    CHECK(s_active.modernInternalScale == 0.0f && s_model.configPending);
    s_model.changedAtMs -= OVERLAY_APPLY_DELAY_MS;
    PortGraphicsOverlayTick();
    CHECK(s_active.modernInternalScale == 3.0f * 0.0f + (scaleAfter == 0 ? 1.0f :
          scaleAfter == 1 ? 2.0f : scaleAfter == 2 ? 3.0f : scaleAfter == 3 ? 4.0f :
          scaleAfter == 4 ? 6.0f : 8.0f));
    CHECK(!s_model.configPending);

    /* Page Down leaves graphics for controls; a session option applies from
     * the next tick without waiting. */
    Press(SDL_SCANCODE_PAGEDOWN);
    CHECK(s_model.page == OVERLAY_PAGE_CONTROLS && s_model.selected == 0);
    Press(SDL_SCANCODE_RIGHT);
    CHECK(s_model.values.session.analog == 0 && s_model.sessionPending);
    PortGraphicsOverlayTick();
    CHECK(s_storedSession.analog == 0 && !s_model.sessionPending);
    Press(SDL_SCANCODE_PAGEDOWN);
    CHECK(s_model.page == OVERLAY_PAGE_AUDIO);
    Press(SDL_SCANCODE_LEFT);
    CHECK(s_model.values.session.music == 14);
    Press(SDL_SCANCODE_PAGEUP);
    Press(SDL_SCANCODE_PAGEUP);
    CHECK(s_model.page == OVERLAY_PAGE_GRAPHICS && s_model.selected == 0);

    /* Closing and then reading the close flag happens exactly once. */
    Press(SDL_SCANCODE_ESCAPE);
    CHECK(!PortGraphicsOverlayActive());
    CHECK(PortGraphicsOverlayTakeClosed());
    CHECK(!PortGraphicsOverlayTakeClosed());

    /* The resume row closes it as well. */
    Press(SDL_SCANCODE_ESCAPE);
    s_model.selected = OverlayModelSettingCount(&s_model);
    Press(SDL_SCANCODE_RETURN);
    CHECK(!PortGraphicsOverlayActive() && PortGraphicsOverlayTakeClosed());
    s_presentationAvailable = 0;
    return 0;
}

static int Differs(const uint8_t *a, const uint8_t *b) {
    return a[0] != b[0] || a[1] != b[1] || a[2] != b[2];
}

int main(void) {
    SDL_GPUDevice *device;
    SDL_GPUTexture *target;
    SDL_GPUTextureCreateInfo textureInfo;
    SDL_GPUColorTargetInfo color;
    SDL_GPUTransferBufferCreateInfo transferInfo;
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *command;
    SDL_GPURenderPass *pass;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureRegion region;
    SDL_GPUTextureTransferInfo destination;
    SDL_GPUFence *fence;
    const uint8_t *pixels;
    uint8_t corner[4], inside[4];
    int panelPixels = 0, rowHighlight = 0, x, y;

    if (TestPanelControls()) return 1;
    if (!SDL_Init(SDL_INIT_VIDEO)) return 77;
    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (device == NULL) { SDL_Quit(); return 77; }

    {
        PortSessionOptions session;

        PortSessionOptionsRead(&session);
        s_active.renderer = RAGE_RENDERER_MODERN;
        OverlayModelOpen(&s_model, &s_active, &session);
    }
    s_windowW = TARGET_W;
    s_windowH = TARGET_H;
    OverlayComputeLayout(OverlayModelRowCount(&s_model), TARGET_W, TARGET_H,
                         &s_layout);
    CHECK(s_layout.scale >= 0.99f && s_layout.scale <= 1.01f);
    CHECK(OverlayFontReady());
    CHECK(OverlaySurfaceResize(&s_surface, s_layout.w, s_layout.h));
    OverlayPaint(&s_surface, &s_model, &s_layout);
    CHECK(EnsurePipeline(device, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM));
    CHECK(UploadSurface(device));

    SDL_zero(textureInfo);
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    textureInfo.width = TARGET_W;
    textureInfo.height = TARGET_H;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    target = SDL_CreateGPUTexture(device, &textureInfo);
    CHECK(target);
    SDL_zero(transferInfo);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transferInfo.size = TARGET_W * TARGET_H * 4;
    transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    CHECK(transfer);

    command = SDL_AcquireGPUCommandBuffer(device);
    CHECK(command);
    SDL_zero(color);
    color.texture = target;
    color.clear_color = (SDL_FColor){0.5f, 0.5f, 0.5f, 1.0f};
    color.load_op = SDL_GPU_LOADOP_CLEAR;
    color.store_op = SDL_GPU_STOREOP_STORE;
    pass = SDL_BeginGPURenderPass(command, &color, 1, NULL);
    CHECK(pass);
    DrawPanel(command, pass);
    SDL_EndGPURenderPass(pass);
    copy = SDL_BeginGPUCopyPass(command);
    SDL_zero(region);
    region.texture = target;
    region.w = TARGET_W;
    region.h = TARGET_H;
    region.d = 1;
    SDL_zero(destination);
    destination.transfer_buffer = transfer;
    SDL_DownloadFromGPUTexture(copy, &region, &destination);
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
    CHECK(fence);
    CHECK(SDL_WaitForGPUFences(device, true, &fence, 1));
    SDL_ReleaseGPUFence(device, fence);
    pixels = (const uint8_t *)SDL_MapGPUTransferBuffer(device, transfer, false);
    CHECK(pixels);

    /* The clear is mid-grey; the backdrop must darken it everywhere, even
     * outside the panel. */
    memcpy(corner, pixels, 4);
    CHECK(corner[0] < 0x70 && corner[0] > 0x20);
    CHECK(corner[0] == corner[1] && corner[1] == corner[2]);

    /* Inside the panel the dark panel colour replaces the grey, which the
     * backdrop alone could not produce, and only within the panel rectangle. */
    memcpy(inside, pixels + ((size_t)(s_layout.y + 6) * TARGET_W +
                             (size_t)(s_layout.x + s_layout.w / 2)) * 4u, 4);
    CHECK(inside[2] > inside[0]);
    CHECK(inside[0] < corner[0]);
    for (y = 0; y < TARGET_H; y++) {
        for (x = 0; x < TARGET_W; x++) {
            const uint8_t *p = pixels + ((size_t)y * TARGET_W + (size_t)x) * 4u;
            int within = x >= s_layout.x && x < s_layout.x + s_layout.w &&
                         y >= s_layout.y && y < s_layout.y + s_layout.h;

            if (!within) {
                CHECK(!Differs(p, corner));
            } else if (Differs(p, corner)) {
                panelPixels++;
            }
            /* The selected row carries the accent bar and arrows in blue. */
            if (within && p[2] > 100 && p[2] > p[0] + 50) rowHighlight++;
        }
    }
    CHECK(panelPixels > s_layout.w * s_layout.h / 2);
    CHECK(rowHighlight > 100);

    /* The panel keeps its margins: the middle of the top edge is outside. */
    CHECK(s_layout.x > 100 && s_layout.y > 0);

    SDL_UnmapGPUTransferBuffer(device, transfer);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    SDL_ReleaseGPUTexture(device, target);
    PortGraphicsOverlayReleaseGpu();
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    puts("graphics overlay draws a dimmed backdrop and a centred panel");
    return 0;
}
