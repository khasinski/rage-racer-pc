#include "wav.h"
#include <stdio.h>
#include <string.h>
static uint32_t Read32(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void Put32(unsigned char *p,uint32_t n){for(int i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static void Header(unsigned char h[44],size_t bytes){
 memset(h,0,44);memcpy(h,"RIFF",4);Put32(h+4,(uint32_t)bytes+36);memcpy(h+8,"WAVEfmt ",8);Put32(h+16,16);
 h[20]=1;h[22]=2;Put32(h+24,44100);Put32(h+28,176400);h[32]=4;h[34]=16;memcpy(h+36,"data",4);Put32(h+40,(uint32_t)bytes);
}
int WavInfo(const char *path,uint32_t *frames){
 unsigned char h[44],expected[44];FILE *f=path?fopen(path,"rb"):NULL;int ok=0;
 if(!f)return 0;
 if(fread(h,1,44,f)==44){uint32_t bytes=Read32(h+40);Header(expected,bytes);
  if(bytes&&bytes<=WAV_MAX_BYTES-44&&bytes%4==0&&!memcmp(h,expected,44)&&!fseek(f,0,SEEK_END)&&ftell(f)==(long)bytes+44){if(frames)*frames=bytes/4;ok=1;}}
 fclose(f);return ok;
}
int WavWrite(const char *path,const void *samples,size_t bytes){
 if(!path||!samples||!bytes||bytes%4||bytes>WAV_MAX_BYTES-44)return 0;
 unsigned char h[44];Header(h,bytes);FILE *f=fopen(path,"wbx");if(!f)return 0;
 int ok=fwrite(h,1,44,f)==44&&fwrite(samples,1,bytes,f)==bytes;if(fclose(f))ok=0;if(!ok)remove(path);return ok;
}
