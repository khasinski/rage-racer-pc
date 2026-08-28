#ifndef RAGE_CHD_DISC_H
#define RAGE_CHD_DISC_H

int RageChdOpen(const char *path);
void RageChdClose(void);
int RageChdReadRawSector(unsigned int sector, unsigned char *raw);

#endif
