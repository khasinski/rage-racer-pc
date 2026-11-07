#include "online.h"
#include "model.h"
#include "port/analog_pad.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "port/graphics_overlay.h"
#include "port/port_config.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "port/runtime_config.h"
#include "port/main_menu_ui.h"
#include "game/audio.h"
#include "psyz/audio.h"
void PortUpdateForceFeedback(void);
static OverlayLayout layout;
static int replaceSelection;
static void activate(SDL_Window*w,int direction){
    const OnlineView*v=OnlineViewState();if(v->selected<0||v->selected>=v->count)return;
    const OnlineRow*r=&v->rows[v->selected];if(!r->enabled)return;
    OnlineAction action=r->action;int argument=r->argument;
    if(v->editing==(int)action&&r->editable)return;
    if(v->editing){if(action==OA_BACK)OnlineCancelEditing();else OnlineFinishEditing();if(w)SDL_StopTextInput(w);}
    if(action==OA_SETTINGS){PortGraphicsOverlayOpen();return;}
    OnlineActionDo(action,argument,direction);
    if(OnlineViewState()->editing&&w){replaceSelection=1;SDL_StartTextInput(w);}
}
int OnlineEvent(const SDL_Event*e,SDL_Window*w){
    if(!OnlineActive())return 0;const OnlineView*v=OnlineViewState();
    if(e->type==SDL_EVENT_KEY_DOWN){
        SDL_Scancode key=e->key.scancode;
        if(key==SDL_SCANCODE_ESCAPE){if(!e->key.repeat)OnlinePauseMenu();if(w&&!OnlineViewState()->editing)SDL_StopTextInput(w);return 1;}
        if(v->editing){
            if((e->key.mod&SDL_KMOD_CTRL)&&key==SDL_SCANCODE_A){replaceSelection=1;return 1;}
            if((e->key.mod&SDL_KMOD_CTRL)&&key==SDL_SCANCODE_V){char*p=SDL_GetClipboardText();if(p){if(replaceSelection){for(int i=0;i<512;i++)OnlineBackspace();replaceSelection=0;}OnlineText(p);SDL_free(p);}return 1;}
            if(key==SDL_SCANCODE_BACKSPACE){if(replaceSelection){for(int i=0;i<512;i++)OnlineBackspace();replaceSelection=0;}else OnlineBackspace();}
            if(key==SDL_SCANCODE_RETURN||key==SDL_SCANCODE_KP_ENTER){OnlineFinishEditing();if(w)SDL_StopTextInput(w);}return 1;
        }
        if(v->panel){if(key==SDL_SCANCODE_UP)OnlineSelect((v->selected+v->count-1)%v->count);
            else if(key==SDL_SCANCODE_DOWN)OnlineSelect((v->selected+1)%v->count);
            else if(!e->key.repeat&&(key==SDL_SCANCODE_LEFT||key==SDL_SCANCODE_RIGHT))activate(w,key==SDL_SCANCODE_LEFT?-1:1);
            else if(!e->key.repeat&&(key==SDL_SCANCODE_RETURN||key==SDL_SCANCODE_KP_ENTER||key==SDL_SCANCODE_SPACE))activate(w,1);return 1;}
    }else if(e->type==SDL_EVENT_TEXT_INPUT&&v->editing){if(replaceSelection){for(int i=0;i<512;i++)OnlineBackspace();replaceSelection=0;}OnlineText(e->text.text);return 1;}
    else if(e->type==SDL_EVENT_GAMEPAD_BUTTON_DOWN){
        if(e->gbutton.button==SDL_GAMEPAD_BUTTON_START||e->gbutton.button==SDL_GAMEPAD_BUTTON_EAST){OnlinePauseMenu();return 1;}
        if(v->panel){switch(e->gbutton.button){case SDL_GAMEPAD_BUTTON_DPAD_UP:OnlineSelect((v->selected+v->count-1)%v->count);break;case SDL_GAMEPAD_BUTTON_DPAD_DOWN:OnlineSelect((v->selected+1)%v->count);break;case SDL_GAMEPAD_BUTTON_DPAD_LEFT:activate(w,-1);break;case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:activate(w,1);break;case SDL_GAMEPAD_BUTTON_SOUTH:activate(w,1);break;default:break;}return 1;}
    }else if(v->panel&&(e->type==SDL_EVENT_MOUSE_MOTION||e->type==SDL_EVENT_MOUSE_BUTTON_DOWN)){
        float density=w?SDL_GetWindowPixelDensity(w):1;float x=(e->type==SDL_EVENT_MOUSE_MOTION?e->motion.x:e->button.x)*density;
        float y=(e->type==SDL_EVENT_MOUSE_MOTION?e->motion.y:e->button.y)*density;
        if(layout.scale<=0)return 1;
        int row=(int)floorf(((y-layout.y)/layout.scale-128)/46);
        if(x>=layout.x&&x<layout.x+layout.w&&row>=0&&row<v->count){OnlineSelect(row);if(e->type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e->button.button==SDL_BUTTON_LEFT)activate(w,1);}return 1;
    }
    return v->panel;
}
int OnlinePaint(OverlaySurface*s,int width,int height,OverlayLayout*out){
    const OnlineView*v=OnlineViewState();if(!v->active||width<=0||height<=0)return 0;
    if((OnlineViewJoining(v)||OnlineViewStarting(v))&&!v->panel){float scale=fminf((float)height/820.0f,(float)width/520.0f);if(scale>2)scale=2;layout.scale=scale;layout.w=(int)(440*scale);layout.h=(int)(208*scale);layout.x=(width-layout.w)/2;layout.y=(height-layout.h)/2;}
    else if(v->racing&&!v->panel){float scale=fminf((float)height/820.0f,2.0f);if(scale<0.5f)scale=0.5f;layout.scale=scale;layout.w=(int)(320*scale);layout.h=(int)(132*scale);layout.x=20;layout.y=20;}
    else OverlayComputeLayout(v->count+((v->screen==OS_ROOM||v->screen==OS_ARCADE)?2:0),width,height,&layout);
    if(!OverlaySurfaceResize(s,layout.w,layout.h))return 0;OverlayPaintOnline(s,v,layout.scale);*out=layout;return 1;
}

