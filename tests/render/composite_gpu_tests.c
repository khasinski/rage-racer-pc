#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "post_vert_spv.h"
#include "post_vert_msl.h"
#include "composite_frag_spv.h"
#include "composite_frag_msl.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s: %s\n",__LINE__,#x,SDL_GetError()); return 1; } } while(0)
int main(void) {
    const unsigned char input[32] = {
        0,0,0,0, 255,255,255,255, 128,128,128,128, 255,0,0,255,
        0,255,0,255, 0,0,255,255, 60,120,180,100, 200,140,80,255};
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr,"GPU test unavailable: %s\n",SDL_GetError()); return 77;
    }
    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL,false,NULL);
    if (!device) { fprintf(stderr,"GPU test unavailable: %s\n",SDL_GetError()); SDL_Quit(); return 77; }
    const bool spirv = (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV) != 0;
    SDL_GPUShaderCreateInfo shader = {0};
    shader.format = spirv ? SDL_GPU_SHADERFORMAT_SPIRV : SDL_GPU_SHADERFORMAT_MSL;
    shader.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader.code = spirv ? post_vert_spv : post_vert_msl;
    shader.code_size = spirv ? post_vert_spv_len : post_vert_msl_len;
    shader.entrypoint = spirv ? "main" : "vs_post";
    SDL_GPUShader *vs = SDL_CreateGPUShader(device,&shader); CHECK(vs);
    shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader.code = spirv ? composite_frag_spv : composite_frag_msl;
    shader.code_size = spirv ? composite_frag_spv_len : composite_frag_msl_len;
    shader.entrypoint = spirv ? "main" : "fs_composite";
    shader.num_samplers = 1;
    SDL_GPUShader *fs = SDL_CreateGPUShader(device,&shader); CHECK(fs);
    SDL_GPUColorTargetDescription targetDesc = {0};
    targetDesc.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.vertex_shader=vs; pipelineInfo.fragment_shader=fs;
    pipelineInfo.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.target_info.num_color_targets=1;
    pipelineInfo.target_info.color_target_descriptions=&targetDesc;
    SDL_GPUGraphicsPipeline *pipeline=SDL_CreateGPUGraphicsPipeline(device,&pipelineInfo); CHECK(pipeline);
    SDL_GPUTextureCreateInfo textureInfo = {0};
    textureInfo.type=SDL_GPU_TEXTURETYPE_2D; textureInfo.format=targetDesc.format;
    textureInfo.width=8; textureInfo.height=1; textureInfo.layer_count_or_depth=1; textureInfo.num_levels=1;
    textureInfo.usage=SDL_GPU_TEXTUREUSAGE_SAMPLER;
    SDL_GPUTexture *source=SDL_CreateGPUTexture(device,&textureInfo); CHECK(source);
    textureInfo.usage=SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    SDL_GPUTexture *target=SDL_CreateGPUTexture(device,&textureInfo); CHECK(target);
    SDL_GPUTransferBufferCreateInfo transferInfo={0};
    transferInfo.size=sizeof(input); transferInfo.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer *upload=SDL_CreateGPUTransferBuffer(device,&transferInfo); CHECK(upload);
    transferInfo.usage=SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    SDL_GPUTransferBuffer *download=SDL_CreateGPUTransferBuffer(device,&transferInfo); CHECK(download);
    void *mapped=SDL_MapGPUTransferBuffer(device,upload,false); CHECK(mapped);
    memcpy(mapped,input,sizeof(input)); SDL_UnmapGPUTransferBuffer(device,upload);
    SDL_GPUSamplerCreateInfo samplerInfo={0};
    SDL_GPUSampler *sampler=SDL_CreateGPUSampler(device,&samplerInfo); CHECK(sampler);
    SDL_GPUCommandBuffer *cmd=SDL_AcquireGPUCommandBuffer(device); CHECK(cmd);
    SDL_GPUCopyPass *copy=SDL_BeginGPUCopyPass(cmd); CHECK(copy);
    SDL_GPUTextureTransferInfo transfer={0}; transfer.transfer_buffer=upload;
    SDL_GPUTextureRegion region={0}; region.texture=source; region.w=8; region.h=1; region.d=1;
    SDL_UploadToGPUTexture(copy,&transfer,&region,false); SDL_EndGPUCopyPass(copy);
    SDL_GPUColorTargetInfo color={0}; color.texture=target;
    color.load_op=SDL_GPU_LOADOP_DONT_CARE; color.store_op=SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass *pass=SDL_BeginGPURenderPass(cmd,&color,1,NULL); CHECK(pass);
    SDL_BindGPUGraphicsPipeline(pass,pipeline);
    SDL_GPUTextureSamplerBinding binding={source,sampler};
    SDL_BindGPUFragmentSamplers(pass,0,&binding,1);
    SDL_DrawGPUPrimitives(pass,3,1,0,0); SDL_EndGPURenderPass(pass);
    copy=SDL_BeginGPUCopyPass(cmd); CHECK(copy);
    region.texture=target; transfer.transfer_buffer=download;
    SDL_DownloadFromGPUTexture(copy,&region,&transfer); SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence=SDL_SubmitGPUCommandBufferAndAcquireFence(cmd); CHECK(fence);
    CHECK(SDL_WaitForGPUFences(device,true,&fence,1));
    const unsigned char *output=SDL_MapGPUTransferBuffer(device,download,false); CHECK(output);
    for (int pixel=0;pixel<8;++pixel) {
        float luma=(input[pixel*4]*0.299f+input[pixel*4+1]*0.587f+input[pixel*4+2]*0.114f)/255.0f;
        for (int channel=0;channel<3;++channel) {
            float c=luma+(input[pixel*4+channel]/255.0f-luma)*1.16f;
            c=(c-0.5f)*1.04f+0.5f;
            int expected=(int)lroundf(fminf(1.0f,fmaxf(0.0f,c))*255.0f);
            CHECK(abs((int)output[pixel*4+channel]-expected)<=1);
        }
        CHECK(output[pixel*4+3]==255);
    }
    SDL_UnmapGPUTransferBuffer(device,download);
    SDL_ReleaseGPUFence(device,fence);
    SDL_ReleaseGPUSampler(device,sampler);
    SDL_ReleaseGPUTransferBuffer(device,upload); SDL_ReleaseGPUTransferBuffer(device,download);
    SDL_ReleaseGPUTexture(device,source); SDL_ReleaseGPUTexture(device,target);
    SDL_ReleaseGPUGraphicsPipeline(device,pipeline);
    SDL_ReleaseGPUShader(device,vs); SDL_ReleaseGPUShader(device,fs);
    SDL_DestroyGPUDevice(device); SDL_Quit();
    puts("Composite GPU: all reference colours and alpha passed");
    return 0;
}
