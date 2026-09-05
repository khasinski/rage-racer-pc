#include <stdio.h>
#include "debug_gpu_capture.h"
#include "debug/renderdoc_app.h"
#include "runtime_config.h"
#include "platform_paths.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static RENDERDOC_API_1_1_2 *s_api;
static int s_initialized, s_requests, s_limit, s_burst;
static uint32_t s_captures;
static char s_directory[4096];

void DebugGpuCaptureInit(void) {
    pRENDERDOC_GetAPI getApi = NULL;
    if (s_initialized) return;
    s_initialized = 1;
    if (!RuntimeConfigEnabled("diagnostics.renderdoc")) return;
#ifdef _WIN32
    {
        HMODULE module = GetModuleHandleA("renderdoc.dll");
        if (module) getApi = (pRENDERDOC_GetAPI)GetProcAddress(module, "RENDERDOC_GetAPI");
    }
#else
    getApi = (pRENDERDOC_GetAPI)dlsym(RTLD_DEFAULT, "RENDERDOC_GetAPI");
#endif
    /* Never load an uninjected library after Vulkan initialization. */
    if (!getApi || !getApi(eRENDERDOC_API_Version_1_1_2, (void **)&s_api)) {
        fprintf(stderr, "rage-port: RenderDoc unavailable; launch through renderdoccmd capture (no GPU captures will be made)\n");
        return;
    }
    s_limit = RuntimeConfigInt("diagnostics.renderdoc_limit", 8, 1, 100);
    s_burst = RuntimeConfigInt("diagnostics.renderdoc_burst", 3, 1, 8);
    if (!PlatformUserStateDirectory(s_directory, sizeof(s_directory)) ||
        !PlatformEnsureDirectory(s_directory)) {
        fprintf(stderr, "rage-port: RenderDoc output directory unavailable\n");
        s_api = NULL;
        return;
    }
    s_captures = s_api->GetNumCaptures();
    fprintf(stderr, "rage-port: RenderDoc connected burst=%d request_limit=%d directory=%s\n",
            s_burst, s_limit, s_directory);
}

void DebugGpuCapturePoll(void) {
    if (!s_api) return;
    while (s_captures < s_api->GetNumCaptures()) {
        char path[8192];
        uint32_t length = sizeof(path);
        uint64_t timestamp;
        if (s_api->GetCapture(s_captures, path, &length, &timestamp))
            fprintf(stderr, "rage-port: RenderDoc saved capture=%u path=%s\n", s_captures, path);
        s_captures++;
    }
}

void DebugGpuCaptureRequest(uint64_t sourceFrame) {
    char path[4608];
    DebugGpuCaptureInit();
    DebugGpuCapturePoll();
    if (!s_api || s_requests >= s_limit || s_api->IsFrameCapturing()) return;
    snprintf(path, sizeof(path), "%s/gpu-%03d-after-logic-%llu", s_directory,
             ++s_requests, (unsigned long long)sourceFrame);
    s_api->SetCaptureFilePathTemplate(path);
    s_api->TriggerMultiFrameCapture((uint32_t)s_burst);
    fprintf(stderr, "rage-port: RenderDoc queued request=%d after_logic=%llu frames=%d (subsequent GPU presents, not retroactive)\n",
            s_requests, (unsigned long long)sourceFrame, s_burst);
}
