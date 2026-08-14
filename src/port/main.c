#include <psyz.h>
#include <psyz/video.h>
#include <libetc.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "input_config.h"

#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
/* The recovered game headers provide only the PS1-era stdio surface. */
extern int snprintf(char *buffer, unsigned long long size, const char *format, ...);
extern int fprintf(void *stream, const char *format, ...);
extern void *stderr;
#endif

void MainLoop(void);
int RageMapPs1Scratchpad(void);
int RageInitNativeGameData(void);
int RageHostInitDisc(void);

static void RageLoadInputConfig(RageInputConfig *config) {
#ifdef _WIN32
    const char *home = getenv("APPDATA");
#else
    const char *home = getenv("HOME");
#endif
    char path[PATH_MAX];

    if (home != NULL && home[0] != '\0') {
#ifdef _WIN32
        snprintf(path, sizeof(path), "%s\\Rage Racer\\rage-input.cfg", home);
#else
        snprintf(path, sizeof(path), "%s/Library/Application Support/Rage Racer/rage-input.cfg", home);
#endif
        if (RageInputConfigLoad(config, path) > 0) return;
    }
    RageInputConfigLoad(config, "rage-input.cfg");
}

int main(void) {
    RageInputConfig inputConfig;
    int inputIndex;

    Psyz_SetTitle("Rage Racer");
    Psyz_VideoSetAspectMode(PSYZ_ASPECT_SQUARE);
    /* Initialize SDL input before configurable names are resolved to
     * scancodes. GameInitPad later attaches the game's BIOS buffers. */
    PadInit(0);
    RageInputConfigDefaults(&inputConfig);
    RageLoadInputConfig(&inputConfig);
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
