#include "sha256.h"
#include <stdio.h>
#include <string.h>

static uint32_t Rotate(uint32_t n,unsigned bits){return n>>bits|n<<(32-bits);}
static void Block(Sha256 *hash,const unsigned char *bytes){
    static const uint32_t k[64]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    uint32_t words[64],a,b,c,d,e,f,g,h;
    unsigned i;
    for(i=0;i<16;i++)words[i]=(uint32_t)bytes[i*4]<<24|(uint32_t)bytes[i*4+1]<<16|(uint32_t)bytes[i*4+2]<<8|bytes[i*4+3];
    for(i=16;i<64;i++){
        uint32_t x=words[i-15],y=words[i-2];
        words[i]=words[i-16]+(Rotate(x,7)^Rotate(x,18)^(x>>3))+words[i-7]+(Rotate(y,17)^Rotate(y,19)^(y>>10));
    }
    a=hash->state[0];b=hash->state[1];c=hash->state[2];d=hash->state[3];
    e=hash->state[4];f=hash->state[5];g=hash->state[6];h=hash->state[7];
    for(i=0;i<64;i++){
        uint32_t t1=h+(Rotate(e,6)^Rotate(e,11)^Rotate(e,25))+((e&f)^(~e&g))+k[i]+words[i];
        uint32_t t2=(Rotate(a,2)^Rotate(a,13)^Rotate(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    hash->state[0]+=a;hash->state[1]+=b;hash->state[2]+=c;hash->state[3]+=d;
    hash->state[4]+=e;hash->state[5]+=f;hash->state[6]+=g;hash->state[7]+=h;
}
void Sha256Init(Sha256 *hash){
    *hash=(Sha256){{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19},0,{0},0};
}
void Sha256Update(Sha256 *hash,const void *data,size_t size){
    const unsigned char *bytes=data;
    hash->bytes+=size;
    while(size){
        size_t chunk=64-hash->used;if(chunk>size)chunk=size;
        memcpy(hash->tail+hash->used,bytes,chunk);hash->used+=chunk;bytes+=chunk;size-=chunk;
        if(hash->used==64){Block(hash,hash->tail);hash->used=0;}
    }
}
void Sha256Finish(Sha256 *hash,char hex[65]){
    static const char digits[]="0123456789abcdef";
    unsigned char padding[128]={0x80},length[8];
    uint64_t bits=hash->bytes*8;
    size_t pad=hash->used<56?56-hash->used:120-hash->used;
    unsigned i;
    for(i=0;i<8;i++)length[7-i]=(unsigned char)(bits>>(i*8));
    Sha256Update(hash,padding,pad);Sha256Update(hash,length,8);
    for(i=0;i<32;i++){
        unsigned n=(hash->state[i/4]>>(24-(i%4)*8))&255;
        hex[i*2]=digits[n>>4];hex[i*2+1]=digits[n&15];
    }
    hex[64]=0;
}
int Sha256File(const char *path,size_t limit,char hex[65]){
    unsigned char bytes[65536];size_t n,total=0;Sha256 hash;int ok=1;
    FILE *file=fopen(path,"rb");if(!file)return 0;
    Sha256Init(&hash);
    while((n=fread(bytes,1,sizeof(bytes),file))!=0){
        if(n>limit-total){ok=0;break;}
        total+=n;Sha256Update(&hash,bytes,n);
    }
    if(ferror(file))ok=0;
    if(fclose(file))ok=0;
    if(ok)Sha256Finish(&hash,hex);
    return ok;
}
