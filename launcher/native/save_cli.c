/* JSON/argv boundary around the same compiled save library as the save editor.
 * Input edits are validated completely before writing a separate output file. */
#include "rage_save.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Field {
    char key[128];
    void *address;
    int size, sign, time, text;
} Field;
static Field fields[2048];
static int count;
static void Json(const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    putchar('"');
    for (; *p; p++) {
        if (*p == '"' || *p == '\\') printf("\\%c", *p);
        else if (*p < 32 || *p >= 127) printf("\\u%04x", *p);
        else putchar(*p);
    }
    putchar('"');
}
static void Add(const char *key, void *address, int size, int sign, int time) {
    Field *f;
    if (count >= (int)(sizeof(fields)/sizeof(fields[0]))) abort();
    f = &fields[count++];
    snprintf(f->key, sizeof(f->key), "%s", key);
    f->address = address; f->size = size; f->sign = sign; f->time = time;
    f->text = 0;
}
static void AddName(const char *key, char *address, int size) {
    Add(key,address,size,0,0);fields[count-1].text=1;
}
#define FIELD(b, f, sign) Add(#f, &(b)->f, sizeof((b)->f), sign, 0)
static void Describe(RageSaveFile *save) {
    GameSaveBlock *b = &save->block;
    int a, c, d;
    char key[128];
    const char *modes[] = {"Grand Prix", "Extra Grand Prix", "Time Attack"};
    SavedRaceProgress *progress[] = {&b->grandPrixProgress, &b->extraGrandPrixProgress,
                                     &b->timeAttackProgress};
    count = 0;
    Add("Advanced / Save counter", &save->header.fields.saveCounter,
        sizeof(save->header.fields.saveCounter), 0, 0);
    for (a = 0; a < (int)sizeof(b->reserved); ++a) {
        snprintf(key, sizeof(key), "Advanced / Reserved byte %02d", a);
        Add(key, &b->reserved[a], 1, 0, 0);
    }
    FIELD(b,padMappingIndex,0); FIELD(b,negconMappingIndex,0);
    FIELD(b,negconSteerNeutral,0); FIELD(b,negconSteerPlay,0);
    FIELD(b,negconNeutralI,0); FIELD(b,negconNeutralII,0); FIELD(b,negconNeutralL,0);
    FIELD(b,negconMaxTwist,0); FIELD(b,bgmSelection,1);
    FIELD(b,extraGrandPrixUnlocked,0); FIELD(b,bgmVolume,1);
    FIELD(b,sfxVolume,1); FIELD(b,monoOutput,1);
    for (a = 0; a < 3; a++) {
        const char *names[] = {"Course", "Car", "Class", "Highest class", "Credits"};
        for (d = 0; d < 5; d++) {
            snprintf(key,sizeof(key),"%s / %s",modes[a],a==2 && d==4?"Series":names[d]);
            Add(key,(unsigned char *)progress[a]+d*4,4,1,0);
        }
        for (c = 0; c < GAME_CAR_COUNT; c++) {
            const char *names[] = {"Grade", "Tires", "Transmission", "Paint 1", "Paint 2", "Owned"};
            for (d = 0; d < 6; d++) {
                snprintf(key,sizeof(key),"%s / Car %02d / %s",modes[a],c,names[d]);
                Add(key,(unsigned char *)&b->carSetup[a][c]+d,1,0,0);
            }
        }
    }
    for (a = 0; a < 2; a++) {
        snprintf(key,sizeof(key),"Progress / Highest class %d",a);
        Add(key,&b->maxClassReached[a],4,1,0);
        for (c = 0; c < 4; c++) {
            for (d = 0; d < 2; d++) {
                snprintf(key,sizeof(key),"Best lap / Set %d / Course %d / Slot %d",a,c,d);
                Add(key,&b->bestLapTimes[a][c][d],4,1,1);
                snprintf(key,sizeof(key),"Total time / Set %d / Course %d / Slot %d",a,c,d);
                Add(key,&b->bestTotalTimes[a][c][d],4,1,1);
            }
            for (d = 0; d < 3; d++) {
                snprintf(key,sizeof(key),"Sector / Set %d / Course %d / Sector %d",a,c,d);
                Add(key,&b->bestSectorTimes[a][c][d],4,1,1);
            }
            for (d = 0; d < 5; d++) {
                snprintf(key,sizeof(key),"Ranking / Set %d / Course %d / Slot %d / Time",a,c,d);
                Add(key,&b->rankingRecords[a][c][d].raceTime,4,1,1);
                snprintf(key,sizeof(key),"Ranking / Set %d / Course %d / Slot %d / Driver",a,c,d);
                AddName(key,b->rankingRecords[a][c][d].driverName,8);
                snprintf(key,sizeof(key),"Ranking / Set %d / Course %d / Slot %d / Car",a,c,d);
                Add(key,&b->rankingRecords[a][c][d].carIndex,2,1,0);
                snprintf(key,sizeof(key),"Time attack / Set %d / Course %d / Slot %d / Time",a,c,d);
                Add(key,&b->timeRecords[a][c][d].raceTime,4,1,1);
                snprintf(key,sizeof(key),"Time attack / Set %d / Course %d / Slot %d / Driver",a,c,d);
                AddName(key,b->timeRecords[a][c][d].driverName,8);
                snprintf(key,sizeof(key),"Time attack / Set %d / Course %d / Slot %d / Car",a,c,d);
                Add(key,&b->timeRecords[a][c][d].carIndex,2,1,0);
            }
        }
    }
    for (a = 0; a < CLASS_RECORD_COUNT; a++) {
        snprintf(key,sizeof(key),"Class records / %d / Grade",a);
        Add(key,&b->classRecords[a].grade,2,0,0);
        snprintf(key,sizeof(key),"Class records / %d / Clears",a);
        Add(key,&b->classRecords[a].clears,2,0,0);
    }
    for (a = 0; a < 8; a++) {
        snprintf(key,sizeof(key),"Grand Prix / Course progress %d",a);
        Add(key,&b->grandPrixCourseProgress[a],1,0,0);
        snprintf(key,sizeof(key),"Extra Grand Prix / Course progress %d",a);
        Add(key,&b->extraGrandPrixCourseProgress[a],1,0,0);
    }
}
static long long Value(const Field *f) {
    if (f->size==1) return *(uint8_t *)f->address;
    if (f->size==2) return f->sign?*(int16_t *)f->address:*(uint16_t *)f->address;
    return f->sign?(long long)*(int32_t *)f->address:(long long)*(uint32_t *)f->address;
}
static long long Minimum(const Field *f) { return f->sign?-(1LL<<(8*f->size-1)):0; }
static long long Maximum(const Field *f) { return (1LL<<(8*f->size-f->sign))-1; }
static int Edit(const char *key, const char *text) {
    int i; char *end; long long v;
    for(i=0;i<count;i++) if(!strcmp(fields[i].key,key)) {
        Field *f=&fields[i];
        if(f->text){
            const unsigned char *p=(const unsigned char *)text;
            size_t n=strlen(text);
            if(n>=(size_t)f->size)return 0;
            for(;*p;p++)if(*p<32||*p>126)return 0;
            memset(f->address,0,(size_t)f->size);memcpy(f->address,text,n);return 1;
        }
        errno=0; v=strtoll(text,&end,10);
        if(errno || !*text || *end) return 0;
        if(v<Minimum(f) || v>Maximum(f)) return 0;
        if(f->size==1) *(uint8_t *)f->address=(uint8_t)v;
        else if(f->size==2) { uint16_t n=(uint16_t)v; memcpy(f->address,&n,2); }
        else { uint32_t n=(uint32_t)v; memcpy(f->address,&n,4); }
        return 1;
    }
    return 0;
}
static int Hex(int c) {
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
static int EditLogo(GameSaveBlock *block,const char *key,const char *text) {
    size_t i;
    if(!strcmp(key,"logo.pixels")) {
        if(strlen(text)!=RAGE_LOGO_WIDTH*RAGE_LOGO_HEIGHT)return 0;
        for(i=0;text[i];i++)if(Hex(text[i])<0)return 0;
        for(i=0;text[i];i++)RageLogoSetPixel(block,(int)(i%64),(int)(i/64),Hex(text[i]));
        return 1;
    }
    if(!strncmp(key,"logo.color.",11)) {
        char *end;long index;unsigned char rgb[3];
        index=strtol(key+11,&end,10);
        if(!key[11]||*end||index<0||index>=16||strlen(text)!=7||(text[6]!='0'&&text[6]!='1'))return 0;
        for(i=0;i<6;i++)if(Hex(text[i])<0)return 0;
        for(i=0;i<3;i++)rgb[i]=(unsigned char)(16*Hex(text[i*2])+Hex(text[i*2+1]));
        block->teamLogoClut[index]=RageLogoPackColour(rgb,text[6]=='1');return 1;
    }
    return 0;
}
int main(int argc, char **argv) {
    RageSaveFile save; RageSaveReport report;
    RageCard card={0}; int cardIndex=-1, i, start=3, writing;
    char team[8]; const char *out=NULL;
    if(argc==4 && !strcmp(argv[1],"new")) {
        RageRegion region = !strcmp(argv[3],"PAL") ? RAGE_REGION_PAL :
            !strcmp(argv[3],"NTSC-U") ? RAGE_REGION_NTSC_U :
            !strcmp(argv[3],"NTSC-J") ? RAGE_REGION_NTSC_J : RAGE_REGION_UNKNOWN;
        if(region==RAGE_REGION_UNKNOWN) { fprintf(stderr,"Invalid save region.\n");return 1; }
        RageSaveInit(&save,region,0);
        if(!RageSaveStore(argv[2],&save,&report)) goto fail;
        /* Use the same JSON representation as opening an existing save. */
        argc=3;argv[1]="read";
    }
    if(argc==2 && !strcmp(argv[1],"car-names")) {
        printf("{\"international\":[");
        for(i=0;i<GAME_CAR_COUNT;i++){if(i)putchar(',');Json(RageCarName(i,RAGE_REGION_NTSC_U));}
        printf("],\"japanese\":[");
        for(i=0;i<GAME_CAR_COUNT;i++){if(i)putchar(',');Json(RageCarName(i,RAGE_REGION_NTSC_J));}
        puts("]}");return 0;
    }
    if(argc==2 && !strcmp(argv[1],"discover")) {
        RageSaveEntry entries[RAGE_SAVE_DISCOVER_MAX];
        int n=RageSaveDiscover(entries,RAGE_SAVE_DISCOVER_MAX);
        printf("["); for(i=0;i<n;i++) { if(i)putchar(',');
            printf("{\"path\":");Json(entries[i].path);printf(",\"team\":");Json(entries[i].team);
            printf(",\"money\":%d}",entries[i].money); }
        puts("]"); return 0;
    }
    if(argc<3 || (strcmp(argv[1],"read") && strcmp(argv[1],"write"))) {
        fprintf(stderr,"usage: rage-save-cli discover | new FILE PAL|NTSC-U|NTSC-J | read FILE [--card INDEX] | write FILE OUTPUT [--card INDEX] [KEY VALUE ...]\n");return 1;
    }
    writing=!strcmp(argv[1],"write");
    if(writing) { if(argc<4) return 1; out=argv[3];start=4;
        if(!strcmp(argv[2],out)) { fprintf(stderr,"Write a separate copy.\n"); return 1; } }
    if(argc>start && !strcmp(argv[start],"--card")) {
        char *end; long index;
        if(argc<=start+1) return 1;
        index=strtol(argv[start+1],&end,10);
        if(*end || index<0 || index>=RAGE_CARD_BLOCKS) return 1;
        cardIndex=(int)index;start+=2;
    }
    if(RageCardLooksLikeCard(argv[2])) {
        if(!RageCardLoad(argv[2],&card,&report)) goto fail;
        if(cardIndex<0) {
            printf("{\"card\":true,\"entries\":[");
            for(i=0;i<card.count;i++) { if(i)putchar(',');printf("{\"index\":%d,\"name\":",i);
                Json(card.entries[i].name);printf(",\"team\":");Json(card.entries[i].team);putchar('}'); }
            puts("]}");RageCardFree(&card);return writing?1:0;
        }
        if(!RageCardRead(&card,cardIndex,&save)) { RageCardFree(&card);return 1; }
        RageSaveCheck(&save,&report);
    } else if(!RageSaveLoad(argv[2],&save,&report)) goto fail;
    Describe(&save);
    if(writing) {
        if((argc-start)%2) { fprintf(stderr,"Expected key/value pairs.\n");RageCardFree(&card);return 1; }
        for(i=start;i<argc;i+=2) {
            if(!strcmp(argv[i],"team")) {
                if(strlen(argv[i+1])>7 || strspn(argv[i+1],kRageNameCharset)!=strlen(argv[i+1])) {
                    fprintf(stderr,"Team name has unsupported characters or exceeds 7 characters.\n");RageCardFree(&card);return 1; }
                RageSaveWriteTeamName(&save.header,argv[i+1]);
            } else if(!(strncmp(argv[i],"logo.",5)?Edit(argv[i],argv[i+1]):EditLogo(&save.block,argv[i],argv[i+1]))) { fprintf(stderr,"Invalid edit: %s\n",argv[i]);RageCardFree(&card);return 1; }
        }
        if(card.bytes) {
            if(!RageCardWrite(&card,cardIndex,&save) || !RageCardStore(out,&card,&report)) goto fail;
        } else if(!RageSaveStore(out,&save,&report)) goto fail;
    }
    RageSaveReadTeamName(&save.header,team,sizeof(team));
    printf("{\"region\":");
    RageRegion region=RageRegionFromPath(card.bytes?card.entries[cardIndex].name:argv[2]);
    const RageRegionInfo *regionInfo=RageRegionFind(region);
    Json(regionInfo?regionInfo->name:"Unknown");
    printf(",\"team\":");Json(team);printf(",\"charset\":");Json(kRageNameCharset);
    printf(",\"carNames\":{\"international\":[");
    for(i=0;i<GAME_CAR_COUNT;i++){if(i)putchar(',');Json(RageCarName(i,RAGE_REGION_NTSC_U));}
    printf("],\"japanese\":[");
    for(i=0;i<GAME_CAR_COUNT;i++){if(i)putchar(',');Json(RageCarName(i,RAGE_REGION_NTSC_J));}
    printf("]}");
    printf(",\"logo\":{\"pixels\":\"");
    for(i=0;i<4096;i++)putchar("0123456789abcdef"[RageLogoPixel(&save.block,i%64,i/64)]);
    printf("\",\"colors\":[");
    for(i=0;i<16;i++) { unsigned char rgb[3];int transparent;
        RageLogoColour(save.block.teamLogoClut[i],rgb,&transparent);
        if(i)putchar(',');printf("{\"rgb\":\"#%02x%02x%02x\",\"transparent\":%s}",rgb[0],rgb[1],rgb[2],transparent?"true":"false");
    }
    printf("]}");
    printf(",\"checksumsValid\":%s,\"fields\":[",report.headerChecksumValid&&report.blockChecksumValid&&report.trailerChecksumValid&&report.trailerMatchesHeader?"true":"false");
    for(i=0;i<count;i++) { Field *f=&fields[i];if(i)putchar(',');printf("{\"key\":");Json(f->key);
        if(f->text){char value[16];snprintf(value,sizeof(value),"%.*s",f->size,(const char *)f->address);
            printf(",\"value\":");Json(value);printf(",\"text\":true,\"maxLength\":%d}",f->size-1);continue;}
        printf(",\"value\":%lld,\"min\":%lld,\"max\":%lld,\"time\":%s}",Value(f),Minimum(f),Maximum(f),f->time?"true":"false"); }
    puts("]}");RageCardFree(&card);return 0;
fail:
    fprintf(stderr,"%s\n",report.detail);RageCardFree(&card);return 1;
}
