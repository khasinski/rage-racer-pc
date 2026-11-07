#include "render/retail_tachometer.h"
#include <stdio.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    uint8_t atlas[4]={0};RageRetailTachometer frame={.atlas=atlas};RageTachometerQuad q[7];
    CHECK(!RenderRetailTachometerBuild(&frame,0,q));
    int32_t *t=frame.words;t[RAGE_TACHO_VISIBLE]=1;
    t[RAGE_TACHO_FACE_SIZE]=96;t[RAGE_TACHO_FACE_SIZE+1]=94;
    t[RAGE_TACHO_FACE]=220;t[RAGE_TACHO_FACE+1]=140;t[RAGE_TACHO_FACE+2]=128;
    t[RAGE_TACHO_SPEED]=250;t[RAGE_TACHO_SPEED+1]=220;t[RAGE_TACHO_SPEED+2]=237;
    t[RAGE_TACHO_GEAR+2]=4;t[RAGE_TACHO_SHIFT_LIGHT+2]=255;
    for(int i=0;i<8;i++)t[RAGE_TACHO_NEEDLE+i]=100+i;
    CHECK(RenderRetailTachometerBuild(&frame,53,q)==7);
    CHECK(!q[0].textured&&q[0].color[0]==1);
    CHECK(q[1].textured&&q[1].xy[0][0]==273&&q[1].xy[3][0]==369);
    CHECK(q[1].w==96&&q[1].h==94&&q[1].color[0]==1);
    CHECK(q[2].u==16&&q[3].u==24&&q[4].u==56&&q[5].u==32);
    CHECK(q[2].v==96&&q[2].xy[0][0]==303&&q[4].xy[0][0]==319);
    CHECK(!q[6].textured&&q[6].xy[3][0]==159&&q[6].xy[3][1]==107);
    t[RAGE_TACHO_FACE+2]=32;t[RAGE_TACHO_FACE+3]=1;t[RAGE_TACHO_DIGIT_CLUT]=1;
    CHECK(RenderRetailTachometerBuild(&frame,0,q)==7);
    CHECK(q[1].u==96&&q[1].color[0]==0.25f&&q[2].v==104);
    t[RAGE_TACHO_SPEED+2]=1000;CHECK(!RenderRetailTachometerBuild(&frame,0,q));
    frame.atlas=NULL;CHECK(!RenderRetailTachometerBuild(&frame,0,q));
    return 0;
}
