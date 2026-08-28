#ifndef RAGE_CHD_DISC_H
#define RAGE_CHD_DISC_H

int ChdOpen(const char *path);
void ChdClose(void);
int ChdReadRawSector(unsigned int sector, unsigned char *raw);

#endif
