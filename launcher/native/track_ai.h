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
enum { AI_SPEED_OFFSET=1140, AI_SPEED_COUNT=48, AI_SPEED_SIZE=12 };
_Static_assert(offsetof(TrackEventData,aiSpeedKeys)==AI_SPEED_OFFSET&&TRACK_AI_SPEED_KEY_COUNT==AI_SPEED_COUNT&&sizeof(TrackAiSpeedKey)==AI_SPEED_SIZE,"AI speed codec differs from runtime");
static int AiSpeeds(const TrackBlock *block,unsigned series,unsigned *count){
 if(series>1||block->size<AI_SPEED_OFFSET+2*AI_SPEED_COUNT*AI_SPEED_SIZE)return 0;
 for(unsigned i=0;i<AI_SPEED_COUNT;i++)if(AiS16(block->data+AI_SPEED_OFFSET+(series*AI_SPEED_COUNT+i)*AI_SPEED_SIZE)==-1){*count=i;return 1;}*count=AI_SPEED_COUNT;return 1;
}
static int AiSpeedEdit(TrackBlock *block,uint32_t index,const char *field,int64_t value){
 unsigned series=index/AI_SPEED_COUNT,count;index%=AI_SPEED_COUNT;if(!AiSpeeds(block,series,&count)||index>=count||value<INT16_MIN||value>INT16_MAX)return 0;
 unsigned offset;if(!strcmp(field,"progress")){if(value==-1)return 0;offset=0;}else if(!strcmp(field,"pitch"))offset=2;else if(!strncmp(field,"target_",7)&&strlen(field)==8&&field[7]>='0'&&field[7]<='3')offset=4+(field[7]-'0')*2;else return 0;
 unsigned char *p=(unsigned char *)block->data+AI_SPEED_OFFSET+(series*AI_SPEED_COUNT+index)*AI_SPEED_SIZE+offset;p[0]=(unsigned char)value;p[1]=(unsigned char)((uint16_t)value>>8);return 1;
}
enum { AI_START_OFFSET=852, AI_START_COUNT=12, AI_START_SIZE=12 };
_Static_assert(offsetof(TrackEventData,rivalStarts)==AI_START_OFFSET&&TRACK_RIVAL_COUNT==AI_START_COUNT&&sizeof(TrackRivalStart)==AI_START_SIZE,"Start codec differs from runtime");
static int AiStartEdit(TrackBlock *block,uint32_t index,const char *field,int64_t value,uint32_t points){
 if(index>=2*AI_START_COUNT||block->size<AI_START_OFFSET+2*AI_START_COUNT*AI_START_SIZE)return 0;unsigned char *p=(unsigned char *)block->data+AI_START_OFFSET+index*AI_START_SIZE;
 if(!strcmp(field,"point")){if(value<0||value>=points||value>INT16_MAX)return 0;p[8]=(unsigned char)value;p[9]=(unsigned char)((uint16_t)value>>8);}
 else if(!strcmp(field,"x")||!strcmp(field,"z")){if(value<INT32_MIN||value>INT32_MAX)return 0;TrackStore32(p+(!strcmp(field,"z")?4:0),(uint32_t)value);}else return 0;return 1;
}
static void AiPrint(const TrackBlock *block){
 unsigned counts[2]={0};int valid[2]={AiHints(block,0,&counts[0]),AiHints(block,1,&counts[1])};unsigned speedCounts[2]={0};int speedValid[2]={AiSpeeds(block,0,&speedCounts[0]),AiSpeeds(block,1,&speedCounts[1])};if(!valid[0]&&!valid[1]&&!speedValid[0]&&!speedValid[1])return;
 printf(",\"ai\":{\"hints\":[");for(unsigned s=0;s<2;s++){if(!valid[s]){printf("%snull",s?",":"");continue;}printf("%s[",s?",":"");for(unsigned i=0;i<counts[s];i++){const unsigned char *p=block->data+AI_HINT_OFFSET+(s*AI_HINT_COUNT+i)*AI_HINT_SIZE;printf("%s{\"start\":%d,\"end\":%d,\"min_offset\":%d,\"max_offset\":%d,\"adjustment\":%d}",i?",":"",AiS16(p),AiS16(p+2),AiS16(p+4),AiS16(p+6),AiS16(p+8));}putchar(']');}printf("],\"speeds\":[");for(unsigned s=0;s<2;s++){if(!speedValid[s]){printf("%snull",s?",":"");continue;}printf("%s[",s?",":"");for(unsigned i=0;i<speedCounts[s];i++){const unsigned char *p=block->data+AI_SPEED_OFFSET+(s*AI_SPEED_COUNT+i)*AI_SPEED_SIZE;printf("%s{\"progress\":%d,\"pitch\":%d,\"targets\":[%d,%d,%d,%d]}",i?",":"",AiS16(p),AiS16(p+2),AiS16(p+4),AiS16(p+6),AiS16(p+8),AiS16(p+10));}putchar(']');}printf("],\"starts\":[");if(block->size>=AI_START_OFFSET+2*AI_START_COUNT*AI_START_SIZE)for(unsigned s=0;s<2;s++){printf("%s[",s?",":"");for(unsigned i=0;i<AI_START_COUNT;i++){const unsigned char *p=block->data+AI_START_OFFSET+(s*AI_START_COUNT+i)*AI_START_SIZE;printf("%s{\"x\":%d,\"z\":%d,\"point\":%d,\"seed\":%d}",i?",":"",(int32_t)SpecU32(p),(int32_t)SpecU32(p+4),AiS16(p+8),AiS16(p+10));}putchar(']');}printf("]}");
}
static int AiEdit(TrackBlock *block,uint32_t index,const char *field,int64_t value,const int valid[2],const unsigned counts[2]){
 unsigned series=index/AI_HINT_COUNT;index%=AI_HINT_COUNT;if(series>1||!valid[series]||index>=counts[series]||value<INT16_MIN||value>INT16_MAX)return 0;
 unsigned offset;if(!strcmp(field,"start"))offset=0;else if(!strcmp(field,"end"))offset=2;else if(!strcmp(field,"min_offset"))offset=4;else if(!strcmp(field,"max_offset"))offset=6;else if(!strcmp(field,"adjustment"))offset=8;else return 0;
 if(offset<4&&value<0)return 0;unsigned char *p=(unsigned char *)block->data+AI_HINT_OFFSET+(series*AI_HINT_COUNT+index)*AI_HINT_SIZE+offset;p[0]=(unsigned char)value;p[1]=(unsigned char)((uint16_t)value>>8);return 1;
}
#endif
