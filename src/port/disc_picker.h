#ifndef RAGE_DISC_PICKER_H
#define RAGE_DISC_PICKER_H

#include <stddef.h>

/*
 * Ask the desktop for a disc image.
 *
 * The picker owns the asynchronous SDL dialog and answers with a path. Disc
 * validation and mounting remain in the caller, so this module has no disc
 * format or game-state policy.
 */
int HostShowDiscPicker(char *path, size_t size);

#endif
