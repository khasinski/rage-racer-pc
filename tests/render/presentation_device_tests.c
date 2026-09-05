#include <stdio.h>
#include <psyz/audio.h>
#include <psyz/overlay.h>
#include <psyz/overlay_sdl3_gpu.h>
#include <psyz/present_sdl3_gpu.h>
#include <psyz/video.h>

/* Backend reset entry point, exercised here without a game loop. */
void ResetPlatform(void);
bool InitPlatform(void);
static int failures, initialized, destroyed;
static SDL_Window *currentWindow;
static SDL_GPUDevice *currentDevice;
#define CHECK(x) do { if (!(x)) { ++failures; \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); } } while (0)

static void Init(SDL_Window *window, SDL_GPUDevice *device) {
    ++initialized;
    currentWindow = window;
    currentDevice = device;
    SDL_Window *queriedWindow = NULL;
    SDL_GPUDevice *queriedDevice = NULL;
    CHECK(Psyz_VideoGetPresentationDevice_SDL3GPU(&queriedWindow, &queriedDevice));
    CHECK(queriedWindow == window && queriedDevice == device);
}
static void Destroy(void) {
    ++destroyed;
    SDL_Window *window = NULL;
    SDL_GPUDevice *device = NULL;
    CHECK(Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device));
    CHECK(window == currentWindow && device == currentDevice);
}
int main(void) {
    SDL_Window *window = NULL;
    SDL_GPUDevice *device = NULL;
    CHECK(!Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device));
    CHECK(window == NULL && device == NULL);
    if (!SDL_Init(SDL_INIT_VIDEO)) return 77;
    SDL_GPUDevice *probe = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (probe == NULL) { SDL_Quit(); return 77; }
    SDL_DestroyGPUDevice(probe);
    PsyzOverlayInitCB_SDL3GPU previousInit = Psyz_OverlayInit_SDL3GPU(Init);
    PsyzOverlayDestroyCB previousDestroy = Psyz_OverlayDestroyCB(Destroy);
    CHECK(SDL_SetHintWithPriority(SDL_HINT_GPU_DRIVER, "rage-invalid-gpu-driver",
                                 SDL_HINT_OVERRIDE));
    CHECK(!InitPlatform());
    CHECK(initialized == 0);
    CHECK(!Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device));
    CHECK(window == NULL && device == NULL);
    ResetPlatform();
    CHECK(destroyed == 0); /* No consumer was initialized on the failure path. */
    ResetPlatform();
    CHECK(destroyed == 0);
    for (int pass = 1; pass <= 2; ++pass) {
        /* SDL_Quit clears hints at the end of each platform cycle. */
        CHECK(SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy",
                                     SDL_HINT_OVERRIDE));
        Psyz_VideoVSync(0);
        CHECK(initialized == pass);
        CHECK(Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device));
        CHECK(window == currentWindow && device == currentDevice);
        CHECK(!Psyz_VideoGetPresentationDevice_SDL3GPU(&window, NULL));
        CHECK(window == NULL);
        CHECK(!Psyz_VideoGetPresentationDevice_SDL3GPU(NULL, &device));
        CHECK(device == NULL);
        Psyz_AudioResetMetrics();
        CHECK(Psyz_AudioInit() == 0);
        Uint64 deadline = SDL_GetTicks() + 1000;
        while (Psyz_AudioRenderedFrames() == 0 && SDL_GetTicks() < deadline)
            SDL_Delay(1);
        CHECK(Psyz_AudioRenderedFrames() > 0);
        ResetPlatform();
        unsigned long long stopped = Psyz_AudioRenderedFrames();
        SDL_Delay(30);
        CHECK(Psyz_AudioRenderedFrames() == stopped);
        Psyz_AudioDestroy(); /* Backend already released it. Must be harmless. */
        CHECK(destroyed == pass);
        CHECK(!Psyz_VideoGetPresentationDevice_SDL3GPU(&window, &device));
        CHECK(window == NULL && device == NULL);
        ResetPlatform();
        CHECK(destroyed == pass);
    }
    Psyz_OverlayInit_SDL3GPU(previousInit);
    Psyz_OverlayDestroyCB(previousDestroy);
    return failures != 0;
}
