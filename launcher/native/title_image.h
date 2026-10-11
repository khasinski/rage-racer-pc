#include "content/title_image.h"
static int ExportTitleImage(int argc,char **argv){
 if(argc!=5||strlen(argv[2])!=1||argv[2][0]<'0'||argv[2][0]>'5'||strlen(argv[3])!=1||(argv[3][0]!='0'&&argv[3][0]!='1'))return 1;
 uint8_t pixels[112*16*4];
 for(int y=0;y<16;y++)for(int x=0;x<112;x++){
  uint16_t color=TitleImageColor(argv[3][0]=='1',TitleImageIndex(argv[2][0]-'0',x,y));
  uint8_t *p=pixels+(y*112+x)*4;
  p[0]=(color&31)<<3;p[1]=((color>>5)&31)<<3;p[2]=((color>>10)&31)<<3;p[3]=255;
 }
 FILE *reserved=fopen(argv[4],"wbx");if(!reserved)return 1;if(fclose(reserved)){remove(argv[4]);return 1;}
 if(!WritePng(argv[4],pixels,112,16)){remove(argv[4]);return 1;}
 puts("{\"width\":112,\"height\":16}");return 0;
}
