#ifndef LAUNCHER_CAR_SPEC_H
#define LAUNCHER_CAR_SPEC_H
#include "game/car.h"
#include <stddef.h>
#include <stdint.h>
#include <errno.h>

/* Decode serialized little-endian fields without casting unaligned disc bytes. */
static uint32_t SpecU32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 |
           (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static int EmitCarLayout(int base,int x,int y,int z) {
    /* Imported mesh coordinates invert PS1 Y/Z; runtime scales meshes by
     * 0.25 while suspension offsets remain in game-world units. */
    x*=4; y*=-4; z*=-4;
    printf("{\"instances\":["
           "{\"part\":%d,\"position\":[0,0,0],\"scale\":[1,1,1]},"
           "{\"part\":%d,\"position\":[0,0,0],\"scale\":[1,1,1]},"
           "{\"part\":%d,\"position\":[%d,%d,%d],\"scale\":[1,1,1]},"
           "{\"part\":%d,\"position\":[%d,%d,%d],\"scale\":[-1,1,-1]}]}\n",
           base,base+3,base+2,x,y,z,base+2,-x,y,z);
    return 0;
}
/* Neutral player-car pose, matching GameRenderWorldSubmitCarAssembly.
 * Rear wheels are authored together at their body-relative positions. */
static int CarLayoutCommand(int argc, char **argv) {
    unsigned char header[40];
    FILE *file;
    long size;
    uint32_t model, image;
    int x, y, z;
    if (argc != 3) return 1;
    file=fopen(argv[2], "rb");
    if (!file) return 1;
    if (fread(header,1,sizeof(header),file)!=sizeof(header) ||
        fseek(file,0,SEEK_END) || (size=ftell(file))<40) {
        fclose(file); return 1;
    }
    fclose(file);
    model=SpecU32(header+32); image=SpecU32(header+36);
    if (size>128*1024*1024 || model<40 || image<=model || image>=(uint32_t)size) {
        fprintf(stderr,"Invalid player model pack\n"); return 1;
    }
    x=(int16_t)(header[0]|(unsigned)header[1]<<8);
    y=(int16_t)(header[2]|(unsigned)header[3]<<8);
    z=(int16_t)(header[4]|(unsigned)header[5]<<8);
    return EmitCarLayout(0,x,y,z);
}
static int RivalLayoutCommand(int argc,char **argv) {
    /* Palette-zero entries in g_CarModelBankTable for body meshes 0..30. */
    static const int modelIndex[]={0,1,2,3,4,6,8};
    unsigned char header[44],params[6];
    FILE *file;
    long size,base;
    char *end;
    uint32_t offsets[11];
    int i,x,y,z;
    if(argc!=4)return 1;
    errno=0;base=strtol(argv[3],&end,10);
    if(errno||!argv[3][0]||*end||base<0||base>30||base%5)return 1;
    file=fopen(argv[2],"rb");if(!file)return 1;
    if(fread(header,1,44,file)!=44||fseek(file,0,SEEK_END)||
       (size=ftell(file))<44||size>128*1024*1024)goto invalid;
    for(i=0;i<11;i++){
        offsets[i]=SpecU32(header+i*4);
        if(offsets[i]<44||offsets[i]>=(uint32_t)size||
           (i&&offsets[i]<=offsets[i-1]))goto invalid;
    }
    if(offsets[1]-offsets[0]<12+11*8||
       fseek(file,(long)offsets[0]+12+modelIndex[base/5]*8,SEEK_SET)||
       fread(params,1,6,file)!=6)goto invalid;
    fclose(file);
    x=(int16_t)(params[0]|(unsigned)params[1]<<8);
    y=(int16_t)(params[2]|(unsigned)params[3]<<8);
    z=(int16_t)(params[4]|(unsigned)params[5]<<8);
    return EmitCarLayout((int)base,x,y,z);
invalid:
    fclose(file);fprintf(stderr,"Invalid rival model pack\n");return 1;
}
static const struct SpecField {
    const char *key;
    size_t offset;
    int min, max;
} specFields[] = {
    {"revLimit", offsetof(GameCarSpec, revLimit), 1, 32767},
    {"redline", offsetof(GameCarSpec, redline), 1, 32767},
    {"topGear", offsetof(GameCarSpec, topGear), 1, CAR_FORWARD_GEAR_COUNT},
    {"automaticAccelerationScale", offsetof(GameCarSpec, automaticAccelerationScale), 1, 32767},
#define SHIFT_FIELDS(n) \
    {"downshift" #n, offsetof(GameCarSpec, shiftPoints)+(n-1)*sizeof(GameCarSpecShiftPoint), -32768, 32767}, \
    {"upshift" #n, offsetof(GameCarSpec, shiftPoints)+(n-1)*sizeof(GameCarSpecShiftPoint)+2, -32768, 32767}
    SHIFT_FIELDS(1), SHIFT_FIELDS(2), SHIFT_FIELDS(3),
    SHIFT_FIELDS(4), SHIFT_FIELDS(5), SHIFT_FIELDS(6)
#undef SHIFT_FIELDS
};
static int CarTransmissionCommand(int argc,char **argv) {
    unsigned char *bytes=NULL;
    FILE *file;
    long length;
    size_t count;
    int result=1,writeMode=argc==6&&strcmp(argv[2],"write")==0;
    if(!((argc==4&&strcmp(argv[2],"read")==0)||writeMode))return 1;
    file=fopen(argv[3],"rb");if(!file)return 1;
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<40||length>128*1024*1024||fseek(file,0,SEEK_SET)){fclose(file);return 1;}
    bytes=malloc((size_t)length);if(!bytes){fclose(file);return 1;}
    count=fread(bytes,1,(size_t)length,file);fclose(file);
    if(count!=(size_t)length||SpecU32(bytes+32)!=40||SpecU32(bytes+36)<=40||SpecU32(bytes+36)>=(uint32_t)length)goto done;
    if(writeMode){
        if(strcmp(argv[5],"0")&&strcmp(argv[5],"1"))goto done;
        bytes[8]=(unsigned char)(argv[5][0]=='0');
        file=fopen(argv[4],"wbx");if(!file)goto done;
        count=fwrite(bytes,1,(size_t)length,file);
        if(fclose(file)||count!=(size_t)length){remove(argv[4]);goto done;}
    }
    printf("{\"manualOnly\":%d}\n",bytes[8]==0);result=0;
done:
    free(bytes);if(result)fprintf(stderr,"Invalid transmission model pack or output\n");return result;
}
static int CarSpecCommand(int argc, char **argv) {
    unsigned char *bytes = NULL, *spec;
    FILE *file;
    long length;
    uint32_t offsets[5];
    size_t i, j;
    int writeMode = argc >= 7 && strcmp(argv[2], "write") == 0;
    int result = 1;
    if (!((argc == 4 && strcmp(argv[2], "read") == 0) ||
          (writeMode && (argc-5)%2 == 0))) {
        fprintf(stderr, "usage: --car-spec read FILE | --car-spec write FILE OUTPUT KEY VALUE [KEY VALUE ...]\n");
        return 1;
    }
    file = fopen(argv[3], "rb");
    if (!file) { perror(argv[3]); return 1; }
    if (fseek(file, 0, SEEK_END) || (length=ftell(file)) < 20 ||
        length > 128*1024*1024 || fseek(file, 0, SEEK_SET)) {
        fclose(file); goto done;
    }
    bytes = malloc((size_t)length);
    if (!bytes) { fclose(file); goto done; }
    i = fread(bytes, 1, (size_t)length, file);
    fclose(file);
    if (i != (size_t)length) goto done;
    for (i=0; i<5; ++i) {
        offsets[i] = SpecU32(bytes+i*4);
        if (offsets[i] >= (uint32_t)length ||
            (i && offsets[i] <= offsets[i-1])) goto done;
    }
    if (offsets[0] < 20 || offsets[1]-offsets[0] < sizeof(GameCarSpec)) goto done;
    spec = bytes+offsets[0];
    if (writeMode) {
        for (i=5; i<(size_t)argc; i+=2) {
            char *end;
            long value;
            for (j=0; j<sizeof(specFields)/sizeof(specFields[0]); ++j)
                if (strcmp(argv[i], specFields[j].key) == 0) break;
            if (j == sizeof(specFields)/sizeof(specFields[0])) goto done;
            errno=0; value=strtol(argv[i+1], &end, 10);
            if (errno || !argv[i+1][0] || *end ||
                value<specFields[j].min || value>specFields[j].max) goto done;
            spec[specFields[j].offset]=(unsigned char)value;
            spec[specFields[j].offset+1]=(unsigned char)(value>>8);
        }
        /* Exclusive creation also protects the source through aliases/hard links. */
        file=fopen(argv[4], "wbx");
        if (!file) { perror(argv[4]); goto done; }
        i=fwrite(bytes, 1, (size_t)length, file);
        j=(size_t)fclose(file);
        if (i!=(size_t)length || j) { remove(argv[4]); goto done; }
    }
    printf("{\"fields\":[");
    for (i=0; i<sizeof(specFields)/sizeof(specFields[0]); ++i) {
        const struct SpecField *field=&specFields[i];
        int value=(int16_t)(spec[field->offset] | (unsigned)spec[field->offset+1]<<8);
        printf("%s{\"key\":\"%s\",\"value\":%d,\"min\":%d,\"max\":%d}",
               i?",":"", field->key, value, field->min, field->max);
    }
    puts("]}"); result=0;
done:
    free(bytes);
    if (result) fprintf(stderr, "Invalid car specification, edit, or output path\n");
    return result;
}
#endif
