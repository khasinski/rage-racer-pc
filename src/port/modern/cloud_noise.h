#ifndef RAGE_CLOUD_NOISE_H
#define RAGE_CLOUD_NOISE_H
#include <stdint.h>
#include <math.h>
/* Tileable volume generated once in C. RGBA8 at 64 cubed occupies 1 MiB;
 * no external noise files, runtime shader compilation or download is needed. */
enum { RAGE_CLOUD_NOISE_EDGE=64, RAGE_CLOUD_NOISE_BYTES=64*64*64*4 };
static float CloudHash(int x,int y,int z,int period) {
    uint32_t h=(uint32_t)((x%period+period)%period)*0x8da6b343u;
    h^=(uint32_t)((y%period+period)%period)*0xd8163841u;
    h^=(uint32_t)((z%period+period)%period)*0xcb1ab31fu;
    h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;
    return (float)(h>>8)*(1.0f/16777215.0f);
}
static float CloudMix(float a,float b,float t){return a+(b-a)*t;}
static float CloudValue(float x,float y,float z,int period){
    int ix=(int)floorf(x),iy=(int)floorf(y),iz=(int)floorf(z);
    float fx=x-(float)ix,fy=y-(float)iy,fz=z-(float)iz;
    fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy);fz=fz*fz*(3-2*fz);
    float layer[2];
    for(int k=0;k<2;k++)layer[k]=CloudMix(
        CloudMix(CloudHash(ix,iy,iz+k,period),CloudHash(ix+1,iy,iz+k,period),fx),
        CloudMix(CloudHash(ix,iy+1,iz+k,period),CloudHash(ix+1,iy+1,iz+k,period),fx),fy);
    return CloudMix(layer[0],layer[1],fz);
}
static float CloudWorley(float x,float y,float z){
    int ix=(int)floorf(x),iy=(int)floorf(y),iz=(int)floorf(z);float best=3.0f;
    for(int dz=-1;dz<=1;dz++)for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
        int cx=ix+dx,cy=iy+dy,cz=iz+dz;
        float px=(float)cx+CloudHash(cx,cy,cz,8)-x;
        float py=(float)cy+CloudHash(cx+31,cy+17,cz+47,8)-y;
        float pz=(float)cz+CloudHash(cx+13,cy+53,cz+29,8)-z;
        float d=px*px+py*py+pz*pz;if(d<best)best=d;
    }
    return 1.0f-fminf(sqrtf(best),1.0f);
}
static void CloudNoiseGenerate(uint8_t *pixels){
    for(int z=0;z<64;z++)for(int y=0;y<64;y++)for(int x=0;x<64;x++){
        /* Texel centres make linear filtering periodic across all six faces. */
        float px=((float)x+0.5f)/8.0f,py=((float)y+0.5f)/8.0f,pz=((float)z+0.5f)/8.0f;
        int at=((z*64+y)*64+x)*4;
        float base=0.50f*CloudValue(px*0.5f,py*0.5f,pz*0.5f,4)+
            0.28f*CloudValue(px,py,pz,8)+0.15f*CloudValue(px*2,py*2,pz*2,16)+
            0.07f*CloudValue(px*4,py*4,pz*4,32);
        pixels[at]=(uint8_t)(base*255.0f+0.5f);
        float billows=0.60f*CloudWorley(px,py,pz)+
            0.28f*CloudWorley(px*2,py*2,pz*2)+0.12f*CloudWorley(px*4,py*4,pz*4);
        pixels[at+1]=(uint8_t)(billows*255.0f+0.5f);
        pixels[at+2]=(uint8_t)(CloudWorley(px*4,py*4,pz*4)*255.0f+0.5f);
        pixels[at+3]=(uint8_t)(CloudValue(px*0.5f,0,pz*0.5f,4)*255.0f+0.5f);
    }
}
#endif
