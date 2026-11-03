#include "hd_pack.h"
#include "modern/modern_image.h"
#include "render/asset_path.h"
#include "content/sha256.h"
#include "yyjson.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

enum { MAX_FILES=1026, MAX_FILE=64*1024*1024, MAX_TOTAL=1024*1024*1024 };
static const char magic[8]={'R','R','H','D','0','0','1','\n'};
typedef struct Record { char name[256],sha[65];Uint32 bytes; } Record;
static int fail(char *error,size_t capacity,const char *format,...){va_list a;va_start(a,format);if(error&&capacity)vsnprintf(error,capacity,format,a);va_end(a);return 0;}
static int join(char *out,size_t n,const char *root,const char *name){int k=snprintf(out,n,"%s/%s",root,name);return k>=0&&(size_t)k<n;}
static int hex(const char *s){if(strlen(s)!=64)return 0;for(int i=0;i<64;i++)if(!strchr("0123456789abcdef",s[i]))return 0;return 1;}
static int pathOK(const char *name){
 if(!strcmp(name,"pack.json")||!strcmp(name,"mod.toml"))return 1;
 size_t n=strlen(name);if(n<14||n>=256||strncmp(name,"textures/",9)||(strcmp(name+n-4,".png")&&(n<5||strcmp(name+n-5,".webp")))||strchr(name+9,'/'))return 0;
 if(!AssetPathIsRelativeFile(name,n))return 0;
 for(size_t i=9;i<n;i++)if(!strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.",name[i]))return 0;
 return 1;
}
static int hashIO(const char *path,char out[65]){
 SDL_IOStream *f=SDL_IOFromFile(path,"rb");if(!f)return 0;Sha256 hash;Sha256Init(&hash);char buffer[65536];size_t n;
 while((n=SDL_ReadIO(f,buffer,sizeof(buffer))))Sha256Update(&hash,buffer,n);
 int ok=SDL_GetIOStatus(f)==SDL_IO_STATUS_EOF;SDL_CloseIO(f);Sha256Finish(&hash,out);return ok;
}
static int readRecord(SDL_IOStream *f,Record *r){
 Uint16 n;if(!SDL_ReadU16LE(f,&n)||!SDL_ReadU32LE(f,&r->bytes)||!n||n>=sizeof(r->name)||!r->bytes||r->bytes>MAX_FILE||SDL_ReadIO(f,r->sha,64)!=64||SDL_ReadIO(f,r->name,n)!=n)return 0;
 r->name[n]=0;r->sha[64]=0;return strlen(r->name)==n&&pathOK(r->name)&&hex(r->sha);
}
static int writeRecord(SDL_IOStream *f,const Record *r){size_t n=strlen(r->name);return SDL_WriteU16LE(f,(Uint16)n)&&SDL_WriteU32LE(f,r->bytes)&&SDL_WriteIO(f,r->sha,64)==64&&SDL_WriteIO(f,r->name,n)==n;}
static int readHeader(SDL_IOStream *f,Uint32 *count){char bytes[8];return SDL_ReadIO(f,bytes,8)==8&&!memcmp(bytes,magic,8)&&SDL_ReadU32LE(f,count)&&*count>=2&&*count<=MAX_FILES;}
static int writeHeader(SDL_IOStream *f,Uint32 count){return SDL_WriteIO(f,magic,8)==8&&SDL_WriteU32LE(f,count);}
static int metadata(const char *directory,HdPackState *s){
 char path[1600];size_t n=0;if(!join(path,sizeof(path),directory,"pack.json"))return 0;
 char *data=SDL_LoadFile(path,&n);if(!data)return 0;yyjson_doc *doc=n<=16384?yyjson_read(data,n,0):NULL;SDL_free(data);if(!doc)return 0;
 yyjson_val *root=yyjson_doc_get_root(doc);const char *keys[]={"name","version","region"};char *dest[]={s->name,s->version,s->region};size_t caps[]={sizeof(s->name),sizeof(s->version),sizeof(s->region)};int ok=yyjson_is_obj(root)&&yyjson_get_int(yyjson_obj_get(root,"format"))==1;
 for(int i=0;ok&&i<3;i++){yyjson_val *v=yyjson_obj_get(root,keys[i]);size_t len=yyjson_get_len(v);const char *text=yyjson_get_str(v);if(!text||!len||len>=caps[i]||strlen(text)!=len){ok=0;break;}for(size_t j=0;j<len;j++)if((unsigned char)text[j]<32)ok=0;if(ok)memcpy(dest[i],text,len+1);}
 if(ok&&strcmp(s->region,"PAL")&&strcmp(s->region,"NTSC-J")&&strcmp(s->region,"NTSC-U"))ok=0;
 yyjson_doc_free(doc);return ok;
}
RageModManifest *HdPackReadManifest(const char *directory){
 char path[1600];size_t size;if(!join(path,sizeof(path),directory,"mod.toml"))return NULL;
 char *text=SDL_LoadFile(path,&size);if(!text)return NULL;RageModManifest *m=calloc(1,sizeof(*m));
 if(!m||size>RAGE_MOD_MANIFEST_MAX_BYTES||!ModManifestParse(text,size,m)||m->meshCount||m->audioCount||m->requirementCount||!m->textureCount){free(m);m=NULL;}
 if(m)for(size_t i=0;i<m->textureCount;i++)if(strncmp(m->textures[i].path,"textures/",9)||!pathOK(m->textures[i].path)){free(m);m=NULL;break;}
 SDL_free(text);return m;
}
static int references(const char *directory,const Record *files,Uint32 count){
 RageModManifest *m=HdPackReadManifest(directory);HdPackState state={0};if(!m||!metadata(directory,&state)){free(m);return 0;}
 int ok=1;for(size_t i=0;i<m->textureCount;i++){int found=0;for(Uint32 j=0;j<count;j++)if(!strcmp(m->textures[i].path,files[j].name))found=1;if(!found){ok=0;break;}}
 for(Uint32 j=0;ok&&j<count;j++)if(!strncmp(files[j].name,"textures/",9)){int found=0;for(size_t i=0;i<m->textureCount;i++)if(!strcmp(m->textures[i].path,files[j].name))found=1;if(!found)ok=0;}
 free(m);return ok;
}
static int saveState(const char *store,const HdPackState *s,int enabled){
 char path[1600],temp[1600],name[96],bytes[160];snprintf(name,sizeof(name),"active-%llu.partial",(unsigned long long)SDL_GetPerformanceCounter());
 if(!hex(s->hash)||!hex(s->inventoryHash)||!join(path,sizeof(path),store,"active.txt")||!join(temp,sizeof(temp),store,name))return 0;
 int n=snprintf(bytes,sizeof(bytes),"%d\n%s\n%s\n",enabled!=0,s->hash,s->inventoryHash);
 int ok=SDL_SaveFile(temp,bytes,(size_t)n)&&SDL_RenamePath(temp,path);if(!ok)SDL_RemovePath(temp);return ok;
}
int HdPackReadState(const char *store,const char *region,HdPackState *s){
 memset(s,0,sizeof(*s));char path[1600];size_t n;if(!join(path,sizeof(path),store,"active.txt"))return 0;
 char *data=SDL_LoadFile(path,&n);if(!data)return 0;
 int ok=n==132&&(data[0]=='0'||data[0]=='1')&&data[1]=='\n'&&data[66]=='\n'&&data[131]=='\n';
 if(ok){memcpy(s->hash,data+2,64);memcpy(s->inventoryHash,data+67,64);s->enabled=data[0]=='1';ok=hex(s->hash)&&hex(s->inventoryHash)&&join(s->directory,sizeof(s->directory),store,s->hash)&&metadata(s->directory,s)&&(!region||!*region||!strcmp(region,s->region));}
 SDL_free(data);if(!ok)memset(s,0,sizeof(*s));return ok;
}
int HdPackSetEnabled(const char *store,const HdPackState *s,int enabled){return saveState(store,s,enabled);}
static void clean(const char *directory,const Record *files,Uint32 count){char p[1800];for(Uint32 i=0;i<count;i++)if(join(p,sizeof(p),directory,files[i].name))SDL_RemovePath(p);if(join(p,sizeof(p),directory,"integrity.bin"))SDL_RemovePath(p);if(join(p,sizeof(p),directory,"textures"))SDL_RemovePath(p);SDL_RemovePath(directory);}
int HdPackVerify(const char *store,const HdPackState *s,HdPackProgress progress,void *context,char *error,size_t cap){
 (void)store;char path[1800],actual[65];Record *files=NULL;SDL_IOStream *f=NULL;Uint32 count=0;int ok=0;
 if(!join(path,sizeof(path),s->directory,"integrity.bin")||!hashIO(path,actual)||strcmp(actual,s->inventoryHash))return fail(error,cap,"Pack inventory checksum mismatch.");
 f=SDL_IOFromFile(path,"rb");if(!f||!readHeader(f,&count))goto done;files=calloc(count,sizeof(*files));if(!files)goto done;
 for(Uint32 i=0;i<count;i++){if(!readRecord(f,&files[i]))goto done;for(Uint32 j=0;j<i;j++)if(!strcmp(files[i].name,files[j].name))goto done;if(!join(path,sizeof(path),s->directory,files[i].name)||!hashIO(path,actual)||strcmp(actual,files[i].sha))goto done;if(progress&&!progress(context,(int)((i+1)*100/count)))goto done;}
 {char tail;if(SDL_ReadIO(f,&tail,1)!=0||!references(s->directory,files,count))goto done;}ok=1;
 done:if(f)SDL_CloseIO(f);free(files);if(!ok)fail(error,cap,"Pack verification failed or cancelled.");return ok;
}
int HdPackCreate(const char *source,const char *output,char *error,size_t cap){
 RageModManifest *m=HdPackReadManifest(source);HdPackState state={0};if(!m||!metadata(source,&state)){free(m);return fail(error,cap,"Invalid HD pack folder or metadata.");}
 Record *files=calloc(MAX_FILES,sizeof(*files));SDL_IOStream *f=NULL;Uint32 count=2;int ok=0;char path[1800];SDL_PathInfo info;
 if(!files)goto done;strcpy(files[0].name,"pack.json");strcpy(files[1].name,"mod.toml");
 for(size_t i=0;i<m->textureCount;i++){int found=0;for(Uint32 j=2;j<count;j++)if(!strcmp(files[j].name,m->textures[i].path))found=1;if(!found){if(count==MAX_FILES)goto done;SDL_strlcpy(files[count++].name,m->textures[i].path,256);}}
 if(SDL_GetPathInfo(output,&info))goto done;
 f=SDL_IOFromFile(output,"wb");if(!f||!writeHeader(f,count))goto done;Uint64 total=0;
 for(Uint32 i=0;i<count;i++){
  size_t n;if(!join(path,sizeof(path),source,files[i].name)||!SDL_GetPathInfo(path,&info)||info.type!=SDL_PATHTYPE_FILE||!info.size||info.size>MAX_FILE)goto done;
  total+=info.size;if(total>MAX_TOTAL)goto done;
  void *bytes=SDL_LoadFile(path,&n);if(!bytes||n!=info.size){SDL_free(bytes);goto done;}files[i].bytes=(Uint32)n;Sha256 h;Sha256Init(&h);Sha256Update(&h,bytes,n);Sha256Finish(&h,files[i].sha);
  int wrote=writeRecord(f,&files[i])&&SDL_WriteIO(f,bytes,n)==n;SDL_free(bytes);if(!wrote)goto done;
 }
 ok=1;
 done:if(f&&!SDL_CloseIO(f))ok=0;if(!ok&&f)SDL_RemovePath(output);free(files);free(m);if(!ok)fail(error,cap,"Could not create HD pack (existing output is preserved).");return ok;
}
int HdPackInstall(const char *archive,const char *store,const char *region,HdPackProgress progress,void *context,char *error,size_t cap){
 char archiveHash[65],stage[1600]={0},final[1600],path[1800],tempName[100];SDL_IOStream *f=NULL,*index=NULL;Record *files=NULL;Uint32 count=0,created=0;int ok=0,ownedStage=0;HdPackState state={0};
 SDL_PathInfo archiveInfo;if(!SDL_GetPathInfo(archive,&archiveInfo)||archiveInfo.size>MAX_TOTAL+MAX_FILES*326ull)return fail(error,cap,"HD pack exceeds size limit.");
 if(!hashIO(archive,archiveHash))return fail(error,cap,"Cannot read local HD pack.");
 f=SDL_IOFromFile(archive,"rb");if(!f||!readHeader(f,&count))goto done;files=calloc(count,sizeof(*files));if(!files)goto done;
 snprintf(tempName,sizeof(tempName),"install-%llu.partial",(unsigned long long)SDL_GetPerformanceCounter());SDL_PathInfo info;
 if(!SDL_CreateDirectory(store)||!join(stage,sizeof(stage),store,tempName)||SDL_GetPathInfo(stage,&info)||!SDL_CreateDirectory(stage) )goto done;
 ownedStage=1;
 if(!join(path,sizeof(path),stage,"textures")||!SDL_CreateDirectory(path)||!join(path,sizeof(path),stage,"integrity.bin"))goto done;
 index=SDL_IOFromFile(path,"wb");if(!index||!writeHeader(index,count))goto done;Uint64 total=0;
 for(Uint32 i=0;i<count;i++){
  if(!readRecord(f,&files[i]))goto done;for(Uint32 j=0;j<i;j++)if(!strcmp(files[i].name,files[j].name))goto done;
  if((i==0&&strcmp(files[i].name,"pack.json"))||(i==1&&strcmp(files[i].name,"mod.toml")))goto done;
  total+=files[i].bytes;if(total>MAX_TOTAL)goto done;
  void *bytes=SDL_malloc(files[i].bytes);if(!bytes)goto done;int valid=SDL_ReadIO(f,bytes,files[i].bytes)==files[i].bytes;
  Sha256 h;char actual[65];Sha256Init(&h);Sha256Update(&h,bytes,valid?files[i].bytes:0);Sha256Finish(&h,actual);valid=valid&&!strcmp(actual,files[i].sha);
  if(valid&&!strncmp(files[i].name,"textures/",9)){
   SDL_Surface *image=ModernLoadImage(SDL_IOFromConstMem(bytes,files[i].bytes),4096);
   valid=image!=NULL;SDL_DestroySurface(image);
  }
  if(valid){valid=join(path,sizeof(path),stage,files[i].name)&&SDL_SaveFile(path,bytes,files[i].bytes);created=i+1;}
  SDL_free(bytes);if(!valid||!writeRecord(index,&files[i])||(progress&&!progress(context,(int)((i+1)*95/count))))goto done;
 }
 {char tail;if(SDL_ReadIO(f,&tail,1)!=0||!references(stage,files,count)||!metadata(stage,&state)||(region&&*region&&strcmp(region,state.region)))goto done;}
 if(!SDL_CloseIO(index)){index=NULL;goto done;}index=NULL;
 if(!join(path,sizeof(path),stage,"integrity.bin")||!hashIO(path,state.inventoryHash)||(progress&&!progress(context,99))||!join(final,sizeof(final),store,archiveHash))goto done;
 if(SDL_GetPathInfo(final,&info)){
  HdPackState old=state;SDL_strlcpy(old.directory,final,sizeof(old.directory));if(!HdPackVerify(store,&old,NULL,NULL,error,cap))goto done;
  clean(stage,files,created);stage[0]=0;
 }else {if(!SDL_RenamePath(stage,final))goto done;stage[0]=0;}
 SDL_strlcpy(state.hash,archiveHash,sizeof(state.hash));state.enabled=1;
 if(!saveState(store,&state,1))goto done;
 if(progress)progress(context,100);ok=1;
 done:if(index)SDL_CloseIO(index);if(f)SDL_CloseIO(f);if(ownedStage&&*stage)clean(stage,files,created);free(files);if(!ok)fail(error,cap,"Install failed/cancelled: invalid files, region or checksum. Previous pack kept.");return ok;
}
