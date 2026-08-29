#ifndef RAGE_PORT_COMPAT_H
#define RAGE_PORT_COMPAT_H

#include <stdio.h>
#include <libetc.h>

/* Game-facing APIs absent from the current PSY-Z public headers. Keep these
 * declarations in one place until their implementations can move upstream. */
long SpuTransferStatus(void *address, long mode);
/* Spelled out rather than in the game's s32/u8/u16, because this header is
 * force-included ahead of the one that defines those. */
int InitSoundWithVab(unsigned char *header, unsigned char *body);
int LoadExtraVabSlotWithTable(unsigned char *header, unsigned char *body,
                              unsigned short *table);
void SetEffectVoicesEnabled(int enabled);
void SetReverbPreset(int type, int left, int right);
struct CdlLOC;
long StGetBackloc(struct CdlLOC *location);
void StUnSetRing(void);
int HostLoadArchiveIndex(void *entries, int count);
int HostLoadAsset(unsigned int byte_offset, unsigned int size,
                  void *destination);
int PortShouldExit(int frame_number);
void PortBeforeSceneHandler(void);
void PortAfterSceneHandler(void);
void PortDuringFrameWait(int frameLimit);
int PortMirrorFarDepth(int retailFar);
void PortSampleAnalogPad(void);

#endif
