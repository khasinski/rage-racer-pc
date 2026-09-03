#ifndef RAGE_PORT_PSYQ_SND_H
#define RAGE_PORT_PSYQ_SND_H

#include <libsnd.h>
#include <libspu.h>

void SpuVmDamperStep(void);
void SsSeqCalledTbyT(void);
void SsSetSpuInputAttr(unsigned char source, unsigned char field, unsigned char value);
unsigned char SsSetVoiceCount(unsigned char voices);
void ssinit(void);
void _SsVmInit(int voices);
void SsSeqCloseWrapper(short sequence);

#endif
