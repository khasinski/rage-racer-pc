#ifndef RAGE_MOD_PROFILE_CLI_H
#define RAGE_MOD_PROFILE_CLI_H
#include "yyjson.h"

/* Serialize only the composed tables, never package identities/dependencies.
 * Validate the complete document before exclusively creating private output. */
static int ProfileCommand(const char *output) {
    enum { LIMIT=2*1024*1024 };
    char *input=malloc(LIMIT+1u), *text=malloc(LIMIT+1u);
    yyjson_doc *doc=NULL;
    int ok=0;
    if(!input||!text)goto done;
#ifdef _WIN32
    if(_setmode(_fileno(stdin),_O_BINARY)==-1)goto done;
#endif
    size_t size=fread(input,1,LIMIT+1u,stdin);
    if(size>LIMIT||ferror(stdin))goto done;
    doc=yyjson_read(input,size,0);
    if(!doc)goto done;
    yyjson_val *root=yyjson_doc_get_root(doc);
    if(!yyjson_is_obj(root)||yyjson_obj_size(root)!=3)goto done;
    const char *groups[]={"textures","materials","meshes"};
    size_t used=(size_t)snprintf(text,LIMIT+1u,"[mod]\nid = \"launcher-profile\"\n");
    for(size_t group=0;group<3;group++){
        yyjson_val *table=yyjson_obj_get(root,groups[group]);
        if(!yyjson_is_obj(table))goto done;
        int n=snprintf(text+used,LIMIT+1u-used,"\n[%s]\n",groups[group]);
        if(n<0||(size_t)n>LIMIT-used)goto done;
        used+=(size_t)n;
        size_t i,count;yyjson_val *key,*value;
        yyjson_obj_foreach(table,i,count,key,value){
            if(!yyjson_is_str(value))goto done;
            const char *k=yyjson_get_str(key), *v=yyjson_get_str(value);
            if(strlen(k)!=yyjson_get_len(key)||strlen(v)!=yyjson_get_len(value))goto done;
            /* These fields are identifiers, paths and material tokens; no
             * escaping/control characters are part of their native grammar. */
            for(const unsigned char *p=(const unsigned char *)k;*p;p++)
                if(*p<32||*p=='"'||*p=='\\')goto done;
            for(const unsigned char *p=(const unsigned char *)v;*p;p++)
                if(*p<32||*p=='"'||*p=='\\')goto done;
            n=snprintf(text+used,LIMIT+1u-used,"\"%s\" = \"%s\"\n",k,v);
            if(n<0||(size_t)n>LIMIT-used)goto done;
            used+=(size_t)n;
        }
    }
    if(!ModManifestParse(text,used,&manifest))goto done;
    FILE *file=fopen(output,"wbx");
    if(!file)goto done;
    ok=fwrite(text,1,used,file)==used;
    if(fclose(file))ok=0;
    if(!ok)remove(output);
done:
    yyjson_doc_free(doc);free(input);free(text);
    if(!ok)fputs("Invalid mod manifest or profile output failure\n",stderr);
    return ok?0:1;
}
#endif
