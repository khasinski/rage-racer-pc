#ifndef RAGE_DISC_PICKER_H
#define RAGE_DISC_PICKER_H

#include <stddef.h>

/*
 * Ask the desktop for a disc image.
 *
 * The picker owns the asynchronous SDL dialog and answers with a path. Disc
 * validation and mounting remain in the caller, so this module has no disc
 * format or game-state policy.
 *
 * Returns 1 when a path was selected, 0 when the player cancelled the dialog,
 * and -1 when the dialog could not be shown (or in headless/test runs).
 */
int HostShowDiscPicker(char *path, size_t size);
void HostShowErrorDialog(const char *title, const char *message);
/* Returns 1 when SDL can create a GPU device for the renderer's SPIR-V/MSL
 * shaders (Vulkan or Metal), 0 when no backend is usable on this machine. */
int HostGpuBackendAvailable(void);

#endif
