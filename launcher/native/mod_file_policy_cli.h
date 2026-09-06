#ifndef RAGE_MOD_FILE_POLICY_CLI_H
#define RAGE_MOD_FILE_POLICY_CLI_H
#include "render/mod_file_policy.h"
#include "yyjson.h"
static int FilePolicyCommand(void) {
    enum { LIMIT=8*1024*1024 };
    char *bytes=malloc(LIMIT+1u); yyjson_doc *doc=NULL; int ok=0;
    if (!bytes) return 1;
#ifdef _WIN32
    if (_setmode(_fileno(stdin),_O_BINARY)==-1) goto done;
#endif
    size_t size=fread(bytes,1,LIMIT+1u,stdin);
    if (size>LIMIT || ferror(stdin)) goto done;
    doc=yyjson_read(bytes,size,0); if (!doc) goto done;
    yyjson_val *root=yyjson_doc_get_root(doc), *value;
    if (!yyjson_is_arr(root) || yyjson_arr_size(root)>10000) goto done;
    size_t i,count;
    yyjson_arr_foreach(root,i,count,value) {
        if (!yyjson_is_str(value) || strlen(yyjson_get_str(value))!=yyjson_get_len(value)) goto done;
        if (ModFileClassify(yyjson_get_str(value))==RAGE_MOD_FILE_INVALID) {
            fprintf(stderr,"Unsupported mod file: %s\n",yyjson_get_str(value)); goto done;
        }
    }
    ok=1;
done:
    yyjson_doc_free(doc); free(bytes);
    if (!ok) fputs("Invalid mod file inventory\n",stderr);
    return ok?0:1;
}
#endif
