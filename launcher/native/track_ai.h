#ifndef TRACK_AI_H
#define TRACK_AI_H
#include "game/track.h"
#include <stddef.h>
/* TrackEventData: walk start + 2*8 crest records precede 2*30 hints.
 * Lateral fields have historic 'height' names but drive sideways AI nudges. */
enum { AI_HINT_OFFSET=132, AI_HINT_COUNT=30, AI_HINT_SIZE=12 };
_Static_assert(offsetof(TrackEventData,racingLineHints)==AI_HINT_OFFSET&&
               TRACK_RACING_LINE_HINT_COUNT==AI_HINT_COUNT&&
               sizeof(TrackRacingLineHint)==AI_HINT_SIZE,"AI hint codec layout differs from runtime");
static int AiS16(const unsigned char *p){unsigned n=p[0]|(unsigned)p[1]<<8;return n>=32768?(int)n-65536:(int)n;}
static int AiHints(const TrackBlock *block,unsigned series,unsigned *count){
 if(series>1||block->size<AI_HINT_OFFSET+2*AI_HINT_COUNT*AI_HINT_SIZE)return 0;int previous=-1;
 for(unsigned i=0;i<AI_HINT_COUNT;i++){const unsigned char *p=block->data+AI_HINT_OFFSET+(series*AI_HINT_COUNT+i)*AI_HINT_SIZE;int start=AiS16(p),end=AiS16(p+2);if(start==-1){*count=i;return 1;}if(start<0||end<start||start<previous||AiS16(p+4)>AiS16(p+6))return 0;previous=end;}
 *count=AI_HINT_COUNT;return 1;
}
static void AiPrint(const TrackBlock *block){
 unsigned counts[2]={0};int valid[2]={AiHints(block,0,&counts[0]),AiHints(block,1,&counts[1])};if(!valid[0]&&!valid[1])return;
 printf(",\"ai\":{\"hints\":[");for(unsigned s=0;s<2;s++){if(!valid[s]){printf("%snull",s?",":"");continue;}printf("%s[",s?",":"");for(unsigned i=0;i<counts[s];i++){const unsigned char *p=block->data+AI_HINT_OFFSET+(s*AI_HINT_COUNT+i)*AI_HINT_SIZE;printf("%s{\"start\":%d,\"end\":%d,\"min_offset\":%d,\"max_offset\":%d,\"adjustment\":%d}",i?",":"",AiS16(p),AiS16(p+2),AiS16(p+4),AiS16(p+6),AiS16(p+8));}putchar(']');}printf("]}");
}
static int AiEdit(TrackBlock *block,uint32_t index,const char *field,int64_t value,const int valid[2],const unsigned counts[2]){
 unsigned series=index/AI_HINT_COUNT;index%=AI_HINT_COUNT;if(series>1||!valid[series]||index>=counts[series]||value<INT16_MIN||value>INT16_MAX)return 0;
 unsigned offset;if(!strcmp(field,"start"))offset=0;else if(!strcmp(field,"end"))offset=2;else if(!strcmp(field,"min_offset"))offset=4;else if(!strcmp(field,"max_offset"))offset=6;else if(!strcmp(field,"adjustment"))offset=8;else return 0;
 if(offset<4&&value<0)return 0;unsigned char *p=(unsigned char *)block->data+AI_HINT_OFFSET+(series*AI_HINT_COUNT+index)*AI_HINT_SIZE+offset;p[0]=(unsigned char)value;p[1]=(unsigned char)((uint16_t)value>>8);return 1;
}
#endif
