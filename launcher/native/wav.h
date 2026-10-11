#ifndef RAGE_EDITOR_WAV_H
#define RAGE_EDITOR_WAV_H
#include "content/wav.h"
static uint32_t WavUint(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int WavCommand(int argc,char **argv){
 if(argc==3&&!strcmp(argv[1],"--wav-info")){
  uint32_t frames;if(!WavInfo(argv[2],&frames)){fputs("Expected 44.1 kHz stereo PCM WAV\n",stderr);return 1;}
  printf("{\"frames\":%u,\"sampleRate\":44100,\"channels\":2,\"duration\":%.9f}\n",frames,frames/44100.0);return 0;
 }
 if(argc!=4||strcmp(argv[1],"--wav-normalize"))return 1;
 SDL_PathInfo info;SDL_AudioSpec input,output={SDL_AUDIO_S16LE,2,44100};Uint8 *source=NULL,*samples=NULL;Uint32 sourceBytes=0;int bytes=0,ok=0;
 if(!SDL_GetPathInfo(argv[2],&info)||info.type!=SDL_PATHTYPE_FILE||info.size>WAV_MAX_BYTES)goto done;
 // Read the RIFF declaration before SDL allocates decoded samples.
 FILE *f=fopen(argv[2],"rb");unsigned char h[12];if(!f)goto done;
 int valid=fread(h,1,12,f)==12&&!memcmp(h,"RIFF",4)&&!memcmp(h+8,"WAVE",4)&&
  ((uint32_t)h[4]|(uint32_t)h[5]<<8|(uint32_t)h[6]<<16|(uint32_t)h[7]<<24)==info.size-8;
 // SDL tolerates truncated data chunks. Authoring imports must not.
 Uint64 offset=12,dataBytes=0;unsigned chunks=0,rate=0,channels=0,bits=0;int haveFmt=0,haveData=0;
 while(valid&&offset<info.size){
  unsigned char chunk[8];if(++chunks>10000||info.size-offset<8||fread(chunk,1,8,f)!=8){valid=0;break;}
  Uint64 n=WavUint(chunk+4),padded=n+(n&1);size_t consumed=0;
  if(padded>info.size-offset-8){valid=0;break;}
  if(!memcmp(chunk,"fmt ",4)){
   unsigned char fmt[40]={0};consumed=n<40?(size_t)n:40;
   if(haveFmt||n<16||fread(fmt,1,consumed,f)!=consumed){valid=0;break;}
   unsigned code=fmt[0]|(unsigned)fmt[1]<<8;channels=fmt[2]|(unsigned)fmt[3]<<8;rate=WavUint(fmt+4);bits=fmt[14]|(unsigned)fmt[15]<<8;
   if(code==0xfffe&&n>=40)code=fmt[24]|(unsigned)fmt[25]<<8;
   if(channels<1||channels>8||rate<8000||rate>192000||
      !((code==1&&(bits==8||bits==16||bits==24||bits==32))||(code==3&&(bits==32||bits==64)))||
      (fmt[12]|(unsigned)fmt[13]<<8)!=channels*bits/8||WavUint(fmt+8)!=rate*channels*bits/8){valid=0;break;}
   haveFmt=1;
  }else if(!memcmp(chunk,"data",4)){
   if(haveData||!n){valid=0;break;}dataBytes=n;haveData=1;
  }
  if(fseek(f,(long)(padded-consumed),SEEK_CUR)){valid=0;break;}offset+=8+padded;
 }
 fclose(f);
 if(!valid||offset!=info.size||!haveFmt||!haveData||dataBytes%(channels*bits/8)||
    dataBytes/(double)(channels*bits/8)*44100/rate*4>WAV_MAX_BYTES-44)goto done;
 if(!SDL_LoadWAV(argv[2],&input,&source,&sourceBytes)||!sourceBytes||sourceBytes>WAV_MAX_BYTES||input.channels<1||input.channels>8||input.freq<8000||input.freq>192000)goto done;
 double expected=sourceBytes/(double)(SDL_AUDIO_BYTESIZE(input.format)*input.channels)*44100.0/input.freq*4;
 if(expected>WAV_MAX_BYTES-44)goto done;
 if(!SDL_ConvertAudioSamples(&input,source,(int)sourceBytes,&output,&samples,&bytes)||bytes<=0)goto done;
 ok=WavWrite(argv[3],samples,(size_t)bytes);
 if(ok)printf("{\"frames\":%u,\"sampleRate\":44100,\"channels\":2,\"duration\":%.9f}\n",(unsigned)bytes/4,bytes/176400.0);
 done:SDL_free(source);SDL_free(samples);if(!ok)fputs("Cannot import WAV: use a valid PCM/float WAV under 128 MiB, 8–192 kHz and 1–8 channels; output must be new\n",stderr);return ok?0:1;
}
#endif
