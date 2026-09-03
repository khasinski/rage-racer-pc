#ifndef RAGE_PORT_PSYQ_SND_H
#define RAGE_PORT_PSYQ_SND_H

#include <libsnd.h>
#include <libspu.h>

void SpuVmDamperStep(void);
void SsSeqCalledTbyT(void);
unsigned char SsSetVoiceCount(unsigned char voices);
void _SsVmInit(int voices);

#endif
