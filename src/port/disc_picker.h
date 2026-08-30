#ifndef RAGE_DISC_PICKER_H
#define RAGE_DISC_PICKER_H

#include <stddef.h>

/*
 * Ask the desktop for a disc image.
 *
 * This is deliberately its own translation unit. It talks to SDL, and the
 * platform layer that wants it is compiled with the PS1 compatibility header
 * forced in ahead of everything else; on Windows the two meet inside the
 * toolchain's own intrinsics header and the build comes apart there rather
 * than in any code of ours.
 *
 * It answers with a path and nothing more. Whether that path is a disc this
 * build can read is the caller's question, not the picker's.
 */
int HostShowDiscPicker(char *path, size_t size);

#endif
