#include "rtc_link.h"
#include <rtc/rtc.h>
#include <SDL3/SDL.h>
#include "yyjson.h"
#include "miniz.h"
#include <stdlib.h>
#include <string.h>

struct OnlineRtc {
    int pc,channel,open,confirmed,closed,ping;
    uint64_t started,echo,beat;
    SDL_Mutex *mutex;
    void *context;OnlineRtcSignal signal;OnlineRtcFrame frame;
};
/* C API callbacks may already be queued when an id is deleted. Resolve ids
 * under our own registry lock rather than dereferencing a stale user pointer.
 * Removing an owner waits for the last callback holding its mutex. */
static SDL_SpinLock registryLock;
static OnlineRtc *registry[8];
static OnlineRtc *acquire(int id){
    OnlineRtc *found=NULL;SDL_LockSpinlock(&registryLock);
    for(unsigned i=0;i<SDL_arraysize(registry);i++)if(registry[i]&&(registry[i]->pc==id||registry[i]->channel==id)){found=registry[i];SDL_LockMutex(found->mutex);break;}
    SDL_UnlockSpinlock(&registryLock);return found;
}
static void emitSignal(OnlineRtc *link,const char *type,const char *key,const char *value,const char *mid){
    yyjson_mut_doc *d=yyjson_mut_doc_new(NULL);if(!d)return;
    yyjson_mut_val *r=yyjson_mut_obj(d);yyjson_mut_doc_set_root(d,r);
    yyjson_mut_obj_add_str(d,r,"t",type);yyjson_mut_obj_add_str(d,r,key,value);
    if(mid)yyjson_mut_obj_add_str(d,r,"mid",mid);
    size_t n;char *json=yyjson_mut_write(d,0,&n);if(json){link->signal(link->context,json,n);free(json);}yyjson_mut_doc_free(d);
}
static void RTC_API description(int id,const char *sdp,const char *type,void *ptr){
    (void)ptr;OnlineRtc *link=acquire(id);if(!link)return;
    if(!strcmp(type,"offer"))emitSignal(link,"rtcOffer","sdp",sdp,NULL);
    SDL_UnlockMutex(link->mutex);
}
static void RTC_API candidate(int id,const char *value,const char *mid,void *ptr){
    (void)ptr;OnlineRtc *link=acquire(id);if(!link)return;
    emitSignal(link,"rtcCandidate","candidate",value,mid);SDL_UnlockMutex(link->mutex);
}
static void RTC_API opened(int id,void *ptr){
    (void)ptr;OnlineRtc *link=acquire(id);if(!link)return;
    link->open=1;link->echo=SDL_GetTicks();SDL_UnlockMutex(link->mutex);
}
static void RTC_API closed(int id,void *ptr){
    (void)ptr;OnlineRtc *link=acquire(id);if(!link)return;
    link->open=link->confirmed=0;link->closed=1;SDL_UnlockMutex(link->mutex);
}
static void RTC_API message(int id,const char *data,int size,void *ptr){
    (void)ptr;if(size<=0||size>65536)return;
    OnlineRtc *link=acquire(id);if(!link)return;
    if((uint8_t)data[0]==3&&size==9){
        uint64_t sent=0;for(unsigned i=0;i<8;i++)sent|=(uint64_t)(uint8_t)data[i+1]<<(8*i);
        uint64_t now=SDL_GetTicks();link->echo=now;link->confirmed=1;
        link->ping=sent<=now&&now-sent<10000?(int)(now-sent):-1;
    }else{
        /* Server RTC frames are raw DEFLATE. Bound decompression before any
         * protocol parser, even if a peer sends a compressed expansion bomb. */
        uint8_t *out=malloc(65536);
        if(out){size_t n=tinfl_decompress_mem_to_mem(out,65536,data,(size_t)size,0);
            if(n!=TINFL_DECOMPRESS_MEM_TO_MEM_FAILED&&n>=6&&(out[0]==2||out[0]==4||out[0]==5))link->frame(link->context,out,n);
            free(out);}
    }
    SDL_UnlockMutex(link->mutex);
}
OnlineRtc *OnlineRtcCreate(void *context,OnlineRtcSignal send,OnlineRtcFrame frame){
    OnlineRtc *link=calloc(1,sizeof(*link));if(!link)return NULL;
    link->pc=link->channel=-1;link->context=context;link->signal=send;link->frame=frame;
    link->mutex=SDL_CreateMutex();if(!link->mutex){free(link);return NULL;}
    rtcConfiguration config={0};config.disableAutoNegotiation=true;config.maxMessageSize=65536;
    link->pc=rtcCreatePeerConnection(&config);if(link->pc<0){SDL_DestroyMutex(link->mutex);free(link);return NULL;}
    SDL_LockSpinlock(&registryLock);int registered=0;
    for(unsigned i=0;i<SDL_arraysize(registry);i++)if(!registry[i]){registry[i]=link;registered=1;break;}
    SDL_UnlockSpinlock(&registryLock);
    if(!registered){OnlineRtcFree(link);return NULL;}
    rtcSetUserPointer(link->pc,(void*)1);
    rtcSetLocalDescriptionCallback(link->pc,description);rtcSetLocalCandidateCallback(link->pc,candidate);
    rtcDataChannelInit init={0};init.reliability.unordered=true;init.reliability.unreliable=true;init.reliability.maxRetransmits=0;
    int channel=rtcCreateDataChannelEx(link->pc,"race",&init);
    if(channel<0){OnlineRtcFree(link);return NULL;}
    SDL_LockSpinlock(&registryLock);link->channel=channel;SDL_UnlockSpinlock(&registryLock);
    rtcSetUserPointer(channel,(void*)1);rtcSetOpenCallback(channel,opened);rtcSetClosedCallback(channel,closed);rtcSetMessageCallback(channel,message);
    link->started=SDL_GetTicks();
    if(rtcSetLocalDescription(link->pc,"offer")<0){OnlineRtcFree(link);return NULL;}
    return link;
}
int OnlineRtcSignalMessage(OnlineRtc *link,const char *text,size_t size){
    if(!link)return 0;yyjson_doc *d=yyjson_read(text,size,0);if(!d)return 0;
    yyjson_val *root=yyjson_doc_get_root(d);const char *t=yyjson_get_str(yyjson_obj_get(root,"t"));int handled=0;
    if(t&&!strcmp(t,"rtcAnswer")){const char *sdp=yyjson_get_str(yyjson_obj_get(root,"sdp"));if(sdp)rtcSetRemoteDescription(link->pc,sdp,"answer");handled=1;}
    if(t&&!strcmp(t,"rtcCandidate")){const char *c=yyjson_get_str(yyjson_obj_get(root,"candidate")),*mid=yyjson_get_str(yyjson_obj_get(root,"mid"));if(c)rtcAddRemoteCandidate(link->pc,c,mid?mid:"0");handled=1;}
    yyjson_doc_free(d);return handled;
}
int OnlineRtcReady(OnlineRtc *link){if(!link)return 0;SDL_LockMutex(link->mutex);int ready=link->open&&link->confirmed&&!link->closed;SDL_UnlockMutex(link->mutex);return ready;}
int OnlineRtcTick(OnlineRtc *link,int *ping){
    if(!link)return 0;uint64_t now=SDL_GetTicks();SDL_LockMutex(link->mutex);
    int alive=!link->closed&&(link->open?now-link->echo<=2000:now-link->started<=10000);
    int beat=alive&&link->open&&now-link->beat>=500;
    if(beat)link->beat=now;if(ping)*ping=link->ping;SDL_UnlockMutex(link->mutex);
    if(beat){uint8_t p[9]={3};for(unsigned i=0;i<8;i++)p[i+1]=(uint8_t)(now>>(i*8));if(rtcSendMessage(link->channel,(char*)p,9)<0)alive=0;}
    return alive;
}
int OnlineRtcSend(OnlineRtc *link,const void *bytes,size_t size){
    if(!OnlineRtcReady(link)||size>65536)return 0;
    /* Do not queue stale driver input behind a growing SCTP backlog. */
    if(rtcGetBufferedAmount(link->channel)>16384)return 0;
    return rtcSendMessage(link->channel,bytes,(int)size)>=0;
}
void OnlineRtcFree(OnlineRtc *link){
    if(!link)return;SDL_LockSpinlock(&registryLock);
    for(unsigned i=0;i<SDL_arraysize(registry);i++)if(registry[i]==link)registry[i]=NULL;
    SDL_LockMutex(link->mutex);SDL_UnlockSpinlock(&registryLock);SDL_UnlockMutex(link->mutex);
    if(link->channel>=0){rtcSetMessageCallback(link->channel,NULL);rtcSetOpenCallback(link->channel,NULL);rtcSetClosedCallback(link->channel,NULL);rtcDeleteDataChannel(link->channel);}
    if(link->pc>=0){rtcSetLocalDescriptionCallback(link->pc,NULL);rtcSetLocalCandidateCallback(link->pc,NULL);rtcDeletePeerConnection(link->pc);}
    SDL_DestroyMutex(link->mutex);free(link);
}
