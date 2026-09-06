#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static void Le16(unsigned char *p,unsigned n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void Le32(unsigned char *p,unsigned n){Le16(p,n);Le16(p+2,n>>16);}
static size_t Image(unsigned char *p,unsigned colours,unsigned words,unsigned rows) {
    unsigned palette=12+2*colours,pixels=12+2*words*rows,payload=8+palette+pixels;
    memset(p,0,12+payload);Le32(p+4,payload);Le32(p+12,8);
    unsigned char *clut=p+16;Le32(clut,palette);Le16(clut+6,500);Le16(clut+8,colours);Le16(clut+10,1);
    for(unsigned i=1;i<colours;++i)Le16(clut+12+2*i,((i%31)|((i%32)<<5)|((i%17)<<10))|1);
    unsigned char *pix=clut+palette;Le32(pix,pixels);Le16(pix+4,64);Le16(pix+8,words);Le16(pix+10,rows);
    for(unsigned i=0;i<2*words*rows;++i)pix[12+i]=(unsigned char)i;
    return 12+payload;
}
static void RunExpect(const char *tool,const char *a,const char *b,const char *message) {
    const char *args[]={tool,a,b,NULL};
    SDL_PropertiesID props=SDL_CreateProperties();assert(props);
    assert(SDL_SetPointerProperty(props,SDL_PROP_PROCESS_CREATE_ARGS_POINTER,(void *)args));
    assert(SDL_SetNumberProperty(props,SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER,SDL_PROCESS_STDIO_APP));
    assert(SDL_SetBooleanProperty(props,SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN,true));
    SDL_Process *process=SDL_CreateProcessWithProperties(props);SDL_DestroyProperties(props);assert(process);
    int code=-1;size_t size;char *output=SDL_ReadProcess(process,&size,&code);
    SDL_DestroyProcess(process);assert(output&&code==0);
    if(message)assert(strstr(output,message));
    SDL_free(output);
}
static void Run(const char *tool,const char *a,const char *b){RunExpect(tool,a,b,NULL);}
static void Save(const char *path,const void *bytes,size_t size) {
    FILE *f=fopen(path,"wbx");assert(f);assert(fwrite(bytes,1,size,f)==size);assert(!fclose(f));
}
static void Check(const char *path,const void *expected,size_t size) {
    size_t actualSize;void *actual=SDL_LoadFile(path,&actualSize);assert(actual);
    assert(actualSize==size&&!memcmp(actual,expected,size));SDL_free(actual);
}
int main(int argc,char **argv) {
    assert(argc==3);
    char root[128],archive[160],mod[160],file[256];
    snprintf(root,sizeof(root),"mod_archive_%llu",(unsigned long long)SDL_GetTicksNS());
    assert(SDL_CreateDirectory(root));
    snprintf(archive,sizeof(archive),"%s/RAGE.BIN",root);snprintf(mod,sizeof(mod),"%s/mod",root);
    unsigned char bytes[8192]={0},first[2048],second[2048];
    size_t a=Image(first,16,16,32),b=Image(second,256,16,24);
    const char opaque[]="not an image at all";
    Le32(bytes,1);Le32(bytes+4,(unsigned)a);memcpy(bytes+2048,first,a);
    Le32(bytes+8,2);Le32(bytes+12,(unsigned)b);memcpy(bytes+4096,second,b);
    Le32(bytes+16,3);Le32(bytes+20,sizeof(opaque));memcpy(bytes+6144,opaque,sizeof(opaque));
    Save(archive,bytes,6144+sizeof(opaque));Run(argv[1],archive,mod);
    for(int i=0;i<2;++i) {
        snprintf(file,sizeof(file),"%s/textures/asset_%03d_00.png",mod,i);
        SDL_Surface *surface=SDL_LoadPNG(file);assert(surface);
        assert(surface->w==(i?32:64)&&surface->h==(i?24:32));SDL_DestroySurface(surface);
    }
    Run(argv[2],mod,NULL);
    snprintf(file,sizeof(file),"%s/raw/asset_000.bin",mod);Check(file,first,a);
    snprintf(file,sizeof(file),"%s/raw/asset_001.bin",mod);Check(file,second,b);
    snprintf(file,sizeof(file),"%s/raw/asset_002.bin",mod);Check(file,opaque,sizeof(opaque));
    snprintf(file,sizeof(file),"%s/textures/asset_001_00.png",mod);
    SDL_Surface *surface=SDL_LoadPNG(file);assert(surface);
    Uint8 r,g,blue,alpha;assert(SDL_ReadSurfacePixel(surface,1,0,&r,&g,&blue,&alpha));
    assert(alpha==255);assert(SDL_WriteSurfacePixel(surface,0,0,r,g,blue,alpha));
    assert(SDL_SavePNG(surface,file));SDL_DestroySurface(surface);
    size_t editedPngSize;void *editedPng=SDL_LoadFile(file,&editedPngSize);assert(editedPng);
    Run(argv[2],mod,NULL);
    /* Palette slot 1 is unique; only the first 8-bit texel must change. */
    second[16+12+512+12]=1;
    snprintf(file,sizeof(file),"%s/raw/asset_001.bin",mod);Check(file,second,b);
    snprintf(file,sizeof(file),"%s/raw/asset_000.bin",mod);Check(file,first,a);
    snprintf(file,sizeof(file),"%s/raw/asset_002.bin",mod);Check(file,opaque,sizeof(opaque));
    /* Corrupt texture input must not disturb the last successfully packed edit. */
    snprintf(file,sizeof(file),"%s/textures/asset_001_00.png",mod);
    surface=SDL_CreateSurface(8,8,SDL_PIXELFORMAT_RGBA32);assert(surface);
    assert(SDL_ClearSurface(surface,0,0,0,1));
    assert(SDL_SavePNG(surface,file));SDL_DestroySurface(surface);
    RunExpect(argv[2],mod,NULL,"the size is fixed");
    snprintf(file,sizeof(file),"%s/raw/asset_001.bin",mod);Check(file,second,b);
    snprintf(file,sizeof(file),"%s/raw/asset_000.bin",mod);Check(file,first,a);
    snprintf(file,sizeof(file),"%s/raw/asset_002.bin",mod);Check(file,opaque,sizeof(opaque));
    snprintf(file,sizeof(file),"%s/textures/asset_001_00.png",mod);
    assert(SDL_RemovePath(file));Save(file,"not a PNG",9);RunExpect(argv[2],mod,NULL,"is not a PNG");
    snprintf(file,sizeof(file),"%s/raw/asset_001.bin",mod);Check(file,second,b);
    snprintf(file,sizeof(file),"%s/raw/asset_000.bin",mod);Check(file,first,a);
    snprintf(file,sizeof(file),"%s/raw/asset_002.bin",mod);Check(file,opaque,sizeof(opaque));
    /* Insert one byte before the palette, then edit through the real pack tool. */
    memmove(second+29,second+28,b-28);second[28]=0;++b;
    snprintf(file,sizeof(file),"%s/raw/asset_001.bin",mod);assert(SDL_RemovePath(file));Save(file,second,b);
    snprintf(file,sizeof(file),"%s/textures/asset_001_00.json",mod);assert(SDL_RemovePath(file));
    const char oddJson[]="{\"asset\":1,\"pixels_offset\":553,\"pixel_bytes\":768,\"depth\":8,\"width\":32,\"height\":24,\"clut\":{\"offset\":29,\"colours\":256}}";
    Save(file,oddJson,sizeof(oddJson)-1);
    snprintf(file,sizeof(file),"%s/textures/asset_001_00.png",mod);assert(SDL_RemovePath(file));Save(file,editedPng,editedPngSize);SDL_free(editedPng);
    surface=SDL_LoadPNG(file);assert(surface);
    assert(SDL_ReadSurfacePixel(surface,2,0,&r,&g,&blue,&alpha));assert(alpha==255);
    assert(SDL_WriteSurfacePixel(surface,0,0,r,g,blue,alpha));assert(SDL_SavePNG(surface,file));SDL_DestroySurface(surface);
    Run(argv[2],mod,NULL);second[553]=2;
    snprintf(file,sizeof(file),"%s/raw/asset_001.bin",mod);Check(file,second,b);
    snprintf(file,sizeof(file),"%s/raw/asset_000.bin",mod);Check(file,first,a);
    snprintf(file,sizeof(file),"%s/raw/asset_002.bin",mod);Check(file,opaque,sizeof(opaque));
    for(int i=0;i<3;++i){snprintf(file,sizeof(file),"%s/raw/asset_%03d.bin",mod,i);assert(SDL_RemovePath(file));}
    for(int i=0;i<2;++i)for(int j=0;j<2;++j){snprintf(file,sizeof(file),"%s/textures/asset_%03d_00.%s",mod,i,j?"json":"png");assert(SDL_RemovePath(file));}
    const char *paths[]={"textures/index.txt","textures","raw","manifest.json"};
    for(size_t i=0;i<sizeof(paths)/sizeof(paths[0]);++i){snprintf(file,sizeof(file),"%s/%s",mod,paths[i]);assert(SDL_RemovePath(file));}
    assert(SDL_RemovePath(mod));assert(SDL_RemovePath(archive));assert(SDL_RemovePath(root));
    puts("compiled archive extraction, untouched repack and exact edit isolation passed");return 0;
}
