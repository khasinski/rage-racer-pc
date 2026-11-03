#include "main_menu_ui.h"
#include "third_party/stb_truetype.h"
#include "fonts/archivo_black.ttf.inc"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static stbtt_fontinfo s_font;
static int s_ready;

static void Pixel(OverlaySurface *s, int x, int y, int r, int g, int b, int a) {
    if (x < 0 || y < 0 || x >= s->w || y >= s->h - 1) return;
    unsigned char *p = s->pixels + ((size_t)y * s->w + x) * 4;
    p[0] = (unsigned char)((p[0] * (255 - a) + r * a) / 255);
    p[1] = (unsigned char)((p[1] * (255 - a) + g * a) / 255);
    p[2] = (unsigned char)((p[2] * (255 - a) + b * a) / 255);
    p[3] = (unsigned char)(a + p[3] * (255 - a) / 255);
}
static void Text(OverlaySurface *s, MainMenuRect row, const char *text, int disabled) {
    float scale = stbtt_ScaleForPixelHeight(&s_font, row.h * 0.77f);
    float advance = 0;
    int ascent, descent, gap;
    stbtt_GetFontVMetrics(&s_font, &ascent, &descent, &gap);
    for (const char *c = text; *c; c++) { int a, l; stbtt_GetCodepointHMetrics(&s_font,*c,&a,&l); advance += a * scale; }
    if (advance > row.w * 0.90f) scale *= row.w * 0.90f / advance;
    advance = 0;
    for (const char *c = text; *c; c++) { int a, l; stbtt_GetCodepointHMetrics(&s_font,*c,&a,&l); advance += a * scale; }
    float pen = row.x + (row.w - advance) * 0.5f;
    int baseline = (int)(row.y + (row.h - (ascent - descent) * scale) * 0.5f + ascent * scale);
    for (const char *c = text; *c; c++) {
        int w,h,x0,y0,a,l;
        unsigned char *glyph = stbtt_GetCodepointBitmap(&s_font,scale,scale,*c,&w,&h,&x0,&y0);
        if (glyph) {
            for(int y=0;y<h;y++)for(int x=0;x<w;x++){
                int px=(int)pen+x0+x+(int)((h-y)*0.12f),py=baseline+y0+y;
                int alpha=glyph[y*w+x];
                Pixel(s,px+1,py+1,0,0,0,alpha/3);
                Pixel(s,px,py,disabled?66:8,disabled?67:9,disabled?64:9,alpha);
            }
            stbtt_FreeBitmap(glyph,NULL);
        }
        stbtt_GetCodepointHMetrics(&s_font,*c,&a,&l);pen+=a*scale;
    }
}
int MainMenuPaint(OverlaySurface *surface, int width, int height, const MainMenuState *state) {
    if (!s_ready) {
        if (!stbtt_InitFont(&s_font,archivo_black_ttf,stbtt_GetFontOffsetForIndex(archivo_black_ttf,0))) return 0;
        s_ready=1;
    }
    if (!OverlaySurfaceResize(surface,width,height)) return 0;
    memset(surface->pixels,0,(size_t)surface->w*surface->h*4);
    for(int i=0;i<TITLE_MENU_ITEM_COUNT;i++){
        if(i==TITLE_MENU_EXTRA_GRAND_PRIX&&!state->extraUnlocked) continue;
        MainMenuRect row=MainMenuRow(width,height,i,state->extraUnlocked);
        int selected=i==state->selected;
        int disabled=i==TITLE_MENU_EXTRA_GRAND_PRIX&&!state->extraUnlocked;
        int border=(int)fmaxf(1.0f,height/360.0f);
        int x0=(int)row.x,y0=(int)row.y,x1=(int)(row.x+row.w),y1=(int)(row.y+row.h);
        for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){
            int r=selected?202:136,g=selected?76:138,b=selected?67:134;
            if(disabled){r=92;g=94;b=91;}
            if(x-x0<border||y-y0<border){r=selected?240:184;g=selected?139:184;b=selected?123:180;}
            if(x1-x<=border||y1-y<=border){r=selected?121:66;g=selected?41:67;b=selected?33:63;}
            Pixel(surface,x,y,r,g,b,255);
        }
        Text(surface,row,MainMenuLabel(i),disabled);
    }
    if(state->opacity<1){for(size_t i=0;i<(size_t)surface->w*surface->h*4;i++)surface->pixels[i]=(unsigned char)(surface->pixels[i]*state->opacity);}
    return 1;
}
