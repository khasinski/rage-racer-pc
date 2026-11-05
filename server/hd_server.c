/* Small read-only origin for immutable HD releases. TLS is terminated by the
 * reverse proxy. No game data, admin credentials or download telemetry.
 * Builds on POSIX and on Windows so the HD download integration test can run
 * against the same origin on every desktop platform. */
#include <SDL3/SDL.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
typedef int IoResult;
#define closeSocket closesocket
#define validSocket(fd) ((fd)!=INVALID_SOCKET)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
typedef int Socket;
typedef ssize_t IoResult;
#define closeSocket close
#define validSocket(fd) ((fd)>=0)
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char root[1600];
static SDL_AtomicInt clients;
static int sendAll(Socket fd,const void *bytes,size_t n){
    const char *p=bytes;while(n){int chunk=n>65536?65536:(int)n;IoResult k=send(fd,p,chunk,0);if(k<=0)return 0;p+=k;n-=(size_t)k;}return 1;
}
static void reply(Socket fd,int code,const char *reason,const char *body,int head){
    char header[512];int n=snprintf(header,sizeof(header),"HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\n\r\n",code,reason,strlen(body));
    sendAll(fd,header,(size_t)n);if(!head)sendAll(fd,body,strlen(body));
}
static const char *headerValue(char *request,const char *key){
    size_t n=strlen(key);char *line=strstr(request,"\r\n");
    while(line&&line[2]){line+=2;if(!SDL_strncasecmp(line,key,n)&&line[n]==':'){line+=n+1;while(*line==' ')line++;return line;}line=strstr(line,"\r\n");}return NULL;
}
static int releaseName(const char *name){
    size_t n=strlen(name);if(n<15||n>120||strncmp(name,"hd-",3)||strcmp(name+n-5,".rrhd"))return 0;
    for(size_t i=0;i<n;i++)if(!strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._",name[i]))return 0;
    return !strstr(name,"..");
}
static int range(const char *value,uint64_t size,uint64_t *start,uint64_t *end){
    if(strncmp(value,"bytes=",6))return 0;value+=6;char *tail;errno=0;
    if(*value=='-'){
        if(value[1]<'0'||value[1]>'9')return 0;
        unsigned long long suffix=strtoull(value+1,&tail,10);if(errno||tail==value+1||!suffix||strncmp(tail,"\r\n",2))return 0;
        *start=suffix>=size?0:size-suffix;return 1;
    }
    if(*value<'0'||*value>'9')return 0;
    unsigned long long first=strtoull(value,&tail,10);if(errno||tail==value||*tail!='-'||first>=size)return 0;
    *start=first;value=tail+1;
    if(!strncmp(value,"\r\n",2))return 1;
    if(*value<'0'||*value>'9')return 0;
    errno=0;unsigned long long last=strtoull(value,&tail,10);
    if(errno||tail==value||strncmp(tail,"\r\n",2)||last<first)return 0;
    *end=last>=size?size-1:last;return 1;
}
static int SDLCALL serve(void *context){
    Socket fd=*(Socket*)context;SDL_free(context);char req[8193]={0};size_t used=0;int head=0;
    while(used<8192&&!strstr(req,"\r\n\r\n")){IoResult n=recv(fd,req+used,(int)(8192-used),0);if(n<=0)goto done;used+=(size_t)n;req[used]=0;}
    if(!strstr(req,"\r\n\r\n")){reply(fd,431,"Request Header Fields Too Large","{}",0);goto done;}
    char method[8],url[1024],protocol[16],file[1800],name[128];
    if(sscanf(req,"%7s %1023s %15s",method,url,protocol)!=3)goto done;
    head=!strcmp(method,"HEAD");if(!head&&strcmp(method,"GET")){reply(fd,405,"Method Not Allowed","{}",0);goto done;}
    int catalog=0;
    if(!strcmp(url,"/healthz")){reply(fd,200,"OK","{\"ok\":true}",head);goto done;}
    if(!strcmp(url,"/api/mods/hd/PAL/latest")){strcpy(name,"catalog-PAL.json");catalog=1;}
    else if(!strcmp(url,"/api/mods/hd/NTSC-U/latest")){strcpy(name,"catalog-NTSC-U.json");catalog=1;}
    else if(!strcmp(url,"/api/mods/hd/NTSC-J/latest")){strcpy(name,"catalog-NTSC-J.json");catalog=1;}
    else if(url[0]=='/'&&releaseName(url+1))SDL_strlcpy(name,url+1,sizeof(name));
    else {reply(fd,404,"Not Found","{}",head);goto done;}
    if(snprintf(file,sizeof(file),"%s/%s",root,name)>=(int)sizeof(file))goto done;
    SDL_PathInfo info;if(!SDL_GetPathInfo(file,&info)||info.type!=SDL_PATHTYPE_FILE||!info.size){reply(fd,404,"Not Found","{}",head);goto done;}
    SDL_IOStream *io=SDL_IOFromFile(file,"rb");if(!io){reply(fd,404,"Not Found","{}",head);goto done;}
    uint64_t start=0,end=info.size-1;int code=200;
    char etag[100];snprintf(etag,sizeof(etag),"\"%llx-%llx\"",(unsigned long long)info.size,(unsigned long long)info.modify_time);
    const char *none=headerValue(req,"If-None-Match"),*requested=headerValue(req,"Range"),*ifRange=headerValue(req,"If-Range");
    if(none&&!strncmp(none,etag,strlen(etag))&&!strncmp(none+strlen(etag),"\r\n",2)){
        char h[256];int n=snprintf(h,sizeof(h),"HTTP/1.1 304 Not Modified\r\nETag: %s\r\nConnection: close\r\n\r\n",etag);sendAll(fd,h,(size_t)n);SDL_CloseIO(io);goto done;
    }
    if(ifRange&&(strncmp(ifRange,etag,strlen(etag))||strncmp(ifRange+strlen(etag),"\r\n",2)))requested=NULL;
    if(requested&&!catalog){
        if(!range(requested,info.size,&start,&end)){
            char h[256];int n=snprintf(h,sizeof(h),"HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */%llu\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",(unsigned long long)info.size);sendAll(fd,h,(size_t)n);SDL_CloseIO(io);goto done;
        }code=206;
    }
    char h[1200],contentRange[160]={0};if(code==206)snprintf(contentRange,sizeof(contentRange),"Content-Range: bytes %llu-%llu/%llu\r\n",(unsigned long long)start,(unsigned long long)end,(unsigned long long)info.size);
    int n=snprintf(h,sizeof(h),"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %llu\r\nETag: %s\r\nCache-Control: %s\r\nAccept-Ranges: bytes\r\n%sConnection: close\r\nX-Content-Type-Options: nosniff\r\n\r\n",code,code==206?"Partial Content":"OK",catalog?"application/json":"application/octet-stream",(unsigned long long)(end-start+1),etag,catalog?"no-cache":"public, max-age=31536000, immutable",contentRange);
    if(sendAll(fd,h,(size_t)n)&&!head&&SDL_SeekIO(io,(Sint64)start,SDL_IO_SEEK_SET)>=0){
        char buffer[65536];uint64_t left=end-start+1;
        while(left){size_t got=SDL_ReadIO(io,buffer,left>sizeof(buffer)?sizeof(buffer):(size_t)left);if(!got||!sendAll(fd,buffer,got))break;left-=got;}
    }
    SDL_CloseIO(io);
done:closeSocket(fd);SDL_AddAtomicInt(&clients,-1);return 0;
}
int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"Usage: rage-hd-server RELEASE_DIRECTORY PORT (0 = auto)\n");return 2;}
    char *tail;long port=strtol(argv[2],&tail,10);if(*tail||port<0||port>65535)return 2;
    SDL_strlcpy(root,argv[1],sizeof(root));
