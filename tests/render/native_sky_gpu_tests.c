#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native_sky_vert_spv.h"
#include "native_sky_vert_msl.h"
#include "native_sky_frag_spv.h"
#include "native_sky_frag_msl.h"
#include "cloud_noise.h"

#define CHECK(x) do { if(!(x)){fprintf(stderr,"line %d: %s: %s\n",__LINE__,#x,SDL_GetError());return 1;} } while(0)

/* Exercise the actual sky shaders without a window or disc. A two-colour
 * panorama distinguishes the authored upper/lower rows after projection. */
int main(int argc,char **argv) {
    int benchmark=argc==2&&!strcmp(argv[1],"--benchmark");
    if(argc!=1&&!benchmark)return 2;
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
    si.stage=SDL_GPU_SHADERSTAGE_FRAGMENT;si.num_samplers=2;
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
    ti.type=SDL_GPU_TEXTURETYPE_3D;ti.width=ti.height=ti.layer_count_or_depth=RAGE_CLOUD_NOISE_EDGE;
    SDL_GPUTexture *volume=SDL_CreateGPUTexture(d,&ti);CHECK(volume);
    SDL_GPUSamplerCreateInfo sci={0};
    SDL_GPUSampler *sampler=SDL_CreateGPUSampler(d,&sci);CHECK(sampler);
    sci.min_filter=sci.mag_filter=SDL_GPU_FILTER_LINEAR;
    sci.address_mode_u=sci.address_mode_v=sci.address_mode_w=SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    SDL_GPUSampler *volumeSampler=SDL_CreateGPUSampler(d,&sci);CHECK(volumeSampler);
    SDL_GPUTransferBufferCreateInfo bi={.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,.size=1024+RAGE_CLOUD_NOISE_BYTES};
    SDL_GPUTransferBuffer *upload=SDL_CreateGPUTransferBuffer(d,&bi);CHECK(upload);
    bi.usage=SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;bi.size=64*64*4;
    SDL_GPUTransferBuffer *download=SDL_CreateGPUTransferBuffer(d,&bi);CHECK(download);
    Uint8 *p=SDL_MapGPUTransferBuffer(d,upload,false);CHECK(p);
    for(int y=0;y<256;y++){p[y*4]=y<128?255:0;p[y*4+1]=y<128?0:255;p[y*4+2]=0;p[y*4+3]=255;}
    CloudNoiseGenerate(p+1024);
    for(int axis=0;axis<3;axis++){
        float point[3]={0.125f,2.25f,3.5f};point[axis]+=8;
        CHECK(fabsf(CloudValue(0.125f,2.25f,3.5f,8)-CloudValue(point[0],point[1],point[2],8))<0.00001f);
        CHECK(fabsf(CloudWorley(0.125f,2.25f,3.5f)-CloudWorley(point[0],point[1],point[2]))<0.00001f);
    }
    SDL_UnmapGPUTransferBuffer(d,upload);
    SDL_GPUCommandBuffer *cmd=SDL_AcquireGPUCommandBuffer(d);CHECK(cmd);
    SDL_GPUCopyPass *copy=SDL_BeginGPUCopyPass(cmd);CHECK(copy);
    SDL_GPUTextureTransferInfo source={.transfer_buffer=upload};
    SDL_GPUTextureRegion region={.texture=panorama,.w=1,.h=256,.d=1};
    SDL_UploadToGPUTexture(copy,&source,&region,false);
    source.offset=1024;
    region=(SDL_GPUTextureRegion){.texture=volume,.w=64,.h=64,.d=64};
    SDL_UploadToGPUTexture(copy,&source,&region,false);SDL_EndGPUCopyPass(copy);
    float camera[5][4]={{0},{1,0,0,0},{0,1,0,0},{0,0,1,0},{1,1,0,0}};
    float sky[11][4]={{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1},
        {0,0,0,240},{64,0,0,128},{0,0,4,64}};
    Uint8 modern[64*64*4];
    for(int mode=0;mode<10;mode++){
    if(mode){cmd=SDL_AcquireGPUCommandBuffer(d);CHECK(cmd);sky[7][0]=1;sky[7][2]=1;}
    if(mode==2){camera[0][0]=12345;camera[0][1]=-678;camera[0][2]=500;}
    if(mode==3){memset(sky[4],0,3*4*sizeof(float));} /* No PS1 grid needed. */
    if(mode==4)for(int band=0;band<4;band++)for(int c=0;c<3;c++)sky[band][c]=0.01f;
    if(mode==5){
        for(int band=0;band<4;band++){sky[band][0]=sky[band][1]=0;sky[band][2]=1;}
        sky[8][0]=-0.8f;sky[8][1]=0.1f;sky[8][2]=0.5f;
    }
    if(mode==6){memset(sky[8],0,sizeof(sky[8]));sky[10][0]=17000;}
    if(mode==7){sky[7][2]=0;}
    if(mode==8){memset(sky[10],0,sizeof(sky[10]));sky[7][3]=5000;}
    if(mode==9){sky[7][3]=576000;}
    SDL_PushGPUVertexUniformData(cmd,0,camera,sizeof(camera));
    SDL_PushGPUFragmentUniformData(cmd,0,sky,sizeof(sky));
    SDL_GPUColorTargetInfo color={.texture=target,.load_op=SDL_GPU_LOADOP_CLEAR,.store_op=SDL_GPU_STOREOP_STORE};
    SDL_GPURenderPass *pass=SDL_BeginGPURenderPass(cmd,&color,1,NULL);CHECK(pass);
    SDL_BindGPUGraphicsPipeline(pass,pipeline);
    SDL_GPUTextureSamplerBinding bindings[2]={{panorama,sampler},{volume,volumeSampler}};
    SDL_BindGPUFragmentSamplers(pass,0,bindings,2);SDL_DrawGPUPrimitives(pass,3,1,0,0);
    SDL_EndGPURenderPass(pass);
    copy=SDL_BeginGPUCopyPass(cmd);CHECK(copy);
    region=(SDL_GPUTextureRegion){.texture=target,.w=64,.h=64,.d=1};
    SDL_GPUTextureTransferInfo dest={.transfer_buffer=download};
    SDL_DownloadFromGPUTexture(copy,&region,&dest);SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence=SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);CHECK(fence);
    CHECK(SDL_WaitForGPUFences(d,true,&fence,1));SDL_ReleaseGPUFence(d,fence);
    p=SDL_MapGPUTransferBuffer(d,download,false);CHECK(p);
    const Uint8 *upper=p+(8*64+32)*4,*lower=p+(55*64+32)*4;
    if(!mode){
        CHECK(upper[0]>240 && upper[1]<10 && upper[2]<10);
        CHECK(lower[2]>240 && lower[0]<10 && lower[1]<10);
    }else if(mode==1){
        CHECK(upper[2]>upper[0] && upper[2]>upper[1]);
        CHECK(lower[2]>240 && lower[0]<10 && lower[1]<10);
        memcpy(modern,p,sizeof(modern));
    }else if(mode<4){CHECK(memcmp(modern,p,sizeof(modern))==0);}
    else if(mode==4)for(int i=0;i<64*64;i++){
        CHECK(p[i*4]<40 && p[i*4+1]<40 && p[i*4+2]<40);
        CHECK(p[i*4+3]==255);
    }
    else if(mode<9){CHECK(memcmp(modern,p,sizeof(modern))!=0);}
    else for(int i=0;i<64*64*4;i++)CHECK(abs((int)modern[i]-(int)p[i])<=3);
    SDL_UnmapGPUTransferBuffer(d,download);
    }
    if(benchmark){
        SDL_GPUTextureCreateInfo large={0};large.type=SDL_GPU_TEXTURETYPE_2D;large.format=td.format;
        large.width=1280;large.height=720;large.layer_count_or_depth=large.num_levels=1;
        large.usage=SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        SDL_GPUTexture *bench=SDL_CreateGPUTexture(d,&large);CHECK(bench);
        for(int volumetric=0;volumetric<2;volumetric++){
            memset(sky[8],0,sizeof(sky[8]));memset(sky[10],0,sizeof(sky[10]));
            sky[7][3]=0;
            for(int band=0;band<4;band++){sky[band][0]=0.15f;sky[band][1]=0.35f;sky[band][2]=0.60f;}
            sky[7][2]=(float)volumetric;sky[7][1]=1;
            double elapsed=0;
            for(int frame=0;frame<24;frame++){
                Uint64 start=SDL_GetTicksNS();cmd=SDL_AcquireGPUCommandBuffer(d);CHECK(cmd);
                SDL_PushGPUVertexUniformData(cmd,0,camera,sizeof(camera));
                SDL_PushGPUFragmentUniformData(cmd,0,sky,sizeof(sky));
                SDL_GPUColorTargetInfo color={.texture=bench,.load_op=SDL_GPU_LOADOP_CLEAR,.store_op=SDL_GPU_STOREOP_STORE};
                SDL_GPURenderPass *pass=SDL_BeginGPURenderPass(cmd,&color,1,NULL);CHECK(pass);
                SDL_BindGPUGraphicsPipeline(pass,pipeline);
                SDL_GPUTextureSamplerBinding bindings[2]={{panorama,sampler},{volume,volumeSampler}};
                SDL_BindGPUFragmentSamplers(pass,0,bindings,2);SDL_DrawGPUPrimitives(pass,3,1,0,0);SDL_EndGPURenderPass(pass);
                SDL_GPUFence *fence=SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);CHECK(fence);
                CHECK(SDL_WaitForGPUFences(d,true,&fence,1));SDL_ReleaseGPUFence(d,fence);
                if(frame>=4)elapsed+=(double)(SDL_GetTicksNS()-start)/1000000.0;
            }
            printf("sky benchmark 1280x720 %s: %.3f ms/frame (submit + fence)\n",volumetric?"volume":"flat",elapsed/20.0);
        }
        SDL_ReleaseGPUTexture(d,bench);
    }
    SDL_ReleaseGPUTransferBuffer(d,upload);SDL_ReleaseGPUTransferBuffer(d,download);
    SDL_ReleaseGPUTexture(d,target);SDL_ReleaseGPUTexture(d,panorama);SDL_ReleaseGPUSampler(d,sampler);
    SDL_ReleaseGPUTexture(d,volume);SDL_ReleaseGPUSampler(d,volumeSampler);
    SDL_ReleaseGPUGraphicsPipeline(d,pipeline);SDL_ReleaseGPUShader(d,vs);SDL_ReleaseGPUShader(d,fs);
    SDL_DestroyGPUDevice(d);SDL_Quit();return 0;
}
