#include "retail_tachometer.h"
#include <string.h>

static void rect(RageTachometerQuad *q,int x,int y,int w,int h){
    q->xy[0][0]=q->xy[2][0]=(float)x;q->xy[1][0]=q->xy[3][0]=(float)(x+w);
    q->xy[0][1]=q->xy[1][1]=(float)y;q->xy[2][1]=q->xy[3][1]=(float)(y+h);
    for(int c=0;c<4;c++)q->color[c]=1;
}
int RenderRetailTachometerBuild(const RageRetailTachometer *frame,int edge,RageTachometerQuad q[7]){
    if(!frame||!q||!frame->atlas||!frame->words[RAGE_TACHO_VISIBLE])return 0;
    const int32_t *t=frame->words;
    int w=t[RAGE_TACHO_FACE_SIZE],h=t[RAGE_TACHO_FACE_SIZE+1];
    int speed=t[RAGE_TACHO_SPEED+2],gear=t[RAGE_TACHO_GEAR+2];
    if(w!=96||h!=94||speed<0||speed>999||gear<0||gear>9||
       t[RAGE_TACHO_DIGIT_CLUT]<0||t[RAGE_TACHO_DIGIT_CLUT]>1)return 0;
    memset(q,0,7*sizeof(*q));
    rect(&q[0],t[RAGE_TACHO_SHIFT_LIGHT]+edge,t[RAGE_TACHO_SHIFT_LIGHT+1],16,16);
    q[0].color[0]=(float)t[RAGE_TACHO_SHIFT_LIGHT+2]/255.0f;
    q[0].color[1]=q[0].color[2]=32.0f/255.0f;
    rect(&q[1],t[RAGE_TACHO_FACE]+edge,t[RAGE_TACHO_FACE+1],w,h);
    q[1].textured=1;q[1].u=t[RAGE_TACHO_FACE+3]?w:0;q[1].w=w;q[1].h=h;
    for(int c=0;c<3;c++)q[1].color[c]=(float)t[RAGE_TACHO_FACE+2]/128.0f;
    const int digits[]={speed/100,speed/10%10,speed%10,gear};
    for(int i=0;i<4;i++){
        RageTachometerQuad *digit=&q[2+i];
        rect(digit,(i<3?t[RAGE_TACHO_SPEED]+i*8:t[RAGE_TACHO_GEAR])+edge,
             i<3?t[RAGE_TACHO_SPEED+1]:t[RAGE_TACHO_GEAR+1],8,8);
        digit->textured=1;digit->u=digits[i]*8;digit->v=96+t[RAGE_TACHO_DIGIT_CLUT]*8;
        digit->w=digit->h=8;
    }
    for(int i=0;i<4;i++){
        q[6].xy[i][0]=(float)(t[RAGE_TACHO_NEEDLE+i*2]+edge);
        q[6].xy[i][1]=(float)t[RAGE_TACHO_NEEDLE+i*2+1];
    }
    for(int c=0;c<3;c++)q[6].color[c]=(float)t[RAGE_TACHO_NEEDLE_COLOR+c]/255.0f;
    q[6].color[3]=1;
    return 7;
}
