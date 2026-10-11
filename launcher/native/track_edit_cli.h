#ifndef TRACK_EDIT_CLI_H
#define TRACK_EDIT_CLI_H
/* Edits are [object index, field, integer]. Private output is created only
 * after every edit validates; original offsets and all other bytes survive. */
static int TrackEditCommand(const char *source,const char *output) {
    enum { LIMIT=1024*1024 };
    char *input=malloc(LIMIT+1u);void *bytes=NULL;yyjson_doc *doc=NULL;
    int ok=0;size_t size=0;
#ifdef _WIN32
    if(_setmode(_fileno(stdin),_O_BINARY)==-1){free(input);return 1;}
#endif
    if(!input)return 1;
    size_t length=fread(input,1,LIMIT+1u,stdin);
    if(ferror(stdin)||length>LIMIT)goto done;
    doc=yyjson_read(input,length,0);if(!doc)goto done;
    yyjson_val *edits=yyjson_doc_get_root(doc);
    if(!yyjson_is_arr(edits)||!yyjson_arr_size(edits)||yyjson_arr_size(edits)>10000)goto done;
    SDL_PathInfo info;
    if(!SDL_GetPathInfo(source,&info)||info.type!=SDL_PATHTYPE_FILE||info.size>128u*1024u*1024u)goto done;
    bytes=SDL_LoadFile(source,&size);if(!bytes||size>128u*1024u*1024u)goto done;
    TrackBlock blocks[TRACK_PACK_BLOCK_COUNT];Placements placements;
    if(!TrackPackOpen(bytes,size,blocks)||blocks[5].size<4)goto done;
    uint32_t models=SpecU32(blocks[5].data);
    if(!PlacementsOpen(blocks[8].data,blocks[8].size,models,&placements))goto done;
    size_t i,count;yyjson_val *edit;
    yyjson_arr_foreach(edits,i,count,edit){
        if(!yyjson_is_arr(edit)||yyjson_arr_size(edit)!=3)goto done;
        yyjson_val *index=yyjson_arr_get(edit,0),*field=yyjson_arr_get(edit,1),*number=yyjson_arr_get(edit,2);
        if(!yyjson_is_uint(index)||!yyjson_is_str(field)||!yyjson_is_int(number))goto done;
        if(yyjson_is_uint(number)&&yyjson_get_uint(number)>INT32_MAX)goto done;
        int64_t value=yyjson_get_sint(number);
        if(value<INT32_MIN||value>INT32_MAX)goto done;
        const char *name=yyjson_get_str(field);if(strlen(name)!=yyjson_get_len(field))goto done;
        if(yyjson_get_uint(index)>UINT32_MAX)goto done;
        uint32_t at=(uint32_t)yyjson_get_uint(index);
        if(!strcmp(name,"cell.model")){
            if(value<0||value>1023||!TerrainWriteModel((unsigned char *)bytes+
                (blocks[7].data-(unsigned char *)bytes),blocks[7].size,at,(uint16_t)value))goto done;
            continue;
        }
        if(!strncmp(name,"arc.",4)){
            Road road;RoadArc arc;
            if(!RoadOpen(blocks[4].data,blocks[4].size,&road)||!RoadArcRead(&road,at,&arc))goto done;
            if(!strcmp(name+4,"x"))arc.x=(int32_t)value;
            else if(!strcmp(name+4,"z"))arc.z=(int32_t)value;
            else goto done;
            if(!RoadArcWrite((unsigned char *)bytes+(blocks[4].data-(unsigned char *)bytes),blocks[4].size,at,&arc))goto done;
            continue;
        }
        if(!strncmp(name,"road.",5)){
            Road road;RoadPoint point;
            if(!RoadOpen(blocks[4].data,blocks[4].size,&road)||!RoadRead(&road,at,&point))goto done;
            name+=5;
            if(!strcmp(name,"x"))point.x=(int32_t)value;
            else if(!strcmp(name,"z"))point.z=(int32_t)value;
            else {
                if(value<INT16_MIN||value>INT16_MAX)goto done;
                if(!strcmp(name,"y"))point.y=(int16_t)value;
                else if(!strcmp(name,"angle"))point.angle=(int16_t)value;
                else if(!strcmp(name,"pitch"))point.pitch=(int16_t)value;
                else if(!strcmp(name,"crossSlope"))point.crossSlope=(int16_t)value;
                else if(!strcmp(name,"leftWidth"))point.leftWidth=(int16_t)value;
                else if(!strcmp(name,"rightWidth"))point.rightWidth=(int16_t)value;
                else if(!strcmp(name,"length"))point.length=(uint16_t)value;
                else if(!strcmp(name,"arc"))point.arc=(uint16_t)value;
                else goto done;
            }
            if(!RoadWrite((unsigned char *)bytes+(blocks[4].data-(unsigned char *)bytes),blocks[4].size,at,&point))goto done;
            continue;
        }
        Placement p;if(!PlacementRead(&placements,at,&p))goto done;
        if(!strcmp(name,"x"))p.x=(int32_t)value;
        else if(!strcmp(name,"y"))p.y=(int32_t)value;
        else if(!strcmp(name,"z"))p.z=(int32_t)value;
        else if(!strcmp(name,"model")||!strcmp(name,"yaw")){
            if(value<INT16_MIN||value>INT16_MAX)goto done;
            if(!strcmp(name,"model"))p.model=(int16_t)value;else p.yaw=(int16_t)value;
        }else if(!strcmp(name,"flags")){if(value<0)goto done;p.flags=(uint32_t)value;}
        else goto done;
        if(!PlacementWrite((unsigned char *)bytes+(blocks[8].data-(unsigned char *)bytes),blocks[8].size,models,at,&p))goto done;
    }
    ok=ModFileWriteExclusive(output,bytes,size);
done:
    SDL_free(bytes);yyjson_doc_free(doc);free(input);
    if(!ok)fputs("Invalid track object edit or output failure\n",stderr);
    return ok?0:1;
}
#endif
