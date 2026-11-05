/* Desktop disc-image selection; validation and mounting belong to host_disc. */

#include "disc_picker.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct RageHostDiscDialog {
    SDL_AtomicInt completed;
    char *path;
    size_t pathSize;
    int accepted;
    int failed;
} RageHostDiscDialog;

static int HostGuiDialogsSuppressed(void) {
    const char *headless = getenv("PSYZ_HEADLESS");
    const char *testMode = getenv("RAGE_PORT_TEST_MODE");

    return (headless != NULL && headless[0] != '\0' &&
            strcmp(headless, "0") != 0) ||
           (testMode != NULL && testMode[0] != '\0' &&
            strcmp(testMode, "0") != 0);
}

static void SDLCALL HostDiscDialogComplete(
    void *userdata, const char *const *files, int filter) {
    RageHostDiscDialog *dialog = userdata;
    int written;

    (void)filter;
    dialog->failed = files == NULL;
    if (files != NULL && files[0] != NULL) {
        written = snprintf(dialog->path, dialog->pathSize, "%s", files[0]);
        dialog->accepted = written >= 0 && (size_t)written < dialog->pathSize;
        if (!dialog->accepted) {
            dialog->path[0] = '\0';
            dialog->failed = 1;
        }
    }
    SDL_SetAtomicInt(&dialog->completed, 1);
}

int HostShowDiscPicker(char *path, size_t size) {
    static const SDL_DialogFileFilter filters[] = {
        {"Rage Racer disc images", "cue;bin;chd"},
        {"All files", "*"},
    };
    RageHostDiscDialog dialog;

    if (path == NULL || size == 0) return -1;
    memset(&dialog, 0, sizeof(dialog));
    dialog.path = path;
    dialog.pathSize = size;
    path[0] = '\0';
    if (HostGuiDialogsSuppressed()) {
        fprintf(stderr,
                "rage-port: disc picker suppressed in headless/test mode\n");
        return -1;
    }
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        fprintf(stderr, "rage-port: cannot initialize disc picker: %s\n",
                SDL_GetError());
        return -1;
    }
    SDL_ShowOpenFileDialog(HostDiscDialogComplete, &dialog, NULL, filters,
                           (int)(sizeof(filters) / sizeof(filters[0])), NULL,
                           false);
    while (!SDL_GetAtomicInt(&dialog.completed)) {
        SDL_PumpEvents();
        SDL_Delay(10);
    }
    if (dialog.failed) {
        fprintf(stderr, "rage-port: disc picker failed: %s\n", SDL_GetError());
        return -1;
    }
    return dialog.accepted ? 1 : 0;
}

void HostShowErrorDialog(const char *title, const char *message) {
    const char *safeTitle =
        (title != NULL && title[0] != '\0') ? title : "Rage Racer Error";
    const char *safeMessage =
        (message != NULL && message[0] != '\0')
            ? message
            : "An unexpected startup error occurred.";

    if (HostGuiDialogsSuppressed()) return;
    /* A startup error can surface before any video subsystem init; without
     * it SDL_ShowSimpleMessageBox fails and the player sees nothing. */
    if (!SDL_WasInit(SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        fprintf(stderr, "rage-port: error dialog unavailable: %s\n",
                SDL_GetError());
        return;
    }
    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, safeTitle, safeMessage,
                                  NULL)) {
        fprintf(stderr, "rage-port: error dialog failed: %s\n", SDL_GetError());
    }
}

int HostGpuBackendAvailable(void) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        fprintf(stderr, "rage-port: video initialization failed: %s\n",
                SDL_GetError());
        return 0;
    }
    /* The video subsystem stays initialized for PsyZ, which uses it next. */
    if (!SDL_GPUSupportsShaderFormats(
            SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, NULL)) {
        fprintf(stderr, "rage-port: no usable SDL GPU backend: %s\n",
                SDL_GetError());
        return 0;
    }
    return 1;
}