#ifdef _WIN32
    WSADATA wsa;if(WSAStartup(MAKEWORD(2,2),&wsa))return 1;
#else
    signal(SIGPIPE,SIG_IGN);
#endif
    Socket fd=socket(AF_INET,SOCK_STREAM,0);int yes=1;if(!validSocket(fd))return 1;setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&yes,sizeof(yes));
    struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_port=htons((uint16_t)port);
    const char *host=SDL_getenv("HD_BIND");if(!host)host="127.0.0.1";
    if(inet_pton(AF_INET,host,&addr.sin_addr)!=1||bind(fd,(struct sockaddr*)&addr,sizeof(addr))||listen(fd,32))return 1;
    socklen_t length=sizeof(addr);getsockname(fd,(struct sockaddr*)&addr,&length);
    printf("HD_SERVER_PORT=%u\n",(unsigned)ntohs(addr.sin_port));fflush(stdout);
    for(;;){Socket client=accept(fd,NULL,NULL);if(!validSocket(client))continue;
#ifdef _WIN32
        DWORD timeout=30000;
#else
        struct timeval timeout={30,0};
#endif
        setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
        if(SDL_GetAtomicInt(&clients)>=32){closeSocket(client);continue;}
        Socket *context=SDL_malloc(sizeof(*context));if(!context){closeSocket(client);continue;}*context=client;
        SDL_AddAtomicInt(&clients,1);SDL_Thread *thread=SDL_CreateThread(serve,"HD request",context);
        if(thread)SDL_DetachThread(thread);else{SDL_free(context);closeSocket(client);SDL_AddAtomicInt(&clients,-1);}
    }
}
