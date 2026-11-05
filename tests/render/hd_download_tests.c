#include "port/hd_download.h"
#include "port/modern/modern_image.h"
#include <SDL3/SDL.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <wchar.h>
#else
#include <curl/curl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static SDL_Process *server;
static void stop(void){if(server){SDL_KillProcess(server,true);SDL_WaitProcess(server,true,NULL);SDL_DestroyProcess(server);server=NULL;}}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s (%s)\n",__LINE__,#x,error);stop();return 1;}}while(0)
typedef struct Response {char body[32768];size_t used;} Response;
#ifdef _WIN32
/* Probe the origin through the same system HTTP stack the Windows game uses. */
static long get(const char *url,const char *range,int head,Response *response){
 wchar_t wide[1400],host[256],header[128];URL_COMPONENTS parts;long status=0;DWORD code=0,size=sizeof(code);
 if(!MultiByteToWideChar(CP_UTF8,0,url,-1,wide,1400))return 0;
 memset(&parts,0,sizeof(parts));parts.dwStructSize=sizeof(parts);parts.lpszHostName=host;parts.dwHostNameLength=256;parts.dwUrlPathLength=(DWORD)-1;
 if(!WinHttpCrackUrl(wide,0,0,&parts))return 0;
 HINTERNET s=WinHttpOpen(L"rage-hd-test",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
 HINTERNET c=s?WinHttpConnect(s,host,parts.nPort,0):NULL;
 HINTERNET r=c?WinHttpOpenRequest(c,head?L"HEAD":L"GET",parts.lpszUrlPath,NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,0):NULL;
 if(r)WinHttpSetTimeouts(s,5000,5000,5000,5000);
 if(r&&range)swprintf(header,128,L"Range: bytes=%hs",range);
 if(r&&WinHttpSendRequest(r,range?header:WINHTTP_NO_ADDITIONAL_HEADERS,range?(DWORD)-1L:0,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(r,NULL)&&
    WinHttpQueryHeaders(r,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&size,WINHTTP_NO_HEADER_INDEX)){
  status=(long)code;
  for(;;){DWORD got=0;if(!WinHttpReadData(r,response->body+response->used,(DWORD)(sizeof(response->body)-response->used),&got)){status=0;break;}if(!got)break;response->used+=got;if(response->used==sizeof(response->body)){status=0;break;}}
 }
 if(r)WinHttpCloseHandle(r);
 if(c)WinHttpCloseHandle(c);
 if(s)WinHttpCloseHandle(s);
 return status;
}
#else
static size_t receive(char *p,size_t a,size_t b,void *context){Response *r=context;size_t n=a*b;if(n>sizeof(r->body)-r->used)return 0;memcpy(r->body+r->used,p,n);r->used+=n;return n;}
static long get(const char *url,const char *range,int head,Response *response){
 CURL *c=curl_easy_init();curl_easy_setopt(c,CURLOPT_URL,url);curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(c,CURLOPT_WRITEDATA,response);curl_easy_setopt(c,CURLOPT_TIMEOUT,5L);if(range)curl_easy_setopt(c,CURLOPT_RANGE,range);if(head)curl_easy_setopt(c,CURLOPT_NOBODY,1L);CURLcode result=curl_easy_perform(c);long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);curl_easy_cleanup(c);return result==CURLE_OK?status:0;
}
#endif
static const char *tempRoot(void){
 const char *dir=getenv("TMPDIR");
#ifdef _WIN32
 if(!dir)dir=getenv("TEMP");
#endif
 return dir&&*dir?dir:"/tmp";
}
static uint64_t cancelledAfterBytes;
static int cancel(void *context,const char *stage,uint64_t done,uint64_t total){(void)context;(void)total;if(!strcmp(stage,"Downloading")&&done){cancelledAfterBytes=done;return 0;}return 1;}
int main(int argc,char **argv){
 char error[256]={0},root[1000],source[1100],releases[1100],store[1100],file[1400],base[128],url[1400];
 CHECK(argc==3);atexit(stop);
 snprintf(root,sizeof(root),"%s/rage-hd-net-%llu",tempRoot(),(unsigned long long)SDL_GetPerformanceCounter());
 snprintf(source,sizeof(source),"%s/source",root);snprintf(releases,sizeof(releases),"%s/releases",root);snprintf(store,sizeof(store),"%s/store",root);
 CHECK(SDL_CreateDirectory(source)&&SDL_CreateDirectory(releases));snprintf(file,sizeof(file),"%s/textures",source);CHECK(SDL_CreateDirectory(file));
 const char *metadata="{\"format\":1,\"name\":\"Download test\",\"version\":\"1.0\",\"region\":\"PAL\"}";
 snprintf(file,sizeof(file),"%s/pack.json",source);CHECK(SDL_SaveFile(file,metadata,strlen(metadata)));
 const char *manifest="[textures]\n\"ui.hud.page.9.clut.30924\"=\"textures/test.png\"\n";
 snprintf(file,sizeof(file),"%s/mod.toml",source);CHECK(SDL_SaveFile(file,manifest,strlen(manifest)));
 SDL_Surface *surface=SDL_CreateSurface(8,8,SDL_PIXELFORMAT_RGBA32);CHECK(surface);SDL_FillSurfaceRect(surface,NULL,0xff773311);snprintf(file,sizeof(file),"%s/textures/test.png",source);CHECK(SDL_SavePNG(surface,file));SDL_DestroySurface(surface);
 const char *args[]={argv[1],releases,"0",NULL};server=SDL_CreateProcess(args,true);CHECK(server);
 SDL_IOStream *output=SDL_GetProcessOutput(server);char line[100]={0};unsigned at=0;
 while(at+1<sizeof(line)){char ch;if(SDL_ReadIO(output,&ch,1)!=1){SDL_Delay(1);continue;}if(ch=='\n')break;line[at++]=ch;}
 unsigned port=0;CHECK(sscanf(line,"HD_SERVER_PORT=%u",&port)==1&&port);
 snprintf(base,sizeof(base),"http://127.0.0.1:%u",port);
 const char *publish[]={argv[2],source,releases,base,NULL};SDL_Process *process=SDL_CreateProcess(publish,false);CHECK(process);int status=-1;CHECK(SDL_WaitProcess(process,true,&status)&&!status);SDL_DestroyProcess(process);
 snprintf(url,sizeof(url),"%s/healthz",base);Response r={0};CHECK(get(url,NULL,0,&r)==200);
 snprintf(url,sizeof(url),"%s/hd-PAL-v1.0.rrhd",base);r.used=0;CHECK(get(url,"0-7",0,&r)==206&&r.used==8&&!memcmp(r.body,"RRHD001\n",8));
 r.used=0;CHECK(get(url,NULL,1,&r)==200&&!r.used);CHECK(get(url,"99999999-",0,&r)==416);
 r.used=0;CHECK(get(url,"-8",0,&r)==206&&r.used==8);
 CHECK(!HdDownloadInstall(base,store,"PAL",cancel,NULL,error,sizeof(error)));CHECK(cancelledAfterBytes>0);HdPackState before,after;CHECK(!HdPackReadState(store,"PAL",&before));
 CHECK(HdDownloadInstall(base,store,"PAL",NULL,NULL,error,sizeof(error))==1);CHECK(HdPackReadState(store,"PAL",&before)&&before.enabled);
 CHECK(HdPackSetEnabled(store,&before,0));CHECK(HdDownloadInstall(base,store,"PAL",NULL,NULL,error,sizeof(error))==2);CHECK(HdPackReadState(store,"PAL",&after)&&!after.enabled);
 snprintf(file,sizeof(file),"%s/catalog-PAL.json",releases);size_t length;char *catalog=SDL_LoadFile(file,&length);CHECK(catalog);
 HdRelease release;CHECK(HdDownloadParseCatalog(catalog,length,base,"PAL",&release));CHECK(!HdDownloadParseCatalog(catalog,length,"http://example.com","PAL",&release));CHECK(!HdDownloadParseCatalog(catalog,length,base,"NTSC-U",&release));
 char *sha=strstr(catalog,"\"sha256\"");CHECK(sha);sha=strchr(sha,':');CHECK(sha);sha=strchr(sha,'"')+1;sha[0]=sha[0]=='a'?'b':'a';CHECK(SDL_SaveFile(file,catalog,length));
 CHECK(!HdDownloadInstall(base,store,"PAL",NULL,NULL,error,sizeof(error)));CHECK(HdPackReadState(store,"PAL",&after)&&!strcmp(before.hash,after.hash)&&!after.enabled);
 SDL_free(catalog);
 process=SDL_CreateProcess(publish,false);CHECK(process);CHECK(SDL_WaitProcess(process,true,&status)&&status!=0);SDL_DestroyProcess(process);
 stop();puts("HTTP catalog, HEAD/Range, download/install, cancellation, checksum, immutable publish and old-pack preservation passed");return 0;
}
