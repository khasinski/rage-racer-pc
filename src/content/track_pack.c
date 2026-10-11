#include "track_pack.h"
#include <stdint.h>
#include <string.h>

static uint32_t Read32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int TrackPackOpen(const void *data, size_t size,
                  TrackBlock blocks[TRACK_PACK_BLOCK_COUNT]) {
    const unsigned char *bytes = data;
    TrackBlock decoded[TRACK_PACK_BLOCK_COUNT];
    unsigned i;
    if (!bytes || !blocks || size < TRACK_PACK_BLOCK_COUNT * 4 ||
        size > INT32_MAX) return 0;
    for (i = 0; i < TRACK_PACK_BLOCK_COUNT; i++) {
        size_t start = Read32(bytes + i * 4);
        size_t end = i + 1 < TRACK_PACK_BLOCK_COUNT
                         ? Read32(bytes + (i + 1) * 4) : size;
        if (start < TRACK_PACK_BLOCK_COUNT * 4 || end <= start || end > size)
            return 0;
        decoded[i].data = bytes + start;
        decoded[i].size = end - start;
    }
    memcpy(blocks, decoded, sizeof(decoded));
    return 1;
}

const char *TrackBlockName(unsigned index) {
    static const char *const names[TRACK_PACK_BLOCK_COUNT] = {
        "render-table", "environment-palette", "environment-timeline",
        "rivals-and-scenery", "road", "course-models", "secondary-models",
        "terrain", "object-placements", "race-events", "cameras"
    };
    return index < TRACK_PACK_BLOCK_COUNT ? names[index] : NULL;
}

static uint16_t Read16(const unsigned char *p) {
    return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}

int PlacementRead(const Placements *placements, uint32_t index, Placement *out) {
    if (!placements || !out || !placements->entries || index >= placements->count) return 0;
    const unsigned char *p=placements->entries+(size_t)index*20;
    Placement value={(int16_t)Read16(p),(int16_t)Read16(p+2),
        (int32_t)Read32(p+4),(int32_t)Read32(p+8),(int32_t)Read32(p+12),Read32(p+16)};
    *out=value;return 1;
}
static int PlacementValid(const Placement *p,uint32_t models) {
    return (p->model==-1 || (p->model>=0 && (uint32_t)p->model<models)) && !(p->flags & ~15u);
}
int PlacementsOpen(const void *data,size_t size,uint32_t models,Placements *out) {
    if(!data||!out||size<4||size>INT32_MAX)return 0;
    const unsigned char *bytes=data;
    Placements value={bytes+4,Read32(bytes)};
    if(value.count>(size-4)/20)return 0;
    for(uint32_t i=0;i<value.count;i++){
        Placement p;PlacementRead(&value,i,&p);
        if(!PlacementValid(&p,models))return 0;
    }
    *out=value;return 1;
}
static void Store32(unsigned char *p,uint32_t value) {
    for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(value>>(i*8));
}
int PlacementWrite(void *data,size_t size,uint32_t models,uint32_t index,const Placement *value) {
    Placements placements;
    if(!value||!PlacementsOpen(data,size,models,&placements)||index>=placements.count||!PlacementValid(value,models))return 0;
    Placement copy=*value;
    unsigned char *p=(unsigned char *)data+4+(size_t)index*20;
    uint16_t model=(uint16_t)copy.model,yaw=(uint16_t)copy.yaw;
    p[0]=(unsigned char)model;p[1]=(unsigned char)(model>>8);
    p[2]=(unsigned char)yaw;p[3]=(unsigned char)(yaw>>8);
    Store32(p+4,(uint32_t)copy.x);Store32(p+8,(uint32_t)copy.y);Store32(p+12,(uint32_t)copy.z);Store32(p+16,copy.flags);
    return 1;
}

