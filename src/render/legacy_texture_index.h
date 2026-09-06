#ifndef RAGE_LEGACY_TEXTURE_INDEX_H
#define RAGE_LEGACY_TEXTURE_INDEX_H
#include <stddef.h>
#include <string.h>
static int LegacyIndexSpace(unsigned char c) {
    return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\v'||c=='\f';
}
/* One bounded line: 0 blank/comment, 1 entry, -1 malformed. No I/O.
 * Names are relative JSON paths under textures/, never executable paths. */
static int LegacyTextureIndexLine(const char *line,size_t size,int *owner,char stem[256]) {
    size_t i=0,start,n,part;
    unsigned value=0;
    if(!line||!owner||!stem||size>510)return -1;
    *owner=0;stem[0]=0;
    while(i<size&&LegacyIndexSpace((unsigned char)line[i]))++i;
    if(i==size||line[i]=='#')return 0;
    start=i;
    while(i<size&&line[i]>='0'&&line[i]<='9') {
        value=value*10+(unsigned)(line[i++]-'0');
        if(value>=135)return -1;
    }
    if(i==start||i==size||!LegacyIndexSpace((unsigned char)line[i]))return -1;
    while(i<size&&LegacyIndexSpace((unsigned char)line[i]))++i;
    start=i;
    while(i<size&&!LegacyIndexSpace((unsigned char)line[i]))++i;
    n=i-start;
    if(n<6||n>255||memcmp(line+start+n-5,".json",5))return -1;
    while(i<size&&LegacyIndexSpace((unsigned char)line[i]))++i;
    if(i!=size)return -1;
    part=0;
    for(i=0;i<=n;++i) {
        unsigned char c=i<n?(unsigned char)line[start+i]:'/';
        if(c=='/') {
            size_t count=i-part;
            if(!count||(count==1&&line[start+part]=='.')||
               (count==2&&!memcmp(line+start+part,"..",2)))return -1;
            part=i+1;
        } else if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||
                    c=='_'||c=='.'||c=='-'))return -1;
    }
    memcpy(stem,line+start,n);stem[n]=0;*owner=(int)value;return 1;
}
#endif
