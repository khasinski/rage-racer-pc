#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "native_sky_vert_spv.h"
#include "native_sky_vert_msl.h"
#include "atmosphere_probe_frag_spv.h"
#include "atmosphere_probe_frag_msl.h"
#define CHECK(x) do { if(!(x)){fprintf(stderr,"line %d: %s: %s\n",__LINE__,#x,SDL_GetError());return 1;} } while(0)

/* Read back the production atmosphere function. No window or game disc. */
int main(void) {
    if(!SDL_Init(SDL_INIT_VIDEO))return 77;
    SDL_GPUDevice *d=SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_MSL,false,NULL);
    if(!d){SDL_Quit();return 77;}
    int spirv=(SDL_GetGPUShaderFormats(d)&SDL_GPU_SHADERFORMAT_SPIRV)!=0;
    SDL_GPUShaderCreateInfo si={0};si.stage=SDL_GPU_SHADERSTAGE_VERTEX;
    si.num_uniform_buffers=1;si.format=spirv?SDL_GPU_SHADERFORMAT_SPIRV:SDL_GPU_SHADERFORMAT_MSL;
    si.code=spirv?native_sky_vert_spv:native_sky_vert_msl;
    si.code_size=spirv?native_sky_vert_spv_len:native_sky_vert_msl_len;
    si.entrypoint=spirv?"main":"vs_native_sky";
    SDL_GPUShader *vs=SDL_CreateGPUShader(d,&si);CHECK(vs);
    si.stage=SDL_GPU_SHADERSTAGE_FRAGMENT;si.code=spirv?atmosphere_probe_frag_spv:atmosphere_probe_frag_msl;
    si.code_size=spirv?atmosphere_probe_frag_spv_len:atmosphere_probe_frag_msl_len;
    si.entrypoint=spirv?"main":"fs_atmosphere_probe";
    SDL_GPUShader *fs=SDL_CreateGPUShader(d,&si);CHECK(fs);
    SDL_GPUColorTargetDescription td={.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM};
    SDL_GPUGraphicsPipelineCreateInfo pi={0};pi.vertex_shader=vs;pi.fragment_shader=fs;
    pi.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pi.target_info.color_target_descriptions=&td;pi.target_info.num_color_targets=1;
    SDL_GPUGraphicsPipeline *pipeline=SDL_CreateGPUGraphicsPipeline(d,&pi);CHECK(pipeline);
    SDL_GPUTextureCreateInfo ti={0};ti.type=SDL_GPU_TEXTURETYPE_2D;ti.format=td.format;
    ti.width=8;ti.height=8;ti.layer_count_or_depth=1;ti.num_levels=1;ti.usage=SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    SDL_GPUTexture *target=SDL_CreateGPUTexture(d,&ti);CHECK(target);
    SDL_GPUTransferBufferCreateInfo bi={.usage=SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,.size=8*8*4};
    SDL_GPUTransferBuffer *download=SDL_CreateGPUTransferBuffer(d,&bi);CHECK(download);
    const float camera[5][4]={{0},{1,0,0,0},{0,1,0,0},{0,0,1,0},{1,1,0,0}};
    unsigned char samples[13][4];
    for(int test=0;test<13;test++){
        /* eye, range, fog colour, horizon, zenith, surface, position, openness */
        float u[8][4]={{0,0,0,100},{500,100,0,1},{0.6f,0.7f,0.8f,0},
            {0.6f,0.7f,0.8f,0},{0.6f,0.7f,0.8f,0},{0.1f,0.1f,0.1f,1},
            {0,0,-100,1},{1,1,1,0}};
        if(test==0)u[6][2]=0; /* Camera point: unchanged. */
        if(test==2)u[6][2]=-500;
        if(test==3)u[6][2]=-1500;
        if(test==4){u[0][1]=u[6][1]=1000;} /* Same distance, above height fog. */
        if(test==5)u[7][0]=u[7][1]=u[7][2]=0; /* Enclosed light zone. */
        if(test==6)u[6][3]=0; /* Mesh opts out. */
        if(test==7)u[1][3]=0; /* Stage/diagnostic opts out. */
        if(test==8){u[2][0]=u[2][1]=u[2][2]=0.02f;u[3][0]=u[3][1]=u[3][2]=0.02f;u[6][2]=-1500;}
        if(test==9)u[6][0]=100,u[6][2]=0; /* Equal radial distance: yaw invariant. */
        if(test==10)u[6][2]=-500,u[5][0]=u[5][1]=u[5][2]=0.9f;
        if(test==11)u[0][1]=u[6][1]=-200; /* Below datum: finite, bounded. */
        if(test==12){u[0][1]=0;u[6][1]=0.0001f;} /* Nearly horizontal: stable limit. */
        SDL_GPUCommandBuffer *cmd=SDL_AcquireGPUCommandBuffer(d);CHECK(cmd);
        SDL_PushGPUVertexUniformData(cmd,0,camera,sizeof(camera));
        SDL_PushGPUFragmentUniformData(cmd,0,u,sizeof(u));
        SDL_GPUColorTargetInfo color={.texture=target,.load_op=SDL_GPU_LOADOP_CLEAR,.store_op=SDL_GPU_STOREOP_STORE};
        SDL_GPURenderPass *pass=SDL_BeginGPURenderPass(cmd,&color,1,NULL);CHECK(pass);
        SDL_BindGPUGraphicsPipeline(pass,pipeline);SDL_DrawGPUPrimitives(pass,3,1,0,0);SDL_EndGPURenderPass(pass);
        SDL_GPUCopyPass *copy=SDL_BeginGPUCopyPass(cmd);CHECK(copy);
        SDL_GPUTextureRegion region={.texture=target,.w=8,.h=8,.d=1};
        SDL_GPUTextureTransferInfo dest={.transfer_buffer=download};
        SDL_DownloadFromGPUTexture(copy,&region,&dest);SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence=SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);CHECK(fence);
        CHECK(SDL_WaitForGPUFences(d,true,&fence,1));SDL_ReleaseGPUFence(d,fence);
        unsigned char *pixels=SDL_MapGPUTransferBuffer(d,download,false);CHECK(pixels);
        memcpy(samples[test],pixels+4*(4*8+4),4);SDL_UnmapGPUTransferBuffer(d,download);
    }
    CHECK(abs((int)samples[0][0]-26)<=1);
    CHECK(samples[1][0]>samples[0][0] && samples[2][0]>samples[1][0] && samples[3][0]>samples[2][0]);
    CHECK(samples[4][0]<samples[1][0]);CHECK(samples[5][0]<samples[1][0]);
    CHECK(abs((int)samples[6][0]-26)<=1 && abs((int)samples[7][0]-26)<=1);
    CHECK(samples[8][0]<26); /* Night converges to dark horizon, never white. */
    CHECK(abs((int)samples[9][0]-samples[1][0])<=1);
    CHECK(samples[10][0]-samples[2][0]<50); /* Texture/surface contrast attenuates. */
    CHECK(samples[11][0]>=samples[1][0] && samples[11][0]<=154);
    CHECK(abs((int)samples[12][0]-samples[1][0])<=1);
    for(int i=0;i<13;i++)CHECK(samples[i][3]==255);
    SDL_ReleaseGPUTransferBuffer(d,download);SDL_ReleaseGPUTexture(d,target);
    SDL_ReleaseGPUGraphicsPipeline(d,pipeline);SDL_ReleaseGPUShader(d,vs);SDL_ReleaseGPUShader(d,fs);
    SDL_DestroyGPUDevice(d);SDL_Quit();return 0;
}
