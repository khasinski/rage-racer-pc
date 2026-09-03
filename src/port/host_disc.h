#ifndef RAGE_HOST_DISC_H
#define RAGE_HOST_DISC_H

#include <stddef.h>

int HostInitDisc(void);
int HostDumpArchive(const char *path);
int HostReadStreamSector(unsigned int sector, unsigned char *raw);
unsigned int HostStreamSectorSpan(int stream);
int HostStreamAbsoluteRange(unsigned int firstSector,
                            unsigned int sectorCount, int *absoluteFirst,
                            int *absoluteEnd);

#endif
