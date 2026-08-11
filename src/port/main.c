#include <psyz.h>
#include <psyz/video.h>
#include <libetc.h>

#include <stdio.h>
#include <stdlib.h>

#include "input_config.h"

void MainLoop(void);
int RageMapPs1Scratchpad(void);
int RageInitNativeGameData(void);
int RageHostInitDisc(void);
int main(void) {
    RageInputConfig inputConfig;
    int inputIndex;

    Psyz_SetTitle("Rage Racer");
    Psyz_VideoSetAspectMode(PSYZ_ASPECT_SQUARE);
    /* Initialize SDL input before configurable names are resolved to
     * scancodes. GameInitPad later attaches the game's BIOS buffers. */
    PadInit(0);
    RageInputConfigDefaults(&inputConfig);
    RageInputConfigLoad(&inputConfig, "rage-input.cfg");
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
