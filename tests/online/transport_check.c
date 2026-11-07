#include "online/transport.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
    CHECK(argc==3);CHECK(SDL_Init(SDL_INIT_EVENTS));
    char accept[29];OnlineWebSocketAccept("dGhlIHNhbXBsZSBub25jZQ==",accept);
    CHECK(!strcmp(accept,"s3pPLMBiTxaQ9kYGzzhZRbK+xOo="));
    CHECK(!OnlineTransportStart("ftp://localhost","guest","{}"));
    CHECK(!OnlineTransportStart("http://localhost\r\nX: bad","guest","{}"));
    char json[80];snprintf(json,sizeof(json),"{\"name\":\"%s\"}",argv[2]);
    OnlineTransport *t=OnlineTransportStart(argv[1],"guest",json);CHECK(t);
    uint8_t packet[65537];int auth=0,text=0,frame=0,replaced=0,error=0;
    uint64_t deadline=SDL_GetTicks()+8000;
    while(SDL_GetTicks()<deadline){
        int type;size_t size;
        while(OnlineTransportPoll(t,&type,packet,sizeof(packet),&size)){
            if(type==ONLINE_AUTH)auth++;
            if(type==ONLINE_TEXT){CHECK(!strcmp((char*)packet,"{\"t\":\"welcome\"}"));text++;
                CHECK(OnlineTransportSend(t,0,"{\"t\":\"probe\"}",13));}
            if(type==ONLINE_FRAME){CHECK(size==4096&&packet[0]==2&&packet[1]==77);for(size_t i=2;i<size;i++)CHECK(packet[i]==(uint8_t)i);frame++;}
            if(type==ONLINE_REPLACED)replaced++;
            if(type==ONLINE_ERROR)error++;
        }
        if(OnlineTransportDone(t))break;
        SDL_Delay(2);
    }
    OnlineTransportFree(t);SDL_Quit();
    CHECK(auth==1);
    if(!strcmp(argv[2],"good"))CHECK(text==1&&frame==1&&replaced==1&&error==0);
    else CHECK(error==1&&frame==0&&replaced==0);
    puts("native transport passed");return 0;
}
