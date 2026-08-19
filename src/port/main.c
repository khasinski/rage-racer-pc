#include <psyz.h>
#include <psyz/video.h>
#include <libetc.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "input_config.h"
#include "diagnostic_log.h"
#include "port_config.h"
#include "runtime_config.h"
#include "timing_control.h"
#include "modern/modern_renderer.h"
#include "platform_paths.h"

#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

void MainLoop(void);
int RageMapPs1Scratchpad(void);
int RageInitNativeGameData(void);
int RageHostInitDisc(void);

static void RageLoadInputConfig(RageInputConfig *config, const char *argv0) {
    char path[PATH_MAX];
    if (RagePlatformFindConfigFile(argv0, "rage-input.cfg", path,
                                   sizeof(path)))
        RageInputConfigLoad(config, path);
}

int main(int argc, char **argv) {
    RageInputConfig inputConfig;
    RagePortConfig portConfig;
    int inputIndex;
    char logPath[PATH_MAX];

    if (!RageRuntimeConfigInit(argc, argv)) return EXIT_FAILURE;
    if (!RageDiagnosticLogOpen(logPath, sizeof(logPath))) {
        fprintf(stderr, "rage-port: could not open diagnostic log\n");
    }

    Psyz_SetTitle("Rage Racer");
    Psyz_VideoSetAspectMode(PSYZ_ASPECT_SQUARE);
    RagePortConfigDefaults(&portConfig);
    RagePortConfigApplyRuntime(&portConfig);
    RagePortConfigSetActive(&portConfig);
    RageTimingInit();
    fprintf(stderr,
            "rage-port: renderer=%s scale=%.2f aspect=%d fps=%d draw_distance=%.2f post=%d\n",
            portConfig.renderer == RAGE_RENDERER_MODERN ? "modern" : "classic",
            portConfig.modernInternalScale, portConfig.modernAspect,
            portConfig.modernFps, portConfig.modernDrawDistance,
            portConfig.modernPost);
    RageModernInit(&portConfig);
    /* Initialize SDL input before configurable names are resolved to
     * scancodes. GameInitPad later attaches the game's BIOS buffers. */
    PadInit(0);
    RageInputConfigDefaults(&inputConfig);
    RageLoadInputConfig(&inputConfig, argc > 0 ? argv[0] : NULL);
    RageInputConfigApplyRuntime(&inputConfig);
    for (inputIndex = 0; inputIndex < RAGE_INPUT_BUTTON_COUNT; inputIndex++) {
        Psyz_SetKeyboardKey(inputIndex, inputConfig.keys[inputIndex]);
    }
    if (!RageHostInitDisc()) {
        fprintf(stderr, "failed to initialize disc cue sheet\n");
        return EXIT_FAILURE;
    }
    if (!RageInitNativeGameData()) {
        fprintf(stderr, "failed to initialize retail game data\n");
        return EXIT_FAILURE;
    }
    if (!RageMapPs1Scratchpad()) {
        fprintf(stderr, "failed to initialize renderer scratchpad\n");
        return EXIT_FAILURE;
    }
    MainLoop();
    return EXIT_SUCCESS;
}
