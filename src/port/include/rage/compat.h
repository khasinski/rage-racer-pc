#ifndef RAGE_PORT_COMPAT_H
#define RAGE_PORT_COMPAT_H

#include <stdio.h>
#include <libetc.h>

/* Game-facing APIs absent from the current PSY-Z public headers. Keep these
 * declarations in one place until their implementations can move upstream. */
long SpuTransferStatus(void *address, long mode);
/* Spelled out rather than in the game's s32/u8/u16, because this header is
 * force-included ahead of the one that defines those. */
void SetReverbPreset(int type, int left, int right);
struct CdlLOC;
int HostLoadArchiveIndex(void *entries, int count);
int HostLoadAsset(unsigned int byte_offset, unsigned int size,
                  void *destination);
int PortShouldExit(int frame_number);
void PortBeforeSceneHandler(void);
void PortAfterSceneHandler(void);
void PortDuringFrameWait(int frameLimit);
int PortMirrorFarDepth(int retailFar);
void PortSampleAnalogPad(void);

/*
 * Says that a case falling into the next one is meant. A comment saying so is
 * not enough for gcc, which fails the build over it.
 */
#if defined(__GNUC__) || defined(__clang__)
#define RAGE_FALLTHROUGH __attribute__((fallthrough))
#else
#define RAGE_FALLTHROUGH ((void)0)
#endif

#endif
