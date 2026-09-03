#include <psyz.h>
#include <psyz/video.h>
#include <libetc.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "input_config.h"
#include "content_options.h"
#include "diagnostic_log.h"
#include "host_storage.h"
#include "host_disc.h"
#include "runtime_config.h"
#include "timing_control.h"
#include "modern/modern_renderer.h"
#include "native_asset_importer.h"
#include "platform_paths.h"

#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

void MainLoop(void);
int InitNativeGameData(void);

typedef enum ArchiveDumpResult {
    ARCHIVE_DUMP_NOT_REQUESTED,
    ARCHIVE_DUMP_COMPLETE,
    ARCHIVE_DUMP_FAILED,
} ArchiveDumpResult;

static void LoadInputConfig(RageInputConfig *config, const char *argv0) {
    char path[PATH_MAX];
    if (PlatformFindConfigFile(argv0, "rage-input.cfg", path,
                                   sizeof(path)))
        InputConfigLoad(config, path);
}

static ArchiveDumpResult DumpRequestedArchive(void) {
    const char *path = RuntimeConfigGet("tools.dump_archive");

    if (path == NULL || path[0] == '\0') {
        return ARCHIVE_DUMP_NOT_REQUESTED;
    }
    if (!HostInitDisc()) {
        return ARCHIVE_DUMP_FAILED;
    }
    if (!HostDumpArchive(path)) {
        fprintf(stderr, "rage-port: cannot write %s\n", path);
        return ARCHIVE_DUMP_FAILED;
    }
    fprintf(stderr, "rage-port: archive written to %s\n", path);
    return ARCHIVE_DUMP_COMPLETE;
}

int main(int argc, char **argv) {
    RageInputConfig inputConfig;
    RagePortConfig portConfig;
    ArchiveDumpResult archiveDumpResult;
    int inputIndex;
    char logPath[PATH_MAX];

    if (!RuntimeConfigInit(argc, argv)) return EXIT_FAILURE;
    if (!DiagnosticLogOpen(logPath, sizeof(logPath))) {
        fprintf(stderr, "rage-port: could not open diagnostic log\n");
    }

    Psyz_SetTitle("Rage Racer");
    if (!HostInitStorage()) {
        fprintf(stderr, "failed to initialize user save storage\n");
        return EXIT_FAILURE;
    }
    Psyz_VideoSetAspectMode(PSYZ_ASPECT_SQUARE);
    PortConfigDefaults(&portConfig);
    PortConfigApplyRuntime(&portConfig);
    PortConfigSetActive(&portConfig);
    TimingInit();
    fprintf(stderr,
            "rage-port: renderer=%s scale=%.2f aspect=%d fps=%d draw_distance=%.2f post=%d\n",
            portConfig.renderer == RAGE_RENDERER_MODERN ? "modern" : "classic",
            portConfig.modernInternalScale, portConfig.modernAspect,
            portConfig.modernFps, portConfig.modernDrawDistance,
            portConfig.modernPost);
    /* Initialize SDL input before configurable names are resolved to
     * scancodes. GameInitPad later attaches the game's BIOS buffers. */
    PadInit(0);
    InputConfigDefaults(&inputConfig);
    LoadInputConfig(&inputConfig, argc > 0 ? argv[0] : NULL);
    InputConfigApplyRuntime(&inputConfig);
    for (inputIndex = 0; inputIndex < RAGE_INPUT_BUTTON_COUNT; inputIndex++) {
        Psyz_SetKeyboardKey(inputIndex, inputConfig.keys[inputIndex]);
    }
    archiveDumpResult = DumpRequestedArchive();
    if (archiveDumpResult != ARCHIVE_DUMP_NOT_REQUESTED) {
        return archiveDumpResult == ARCHIVE_DUMP_COMPLETE
                   ? EXIT_SUCCESS
                   : EXIT_FAILURE;
    }
    if (!HostInitDisc()) {
        fprintf(stderr, "failed to initialize disc image\n");
        return EXIT_FAILURE;
    }
    if (!NativeAssetImporterInit() || !ModernInit(&portConfig))
        return EXIT_FAILURE;
    if (!InitNativeGameData()) {
        fprintf(stderr, "failed to initialize retail game data\n");
        return EXIT_FAILURE;
    }
    ContentOptionsApply();
    MainLoop();
    return EXIT_SUCCESS;
}
