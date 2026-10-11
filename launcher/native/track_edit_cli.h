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
        if(!yyjson_is_uint(index)||yyjson_get_uint(index)>=placements.count||!yyjson_is_str(field)||!yyjson_is_int(number))goto done;
        if(yyjson_is_uint(number)&&yyjson_get_uint(number)>INT32_MAX)goto done;
        int64_t value=yyjson_get_sint(number);
        if(value<INT32_MIN||value>INT32_MAX)goto done;
        const char *name=yyjson_get_str(field);if(strlen(name)!=yyjson_get_len(field))goto done;
        Placement p;uint32_t at=(uint32_t)yyjson_get_uint(index);PlacementRead(&placements,at,&p);
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
