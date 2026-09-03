/*
 * Asking the desktop for a disc image. See disc_picker.h for why this is not
 * part of the platform layer that calls it.
 */

#include "disc_picker.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>

typedef struct RageHostDiscDialog {
    SDL_AtomicInt completed;
    char *path;
    size_t pathSize;
    int accepted;
    int failed;
} RageHostDiscDialog;

static void SDLCALL HostDiscDialogComplete(
    void *userdata, const char *const *files, int filter) {
    RageHostDiscDialog *dialog = userdata;
    int written;

    (void)filter;
    dialog->failed = files == NULL;
    if (files != NULL && files[0] != NULL) {
        written = snprintf(dialog->path, dialog->pathSize, "%s", files[0]);
        dialog->accepted = written >= 0 && (size_t)written < dialog->pathSize;
        if (!dialog->accepted) dialog->path[0] = '\0';
    }
    SDL_SetAtomicInt(&dialog->completed, 1);
}

int HostShowDiscPicker(char *path, size_t size) {
    static const SDL_DialogFileFilter filters[] = {
        {"Rage Racer disc images", "cue;bin;chd"},
        {"All files", "*"},
    };
    RageHostDiscDialog dialog;

    if (path == NULL || size == 0) return 0;
    memset(&dialog, 0, sizeof(dialog));
    dialog.path = path;
    dialog.pathSize = size;
    path[0] = '\0';
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        fprintf(stderr, "rage-port: cannot initialize disc picker: %s\n",
                SDL_GetError());
        return 0;
    }
    SDL_ShowOpenFileDialog(HostDiscDialogComplete, &dialog, NULL, filters,
                           (int)(sizeof(filters) / sizeof(filters[0])), NULL,
                           false);
    while (!SDL_GetAtomicInt(&dialog.completed)) {
        SDL_PumpEvents();
        SDL_Delay(10);
    }
    if (dialog.failed)
        fprintf(stderr, "rage-port: disc picker failed: %s\n", SDL_GetError());
    return dialog.accepted;
}