#include "port/input_config.h"
void OnlineSampleInput(void){
    static int wasRacing;
    int racing=OnlineRaceActive();
    if(racing&&!wasRacing)Psyz_PcmMusicStop();
    if(!racing&&wasRacing){Psyz_PcmMusicPlay(1);PortUpdateForceFeedback();}
    wasRacing=racing;
    if(!OnlineActive())return;
    OnlineSetCarLightsEnabled(PortActiveConfig()->carLights);
    RageInputConfig config;InputConfigDefaults(&config);InputConfigApplyRuntime(&config);
    const bool*keys=SDL_GetKeyboardState(NULL);unsigned held=0;
    for(unsigned i=0;i<16;i++){SDL_Scancode key=SDL_GetScancodeFromName(config.keys[i]);if(key!=SDL_SCANCODE_UNKNOWN&&keys[key])held|=1u<<i;}
    /* Fresh keyboard packet, then the existing calibrated wheel/pad layer. */
    memset(g_PadBuffers,0,8);g_PadBuffers[1]=0x41;g_PadBuffers[2]=(uint8_t)~(held>>8);g_PadBuffers[3]=(uint8_t)~held;
    PortSampleAnalogPad();UpdatePadState();
    int32_t words[8]={0};int base=g_PadType==PAD_TYPE_NEGCON?8:0;
    words[0]=base?2:1;words[1]=(g_PadHeld&PAD_LEFT)!=0;words[2]=(g_PadHeld&PAD_RIGHT)!=0;
    if(base){int range=GetNegconSteerRange();words[3]=range?g_NegconSteer*(13*512)/range:0;if(words[3]>13*512)words[3]=13*512;if(words[3]<-13*512)words[3]=-13*512;words[1]=words[2]=0;}
    words[4]=(g_PadHeld&g_PadButtonMapping[base+2])?256:0;words[5]=(g_PadHeld&g_PadButtonMapping[base+3])?256:0;
    if(base){switch(g_NegconMappingIndex){case 0:case 5:words[4]=g_NegconAnalogI*256/106;words[5]=g_NegconAnalogII*256/106;break;case 1:case 6:words[4]=g_NegconAnalogII*256/106;words[5]=g_NegconAnalogI*256/106;break;case 2:words[5]=g_NegconAnalogL*256/106;break;case 3:words[4]=g_NegconAnalogII*256/106;words[5]=g_NegconAnalogL*256/106;break;default:break;}}
    for(int i=4;i<=5;i++){if(words[i]<0)words[i]=0;if(words[i]>256)words[i]=256;}
    words[6]=(g_PadPressed&g_PadButtonMapping[base+4])!=0;words[7]=(g_PadPressed&g_PadButtonMapping[base+5])!=0;
    OnlineSetDriverInput(g_PadHeld,words);
    OnlineEffectsVolume((float)ClampAudioSetting(g_SfxVolumeSetting)/(float)AUDIO_SETTING_MAX);
    static uint64_t feedbackAt;
    uint64_t now=SDL_GetTicks();if(now-feedbackAt>=20){PortUpdateForceFeedback();feedbackAt=now;}
}

