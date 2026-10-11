#include "title_image.h"
static const uint32_t retail[]={
#include "main/PAL/main/menu/assets/title_menu_textures.inc"
};
static const uint32_t custom[]={
#include "main/PAL/main/menu/assets/custom_menu_texture.inc"
};
static const uint16_t palettes[2][16]={
 {0x8000,0xCE73,0xC631,0xC210,0xBDEF,0xB9CE,0xB5AD,0xB18C,0xA529,0xA108,0x98C6,0x94A5,0x9084,0x8C63,0x8842,0x8421},
 {0x8000,0xB9DF,0xB19D,0xA97C,0xA55A,0xA139,0x9D18,0x98F6,0x9092,0x8C70,0x884D,0x842B,0x8429,0x8006,0x8001,0x8000}
};
_Static_assert(sizeof(retail)==5*112*16/2,"five retail label rows");
_Static_assert(sizeof(custom)==112*16/2,"custom label row");
uint8_t TitleImageIndex(int item,int x,int y){
 if(item<0||item>=6||x<0||x>=112||y<0||y>=16)return 4;
 const uint32_t *texture=item==3?custom:retail+(item>3?item-1:item)*112*16/8;
 return (uint8_t)((texture[(y*112+x)/8]>>((x&7)*4))&15);
}
uint16_t TitleImageColor(int selected,int index){return palettes[selected!=0][index&15];}
