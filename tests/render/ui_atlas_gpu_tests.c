#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "overlay_vert_spv.h"
#include "overlay_vert_msl.h"
#include "ui_atlas_frag_spv.h"
#include "ui_atlas_frag_msl.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s: %s\n",__LINE__,#x,SDL_GetError());return 1;}}while(0)
static SDL_GPUTexture *texture(SDL_GPUDevice*d,unsigned w,unsigned h,SDL_GPUTextureUsageFlags usage){SDL_GPUTextureCreateInfo i={.type=SDL_GPU_TEXTURETYPE_2D,.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,.width=w,.height=h,.layer_count_or_depth=1,.num_levels=1,.usage=usage};return SDL_CreateGPUTexture(d,&i);}
static int upload(SDL_GPUDevice*d,SDL_GPUTexture*t,const void*p,unsigned w,unsigned h){SDL_GPUTransferBufferCreateInfo i={.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,.size=w*h*4};SDL_GPUTransferBuffer*b=SDL_CreateGPUTransferBuffer(d,&i);if(!b)return 0;void*q=SDL_MapGPUTransferBuffer(d,b,false);if(!q)return 0;memcpy(q,p,w*h*4);SDL_UnmapGPUTransferBuffer(d,b);SDL_GPUCommandBuffer*c=SDL_AcquireGPUCommandBuffer(d);if(!c)return 0;SDL_GPUCopyPass*copy=SDL_BeginGPUCopyPass(c);SDL_GPUTextureTransferInfo from={.transfer_buffer=b,.pixels_per_row=w,.rows_per_layer=h};SDL_GPUTextureRegion to={.texture=t,.w=w,.h=h,.d=1};SDL_UploadToGPUTexture(copy,&from,&to,false);SDL_EndGPUCopyPass(copy);int ok=SDL_SubmitGPUCommandBuffer(c);SDL_ReleaseGPUTransferBuffer(d,b);return ok;}
static void setWord(unsigned char*p,unsigned word){p[0]=(unsigned char)((word&31)*255/31);p[1]=(unsigned char)(((word>>5)&31)*255/31);p[2]=(unsigned char)(((word>>10)&31)*255/31);p[3]=(word&0x8000)?255:0;}
int main(void){
 if(!SDL_Init(SDL_INIT_VIDEO))return 77;
 SDL_GPUDevice*d=SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_MSL,false,NULL);if(!d){SDL_Quit();return 77;}
 int spirv=(SDL_GetGPUShaderFormats(d)&SDL_GPU_SHADERFORMAT_SPIRV)!=0;
 SDL_GPUShaderCreateInfo shader={.format=spirv?SDL_GPU_SHADERFORMAT_SPIRV:SDL_GPU_SHADERFORMAT_MSL,.stage=SDL_GPU_SHADERSTAGE_VERTEX,.code=spirv?overlay_vert_spv:overlay_vert_msl,.code_size=spirv?overlay_vert_spv_len:overlay_vert_msl_len,.entrypoint=spirv?"main":"vs_overlay",.num_uniform_buffers=1};
 SDL_GPUShader*vs=SDL_CreateGPUShader(d,&shader);CHECK(vs);
 shader.stage=SDL_GPU_SHADERSTAGE_FRAGMENT;shader.code=spirv?ui_atlas_frag_spv:ui_atlas_frag_msl;shader.code_size=spirv?ui_atlas_frag_spv_len:ui_atlas_frag_msl_len;shader.entrypoint=spirv?"main":"fs_ui_atlas";shader.num_samplers=2;
 SDL_GPUShader*fs=SDL_CreateGPUShader(d,&shader);CHECK(fs);
 SDL_GPUColorTargetDescription desc={.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,.blend_state={.enable_blend=true,.src_color_blendfactor=SDL_GPU_BLENDFACTOR_ONE,.dst_color_blendfactor=SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,.color_blend_op=SDL_GPU_BLENDOP_ADD,.src_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ONE,.dst_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,.alpha_blend_op=SDL_GPU_BLENDOP_ADD}};
 SDL_GPUGraphicsPipelineCreateInfo pi={.vertex_shader=vs,.fragment_shader=fs,.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,.target_info={.num_color_targets=1,.color_target_descriptions=&desc}};
 SDL_GPUGraphicsPipeline*pipe=SDL_CreateGPUGraphicsPipeline(d,&pi);CHECK(pipe);
 SDL_GPUTexture*atlas=texture(d,256,256,SDL_GPU_TEXTUREUSAGE_SAMPLER),*vram=texture(d,1024,512,SDL_GPU_TEXTUREUSAGE_SAMPLER),*target=texture(d,8,1,SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);CHECK(atlas&&vram&&target);
 unsigned char*memory=calloc(1024*512,4),*image=calloc(256*256,4);CHECK(memory&&image);
 // Page 31, first row: packed colour indices 0..7. Palette entries alternate
 // opaque/STP. RGB of the original is irrelevant: new art supplies its colour.
 setWord(memory+(256*1024+960)*4,0x3210);setWord(memory+(256*1024+961)*4,0x7654);
 for(unsigned i=0;i<8;i++)setWord(memory+i*4,0x4210|((i&1)?0x8000:0));CHECK(upload(d,vram,memory,1024,512));free(memory);
 SDL_GPUSamplerCreateInfo si={0};SDL_GPUSampler*sampler=SDL_CreateGPUSampler(d,&si);CHECK(sampler);
 SDL_GPUTransferBufferCreateInfo bi={.size=32,.usage=SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD};SDL_GPUTransferBuffer*download=SDL_CreateGPUTransferBuffer(d,&bi);CHECK(download);
 const int abrModes[]={0,1,3};const float background[]={80,100,120};
 for(int coverage=0;coverage<2;coverage++){
  int alpha=coverage?128:255;for(int i=0;i<256*256;i++){image[i*4]=(unsigned char)(coverage?32:64);image[i*4+1]=(unsigned char)(coverage?16:32);image[i*4+2]=(unsigned char)(coverage?8:16);image[i*4+3]=(unsigned char)alpha;}CHECK(upload(d,atlas,image,256,256));
  for(int localRegion=0;localRegion<2;localRegion++)for(int reflected=0;reflected<2;reflected++)for(int semi=0;semi<2;semi++)for(int mode=0;mode<3;mode++){
   unsigned abr=(unsigned)abrModes[mode];SDL_GPUCommandBuffer*c=SDL_AcquireGPUCommandBuffer(d);CHECK(c);
   SDL_GPUColorTargetInfo color={.texture=target,.load_op=SDL_GPU_LOADOP_CLEAR,.store_op=SDL_GPU_STOREOP_STORE,.clear_color={80.f/255.f,100.f/255.f,120.f/255.f,1}};
   SDL_GPURenderPass*pass=SDL_BeginGPURenderPass(c,&color,1,NULL);CHECK(pass);SDL_BindGPUGraphicsPipeline(pass,pipe);
   SDL_GPUTextureSamplerBinding bindings[2]={{atlas,sampler},{vram,sampler}};SDL_BindGPUFragmentSamplers(pass,0,bindings,2);
   struct{float rect[4],uv[4],tint[4];}quad={{-1,1,1,-1},{0,0,8.f/256.f,1.f/256.f},{1,1,1,1}};struct{unsigned state[4];float region[4];}source={{31u|(abr<<5),0,(unsigned)semi,0},{0,0,256,256}};if(localRegion){quad.uv[2]=quad.uv[3]=1;source.region[2]=8;source.region[3]=1;}if(reflected){quad.uv[0]=quad.uv[2];quad.uv[2]=0;}SDL_PushGPUVertexUniformData(c,0,&quad,sizeof(quad));SDL_PushGPUFragmentUniformData(c,0,&source,sizeof(source));SDL_DrawGPUPrimitives(pass,6,1,0,0);SDL_EndGPURenderPass(pass);
   SDL_GPUCopyPass*copy=SDL_BeginGPUCopyPass(c);SDL_GPUTextureRegion from={.texture=target,.w=8,.h=1,.d=1};SDL_GPUTextureTransferInfo to={.transfer_buffer=download};SDL_DownloadFromGPUTexture(copy,&from,&to);SDL_EndGPUCopyPass(copy);SDL_GPUFence*fence=SDL_SubmitGPUCommandBufferAndAcquireFence(c);CHECK(fence);CHECK(SDL_WaitForGPUFences(d,true,&fence,1));unsigned char*out=SDL_MapGPUTransferBuffer(d,download,false);CHECK(out);
   for(int x=0;x<8;x++)for(int channel=0;channel<3;channel++){
    float a=alpha/255.f,src=(float)image[channel],expected;
    if(semi&&((reflected?7-x:x)&1))expected=abr==0?src*.5f+background[channel]*(1-a*.5f):src*(abr==3?.25f:1.f)+background[channel];
    else expected=src+background[channel]*(1-a);
    int value=(int)(expected+.5f);if(abs((int)out[x*4+channel]-value)>1){fprintf(stderr,"UI blend mismatch coverage=%d semi=%d abr=%u x=%d channel=%d actual=%u expected=%d\n",coverage,semi,abr,x,channel,out[x*4+channel],value);return 1;}
   }
   SDL_UnmapGPUTransferBuffer(d,download);SDL_ReleaseGPUFence(d,fence);
  }
 }
 free(image);SDL_ReleaseGPUTransferBuffer(d,download);SDL_ReleaseGPUSampler(d,sampler);SDL_ReleaseGPUTexture(d,atlas);SDL_ReleaseGPUTexture(d,vram);SDL_ReleaseGPUTexture(d,target);SDL_ReleaseGPUGraphicsPipeline(d,pipe);SDL_ReleaseGPUShader(d,vs);SDL_ReleaseGPUShader(d,fs);SDL_DestroyGPUDevice(d);SDL_Quit();puts("UI atlas: mixed STP, 3 blend modes and new alpha coverage passed");return 0;
}
