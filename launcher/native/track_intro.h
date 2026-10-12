#ifndef TRACK_INTRO_H
#define TRACK_INTRO_H
#include "game/car_runtime_state.h"
_Static_assert(offsetof(TrackEventData,offsets)==2916&&offsetof(RaceIntroCameraScript,keys)==4&&sizeof(RaceIntroCameraKey)==20,"Starting camera codec differs from runtime");
/* Physical key IDs preserve aliases between forward and reverse scripts. */
static int IntroList(const TrackBlock *block,unsigned series,uint32_t *first,uint32_t *count){
 const uint32_t base=offsetof(TrackEventData,offsets);
 if(series>1||block->size<base+sizeof(TrackEventOffsets))return 0;
 uint32_t start=SpecU32(block->data+base+4),end=SpecU32(block->data+base+8);
 if(start<sizeof(TrackEventOffsets)||start%4||end<start||end>block->size-base||end-start<44)return 0;
 const unsigned char *script=block->data+base+start;
 int index=AiS16(script+series*2);uint32_t keys=(end-start-4)/20;
 if(index<0||(uint32_t)index+1>=keys)return 0;
 *first=base+start+4+(uint32_t)index*20;
 for(uint32_t n=(uint32_t)index;n<keys;n++){
  const unsigned char *p=script+4+n*20;int32_t mode=(int32_t)SpecU32(p+12);int duration=AiS16(p+18);
  if(mode==1){if(duration<0)return 0;*count=n-(uint32_t)index+1;return 1;}
  if(mode!=0||duration<=0||n+1>=keys)return 0;
 }
 return 0;
}
static void IntroPrint(const TrackBlock *block){
 uint32_t first[2],count[2];int valid[2];for(unsigned s=0;s<2;s++)valid[s]=IntroList(block,s,&first[s],&count[s]);
 if(!valid[0]&&!valid[1])return;
 printf(",\"intro\":[");for(unsigned s=0;s<2;s++){if(s)putchar(',');if(!valid[s]){printf("null");continue;}putchar('[');
  for(uint32_t n=0;n<count[s];n++){uint32_t id=first[s]+n*20;const unsigned char *p=block->data+id;
   printf("%s{\"id\":%u,\"x\":%d,\"y\":%d,\"z\":%d,\"mode\":%d,\"frame\":%d,\"duration\":%d}",n?",":"",id,(int32_t)SpecU32(p),(int32_t)SpecU32(p+4),(int32_t)SpecU32(p+8),(int32_t)SpecU32(p+12),AiS16(p+16),AiS16(p+18));
  }putchar(']');
 }putchar(']');
}
static int IntroEdit(TrackBlock *block,uint32_t id,const char *field,int64_t value){
 int found=0;for(unsigned s=0;s<2;s++){uint32_t first,count;if(IntroList(block,s,&first,&count)&&id>=first&&id-first<count*20&&(id-first)%20==0)found=1;}
 if(!found)return 0;unsigned char *p=(unsigned char *)block->data+id;
 if(!strcmp(field,"frame")||!strcmp(field,"duration")){
  if(value<INT16_MIN||value>INT16_MAX)return 0;
  /* Advancing a terminal key could enter unused data. Keep its trigger intact. */
  if(!strcmp(field,"frame")&&(int32_t)SpecU32(p+12)==1)return 0;
  if(!strcmp(field,"duration")&&value<((int32_t)SpecU32(p+12)==1?0:1))return 0;
  unsigned at=!strcmp(field,"frame")?16:18;p[at]=(unsigned char)value;p[at+1]=(unsigned char)((uint16_t)value>>8);return 1;
 }
 unsigned at;if(!strcmp(field,"x"))at=0;else if(!strcmp(field,"y"))at=4;else if(!strcmp(field,"z"))at=8;else return 0;
 if(value<INT32_MIN||value>INT32_MAX)return 0;TrackStore32(p+at,(uint32_t)value);return 1;
}
#endif
