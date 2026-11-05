#include "port/hd_download.h"
#include <stdio.h>
int main(int argc,char **argv){
    if(argc!=4){fprintf(stderr,"Usage: rage-hd-fetch SERVER_ORIGIN STORE REGION\n");return 2;}
    char error[256]={0};int result=HdDownloadInstall(argv[1],argv[2],argv[3],NULL,NULL,error,sizeof(error));
    fprintf(result?stdout:stderr,"%s\n",error);return result?0:1;
}
