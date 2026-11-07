#ifndef RAGE_DISC_CUE_H
#define RAGE_DISC_CUE_H

#include <stddef.h>

/* Resolves the 2352-byte data track named by a CUE sheet. The returned offset
 * is measured in bytes from the start of that file. */
int DiscCueResolveDataTrack(const char *cue_path, char *image_path,
                            size_t image_path_size, long *track_offset);

#endif
