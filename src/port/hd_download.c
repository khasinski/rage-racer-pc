#include "hd_download.h"
#ifdef RAGE_HD_NETWORK_DISABLED
#include <stdio.h>
int HdDownloadParseCatalog(const char *json,size_t size,const char *base,const char *region,HdRelease *out){
    (void)json;(void)size;(void)base;(void)region;(void)out;return 0;
}
int HdDownloadInstall(const char *base,const char *store,const char *region,HdDownloadProgress progress,void *context,char *error,size_t cap){
    (void)base;(void)store;(void)region;(void)progress;(void)context;
    if(error&&cap)snprintf(error,cap,"Online HD downloads are unavailable on this platform. Install a local pack.");return 0;
}
#else
#include "content/sha256.h"

#include "yyjson.h"
#include <SDL3/SDL.h>
#ifdef _WIN32
/* Windows uses the operating system's WinHTTP stack instead of a statically
 * linked curl/Schannel/WinSock client. The bundled client made the game
 * executable look like a self-contained downloader to Microsoft Defender's
 * machine-learning heuristics (Trojan:Win32/Wacatac.C!ml false positive in
 * 0.6.14) and duplicated TLS, proxy and certificate handling that Windows
 * already provides. WinSock's 32-bit u_long differs from the pointer-width
 * PS1 compatibility typedef, so keep any declaration local. */
#define u_long RageDownloadWinULong
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>
#undef u_long
#else
#include <curl/curl.h>
#endif

#include <stdio.h>
#include <string.h>

typedef struct Transfer {
    SDL_IOStream *file;
    char catalog[16385];
    size_t used;
    uint64_t size, written;
    Sha256 sha;
    HdDownloadProgress progress;
    void *context;
} Transfer;
static int baseOK(const char *base) {
    if (!base || !*base || strlen(base) > 800 ||
        strpbrk(base, "\r\n\t ?#@\\") || base[strlen(base)-1]=='/') return 0;
    if (!strncmp(base,"https://",8)) return base[8] && !strchr(base+8,'/');
    unsigned port; char tail;
    return sscanf(base,"http://127.0.0.1:%u%c",&port,&tail)==1 && port && port<=65535;
}
static int text(yyjson_val *root, const char *key, char *out, size_t cap) {
    yyjson_val *v=yyjson_obj_get(root,key);const char *s=yyjson_get_str(v);
    size_t n=yyjson_get_len(v);
    if(!s||!n||n>=cap||strlen(s)!=n) return 0;
    for(size_t i=0;i<n;i++)if((unsigned char)s[i]<32)return 0;
    memcpy(out,s,n+1);return 1;
}
int HdDownloadParseCatalog(const char *json,size_t size,const char *base,
                          const char *region,HdRelease *out) {
    if(!baseOK(base)||!region||size>16384)return 0;
    yyjson_doc *doc=yyjson_read(json,size,0);if(!doc)return 0;
    yyjson_val *root=yyjson_doc_get_root(doc);HdRelease r={0};char format[32],expected[1200];
    yyjson_val *bytes=yyjson_obj_get(root,"size");
    int ok=yyjson_is_obj(root)&&text(root,"format",format,sizeof(format))&&
        !strcmp(format,"rrhd-webp-v1")&&text(root,"version",r.version,sizeof(r.version))&&
        text(root,"region",r.region,sizeof(r.region))&&!strcmp(region,r.region)&&
        text(root,"sha256",r.sha256,sizeof(r.sha256))&&strlen(r.sha256)==64&&
        text(root,"url",r.url,sizeof(r.url))&&yyjson_is_uint(bytes);
    if(ok){
        r.size=yyjson_get_uint(bytes);ok=r.size>0&&r.size<=UINT64_C(1074077200)&&!strstr(r.version,"..");
        for(const char *p=r.version;*p;p++)if(!strchr("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._-",*p))ok=0;
        for(const char *p=r.sha256;*p;p++)if(!strchr("0123456789abcdef",*p))ok=0;
        if(strcmp(region,"PAL")&&strcmp(region,"NTSC-J")&&strcmp(region,"NTSC-U"))ok=0;
        snprintf(expected,sizeof(expected),"%s/hd-%s-v%s.rrhd",base,region,r.version);
        if(strcmp(expected,r.url))ok=0;
    }
    if(ok){*out=r;}yyjson_doc_free(doc);return ok;
}
static size_t writeBytes(char *data,size_t a,size_t b,void *context) {
    Transfer *t=context;if(a&&b>SIZE_MAX/a)return 0;size_t n=a*b;
    if(t->progress&&!t->progress(t->context,t->file?"Downloading":"Checking for updates",t->written,t->size))return 0;
    if(t->file){
        if(t->written>t->size||n>t->size-t->written)return 0;
        size_t wrote=SDL_WriteIO(t->file,data,n);Sha256Update(&t->sha,data,wrote);t->written+=wrote;return wrote;
    }
    if(n>sizeof(t->catalog)-1-t->used)return 0;
    memcpy(t->catalog+t->used,data,n);t->used+=n;t->catalog[t->used]=0;return n;
}
static int cancelled(Transfer *t) {
    return t->progress&&!t->progress(t->context,t->file?"Downloading":"Checking for updates",t->written,t->size);
}
#ifdef _WIN32
#define USER_AGENT L"RageRacer/0.6 (HD packs)"
enum { CONNECT_TIMEOUT_MS=10000, STALL_TIMEOUT_MS=60000 };
static const Uint64 TRANSFER_TIMEOUT_MS=UINT64_C(7200000);
static int transportInit(void) { return 1; }
static void transportCleanup(void) {}
static wchar_t *widen(const char *text) {
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,NULL,0);
    wchar_t *wide=n>0?SDL_malloc((size_t)n*sizeof(*wide)):NULL;
    if(wide&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,wide,n)!=n){SDL_free(wide);wide=NULL;}
    return wide;
}
/* Same contract as the curl transport: HTTPS with certificate and host-name
 * validation, explicit loopback HTTP only after baseOK, no redirects, 10 s
 * connect timeout, a 60 s stall limit, a two-hour ceiling and success only for
 * a complete 200 response. */
