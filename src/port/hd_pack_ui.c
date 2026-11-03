/* File dialog and verification run asynchronously; only the presentation thread
 * changes the HD layer. Native meshes, disc imports and game state stay alive. */
#include "hd_pack_ui.h"
#include "hd_pack.h"
#include "hd_download.h"
#include "menu_images.h"
#include "overlay_model.h"
#include "platform_paths.h"
#include "runtime_config.h"
#include "host_disc.h"
#include "modern/modern_assets.h"
#include "modern/modern_renderer.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct {
 int initialized,have,dialog,job,success,downloader;
 SDL_AtomicInt dialogDone,done,percent,cancel,phase,bytes,total;
 SDL_Thread *thread;
 HdPackState state;
 char store[1400],file[1800],region[16],message[160],server[900];
} s;
static int progress(void *context,int percent){(void)context;SDL_SetAtomicInt(&s.percent,percent);return !SDL_GetAtomicInt(&s.cancel);}
static int downloadProgress(void *context,const char *stage,uint64_t done,uint64_t total){
 (void)context;
 int phase=!strcmp(stage,"Downloading")?1:!strcmp(stage,"Installing and verifying")?2:!strcmp(stage,"Verifying installed pack")?3:0;
 SDL_SetAtomicInt(&s.bytes,(int)done);SDL_SetAtomicInt(&s.total,(int)total);
 SDL_SetAtomicInt(&s.percent,total?(int)(done*100/total):0);SDL_SetAtomicInt(&s.phase,phase);
 return !SDL_GetAtomicInt(&s.cancel);
}
static int SDLCALL work
(void *context){
 (void)context;
 if(s.job==4)s.success=HdDownloadInstall(s.server,s.store,s.region,downloadProgress,NULL,s.message,sizeof(s.message));
 else s.success=s.job==1?HdPackInstall(s.file,s.store,s.region,progress,NULL,s.message,sizeof(s.message)):
 HdPackVerify(s.store,&s.state,progress,NULL,s.message,sizeof(s.message));
 if(!s.success&&SDL_GetAtomicInt(&s.cancel))SDL_strlcpy(s.message,"Cancelled. Previous pack kept.",sizeof(s.message));
 SDL_SetAtomicInt(&s.done,1);return 0;
}
static void SDLCALL chosen(void *context,const char *const *files,int filter){
 (void)context;(void)filter;
 s.file[0]=0;if(files&&files[0])SDL_strlcpy(s.file,files[0],sizeof(s.file));
 SDL_SetAtomicInt(&s.dialogDone,1);
}
static void shutdownWorker(void){SDL_SetAtomicInt(&s.cancel,1);if(s.thread){SDL_WaitThread(s.thread,NULL);s.thread=NULL;}}
static int apply(void){
 RageModManifest *m=s.have&&s.state.enabled?HdPackReadManifest(s.state.directory):NULL;
 if(s.have&&s.state.enabled&&!m)return 0;
 int ok=MenuImagesSetHdPack(m?s.state.directory:NULL,m);
 if(ok){ok=ModernAssetsSetHdPack(m?s.state.directory:NULL,m);}free(m);
 if(ok)ok=ModernRestartPresentation(PortActiveConfig());
 return ok;
}
static void start(int job){
 s.job=job;SDL_SetAtomicInt(&s.phase,0);SDL_SetAtomicInt(&s.cancel,0);SDL_SetAtomicInt(&s.done,0);SDL_SetAtomicInt(&s.percent,0);
 s.thread=SDL_CreateThread(work,"HD pack",NULL);
 if(!s.thread){s.job=0;SDL_strlcpy(s.message,"Could not start HD pack task.",sizeof(s.message));}
}
void HdPackUiTick(OverlayModel *model){
 if(!s.initialized){
  s.initialized=1;atexit(shutdownWorker);
  const char *server=RuntimeConfigGet("hd.server");
  SDL_strlcpy(s.server,server&&*server?server:"https://rage-hd.polanka.ovh",sizeof(s.server));
  const char *store=RuntimeConfigGet("hd.store");
  if(store)SDL_strlcpy(s.store,store,sizeof(s.store));else PlatformUserConfigPath("hd-packs",s.store,sizeof(s.store));
  const char *region=HostDiscRegion();SDL_strlcpy(s.region,region?region:"",sizeof(s.region));
  s.have=HdPackReadState(s.store,s.region,&s.state);
  if(s.have&&s.state.enabled){if(HdPackVerify(s.store,&s.state,NULL,NULL,s.message,sizeof(s.message)))apply();else s.state.enabled=0;}
 }
 if(s.dialog&&SDL_GetAtomicInt(&s.dialogDone)){
  s.dialog=0;if(*s.file)start(1);
 }
 if(s.thread&&SDL_GetAtomicInt(&s.done)){
  SDL_WaitThread(s.thread,NULL);s.thread=NULL;
  if(s.success){
   if(s.job==3){
    if(HdPackSetEnabled(s.store,&s.state,1)){
     s.state.enabled=1;
     SDL_strlcpy(s.message,apply()?"HD textures enabled.":"HD textures enabled in settings; could not refresh textures.",sizeof(s.message));
    }else SDL_strlcpy(s.message,"Could not save HD texture setting.",sizeof(s.message));
   }
   else if(s.job==1||s.job==4){s.have=HdPackReadState(s.store,s.region,&s.state);if(!apply())SDL_strlcpy(s.message,"Pack installed; could not activate textures.",sizeof(s.message));else snprintf(s.message,sizeof(s.message),s.success==2?"HD pack %.64s is up to date and verified.":"HD pack %.64s installed and applied.",s.state.version);}
   else SDL_strlcpy(s.message,"All installed files verified.",sizeof(s.message));
  }
  if(s.job==4)s.downloader=s.success?2:3;
  s.job=0;model->dirty=1;
 }
 int action=model->packAction;model->packAction=0;
 if(action==4&&s.thread&&s.job==4&&SDL_GetAtomicInt(&s.phase)==1)SDL_SetAtomicInt(&s.cancel,1);
 if(action==6&&!s.thread)s.downloader=0;
 if(action&&!s.thread&&!s.dialog){
  if(action==1){
   if(s.have){
    if(!s.state.enabled)start(3);
    else if(HdPackSetEnabled(s.store,&s.state,0)){
     s.state.enabled=0;
     SDL_strlcpy(s.message,apply()?"HD textures disabled.":"HD textures disabled in settings; could not refresh textures.",sizeof(s.message));
    }else SDL_strlcpy(s.message,"Could not save HD texture setting.",sizeof(s.message));
   }
   else SDL_strlcpy(s.message,"Download an HD pack or choose a local file first.",sizeof(s.message));
  }else if(action==2){
   static const SDL_DialogFileFilter filter={"Rage Racer HD pack","rrhd"};
   s.dialog=1;SDL_SetAtomicInt(&s.dialogDone,0);SDL_ShowOpenFileDialog(chosen,NULL,NULL,&filter,1,NULL,false);
  }else if(action==5){model->packDownloadedBytes=model->packTotalBytes=0;s.downloader=1;start(4);if(!s.thread)s.downloader=3;
  }else if(action==3){if(s.have)start(2);else SDL_strlcpy(s.message,"No installed HD pack.",sizeof(s.message));}
  model->dirty=1;
 }
 int phase=SDL_GetAtomicInt(&s.phase),percent=SDL_GetAtomicInt(&s.percent);
 int canCancel=s.job==4&&s.thread&&phase==1;
 unsigned done=phase==1?(unsigned)SDL_GetAtomicInt(&s.bytes):model->packDownloadedBytes;
 unsigned total=phase==1?(unsigned)SDL_GetAtomicInt(&s.total):model->packTotalBytes;
 const char *phaseName=s.thread?(phase==1?"Downloading textures":phase==2?"Installing and verifying":phase==3?"Verifying installed pack":"Checking for updates"):
   s.downloader==2?"Complete":SDL_GetAtomicInt(&s.cancel)?"Cancelled":"Could not complete download";
 if(model->packDownloader!=s.downloader||model->packPercent!=percent||model->packCanCancel!=canCancel||model->packDownloadedBytes!=done||model->packTotalBytes!=total||strcmp(model->packPhase,phaseName))model->dirty=1;
 model->packDownloader=s.downloader;model->packPercent=percent;model->packCanCancel=canCancel;
 model->packDownloadedBytes=done;model->packTotalBytes=total;SDL_strlcpy(model->packPhase,phaseName,sizeof(model->packPhase));
 if(model->values.hdBusy!=(s.thread!=NULL)){model->values.hdBusy=s.thread!=NULL;model->dirty=1;}
 model->values.hdEnabled=s.have&&s.state.enabled;
 char status[160];if(s.thread){
  if(s.job==4){
   int phase=SDL_GetAtomicInt(&s.phase);
   if(phase==1)snprintf(status,sizeof(status),"Downloading: %.1f / %.1f MB (%d%%)",SDL_GetAtomicInt(&s.bytes)/1000000.0,SDL_GetAtomicInt(&s.total)/1000000.0,SDL_GetAtomicInt(&s.percent));
   else snprintf(status,sizeof(status),"%s: %d%%",phase==2?"Installing and verifying":phase==3?"Verifying installed pack":"Checking for updates",SDL_GetAtomicInt(&s.percent));
  }else snprintf(status,sizeof(status),"%s: %d%%",s.job==1?"Installing":"Verifying",SDL_GetAtomicInt(&s.percent));
 }
else if(s.dialog)SDL_strlcpy(status,"Choose a local .rrhd file...",sizeof(status));else if(*s.message)SDL_strlcpy(status,s.message,sizeof(status));else if(s.have)snprintf(status,sizeof(status),"%.100s %.30s",s.state.name,s.state.version);else SDL_strlcpy(status,"No HD pack installed.",sizeof(status));
 if(strcmp(model->packStatus,status)){SDL_strlcpy(model->packStatus,status,sizeof(model->packStatus));if(model->page==OVERLAY_PAGE_MODS)model->dirty=1;}
}
