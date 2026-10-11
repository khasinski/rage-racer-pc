#ifndef TEXTURE_BATCH_CLI_H
#define TEXTURE_BATCH_CLI_H
/* One bounded upload list, many texture pages. Keep VRAM private to this
 * process and decode texture windows by the same sampling rule as runtime. */
static int TextureBatchCommand(const char *directory){
    enum { LIMIT=1024*1024 };int result=1;
    char *input=malloc(LIMIT+1u),**args=NULL;yyjson_doc *doc=NULL;
    if(!input)return 1;
#ifdef _WIN32
    if(_setmode(_fileno(stdin),_O_BINARY)==-1)goto done;
#endif
    size_t size=fread(input,1,LIMIT+1u,stdin);
    if(ferror(stdin)||size>LIMIT)goto done;
    doc=yyjson_read(input,size,0);if(!doc)goto done;
    yyjson_val *root=yyjson_doc_get_root(doc),*uploads=yyjson_obj_get(root,"uploads"),*textures=yyjson_obj_get(root,"textures");
    if(!yyjson_is_arr(uploads)||!yyjson_arr_size(uploads)||yyjson_arr_size(uploads)>1024||!yyjson_is_arr(textures)||!yyjson_arr_size(textures)||yyjson_arr_size(textures)>4096)goto done;
    size_t argc=5+6*yyjson_arr_size(uploads);args=calloc(argc,sizeof(*args));if(!args)goto done;
    char numbers[1024][5][32];size_t i,count;yyjson_val *entry;
    yyjson_arr_foreach(uploads,i,count,entry){
        if(!yyjson_is_arr(entry)||yyjson_arr_size(entry)!=6)goto done;
        yyjson_val *file=yyjson_arr_get(entry,0);
        if(!yyjson_is_str(file)||yyjson_get_len(file)>2048||strlen(yyjson_get_str(file))!=yyjson_get_len(file))goto done;
        args[5+i*6]=(char *)yyjson_get_str(file);
        for(unsigned n=1;n<6;n++){
            yyjson_val *value=yyjson_arr_get(entry,n);
            if(!yyjson_is_int(value))goto done;
            int64_t v=yyjson_get_sint(value);
            if((yyjson_is_uint(value)&&yyjson_get_uint(value)>INT32_MAX)||v<INT32_MIN||v>INT32_MAX)goto done;
            snprintf(numbers[i][n-1],32,"%lld",(long long)v);args[5+i*6+n]=numbers[i][n-1];
        }
    }
    /* Validate every descriptor before touching output. */
    yyjson_arr_foreach(textures,i,count,entry){
        if(!yyjson_is_arr(entry)||yyjson_arr_size(entry)!=6)goto done;
        for(unsigned n=0;n<6;n++){
            yyjson_val *value=yyjson_arr_get(entry,n);
            if(!yyjson_is_uint(value)||yyjson_get_uint(value)>(n<2?65535u:256u))goto done;
        }
        unsigned w=(unsigned)yyjson_get_uint(yyjson_arr_get(entry,2)),h=(unsigned)yyjson_get_uint(yyjson_arr_get(entry,3)),x=(unsigned)yyjson_get_uint(yyjson_arr_get(entry,4)),y=(unsigned)yyjson_get_uint(yyjson_arr_get(entry,5));
        if((!w)!=(!h)||x+w>256||y+h>256||(!w&&(x||y)))goto done;
        char file[4096];int n=snprintf(file,sizeof(file),"%s/texture-%zu.png",directory,i);SDL_PathInfo info;
        if(n<0||(size_t)n>=sizeof(file)||SDL_GetPathInfo(file,&info))goto done;
    }
    yyjson_arr_foreach(textures,i,count,entry){
        char page[16],clut[16],file[4096];
        snprintf(page,sizeof(page),"%u",(unsigned)yyjson_get_uint(yyjson_arr_get(entry,0)));
        snprintf(clut,sizeof(clut),"%u",(unsigned)yyjson_get_uint(yyjson_arr_get(entry,1)));
        snprintf(file,sizeof(file),"%s/texture-%zu.png",directory,i);
        args[0]="rage-mod-cli";args[1]="--texture";args[2]=page;args[3]=clut;args[4]=file;
        if(TexturePreviewWindow(i?5:(int)argc,args,(unsigned)yyjson_get_uint(yyjson_arr_get(entry,2)),(unsigned)yyjson_get_uint(yyjson_arr_get(entry,3)),(unsigned)yyjson_get_uint(yyjson_arr_get(entry,4)),(unsigned)yyjson_get_uint(yyjson_arr_get(entry,5))))goto done;
    }
    puts("{\"decoded\":true}");result=0;
done:
    if(result)fputs("Invalid texture batch or decode failure\n",stderr);
    free(args);yyjson_doc_free(doc);free(input);return result;
}
#endif
