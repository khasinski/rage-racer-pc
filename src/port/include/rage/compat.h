#ifndef RAGE_PORT_COMPAT_H
#define RAGE_PORT_COMPAT_H

#include <stdio.h>
#include <libetc.h>

/* Game-facing APIs absent from the current PSY-Z public headers. Keep these
 * declarations in one place until their implementations can move upstream. */
long SpuTransferStatus(void *address, long mode);
int InitSoundWithVab();
int LoadExtraVabSlotWithTable();
void SetEffectVoicesEnabled(int enabled);
void SetReverbPreset(int type, int left, int right);
struct CdlLOC;
void DecDCTout(volatile unsigned long *cells, long size);
void DecDCToutCallback(long callback);
long StGetBackloc(struct CdlLOC *location);
void StUnSetRing(void);
int RageHostLoadArchiveIndex(void *entries, int count);
int RageHostLoadAsset(unsigned int byte_offset, unsigned int size, void *destination);
int RagePortShouldExit(int frame_number);
void RagePortBeforeSceneHandler(void);

#endif
