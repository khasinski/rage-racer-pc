#ifndef LAUNCHER_CAR_MATERIALS_H
#define LAUNCHER_CAR_MATERIALS_H
#include "game/model_stream.h"
#include "render/asset_id.h"
/* Model-bank traversal order matches NativeAssetImporter ImportVisitModelBank /
 * ImportScanFace. Material slots are first occurrences of a page/CLUT pair. */
static int CarMaterialsCommand(int argc,char **argv) {
    FILE *file;
    unsigned char *bytes=NULL,*bank;
    long length;
    uint32_t offset,size,count,payload,slots=0;
    uint16_t pages[4096],cluts[4096];
    int result=1;
    unsigned long assetKey=0;
    RageRenderAssetSet assetSet;
    if((argc!=4 && argc!=5) || (strcmp(argv[2],"player") && strcmp(argv[2],"rival")))return 1;
    assetSet=!strcmp(argv[2],"player")?RAGE_RENDER_ASSET_MODEL_BANK:RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    if(argc==5){char *end;char id[160];errno=0;assetKey=strtoul(argv[4],&end,10);if(errno||!*argv[4]||*end||assetKey>UINT32_MAX||!AssetMaterialId(id,sizeof(id),(uint32_t)assetKey,assetSet,0))return 1;}
    file=fopen(argv[3],"rb");if(!file)return 1;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<44||length>128*1024*1024||fseek(file,0,SEEK_SET)){fclose(file);return 1;}
    bytes=malloc((size_t)length);if(!bytes){fclose(file);return 1;}
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length){fclose(file);goto done;}
    fclose(file);
    if(!strcmp(argv[2],"player")){
        offset=SpecU32(bytes+32);size=SpecU32(bytes+24);
        if(offset!=40)goto done;
        if(size>(uint32_t)length-offset || SpecU32(bytes+36)<offset+size || SpecU32(bytes+36)>=(uint32_t)length)goto done;
    }else{
        offset=SpecU32(bytes+12);
        uint32_t end=SpecU32(bytes+16);
        if(offset<44 || end<=offset || end>(uint32_t)length)goto done;
        size=end-offset;
    }
    if(size<12)goto done;
    bank=bytes+offset;count=SpecU32(bank);
    if(!count||count>256||count>(size-12)/4)goto done;
    payload=12+count*4;
    if(SpecU32(bank+4)<payload||SpecU32(bank+4)>=size||SpecU32(bank+8)<payload||SpecU32(bank+8)>=size)goto done;
    for(uint32_t model=0;model<count;model++){
        uint32_t cursor=SpecU32(bank+12+model*4);
        if(cursor<payload)goto done;
        for(;;){
            if(cursor>size||size-cursor<4)goto done;
            unsigned prim=bank[cursor]|(unsigned)bank[cursor+1]<<8;
            unsigned faces=bank[cursor+2]|(unsigned)bank[cursor+3]<<8;
            cursor+=4;if(!faces)break;
            int stride=ModelPrimitiveStride((int)prim);
            if(!stride||faces>(size-cursor)/(unsigned)stride)goto done;
            for(unsigned face=0;face<faces;face++,cursor+=(unsigned)stride){
                if(prim!=1&&prim!=3)continue;
                unsigned clutAt=cursor+(prim==1?10:18),pageAt=cursor+(prim==1?14:22);
                uint16_t clut=(uint16_t)(bank[clutAt]|(unsigned)bank[clutAt+1]<<8);
                uint16_t page=(uint16_t)(bank[pageAt]|(unsigned)bank[pageAt+1]<<8);
                uint32_t slot;
                for(slot=0;slot<slots;slot++)if(pages[slot]==page&&cluts[slot]==clut)break;
                if(slot==slots){if(slots==4096)goto done;pages[slots]=page;cluts[slots++]=clut;}
            }
        }
    }
    printf("{\"materials\":[");
    for(uint32_t slot=0;slot<slots;slot++){
        printf("%s{\"slot\":%u,\"page\":%u,\"clut\":%u",slot?",":"",slot,pages[slot],cluts[slot]);
        if(argc==5){char id[160];AssetMaterialId(id,sizeof(id),(uint32_t)assetKey,assetSet,slot);printf(",\"id\":\"%s\"",id);}
        putchar('}');
    }
    puts("]}");result=0;
 done:free(bytes);if(result)fprintf(stderr,"Invalid car model material table\n");return result;
}
#endif
