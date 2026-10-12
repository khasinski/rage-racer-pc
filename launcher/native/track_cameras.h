#ifndef TRACK_CAMERAS_H
#define TRACK_CAMERAS_H
#include "game/track_camera_internal.h"
_Static_assert(sizeof(TrackCameraTable)==12&&sizeof(GameTrackCameraNode)==36,"Camera codec differs from runtime");
static int CameraList(const TrackBlock *block,unsigned table,uint32_t *offset,uint32_t *count){
 if(table>2||block->size<12)return 0;uint32_t at=SpecU32(block->data+table*4);if(at<12||at%4||at>block->size)return 0;uint32_t n=0;
 while(block->size-at>=36){const unsigned char *p=block->data+at;if(AiS16(p+34)==-1){*offset=SpecU32(block->data+table*4);*count=n;return 1;}if(AiS16(p+32)<0||AiS16(p+32)>5)return 0;at+=36;n++;}return 0;
}
static void CamerasPrint(const TrackBlock *block){
 uint32_t offsets[3],counts[3];int valid[3];for(unsigned t=0;t<3;t++)valid[t]=CameraList(block,t,&offsets[t],&counts[t]);if(!valid[0]&&!valid[1]&&!valid[2])return;
 printf(",\"cameras\":[");for(unsigned t=0;t<3;t++){if(t)putchar(',');if(!valid[t]){printf("null");continue;}putchar('[');for(uint32_t n=0;n<counts[t];n++){uint32_t id=offsets[t]+n*36;const unsigned char *p=block->data+id;printf("%s{\"id\":%u,\"mode\":%d,\"section\":%d,\"duration\":%d,\"data\":[",n?",":"",id,AiS16(p+32),AiS16(p+34),(int32_t)SpecU32(p+28));for(unsigned i=0;i<4;i++)printf("%s%d",i?",":"",(int32_t)SpecU32(p+i*4));printf("],\"offset\":[");for(unsigned i=0;i<3;i++)printf("%s%d",i?",":"",(int32_t)SpecU32(p+16+i*4));printf("]}");}putchar(']');}putchar(']');
}
static int CameraEdit(TrackBlock *block,uint32_t id,const char *field,int64_t value){
 int found=0;for(unsigned t=0;t<3;t++){uint32_t offset,count;if(CameraList(block,t,&offset,&count)&&id>=offset&&id-offset<count*36&&(id-offset)%36==0)found=1;}if(!found||value<INT32_MIN||value>INT32_MAX)return 0;
 unsigned char *p=(unsigned char *)block->data+id;
 if(!strcmp(field,"mode")||!strcmp(field,"section")){if(value<0||value>(!strcmp(field,"mode")?5:INT16_MAX))return 0;unsigned offset=!strcmp(field,"mode")?32:34;p[offset]=(unsigned char)value;p[offset+1]=(unsigned char)((uint16_t)value>>8);return 1;}
 unsigned offset;if(!strcmp(field,"duration"))offset=28;else if(!strncmp(field,"data_",5)&&strlen(field)==6&&field[5]>='0'&&field[5]<='3')offset=(field[5]-'0')*4;else if(!strncmp(field,"offset_",7)&&strlen(field)==8&&field[7]>='0'&&field[7]<='2')offset=16+(field[7]-'0')*4;else return 0;TrackStore32(p+offset,(uint32_t)value);return 1;
}
#endif
