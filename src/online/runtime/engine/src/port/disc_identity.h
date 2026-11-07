#ifndef RAGE_DISC_IDENTITY_H
#define RAGE_DISC_IDENTITY_H
#include "disc_iso.h"

/* Read only the boot serial, without scanning movie data. Output is unchanged
 * when identification fails. The reader must already have an open ISO. */
int DiscReadBootName(DiscIsoReader *reader, char *boot, size_t bootSize);
#endif
