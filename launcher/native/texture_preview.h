/* Decode PS1 texture pages from explicitly bounded raw VRAM uploads. */
static unsigned Read16(const unsigned char *p){return p[0]|((unsigned)p[1]<<8);}
static unsigned Read32(const unsigned char *p){return Read16(p)|(Read16(p+2)<<16);}
static int PreviewEntry(unsigned short *vram,const unsigned char *data,size_t size,size_t at){
    unsigned blocks,b;if(at>size||8>size-at)return 0;
    blocks=(Read32(data+at+4)&8)?2:1;at+=8;
    for(b=0;b<blocks;b++){
        unsigned x,y,bx,by,w,h,length;
        if(at>size||12>size-at)return 0;
        length=Read32(data+at);bx=Read16(data+at+4);by=Read16(data+at+6);w=Read16(data+at+8);h=Read16(data+at+10);
        if(bx>=1024||by>=512||w>1024-bx||h>512-by||(size_t)w*h*2>size-at-12)return 0;
        for(y=0;y<h;y++)for(x=0;x<w;x++)vram[(by+y)*1024+bx+x]=(unsigned short)Read16(data+at+12+(y*w+x)*2);
        if(b+1<blocks&&(length<12||length>size-at))return 0;at+=length;
    }
    return 1;
}
static int PreviewChain(unsigned short *vram,const unsigned char *data,size_t size,size_t at){
    if(at>size||4>size-at)return 0;at+=4;
    while(at<=size&&4<=size-at){unsigned n=Read32(data+at);at+=4;if(!n||n>0x7fffffff)return 1;
        if(n>size-at||!PreviewEntry(vram,data,at+n,at))return 0;at+=n;
    }return 0;
}
static int TexturePreview(int argc,char **argv){
    static unsigned short vram[1024*512];
    unsigned page,clut,x,y;int arg;FILE *file;
    unsigned char rgba[256*256*4];
    if(argc<6||(argc-5)%6)return 1;
    page=(unsigned)strtoul(argv[2],NULL,10);clut=(unsigned)strtoul(argv[3],NULL,10);
    if(page>65535||clut>65535)return 1;
    for(arg=5;arg<argc;arg+=6){
        long offset=strtol(argv[arg+1],NULL,10),size;unsigned bx=(unsigned)strtoul(argv[arg+2],NULL,10),by=(unsigned)strtoul(argv[arg+3],NULL,10),w=(unsigned)strtoul(argv[arg+4],NULL,10),h=(unsigned)strtoul(argv[arg+5],NULL,10);unsigned char *data;
        if(!w||!h||bx>=1024||by>=512||w>1024-bx||h>512-by)return 1;
        file=fopen(argv[arg],"rb");if(!file)return 1;
        if(fseek(file,0,SEEK_END)||(size=ftell(file))<0||size>128*1024*1024||fseek(file,0,SEEK_SET)){fclose(file);return 1;}
        data=malloc((size_t)size);if(!data){fclose(file);return 1;}
        if(fread(data,1,(size_t)size,file)!=(size_t)size){free(data);fclose(file);return 1;}fclose(file);
        if(offset==-4){
            if(!PreviewChain(vram,data,(size_t)size,0)){free(data);return 1;}
            free(data);continue;
        }
        if(offset==-3){
            unsigned block;if(size<20){free(data);return 1;}
            for(block=0;block<5;block++){
                size_t at=Read32(data+block*4);
                if(!(block==2?PreviewEntry(vram,data,(size_t)size,at):PreviewChain(vram,data,(size_t)size,at))){free(data);return 1;}
            }
            free(data);continue;
        }
        if(offset==-1 && size>=40)offset=(long)Read32(data+36);
        if(offset<0||offset>size||(size_t)w*h*2>(size_t)(size-offset)){free(data);return 1;}
        for(y=0;y<h;y++)for(x=0;x<w;x++)vram[(by+y)*1024+bx+x]=(unsigned short)Read16(data+offset+(y*w+x)*2);
        free(data);
    }
    for(y=0;y<256;y++)for(x=0;x<256;x++){
        unsigned mode=(page>>7)&3,px=(page&15)*64,py=((page>>4)&1)*256,word=0,cx=(clut&63)*16,cy=(clut>>6)&511,index=0;
        if(mode==0){if(px+x/4<1024)index=(vram[(py+y)*1024+px+x/4]>>((x&3)*4))&15;}
        else if(mode==1){if(px+x/2<1024)index=(vram[(py+y)*1024+px+x/2]>>((x&1)*8))&255;}
        if(mode<=1){if(cx+index<1024)word=vram[cy*1024+cx+index];}
        else if(px+x<1024)word=vram[(py+y)*1024+px+x];
        unsigned char *p=rgba+(y*256+x)*4;p[0]=(unsigned char)((word&31)*255/31);p[1]=(unsigned char)(((word>>5)&31)*255/31);p[2]=(unsigned char)(((word>>10)&31)*255/31);p[3]=word?255:0;
    }
    size_t pathLength=strlen(argv[4]);
    if(pathLength>=4&&!strcmp(argv[4]+pathLength-4,".png"))return WritePng(argv[4],rgba,256,256)?0:1;
    file=fopen(argv[4],"wb");if(!file)return 1;
    int ok=1;
    if(pathLength>=4&&!strcmp(argv[4]+pathLength-4,".tga")){
        const unsigned char header[18]={0,0,2,0,0,0,0,0,0,0,0,0,0,1,0,1,32,0x28};
        ok=fwrite(header,1,sizeof(header),file)==sizeof(header);
        for(x=0;x<256*256;x++){unsigned char red=rgba[x*4];rgba[x*4]=rgba[x*4+2];rgba[x*4+2]=red;}
    }
    if(fwrite(rgba,1,sizeof(rgba),file)!=sizeof(rgba))ok=0;
    if(fclose(file))ok=0;return ok?0:1;
}