static int request(const char *url,Transfer *t) {
    HINTERNET session=NULL,connection=NULL,handle=NULL;
    wchar_t *wide=widen(url),host[256];char *buffer=NULL;int ok=0;
    URL_COMPONENTS parts;DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    DWORD status=0,length=sizeof(status);const Uint64 started=SDL_GetTicks();
    if(!wide)return 0;
    memset(&parts,0,sizeof(parts));parts.dwStructSize=sizeof(parts);
    parts.lpszHostName=host;parts.dwHostNameLength=(DWORD)(sizeof(host)/sizeof(host[0]));
    parts.dwUrlPathLength=(DWORD)-1;parts.dwExtraInfoLength=(DWORD)-1;
    if(!WinHttpCrackUrl(wide,0,0,&parts)||!host[0])goto done;
    if(parts.nScheme!=INTERNET_SCHEME_HTTPS&&parts.nScheme!=INTERNET_SCHEME_HTTP)goto done;
#ifdef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
    session=WinHttpOpen(USER_AGENT,WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
#endif
    /* Windows 8 and older do not know the automatic proxy access type. */
    if(!session)session=WinHttpOpen(USER_AGENT,WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
    if(!session||!WinHttpSetTimeouts(session,CONNECT_TIMEOUT_MS,CONNECT_TIMEOUT_MS,STALL_TIMEOUT_MS,STALL_TIMEOUT_MS))goto done;
    connection=WinHttpConnect(session,host,parts.nPort,0);if(!connection)goto done;
    handle=WinHttpOpenRequest(connection,L"GET",parts.dwUrlPathLength?parts.lpszUrlPath:L"/",NULL,
        WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,parts.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0);
    if(!handle||!WinHttpSetOption(handle,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof(policy)))goto done;
    if(cancelled(t))goto done;
    if(!WinHttpSendRequest(handle,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||
       !WinHttpReceiveResponse(handle,NULL))goto done;
    if(!WinHttpQueryHeaders(handle,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,&status,&length,WINHTTP_NO_HEADER_INDEX)||status!=200)goto done;
    buffer=SDL_malloc(65536);if(!buffer)goto done;
    for(;;){
        DWORD got=0;
        if(cancelled(t)||SDL_GetTicks()-started>TRANSFER_TIMEOUT_MS)goto done;
        if(!WinHttpReadData(handle,buffer,65536,&got))goto done;
        if(!got)break;
        if(writeBytes(buffer,1,got,t)!=got)goto done;
    }
    ok=1;
done:
    SDL_free(buffer);
    if(handle)WinHttpCloseHandle(handle);
    if(connection)WinHttpCloseHandle(connection);
    if(session)WinHttpCloseHandle(session);
    SDL_free(wide);return ok;
}
#else
static int transportInit(void) { return curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK; }
static void transportCleanup(void) { curl_global_cleanup(); }
static int tick(void *context,curl_off_t total,curl_off_t now,curl_off_t up,curl_off_t sent) {
    (void)total;(void)now;(void)up;(void)sent;return cancelled(context);
}
static int request(const char *url,Transfer *t) {
    CURL *c=curl_easy_init();if(!c)return 0;
    curl_easy_setopt(c,CURLOPT_URL,url);
    curl_easy_setopt(c,CURLOPT_USERAGENT,"RageRacer/" "0.6" " (HD packs)");
    curl_easy_setopt(c,CURLOPT_PROTOCOLS_STR,"https,http");
    curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,0L);
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,10L);curl_easy_setopt(c,CURLOPT_TIMEOUT,7200L);
    curl_easy_setopt(c,CURLOPT_LOW_SPEED_LIMIT,1024L);curl_easy_setopt(c,CURLOPT_LOW_SPEED_TIME,60L);
    curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(c,CURLOPT_FAILONERROR,1L);
    curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,writeBytes);curl_easy_setopt(c,CURLOPT_WRITEDATA,t);
    curl_easy_setopt(c,CURLOPT_XFERINFOFUNCTION,tick);curl_easy_setopt(c,CURLOPT_XFERINFODATA,t);
    curl_easy_setopt(c,CURLOPT_NOPROGRESS,0L);
    CURLcode result=curl_easy_perform(c);long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);
    curl_easy_cleanup(c);return result==CURLE_OK&&status==200;
}
#endif
static int installProgress(void *context,int percent) {
    Transfer *t=context;return !t->progress||t->progress(t->context,"Installing and verifying",(uint64_t)percent,100);
}
static int verifyProgress(void *context,int percent) {
    Transfer *t=context;return !t->progress||t->progress(t->context,"Verifying installed pack",(uint64_t)percent,100);
}
int HdDownloadInstall(const char *base,const char *store,const char *region,
                      HdDownloadProgress progress,void *context,char *error,size_t cap) {
    char url[1200],archive[1800]={0},hash[65];HdRelease release;
    Transfer t={0};int ok=0,initialized=0;t.progress=progress;t.context=context;
    if(error&&cap)SDL_strlcpy(error,"Download failed or cancelled. Previous pack kept.",cap);
    if(!baseOK(base)) {if(error&&cap)SDL_strlcpy(error,"HD download server is not configured.",cap);return 0;}
    if(!transportInit()){goto done;}initialized=1;
    snprintf(url,sizeof(url),"%s/api/mods/hd/%s/latest",base,region);
    if(!request(url,&t)||!HdDownloadParseCatalog(t.catalog,t.used,base,region,&release)) {
        if(error&&cap){SDL_strlcpy(error,"Could not read a compatible HD release. Previous pack kept.",cap);}goto done;
    }
    HdPackState state;
    if(HdPackReadState(store,region,&state)&&!strcmp(state.hash,release.sha256)&&
       HdPackVerify(store,&state,verifyProgress,&t,error,cap)) {
        if(error&&cap){SDL_strlcpy(error,"HD pack is up to date and verified.",cap);}ok=2;goto done;
    }
    if(progress&&!progress(context,"Downloading",0,release.size))goto done;
    if(!SDL_CreateDirectory(store))goto done;
    snprintf(archive,sizeof(archive),"%s/download-%llu.partial",store,(unsigned long long)SDL_GetPerformanceCounter());
    t.file=SDL_IOFromFile(archive,"wb");t.size=release.size;Sha256Init(&t.sha);if(!t.file)goto done;
    int fetched=request(release.url,&t),closed=SDL_CloseIO(t.file);t.file=NULL;Sha256Finish(&t.sha,hash);
    if(!fetched||!closed||t.written!=release.size||strcmp(hash,release.sha256))goto done;
    if(!HdPackInstall(archive,store,region,installProgress,&t,error,cap))goto done;
    if(error&&cap){SDL_strlcpy(error,"HD pack downloaded and installed.",cap);}ok=1;
done:
    if(t.file)SDL_CloseIO(t.file);
    if(*archive)SDL_RemovePath(archive);
    if(initialized){transportCleanup();}return ok;
}

#endif
