#ifndef RAGE_MOD_FILE_POLICY_CLI_H
#define RAGE_MOD_FILE_POLICY_CLI_H
#include "render/mod_file_policy.h"
#include "yyjson.h"
static int FilePolicyCommand(int dispositions) {
    enum { LIMIT=8*1024*1024 };
    char *bytes=malloc(LIMIT+1u); yyjson_doc *doc=NULL; int ok=0;
    int *roles=NULL;
    if (!bytes) return 1;
#ifdef _WIN32
    if (_setmode(_fileno(stdin),_O_BINARY)==-1) goto done;
#endif
    size_t size=fread(bytes,1,LIMIT+1u,stdin);
    if (size>LIMIT || ferror(stdin)) goto done;
    doc=yyjson_read(bytes,size,0); if (!doc) goto done;
    yyjson_val *root=yyjson_doc_get_root(doc), *value;
    if (!yyjson_is_arr(root) || yyjson_arr_size(root)>10000) goto done;
    roles=malloc((yyjson_arr_size(root)+1)*sizeof(*roles));
    if (!roles) goto done;
    size_t i,count;
    yyjson_arr_foreach(root,i,count,value) {
        unsigned references=0;
        yyjson_val *pathValue=value;
        if (dispositions==1) {
            if (!yyjson_is_arr(value) || yyjson_arr_size(value)!=2) goto done;
            yyjson_val *flags=yyjson_arr_get(value,1);
            if (!yyjson_is_uint(flags) || yyjson_get_uint(flags)>6) goto done;
            references=(unsigned)yyjson_get_uint(flags);
            pathValue=yyjson_arr_get(value,0);
        }
        if (!yyjson_is_str(pathValue) || strlen(yyjson_get_str(pathValue))!=yyjson_get_len(pathValue)) goto done;
        roles[i]=dispositions==2 ? (ModDirectoryAllowed(yyjson_get_str(pathValue)) ? 0 : -1)
            : ModFileDisposition(yyjson_get_str(pathValue),references);
        if (roles[i]<0) {
            fprintf(stderr,"Unsupported mod file: %s\n",yyjson_get_str(pathValue)); goto done;
        }
    }
    if (dispositions==1) {
        putchar('[');
        for(size_t i=0;i<yyjson_arr_size(root);++i)printf("%s%d",i?",":"",roles[i]);
        puts("]");
    }
    ok=1;
done:
    yyjson_doc_free(doc); free(bytes); free(roles);
    if (!ok) fputs("Invalid mod file inventory\n",stderr);
    return ok?0:1;
}
#endif
