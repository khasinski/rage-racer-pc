#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "native_sky_vert_spv.h"
#include "native_sky_vert_msl.h"
#include "native_sky_frag_spv.h"
#include "native_sky_frag_msl.h"

#define CHECK(x) do { if(!(x)){fprintf(stderr,"line %d: %s: %s\n",__LINE__,#x,SDL_GetError());return 1;} } while(0)

/* Exercise the actual sky shaders without a window or disc. A two-colour
 * panorama distinguishes the authored upper/lower rows after projection. */
int main(void) {
    if(!SDL_Init(SDL_INIT_VIDEO))return 77;
    SDL_GPUDevice *d=SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_MSL,false,NULL);
    if(!d){SDL_Quit();return 77;}
    int spirv=(SDL_GetGPUShaderFormats(d)&SDL_GPU_SHADERFORMAT_SPIRV)!=0;
    SDL_GPUShaderCreateInfo si={0};
    si.stage=SDL_GPU_SHADERSTAGE_VERTEX;si.num_uniform_buffers=1;
    si.format=spirv?SDL_GPU_SHADERFORMAT_SPIRV:SDL_GPU_SHADERFORMAT_MSL;
    si.code=spirv?native_sky_vert_spv:native_sky_vert_msl;
    si.code_size=spirv?native_sky_vert_spv_len:native_sky_vert_msl_len;
    si.entrypoint=spirv?"main":"vs_native_sky";
    SDL_GPUShader *vs=SDL_CreateGPUShader(d,&si);CHECK(vs);
    si.stage=SDL_GPU_SHADERSTAGE_FRAGMENT;si.num_samplers=1;
    si.code=spirv?native_sky_frag_spv:native_sky_frag_msl;
    si.code_size=spirv?native_sky_frag_spv_len:native_sky_frag_msl_len;
    si.entrypoint=spirv?"main":"fs_native_sky";
    SDL_GPUShader *fs=SDL_CreateGPUShader(d,&si);CHECK(fs);
    SDL_GPUColorTargetDescription td={.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
    SDL_GPUGraphicsPipelineCreateInfo pi={0};pi.vertex_shader=vs;pi.fragment_shader=fs;
    pi.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pi.target_info.color_target_descriptions=&td;pi.target_info.num_color_targets=1;
    SDL_GPUGraphicsPipeline *pipeline=SDL_CreateGPUGraphicsPipeline(d,&pi);CHECK(pipeline);
    SDL_GPUTextureCreateInfo ti={0};ti.type=SDL_GPU_TEXTURETYPE_2D;ti.format=td.format;
    ti.width=64;ti.height=64;ti.layer_count_or_depth=1;ti.num_levels=1;
    ti.usage=SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    SDL_GPUTexture *target=SDL_CreateGPUTexture(d,&ti);CHECK(target);
    ti.width=1;ti.height=256;ti.usage=SDL_GPU_TEXTUREUSAGE_SAMPLER;
    SDL_GPUTexture *panorama=SDL_CreateGPUTexture(d,&ti);CHECK(panorama);
    SDL_GPUSamplerCreateInfo sci={0};
    SDL_GPUSampler *sampler=SDL_CreateGPUSampler(d,&sci);CHECK(sampler);
    SDL_GPUTransferBufferCreateInfo bi={.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,.size=1024};
    SDL_GPUTransferBuffer *upload=SDL_CreateGPUTransferBuffer(d,&bi);CHECK(upload);
    bi.usage=SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;bi.size=64*64*4;
    SDL_GPUTransferBuffer *download=SDL_CreateGPUTransferBuffer(d,&bi);CHECK(download);
    Uint8 *p=SDL_MapGPUTransferBuffer(d,upload,false);CHECK(p);
    for(int y=0;y<256;y++){p[y*4]=y<128?255:0;p[y*4+1]=y<128?0:255;p[y*4+2]=0;p[y*4+3]=255;}
    SDL_UnmapGPUTransferBuffer(d,upload);
    SDL_GPUCommandBuffer *cmd=SDL_AcquireGPUCommandBuffer(d);CHECK(cmd);
    SDL_GPUCopyPass *copy=SDL_BeginGPUCopyPass(cmd);CHECK(copy);
    SDL_GPUTextureTransferInfo source={.transfer_buffer=upload};
    SDL_GPUTextureRegion region={.texture=panorama,.w=1,.h=256,.d=1};
    SDL_UploadToGPUTexture(copy,&source,&region,false);SDL_EndGPUCopyPass(copy);
    const float camera[5][4]={{0},{1,0,0,0},{0,1,0,0},{0,0,1,0},{1,1,0,0}};
    const float sky[7][4]={{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1},
        {0,0,0,240},{64,0,0,128},{0,0,4,64}};
    SDL_PushGPUVertexUniformData(cmd,0,camera,sizeof(camera));
    SDL_PushGPUFragmentUniformData(cmd,0,sky,sizeof(sky));
    SDL_GPUColorTargetInfo color={.texture=target,.load_op=SDL_GPU_LOADOP_CLEAR,.store_op=SDL_GPU_STOREOP_STORE};
    SDL_GPURenderPass *pass=SDL_BeginGPURenderPass(cmd,&color,1,NULL);CHECK(pass);
    SDL_BindGPUGraphicsPipeline(pass,pipeline);
    SDL_GPUTextureSamplerBinding binding={panorama,sampler};
    SDL_BindGPUFragmentSamplers(pass,0,&binding,1);SDL_DrawGPUPrimitives(pass,3,1,0,0);
    SDL_EndGPURenderPass(pass);
    copy=SDL_BeginGPUCopyPass(cmd);CHECK(copy);
    region=(SDL_GPUTextureRegion){.texture=target,.w=64,.h=64,.d=1};
    SDL_GPUTextureTransferInfo dest={.transfer_buffer=download};
    SDL_DownloadFromGPUTexture(copy,&region,&dest);SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence=SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);CHECK(fence);
    CHECK(SDL_WaitForGPUFences(d,true,&fence,1));SDL_ReleaseGPUFence(d,fence);
    p=SDL_MapGPUTransferBuffer(d,download,false);CHECK(p);
    const Uint8 *upper=p+(8*64+32)*4,*lower=p+(55*64+32)*4;
    CHECK(upper[0]>240 && upper[1]<10 && upper[2]<10);
    CHECK(lower[2]>240 && lower[0]<10 && lower[1]<10);
    SDL_UnmapGPUTransferBuffer(d,download);
    SDL_ReleaseGPUTransferBuffer(d,upload);SDL_ReleaseGPUTransferBuffer(d,download);
    SDL_ReleaseGPUTexture(d,target);SDL_ReleaseGPUTexture(d,panorama);SDL_ReleaseGPUSampler(d,sampler);
    SDL_ReleaseGPUGraphicsPipeline(d,pipeline);SDL_ReleaseGPUShader(d,vs);SDL_ReleaseGPUShader(d,fs);
    SDL_DestroyGPUDevice(d);SDL_Quit();return 0;
}