int RoadRead(const Road *road, uint32_t index, RoadPoint *point) {
    const unsigned char *p;
    if (!road || !point || !road->points || index >= road->count) return 0;
    p = road->points + (size_t)index * 24;
    point->x = (int32_t)Read32(p);point->z = (int32_t)Read32(p+4);
    point->y = (int16_t)Read16(p+8);point->angle = (int16_t)Read16(p+10);
    point->pitch = (int16_t)Read16(p+12);point->crossSlope = (int16_t)Read16(p+14);
    point->leftWidth = (int16_t)Read16(p+16);point->rightWidth = (int16_t)Read16(p+18);
    point->arc = Read16(p+20);point->length = Read16(p+22);
    return 1;
}

int RoadOpen(const void *data, size_t size, Road *road) {
    const unsigned char *bytes = data;
    Road decoded;
    uint32_t i;
    if (!bytes || !road || size < 4 || size > INT32_MAX) return 0;
    decoded.count = Read32(bytes);
    if (!decoded.count || decoded.count > (size-4)/24) return 0;
    decoded.points = bytes+4;
    decoded.arcs = decoded.points+(size_t)decoded.count*24;
    decoded.arcCount = (uint32_t)((size-4-(size_t)decoded.count*24)/12);
    decoded.length = 0;
    for (i=0;i<decoded.count;i++) {
        RoadPoint point;
        unsigned mode;
        int arcIndex;
        RoadRead(&decoded,i,&point);
        mode=point.arc & 3;
        arcIndex=(int16_t)point.arc >> 4;
        if (!point.length || point.length > INT16_MAX || mode > 2 ||
            decoded.length > ((uint32_t)INT16_MAX << 8)-1-point.length ||
            (mode && (arcIndex < 0 || (uint32_t)arcIndex >= decoded.arcCount))) return 0;
        decoded.length += point.length;
    }
    *road=decoded;
    return 1;
}

int RoadWrite(void *data, size_t size, uint32_t index, const RoadPoint *point) {
    Road road;
    RoadPoint prior;
    if (!point || !RoadOpen(data,size,&road) || !RoadRead(&road,index,&prior)) return 0;
    RoadPoint value=*point;
    unsigned mode=value.arc&3;
    int arcIndex=(int16_t)value.arc>>4;
    if (!value.length || value.length>INT16_MAX || mode>2 ||
        road.length-prior.length>((uint32_t)INT16_MAX<<8)-1-value.length ||
        (mode && (arcIndex<0 || (uint32_t)arcIndex>=road.arcCount))) return 0;
    unsigned char *p=(unsigned char *)data+4+(size_t)index*24;
    Store32(p,(uint32_t)value.x);Store32(p+4,(uint32_t)value.z);
    const uint16_t fields[]={(uint16_t)value.y,(uint16_t)value.angle,
        (uint16_t)value.pitch,(uint16_t)value.crossSlope,(uint16_t)value.leftWidth,
        (uint16_t)value.rightWidth,value.arc,value.length};
    for(unsigned i=0;i<8;i++){p[8+i*2]=(unsigned char)fields[i];p[9+i*2]=(unsigned char)(fields[i]>>8);}
    return 1;
}

int RoadArcRead(const Road *road,uint32_t index,RoadArc *arc) {
    if(!road||!arc||!road->arcs||index>=road->arcCount)return 0;
    const unsigned char *p=road->arcs+(size_t)index*12;
    RoadArc value={(int32_t)Read32(p),(int32_t)Read32(p+4)};
    *arc=value;return 1;
}
int RoadArcWrite(void *data,size_t size,uint32_t index,const RoadArc *arc) {
    Road road;
    if(!arc||!RoadOpen(data,size,&road)||index>=road.arcCount)return 0;
    RoadArc value=*arc;
    unsigned char *p=(unsigned char *)data+(road.arcs-(const unsigned char *)data)+(size_t)index*12;
    Store32(p,(uint32_t)value.x);Store32(p+4,(uint32_t)value.z);return 1;
}
