#ifndef RAGE_ONLINE_RTC_LINK_H
#define RAGE_ONLINE_RTC_LINK_H
#include <stddef.h>
#include <stdint.h>
typedef struct OnlineRtc OnlineRtc;
typedef void (*OnlineRtcSignal)(void *,const char *,size_t);
typedef void (*OnlineRtcFrame)(void *,const uint8_t *,size_t);
/* Worker-thread lifecycle; callbacks only publish into bounded transport queues. */
OnlineRtc *OnlineRtcCreate(void *,OnlineRtcSignal,OnlineRtcFrame);
int OnlineRtcSignalMessage(OnlineRtc *,const char *,size_t);
int OnlineRtcTick(OnlineRtc *,int *ping);
int OnlineRtcSend(OnlineRtc *,const void *,size_t);
int OnlineRtcReady(OnlineRtc *);
void OnlineRtcFree(OnlineRtc *);
#endif
