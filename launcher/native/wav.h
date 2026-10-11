#ifndef RAGE_EDITOR_WAV_H
#define RAGE_EDITOR_WAV_H
#include "content/wav.h"
static uint32_t WavUint(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int WavNumber(const char *text,uint32_t max,uint32_t *out){
 uint32_t n=0;if(!text||!*text)return 0;
 for(const char *p=text;*p;p++){if(*p<'0'||*p>'9'||n>max/10||(n==max/10&&(unsigned)(*p-'0')>max%10))return 0;n=n*10+(unsigned)(*p-'0');}
 *out=n;return 1;
}
static int WavEdit(int argc,char **argv){
 uint32_t frames,start,end,gain;SDL_AudioSpec spec;Uint8 *samples=NULL;Uint32 bytes=0;int ok=0;unsigned clipped=0;
 if(argc!=7||!WavInfo(argv[2],&frames)||!WavNumber(argv[4],frames,&start)||!WavNumber(argv[5],frames,&end)||!WavNumber(argv[6],200,&gain)||start>=end)return 1;
 if(!SDL_LoadWAV(argv[2],&spec,&samples,&bytes)||bytes!=frames*4)goto done;
 for(uint32_t i=start*4;i<end*4;i+=2){
  int n=samples[i]|(unsigned)samples[i+1]<<8;if(n>=32768)n-=65536;n=n*(int)gain/100;
  if(n>32767){n=32767;clipped++;}if(n<-32768){n=-32768;clipped++;}
  samples[i]=(Uint8)n;samples[i+1]=(Uint8)((unsigned)n>>8);
 }
 ok=WavWrite(argv[3],samples+start*4,(end-start)*4);
 if(ok)printf("{\"frames\":%u,\"duration\":%.9f,\"clippedSamples\":%u}\n",end-start,(end-start)/44100.0,clipped);
 done:SDL_free(samples);if(!ok)fputs("Cannot edit WAV; use a valid frame range, gain 0–200%, and a new output\n",stderr);return ok?0:1;
}
static int WavWaveform(int argc,char **argv){
 uint32_t frames;if(argc!=3||!WavInfo(argv[2],&frames))return 1;
 SDL_AudioSpec spec;Uint8 *samples=NULL;Uint32 bytes=0;if(!SDL_LoadWAV(argv[2],&spec,&samples,&bytes)||bytes!=frames*4){SDL_free(samples);return 1;}
 printf("[");unsigned bins=frames<256?frames:256;
 for(unsigned bin=0;bin<bins;bin++){
  int low=32767,high=-32768;uint32_t first=(uint32_t)((uint64_t)frames*bin/bins),end=(uint32_t)((uint64_t)frames*(bin+1)/bins);
  for(uint32_t i=first*4;i<end*4;i+=2){int n=samples[i]|(unsigned)samples[i+1]<<8;if(n>=32768)n-=65536;if(n<low)low=n;if(n>high)high=n;}
  printf("%s[%d,%d]",bin?",":"",low,high);
 }
 puts("]");SDL_free(samples);return 0;
}
static int WavCommand(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"--wav-edit"))return WavEdit(argc,argv);
 if(argc>1&&!strcmp(argv[1],"--wav-waveform"))return WavWaveform(argc,argv);
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
