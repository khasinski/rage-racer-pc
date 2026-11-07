#include "main_menu_ui.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/scene.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
static Frontend s_frontend;
s32 g_SceneId;
s16 g_ExtraGrandPrixUnlocked;
static int s_quits;
const char *HostDiscImagePath(void){return "";}
const char *RuntimeConfigGet(const char *key){(void)key;return NULL;}
void OnlineSetDisc(const char *path){(void)path;}
void OnlineSetServer(const char *url){(void)url;}
void OnlineOpen(void){}

Frontend *MenuFrontend(void) { return &s_frontend; }
bool SDL_PushEvent(SDL_Event *event) { if(event->type==SDL_EVENT_QUIT)s_quits++; return true; }
#define CHECK(x) do { if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;} } while(0)
int main(void) {
    const int dimensions[][2]={{320,240},{960,720},{1280,720},{2560,720},{1920,1080}};
    for(unsigned d=0;d<sizeof(dimensions)/sizeof(dimensions[0]);d++){
        int w=dimensions[d][0],h=dimensions[d][1];
        for(int i=0;i<TITLE_MENU_ITEM_COUNT;i++){
            MainMenuRect r=MainMenuRow(w,h,i,1);
            CHECK(r.y>=0&&r.y+r.h<h);
            CHECK(MainMenuHit(w,h,r.x+r.w/2,r.y+r.h/2,1)==i);
            MainMenuRect locked=MainMenuRow(w,h,i,0);
            if(i==TITLE_MENU_EXTRA_GRAND_PRIX) CHECK(locked.h==0);
            else CHECK(MainMenuHit(w,h,locked.x+locked.w/2,locked.y+locked.h/2,0)==i);
        }
        CHECK(MainMenuHit(w,h,0,0,1)==-1);
    }
    int current=0;
    for(int i=1;i<=TITLE_MENU_ITEM_COUNT;i++){
        current=MoveTitleMenuSelection(current,1,1);
        CHECK(current==i%TITLE_MENU_ITEM_COUNT);
    }
    CHECK(MoveTitleMenuSelection(TITLE_MENU_OPTIONS,1,0)==TITLE_MENU_QUIT);
    CHECK(MoveTitleMenuSelection(TITLE_MENU_GRAND_PRIX,-1,0)==TITLE_MENU_QUIT);
    g_SceneId=GAME_SCENE_FRONTEND;g_ExtraGrandPrixUnlocked=1;
    s_frontend.state=FRONTEND_STATE_MENU_INPUT;
    MainMenuRect r=MainMenuRow(1280,720,TITLE_MENU_QUIT,1);
    PortMainMenuPointer(1280,720,r.x+r.w/2,r.y+r.h/2,1);
    int selection=-1;
    CHECK(PortMainMenuTakeInput(&selection)==3&&selection==TITLE_MENU_QUIT);
    CHECK(PortMainMenuTakeInput(&selection)==0);
    PortMainMenuEdge(0);CHECK(PortMainMenuTakeInput(&selection)==1&&selection==0);
    PortMainMenuEdge(1);CHECK(PortMainMenuTakeInput(&selection)==1&&selection==TITLE_MENU_QUIT);
    s_frontend.state=FRONTEND_STATE_MENU_EXIT;
    PortMainMenuPointer(1280,720,r.x+r.w/2,r.y+r.h/2,1);
    CHECK(PortMainMenuTakeInput(&selection)==0);
    PortMainMenuRequestQuit();CHECK(s_quits==1);
    OverlaySurface surface={0};MainMenuState state={1,1,TITLE_MENU_QUIT,1,1};
    CHECK(MainMenuPaint(&surface,1280,720,&state));
    CHECK(surface.pixels[3]==0); /* background logo remains visible */
    r=MainMenuRow(1280,720,TITLE_MENU_QUIT,1);
    unsigned char *p=surface.pixels+(((int)r.y+5)*surface.w+(int)r.x+5)*4;
    CHECK(p[0]>p[1]*2&&p[3]==255);
    CHECK(!strcmp(MainMenuLabel(TITLE_MENU_LOAD_SAVE),"LOAD GAME"));
    CHECK(!strcmp(MainMenuLabel(TITLE_MENU_OPTIONS),"SETTINGS"));
    CHECK(!strcmp(MainMenuLabel(TITLE_MENU_QUIT),"QUIT GAME"));
    state.extraUnlocked=0;
    CHECK(MainMenuPaint(&surface,1280,720,&state));
    /* The removed eighth row is transparent; Quit moved up into row seven. */
    p=surface.pixels+(((int)r.y+5)*surface.w+(int)r.x+5)*4;
    CHECK(p[3]==0);
    r=MainMenuRow(1280,720,TITLE_MENU_QUIT,0);
    p=surface.pixels+(((int)r.y+5)*surface.w+(int)r.x+5)*4;
    CHECK(p[0]>p[1]*2&&p[3]==255);
    OverlaySurfaceRelease(&surface);
    puts("Native menu: all rows, pointer routing, navigation, quit and text rendering passed");
    return 0;
}
