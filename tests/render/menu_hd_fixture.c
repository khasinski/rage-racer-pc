#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv) {
    if(argc!=3)return 1;
    if(!strcmp(argv[1],"write")||!strcmp(argv[1],"write-caption")||!strcmp(argv[1],"write-atlas")||!strcmp(argv[1],"write-options")||!strcmp(argv[1],"write-sky")||!strcmp(argv[1],"write-transparent-sky")) {
        int transparent=!strcmp(argv[1],"write-transparent-sky");
        int atlas=!strcmp(argv[1],"write-atlas")||!strcmp(argv[1],"write-sky")||transparent;
        int options=!strcmp(argv[1],"write-options");
        int caption=!strcmp(argv[1],"write-caption");
        SDL_Surface *surface=SDL_CreateSurface(caption?960:options?240:atlas?1024:448,caption||options?96:atlas?1024:64,SDL_PIXELFORMAT_RGBA32);
        if(!surface)return 2;
        for(int y=0;y<surface->h;y++)for(int x=0;x<surface->w;x++) {
            uint8_t *p=(uint8_t *)surface->pixels+y*surface->pitch+x*4;
            p[0]=(x&1)?255:0;p[1]=(x&1)?0:255;p[2]=0;p[3]=255;
            if(transparent)memset(p,0,4);
        }
        int ok=SDL_SavePNG(surface,argv[2]);
        SDL_DestroySurface(surface);
        return ok?0:3;
    }
    int region=!strcmp(argv[1],"check-options-region");
    int options=!strcmp(argv[1],"check-options-atlas")||region;
    int atlas=!strcmp(argv[1],"check-atlas")||options;
    if(strcmp(argv[1],"check")&&!atlas)return 4;
    FILE *file=fopen(argv[2],"rb");
    int width,height,max;
    if(!file)return 5;
    if(fscanf(file,"P6 %d %d %d",&width,&height,&max)!=3||
       width!=1280||height!=960||max!=255||fgetc(file)!='\n') {
        fclose(file);return 6;
    }
    unsigned char row[1280*3];
    if (!strcmp(argv[1], "check")) {
        /* The native menu owns its text now. Retired ui.title overrides must
         * neither replace its labels nor suppress the selected row. */
        if (fseek(file, 280L*width*3, SEEK_CUR) || fread(row,1,sizeof(row),file)!=sizeof(row)) {
            fclose(file); return 7;
        }
        fclose(file);
        int ink=0, selected=0;
        for (int x=400; x<880; ++x) {
            const unsigned char *p=row+x*3;
            ink += p[0]<20 && p[1]<20 && p[2]<20;
            selected += p[0]>100 && p[0]>p[1]*3/2 && p[1]>20;
        }
        if (ink<50 || selected<150) {
            fprintf(stderr,"Native menu label/selection missing: ink=%d selected=%d\n",ink,selected);
            return 8;
        }
        return 0;
    }
    /* The first label starts at logical (104,100). Each HD texel must
     * occupy one output pixel, rather than a four-pixel averaged block. */
    if(fseek(file,(options?310L:atlas?750L:410L)*width*3,SEEK_CUR)||fread(row,1,sizeof(row),file)!=sizeof(row)) {
        fclose(file);return 7;
    }
    unsigned char other[1280*3];
    if(region){if(fseek(file,(378L-311L)*width*3,SEEK_CUR)||fread(other,1,sizeof(other),file)!=sizeof(other)){fclose(file);return 7;}}
    fclose(file);
    int blended=0,opaque=0;
    for(int x=options?144:160;x<(options?384:1120);x++) {
        const unsigned char *p=row+x*3;
        if(p[(x&1)?0:1]<(atlas?(options?100:90):240)||(options&&!(x&1)?0:p[(x&1)?1:0])>(options?32:15)||p[2]>(options?20:15)) {
            fprintf(stderr,"HD menu detail lost at x=%d: %u,%u,%u\n",x,p[0],p[1],p[2]);
            return 8;
        }
        if(options&&(x&1)){if(p[1]>10&&p[2]>5)blended++;if(p[1]<3&&p[2]<3)opaque++;}
    }
    if(options&&!blended){fprintf(stderr,"Original additive STP not preserved: blended=%d opaque=%d\n",blended,opaque);return 9;}
    if(region){int stripes=0;for(int x=144;x<200;x++){unsigned char*p=other+x*3;if((x&1)?p[0]>240&&p[1]<40:p[1]>240&&p[2]<20)stripes++;}if(stripes>40){fprintf(stderr,"Uncovered UI region was replaced\n");return 10;}}
    return 0;
}
