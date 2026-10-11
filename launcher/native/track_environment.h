#ifndef TRACK_ENVIRONMENT_H
#define TRACK_ENVIRONMENT_H
static unsigned EnvU16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static int EnvCount(const TrackBlock *block,uint32_t *count){
 if(block->size<104||SpecU32(block->data)>3||!SpecU32(block->data+4)||SpecU32(block->data+4)>INT32_MAX)return 0;
 int32_t previous=-1;uint32_t n=(uint32_t)((block->size-8)/48);
 for(uint32_t i=0;i<n;i++){
  const unsigned char *p=block->data+8+(size_t)i*48;int32_t time=(int32_t)SpecU32(p);
  if(time==-1){if(i==0)return 0;*count=i;return 1;}
  if(time<0||(i==0&&time!=0)||time<=previous||(uint32_t)time>=SpecU32(block->data+4)||EnvU16(p+44)>=5)return 0;previous=time;
 }
 return 0;
}
static void EnvPrint(const TrackBlock *block){
 uint32_t count;if(!EnvCount(block,&count))return;
 printf(",\"environment\":{\"skyRow\":%u,\"length\":%u,\"cues\":[",SpecU32(block->data),SpecU32(block->data+4));
 for(uint32_t i=0;i<count;i++){
  const unsigned char *p=block->data+8+(size_t)i*48;printf("%s{\"time\":%u,\"duration\":%u,\"mode\":%u,\"colors\":[",i?",":"",SpecU32(p),EnvU16(p+40),EnvU16(p+44));
  for(unsigned c=0;c<9;c++){const unsigned char *rgb=p+4+c*4;printf("%s%u",c?",":"",(unsigned)rgb[0]<<16|(unsigned)rgb[1]<<8|rgb[2]);}printf("]}");
 }
 printf("]}");
}
static int EnvEdit(TrackBlock *block,uint32_t count,uint32_t at,const char *field,int64_t value){
 if(at>=count)return 0;
 unsigned char *p=(unsigned char *)block->data+8+(size_t)at*48;
 if(!strcmp(field,"time")){if(value<0||value>=SpecU32(block->data+4)||(at==0&&value!=0))return 0;
  TrackStore32(p,(uint32_t)value);
 }else if(!strcmp(field,"duration")){if(value<0||value>32767)return 0;p[40]=(unsigned char)value;p[41]=(unsigned char)(value>>8);
 }else if(!strcmp(field,"mode")){if(value<0||value>=5)return 0;p[44]=(unsigned char)value;p[45]=0;
 }else if(!strncmp(field,"color_",6)&&strlen(field)==7&&field[6]>='0'&&field[6]<='8'){
  if(value<0||value>0xffffff)return 0;p+=4+(field[6]-'0')*4;p[0]=(unsigned char)(value>>16);p[1]=(unsigned char)(value>>8);p[2]=(unsigned char)value;
 }else return 0;
 return 1;
}
#endif