void OnlineSmokeTick(void){
    const char *mode=RuntimeConfigGet("hooks.online");
    if(!mode)return;
    static uint64_t start;static int opened,requested,ready,started;
    if(!opened&&PortMainMenuState().interactive){
        opened=1;start=SDL_GetTicks();PortMainMenuRequestOnline();
        if(strcmp(mode,"profile"))OnlineActionDo(!strncmp(mode,"arcade",6)?OA_ARCADE:OA_GUEST,0,0);
    }
    if(!opened)return;
    const OnlineView *v=OnlineViewState();
    const char *captures=RuntimeConfigGet("capture.online_lifecycle");static unsigned capturedPhases;
    if(captures){
        unsigned phase=OnlineViewStarting(v)&&v->takeoverSeconds<=2?1u:v->finishing&&v->hud[0]==3?2u:v->screen==OS_RESULTS?4u:0;
        if(phase&&!(capturedPhases&phase)){
            OverlaySurface surface={0};OverlayLayout l;char path[4096];
            snprintf(path,sizeof(path),"%s-%s.ppm",captures,phase==1?"start":phase==2?"finish":"results");
            if(OnlinePaint(&surface,1280,960,&l)){OverlaySurfaceDump(&surface,path);capturedPhases|=phase;}OverlaySurfaceRelease(&surface);
        }
    }
    if(!strcmp(mode,"arcade-lobby")&&v->racing&&v->localSeat>=0){OnlineActionDo(OA_ARCADE_LOBBY,0,0);}
    if(v->screen==OS_ROOMS&&!requested){
        if(!strcmp(mode,"duel-code")){OnlineActionDo(OA_DUEL_CODE,0,0);requested=1;}
        if(!strcmp(mode,"host")){OnlineActionDo(OA_QUICK_ROOM,0,0);requested=1;}
        else if(!strcmp(mode,"join"))for(int i=0;i<v->count;i++)if(v->rows[i].action==OA_JOIN&&v->rows[i].enabled&&strstr(v->rows[i].value,"Join lobby")){OnlineActionDo(OA_JOIN,v->rows[i].argument,0);requested=1;break;}
    }
    if(v->screen==OS_ROOM){
        if(!ready)for(int i=0;i<v->count;i++)if(v->rows[i].action==OA_READY&&v->rows[i].enabled){OnlineActionDo(OA_READY,0,0);ready=1;break;}
        if(!started&&!strcmp(mode,"host")){
            int players=0;for(int i=0;i<v->count;i++)if(v->rows[i].action==OA_PLAYERS)players=atoi(v->rows[i].value);
            for(int i=0;i<v->count;i++)if(players>=2&&v->rows[i].action==OA_START&&v->rows[i].enabled){OnlineActionDo(OA_START,0,0);started=1;}
        }
    }
    if(v->racing){int32_t input[8]={1,0,0,0,256,0,0,0};OnlineSetDriverInput(0,input);}
    const char *panel=RuntimeConfigGet("capture.online_panel");static int panelCaptured;
    if(panel&&!panelCaptured&&SDL_GetTicks()-start>(!strcmp(mode,"arcade-lobby")?7000u:2000u)){OverlaySurface surface={0};OverlayLayout l;
        if(OnlinePaint(&surface,1280,960,&l)){OverlaySurfaceDump(&surface,panel);panelCaptured=1;}OverlaySurfaceRelease(&surface);
    }
    if(SDL_GetTicks()-start>(uint64_t)RuntimeConfigInt("hooks.online_seconds",30,1,600)*1000){
        fprintf(stderr,"online smoke: screen=%d tick=%d speed=%d status=%s\n",v->screen,v->hud[11],v->hud[7],v->status);
        SDL_Event event;SDL_zero(event);event.type=SDL_EVENT_QUIT;SDL_PushEvent(&event);
    }
}
