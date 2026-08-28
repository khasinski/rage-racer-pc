#ifndef RAGE_PC_PSYQ_SND_INTERNAL_H
#define RAGE_PC_PSYQ_SND_INTERNAL_H

#include "common.h"
#include "psyq/snd_types.h"
#include "psyq/spu_internal_types.h"

typedef void (*Callback)(void);

extern SeqStruct *g_SndSeqTable[];
extern ProgAtr *g_SndCurrentProgTable;
extern VagAtr *g_SndCurrentToneTable;
extern VabHdr *g_SndCurrentVabHeader;
extern VabHdr *g_SndVabHeader[];
extern ProgAtr *g_SndVabProgTable[];
extern VagAtr *g_SndVabToneTable[];
extern long g_SndTickResolution;
extern long g_SndUpdateLock;
extern long g_SndVabBodySize[];
extern short g_SndDamper;
extern short g_SndMonoMode;
extern u_char g_SndReservedVoiceCount;
extern SpuReverbAttr g_SndReverbAttr;
/* Retail stored these work areas contiguously at fixed PS1 addresses.  Host
 * globals are independently aligned/reordered, so pointer arithmetic across
 * the generated symbols corrupts unrelated state and drops voice updates. */
extern SpuCommonRegs *g_HostSndSpuRegs;
extern SpuVoiceRegs g_HostSndVoiceRegs[24];
extern u_char g_HostSndVoiceFlags[24];
extern SpuVoice g_HostSndVoiceState[24];
#define g_SndSpuRegs g_HostSndSpuRegs
#define g_SndVoiceRegs g_HostSndVoiceRegs
#define g_SndVoiceFlags g_HostSndVoiceFlags
#define g_SndVoiceState g_HostSndVoiceState
extern SvmCurrentAttr g_SndCurrentAttr;
extern short g_SndCurrentSeqSep;
#ifndef SND_CURRENT_VOICE_QUALIFIER
#define SND_CURRENT_VOICE_QUALIFIER
#endif
#ifndef SND_CURRENT_VOICE_TYPE
#define SND_CURRENT_VOICE_TYPE short
#endif
extern SND_CURRENT_VOICE_QUALIFIER SND_CURRENT_VOICE_TYPE g_SndCurrentVoice;
#undef SND_CURRENT_VOICE_QUALIFIER
#undef SND_CURRENT_VOICE_TYPE
extern u_short g_SndReverbOnHigh;
extern u_short g_SndReverbOnLow;
extern u_short g_SndVabOpenCount;
extern short g_SndVabProgMax;

extern volatile u_short D_801E4B5C;
extern char g_MsgSeqNotSeqData[];
extern char g_MsgSeqOldFormat[];
extern char g_MsgSeqTableFull[];
extern u_char g_SndCurrentMasterPan;
extern u_char g_SndCurrentMasterVolume;
extern u_char g_SndCurrentPan;
extern u_char g_SndCurrentTone;
extern short g_SndCurrentToneIndex;
extern u_char g_SndCurrentToneMode;
extern u_char g_SndCurrentTonePan;
extern u_char g_SndCurrentToneVolume;
extern u_short g_SndCurrentVag;
extern short g_SndCurrentVoiceRegOffset;
extern u_char g_SndCurrentVolume;
extern Callback g_SndPrevVSyncCallback;
extern long g_SndSeqOpenMask;
extern Callback g_SndTickCallback;

#ifndef SND_KEY_ON_QUALIFIER
#define SND_KEY_ON_QUALIFIER
#endif
#ifndef SND_KEY_OFF_QUALIFIER
#define SND_KEY_OFF_QUALIFIER
#endif
#ifndef SND_VOICE_COUNT_QUALIFIER
#define SND_VOICE_COUNT_QUALIFIER
#endif
extern SND_KEY_ON_QUALIFIER u_short g_SndKeyOnLow;
extern SND_KEY_ON_QUALIFIER u_short g_SndKeyOnHigh;
extern SND_KEY_OFF_QUALIFIER u_short g_SndKeyOffLow;
extern SND_KEY_OFF_QUALIFIER u_short g_SndKeyOffHigh;
extern SND_VOICE_COUNT_QUALIFIER u_char g_SndVoiceCount;
#undef SND_KEY_ON_QUALIFIER
#undef SND_KEY_OFF_QUALIFIER
#undef SND_VOICE_COUNT_QUALIFIER

#endif
