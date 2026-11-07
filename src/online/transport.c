#include "transport.h"
#include "rtc_link.h"
#include <SDL3/SDL.h>
#ifdef _WIN32
#define u_long RageOnlineCurlULong
#endif
#include <curl/curl.h>
#ifdef _WIN32
#undef u_long
#include <windows.h>
#include <bcrypt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yyjson.h"
enum { LIMIT=65536, QUEUE=16, SEND_LIMIT=4096 };
typedef struct Packet { int type; size_t size; uint8_t bytes[LIMIT+1]; } Packet;
typedef struct Outgoing { int binary; size_t size; uint8_t bytes[SEND_LIMIT]; } Outgoing;
struct OnlineTransport {
    SDL_Thread *thread; SDL_Mutex *mutex; SDL_AtomicInt stop,done,useRtc,fast;
    char base[512],action[24],auth[512];
    Packet incoming[QUEUE],frame;
    unsigned read,write; int haveFrame;
    Outgoing outgoing[64]; unsigned sendRead,sendWrite;
};
static uint32_t rol(uint32_t x,unsigned n){return (x<<n)|(x>>(32-n));}
static void sha1(const uint8_t *data,size_t size,uint8_t digest[20]) {
    uint8_t bytes[256]={0}; uint32_t h[5]={0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0};
    if(size>183){memset(digest,0,20);return;}
    memcpy(bytes,data,size);bytes[size]=0x80;size_t blocks=(size+9+63)/64;
    uint64_t bits=(uint64_t)size*8;
    for(unsigned i=0;i<8;i++)bytes[blocks*64-1-i]=(uint8_t)(bits>>(i*8));
    for(size_t block=0;block<blocks;block++){
        uint32_t w[80],a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];
        const uint8_t*p=bytes+block*64;
        for(unsigned i=0;i<16;i++)w[i]=(uint32_t)p[i*4]<<24|(uint32_t)p[i*4+1]<<16|(uint32_t)p[i*4+2]<<8|p[i*4+3];
        for(unsigned i=16;i<80;i++)w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        for(unsigned i=0;i<80;i++){
            uint32_t f=i<20?((b&c)|(~b&d)):i<40?(b^c^d):i<60?((b&c)|(b&d)|(c&d)):(b^c^d);
            uint32_t k=i<20?0x5a827999:i<40?0x6ed9eba1:i<60?0x8f1bbcdc:0xca62c1d6;
            uint32_t t=rol(a,5)+f+e+k+w[i];e=d;d=c;c=rol(b,30);b=a;a=t;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;
    }
    for(unsigned i=0;i<20;i++)digest[i]=(uint8_t)(h[i/4]>>(24-(i%4)*8));
}
static void base64(const uint8_t *bytes,size_t n,char*out){
    const char *alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";size_t at=0;
    for(size_t i=0;i<n;i+=3){unsigned v=(unsigned)bytes[i]<<16;if(i+1<n)v|=(unsigned)bytes[i+1]<<8;if(i+2<n)v|=bytes[i+2];
        out[at++]=alphabet[(v>>18)&63];out[at++]=alphabet[(v>>12)&63];out[at++]=i+1<n?alphabet[(v>>6)&63]:'=';out[at++]=i+2<n?alphabet[v&63]:'=';}
    out[at]=0;
}
void OnlineWebSocketAccept(const char *key,char out[29]){
    char text[160];uint8_t digest[20];
    int n=snprintf(text,sizeof(text),"%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11",key);
    if(n<0||n>=(int)sizeof(text)){out[0]=0;return;}sha1((uint8_t*)text,(size_t)n,digest);base64(digest,20,out);
}
static int randomBytes(void*out,size_t n){
#ifdef _WIN32
    return BCryptGenRandom(NULL,(PUCHAR)out,(ULONG)n,BCRYPT_USE_SYSTEM_PREFERRED_RNG)==0;
#else
    FILE*f=fopen("/dev/urandom","rb");if(!f)return 0;int ok=fread(out,1,n,f)==n;fclose(f);return ok;
#endif
}
static void push(OnlineTransport*t,int type,const void*data,size_t size){
    if(size>LIMIT)return;SDL_LockMutex(t->mutex);
    Packet*p;
    if(type==ONLINE_FRAME){p=&t->frame;t->haveFrame=1;}
    else{if(type==ONLINE_TEXT){yyjson_doc*d=yyjson_read((const char*)data,size,0);if(d){const char*kind=yyjson_get_str(yyjson_obj_get(yyjson_doc_get_root(d),"t"));if(kind&&!strcmp(kind,"raceStart"))t->haveFrame=0;yyjson_doc_free(d);}}
        if(t->write-t->read>=QUEUE){SDL_UnlockMutex(t->mutex);SDL_SetAtomicInt(&t->stop,1);return;}p=&t->incoming[t->write++%QUEUE];}
    p->type=type;p->size=size;if(size)memcpy(p->bytes,data,size);p->bytes[size]=0;SDL_UnlockMutex(t->mutex);
}
static void error(OnlineTransport*t,const char*text){push(t,ONLINE_ERROR,text,strlen(text));}
static void rtcSignal(void *ctx,const char *text,size_t size){OnlineTransportSend(ctx,0,text,size);}
static void rtcFrame(void *ctx,const uint8_t *frame,size_t size){push(ctx,ONLINE_FRAME,frame,size);}
static size_t received(char*data,size_t a,size_t b,void*ctx){
    Packet*p=ctx;if(a&&b>SIZE_MAX/a)return 0;size_t n=a*b;if(n>LIMIT-p->size)return 0;
    memcpy(p->bytes+p->size,data,n);p->size+=n;p->bytes[p->size]=0;return n;
}
static int progress(void*ctx,curl_off_t a,curl_off_t b,curl_off_t c,curl_off_t d){(void)a;(void)b;(void)c;(void)d;return SDL_GetAtomicInt(&((OnlineTransport*)ctx)->stop);}
static void options(CURL*c,OnlineTransport*t){
    curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT_MS,8000L);
    curl_easy_setopt(c,CURLOPT_TIMEOUT_MS,12000L);curl_easy_setopt(c,CURLOPT_XFERINFOFUNCTION,progress);
    curl_easy_setopt(c,CURLOPT_XFERINFODATA,t);curl_easy_setopt(c,CURLOPT_NOPROGRESS,0L);
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(c,CURLOPT_PROTOCOLS_STR,"http,https");
}
static int sendBytes(CURL*c,OnlineTransport*t,const uint8_t*p,size_t n){
    uint64_t until=SDL_GetTicks()+5000;
    while(n&&!SDL_GetAtomicInt(&t->stop)){
        size_t sent=0;CURLcode r=curl_easy_send(c,p,n,&sent);
        if(r==CURLE_AGAIN){if(SDL_GetTicks()>until)return 0;SDL_Delay(2);continue;}
        if(r!=CURLE_OK||!sent)return 0;p+=sent;n-=sent;
    }return n==0;
}
static int sendFrame(CURL*c,OnlineTransport*t,unsigned opcode,const void*data,size_t n){
    uint8_t wire[SEND_LIMIT+16],mask[4];if(n>SEND_LIMIT||!randomBytes(mask,4))return 0;
    wire[0]=(uint8_t)(0x80|opcode);size_t at=2;
    if(n<126)wire[1]=(uint8_t)(0x80|n);else{wire[1]=0xfe;wire[2]=(uint8_t)(n>>8);wire[3]=(uint8_t)n;at=4;}
    memcpy(wire+at,mask,4);at+=4;const uint8_t*p=data;for(size_t i=0;i<n;i++)wire[at+i]=p[i]^mask[i&3];
    return sendBytes(c,t,wire,at+n);
}
static int worker(void*context){
    OnlineTransport*t=context;CURL*c=curl_easy_init();Packet*response=calloc(1,sizeof(*response));
    OnlineRtc *rtc=NULL;uint64_t rtcRetry=0;unsigned retryDelay=1000;
    if(!c||!response){error(t,"Cannot allocate connection.");goto done;}
    char url[1024],token[256]={0};snprintf(url,sizeof(url),"%s/api/%s",t->base,t->action);
    CURLcode result;
    if(!strcmp(t->action,"resume")){
        yyjson_doc*d=yyjson_read(t->auth,strlen(t->auth),0);
        const char*saved=d?yyjson_get_str(yyjson_obj_get(yyjson_doc_get_root(d),"token")):NULL;
        int valid=saved&&strlen(saved)<sizeof(token);
        if(valid)SDL_strlcpy(token,saved,sizeof(token));
        if(d)yyjson_doc_free(d);memset(t->auth,0,sizeof(t->auth));
        if(!valid){error(t,"Session is no longer available.");goto done;}
        goto socket_connection;
    }
    options(c,t);curl_easy_setopt(c,CURLOPT_URL,url);curl_easy_setopt(c,CURLOPT_POSTFIELDS,t->auth);
    struct curl_slist*headers=curl_slist_append(NULL,"Content-Type: application/json");
    curl_easy_setopt(c,CURLOPT_HTTPHEADER,headers);curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,received);curl_easy_setopt(c,CURLOPT_WRITEDATA,response);
    result=curl_easy_perform(c);curl_slist_free_all(headers);memset(t->auth,0,sizeof(t->auth));
    if(result!=CURLE_OK){error(t,SDL_GetAtomicInt(&t->stop)?"Connection cancelled.":curl_easy_strerror(result));goto done;}
    long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);
    yyjson_doc*doc=yyjson_read((char*)response->bytes,response->size,0);yyjson_val*root=doc?yyjson_doc_get_root(doc):NULL;
    const char*secret=yyjson_get_str(yyjson_obj_get(root,"token"));
    if(status<200||status>=300||!secret||strlen(secret)>=sizeof(token)){
        const char*why=yyjson_get_str(yyjson_obj_get(root,"error"));error(t,why?why:"Server rejected authentication.");if(doc)yyjson_doc_free(doc);goto done;}
    SDL_strlcpy(token,secret,sizeof(token));push(t,ONLINE_AUTH,response->bytes,response->size);yyjson_doc_free(doc);
 socket_connection:
    curl_easy_reset(c);options(c,t);curl_easy_setopt(c,CURLOPT_URL,t->base);curl_easy_setopt(c,CURLOPT_CONNECT_ONLY,1L);
    if(curl_easy_perform(c)!=CURLE_OK){error(t,"Cannot open the race connection.");goto done;}
    CURLU*u=curl_url();char*host=NULL,*port=NULL,*scheme=NULL;
    if(!u||curl_url_set(u,CURLUPART_URL,t->base,0)!=CURLUE_OK){if(u)curl_url_cleanup(u);error(t,"Invalid server address.");goto done;}
    curl_url_get(u,CURLUPART_HOST,&host,0);curl_url_get(u,CURLUPART_PORT,&port,0);curl_url_get(u,CURLUPART_SCHEME,&scheme,0);
    uint8_t nonce[16];char key[25],accept[29],request[2048];
    if(!host||!scheme||!randomBytes(nonce,sizeof(nonce))){curl_free(host);curl_free(port);curl_free(scheme);curl_url_cleanup(u);goto done;}
    base64(nonce,16,key);OnlineWebSocketAccept(key,accept);
    snprintf(request,sizeof(request),"GET /ws?token=%s HTTP/1.1\r\nHost: %s%s%s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n",token,host,port?":":"",port?port:"",key);
    memset(token,0,sizeof(token));curl_free(host);curl_free(port);curl_free(scheme);curl_url_cleanup(u);
    if(!sendBytes(c,t,(uint8_t*)request,strlen(request))){error(t,"WebSocket handshake could not be sent.");goto done;}
    memset(request,0,sizeof(request));
    uint8_t*rx=malloc(LIMIT*2u+32u);uint8_t*fragment=malloc(LIMIT+1u);size_t used=0,fragmentSize=0;unsigned fragmentOp=0;
    if(!rx||!fragment){free(rx);free(fragment);error(t,"Cannot allocate race buffers.");goto done;}
    int upgraded=0;uint64_t deadline=SDL_GetTicks()+10000;
    while(!SDL_GetAtomicInt(&t->stop)){
        if(upgraded){
            uint64_t now=SDL_GetTicks();
            if(SDL_GetAtomicInt(&t->useRtc)&&!rtc&&now>=rtcRetry){rtc=OnlineRtcCreate(t,rtcSignal,rtcFrame);rtcRetry=now+retryDelay;}
            if(rtc&&!OnlineRtcTick(rtc,NULL)){OnlineRtcFree(rtc);rtc=NULL;rtcRetry=now+retryDelay;if(retryDelay<30000)retryDelay*=2;}
            int fast=OnlineRtcReady(rtc);SDL_SetAtomicInt(&t->fast,fast);if(fast)retryDelay=1000;
            Outgoing packet;int have=0;SDL_LockMutex(t->mutex);
            if(t->sendRead!=t->sendWrite){packet=t->outgoing[t->sendRead++%64];have=1;}SDL_UnlockMutex(t->mutex);
            if(have&&!(packet.binary&&OnlineRtcSend(rtc,packet.bytes,packet.size))&&!sendFrame(c,t,packet.binary?2u:1u,packet.bytes,packet.size)){error(t,"Connection lost while sending.");break;}
        }
        size_t n=0;result=curl_easy_recv(c,rx+used,LIMIT*2u+31u-used,&n);
        if(result!=CURLE_OK&&result!=CURLE_AGAIN){error(t,"Connection lost.");break;}
        if(result==CURLE_OK&&!n){error(t,"Server closed the connection.");break;}
        used+=n;rx[used]=0;
        if(!upgraded){
            uint8_t*end=(uint8_t*)strstr((char*)rx,"\r\n\r\n");
            if(!end){if(used>8192||SDL_GetTicks()>deadline){error(t,"WebSocket handshake timed out.");break;}SDL_Delay(2);continue;}
            size_t header=(size_t)(end-rx)+4;char expected[96];snprintf(expected,sizeof(expected),"Sec-WebSocket-Accept: %s",accept);
            char*lower=SDL_strcasestr((char*)rx,"sec-websocket-accept:");
            if(strncmp((char*)rx,"HTTP/1.1 101",12)||!lower||strncmp(lower+22,accept,28)){
                error(t,"Server did not accept the WebSocket connection.");break;}
            memmove(rx,rx+header,used-header);used-=header;upgraded=1;
        }
        int failed=0;
        while(used>=2){
            unsigned op=rx[0]&15u;int fin=rx[0]&0x80;uint64_t size=rx[1]&127u;size_t offset=2;
            if((rx[0]&0x70)||(rx[1]&0x80)){failed=1;break;}
            if(size==126){if(used<4)break;size=(unsigned)rx[2]*256u+rx[3];offset=4;}
            else if(size==127){if(used<10)break;size=0;for(unsigned i=0;i<8;i++)size=size*256u+rx[2+i];offset=10;}
            if(size>LIMIT||(op>=8&&(!fin||size>125))){failed=1;break;}
            if(used<offset+(size_t)size)break;uint8_t*p=rx+offset;
            if(op==8){
                int replaced=size>=2&&((unsigned)p[0]*256u+p[1])==4001;
                const char *reason=replaced?"This account connected on another device.":"Server closed the connection.";
                push(t,replaced?ONLINE_REPLACED:ONLINE_CLOSED,reason,strlen(reason));
                SDL_SetAtomicInt(&t->stop,1);failed=1;break;
            }
            else if(op==9){if(!sendFrame(c,t,10,p,(size_t)size)){failed=1;break;}}
            else if(op==10){}
            else if(op==0||op==1||op==2){
                if((op&&fragmentOp)||(!op&&!fragmentOp)||fragmentSize+(size_t)size>LIMIT){failed=1;break;}
                if(op)fragmentOp=op;memcpy(fragment+fragmentSize,p,(size_t)size);fragmentSize+=(size_t)size;
                if(fin){if(fragmentOp==2||!OnlineRtcSignalMessage(rtc,(char*)fragment,fragmentSize))push(t,fragmentOp==2?ONLINE_FRAME:ONLINE_TEXT,fragment,fragmentSize);fragmentSize=0;fragmentOp=0;}
            }else{failed=1;break;}
            memmove(rx,rx+offset+(size_t)size,used-offset-(size_t)size);used-=offset+(size_t)size;
        }
        if(failed){if(!SDL_GetAtomicInt(&t->stop))error(t,"Race connection ended.");break;}
        SDL_Delay(1);
    }
    free(fragment);free(rx);
 done:
    OnlineRtcFree(rtc);SDL_SetAtomicInt(&t->fast,0);
    if(c)curl_easy_cleanup(c);free(response);SDL_SetAtomicInt(&t->done,1);return 0;
}
OnlineTransport *OnlineTransportStart(const char*base,const char*action,const char*json){
    static int initialized;
    if(!initialized){if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return NULL;initialized=1;}
    if(!base||strlen(base)>500||strpbrk(base,"\r\n ?#@")||
       (strncmp(base,"http://",7)&&strncmp(base,"https://",8))||!action||strlen(action)>20||!json||strlen(json)>500)return NULL;
    OnlineTransport*t=calloc(1,sizeof(*t));if(!t)return NULL;
    t->mutex=SDL_CreateMutex();if(!t->mutex){free(t);return NULL;}
    SDL_strlcpy(t->base,base,sizeof(t->base));size_t n=strlen(t->base);while(n&&t->base[n-1]=='/')t->base[--n]=0;
    SDL_strlcpy(t->action,action,sizeof(t->action));SDL_strlcpy(t->auth,json,sizeof(t->auth));
    t->thread=SDL_CreateThread(worker,"race-network",t);if(!t->thread){SDL_DestroyMutex(t->mutex);free(t);return NULL;}return t;
}
int OnlineTransportSend(OnlineTransport*t,int binary,const void*bytes,size_t size){
    if(!t||!bytes||size>SEND_LIMIT||SDL_GetAtomicInt(&t->stop)||SDL_GetAtomicInt(&t->done))return 0;
    SDL_LockMutex(t->mutex);if(t->sendWrite-t->sendRead>=64){SDL_UnlockMutex(t->mutex);return 0;}
    Outgoing*p=&t->outgoing[t->sendWrite++%64];p->binary=binary;p->size=size;memcpy(p->bytes,bytes,size);SDL_UnlockMutex(t->mutex);return 1;
}
int OnlineTransportPoll(OnlineTransport*t,int*type,void*out,size_t capacity,size_t*size){
    if(!t||!out||!type||!size)return 0;SDL_LockMutex(t->mutex);Packet*p=NULL;
    if(t->read!=t->write)p=&t->incoming[t->read%QUEUE];else if(t->haveFrame)p=&t->frame;
    if(!p||capacity<=p->size){SDL_UnlockMutex(t->mutex);return 0;}
    *size=p->size;*type=p->type;memcpy(out,p->bytes,p->size+1);
    if(p==&t->frame)t->haveFrame=0;else++t->read;SDL_UnlockMutex(t->mutex);return 1;
}
void OnlineTransportStop(OnlineTransport*t){if(t)SDL_SetAtomicInt(&t->stop,1);}
void OnlineTransportUseRtc(OnlineTransport*t){if(t)SDL_SetAtomicInt(&t->useRtc,1);}
int OnlineTransportFast(OnlineTransport*t){return t&&SDL_GetAtomicInt(&t->fast);}
int OnlineTransportDone(OnlineTransport*t){return !t||SDL_GetAtomicInt(&t->done);}
void OnlineTransportFree(OnlineTransport*t){if(!t)return;OnlineTransportStop(t);SDL_WaitThread(t->thread,NULL);SDL_DestroyMutex(t->mutex);memset(t,0,sizeof(*t));free(t);}
