#ifndef RAGE_FMV_AUDIO_H
#define RAGE_FMV_AUDIO_H

void HostFmvAudioStart(unsigned int firstSector, unsigned int sectorCount);
void HostFmvAudioAllowTail(void);
void HostFmvAudioEnd(void);
void HostFmvAudioTick(void);
int FmvXaStreaming(void);

#endif
