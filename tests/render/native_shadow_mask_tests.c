#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "render/render_native_vertex.h"
#include "native_shadow_vert_spv.h"
#include "native_shadow_vert_msl.h"
#include "native_shadow_masked_frag_spv.h"
#include "native_shadow_masked_frag_msl.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s: %s\n", \
    __LINE__, #x, SDL_GetError()); return 1; } } while (0)

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 77;
    SDL_GPUDevice *device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (!device) { SDL_Quit(); return 77; }
    bool spirv = (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV) != 0;
    SDL_GPUShaderCreateInfo shader = {0};
    shader.format = spirv ? SDL_GPU_SHADERFORMAT_SPIRV : SDL_GPU_SHADERFORMAT_MSL;
    shader.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader.code = spirv ? native_shadow_vert_spv : native_shadow_vert_msl;
    shader.code_size = spirv ? native_shadow_vert_spv_len : native_shadow_vert_msl_len;
    shader.entrypoint = spirv ? "main" : "vs_shadow";
    shader.num_uniform_buffers = 3;
    SDL_GPUShader *vs = SDL_CreateGPUShader(device, &shader); CHECK(vs);
    shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader.code = spirv ? native_shadow_masked_frag_spv : native_shadow_masked_frag_msl;
    shader.code_size = spirv ? native_shadow_masked_frag_spv_len : native_shadow_masked_frag_msl_len;
    shader.entrypoint = spirv ? "main" : "fs_shadow_masked";
    shader.num_uniform_buffers = 0; shader.num_samplers = 1;
    SDL_GPUShader *fs = SDL_CreateGPUShader(device, &shader); CHECK(fs);
    SDL_GPUVertexBufferDescription description = {0};
    description.pitch = sizeof(RageNativeGpuVertex);
    SDL_GPUVertexAttribute attributes[] = {
        {.location=0, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
         .offset=offsetof(RageNativeGpuVertex, position)},
        {.location=1, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
         .offset=offsetof(RageNativeGpuVertex, uv)},
        {.location=3, .format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
         .offset=offsetof(RageNativeGpuVertex, normal)}};
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.vertex_shader = vs; pipelineInfo.fragment_shader = fs;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &description;
    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_attributes = attributes;
    pipelineInfo.vertex_input_state.num_vertex_attributes = 3;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.target_info.has_depth_stencil_target = true;
    pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    pipelineInfo.depth_stencil_state.enable_depth_test = true;
    pipelineInfo.depth_stencil_state.enable_depth_write = true;
    pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo); CHECK(pipeline);
    SDL_GPUTextureCreateInfo texture = {0};
    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    texture.width = texture.height = 8;
    texture.layer_count_or_depth = texture.num_levels = 1;
    texture.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    SDL_GPUTexture *depth = SDL_CreateGPUTexture(device, &texture); CHECK(depth);
    texture.width = 4; texture.height = 1;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    SDL_GPUTexture *mask = SDL_CreateGPUTexture(device, &texture); CHECK(mask);
    SDL_GPUSamplerCreateInfo samplerInfo = {0};
    samplerInfo.address_mode_u = samplerInfo.address_mode_v = samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(device, &samplerInfo); CHECK(sampler);
    SDL_GPUBufferCreateInfo bufferInfo = {0};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = 3 * sizeof(RageNativeGpuVertex);
    SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(device, &bufferInfo); CHECK(buffer);
    SDL_GPUTransferBufferCreateInfo transfer = {0};
    transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; transfer.size = 512;
    SDL_GPUTransferBuffer *upload = SDL_CreateGPUTransferBuffer(device, &transfer); CHECK(upload);
    transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD; transfer.size = 256;
    SDL_GPUTransferBuffer *download = SDL_CreateGPUTransferBuffer(device, &transfer); CHECK(download);
    float results[3][64];
    for (unsigned mode = 0; mode < 3; ++mode) {
        RageNativeGpuVertex vertices[3] = {0};
        for (unsigned v = 0; v < 3; ++v) {
            vertices[v].position[0] = v == 1 ? 3 : -1;
            vertices[v].position[1] = v == 2 ? 3 : -1;
            vertices[v].position[2] = -1;
            vertices[v].uv[0] = (v == 1 ? 2.0f : 0.0f) + (mode == 2 ? 0.5f : 0);
            vertices[v].uv[1] = v == 2 ? 2 : 0;
        }
        unsigned char *mapped = SDL_MapGPUTransferBuffer(device, upload, true); CHECK(mapped);
        memcpy(mapped, vertices, sizeof(vertices));
        memset(mapped + 256, 255, 16);
        mapped[259] = mapped[263] = 0;
        SDL_UnmapGPUTransferBuffer(device, upload);
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device); CHECK(cmd);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd); CHECK(copy);
        SDL_GPUTransferBufferLocation source = {.transfer_buffer=upload};
        SDL_GPUBufferRegion destination = {.buffer=buffer, .size=sizeof(vertices)};
        SDL_UploadToGPUBuffer(copy, &source, &destination, true);
        SDL_GPUTextureTransferInfo maskSource = {.transfer_buffer=upload, .offset=256};
        SDL_GPUTextureRegion maskRegion = {.texture=mask, .w=4, .h=1, .d=1};
        SDL_UploadToGPUTexture(copy, &maskSource, &maskRegion, true);
        SDL_EndGPUCopyPass(copy);
        float camera[5][4] = {{0}};
        camera[1][0] = camera[2][1] = camera[3][2] = 1;
        camera[4][0] = camera[4][1] = 1; camera[4][3] = 0.5f;
        const float offset[4] = {mode == 1 ? 0.5f : 0, 0, 0, 0};
        SDL_PushGPUVertexUniformData(cmd, 0, camera, sizeof(camera));
        SDL_PushGPUVertexUniformData(cmd, 1, offset, sizeof(offset));
        const float local[5][4] = {{0}};
        SDL_PushGPUVertexUniformData(cmd, 2, local, sizeof(local));
        SDL_GPUDepthStencilTargetInfo target = {0};
        target.texture = depth; target.clear_depth = 1;
        target.load_op = SDL_GPU_LOADOP_CLEAR; target.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, NULL, 0, &target); CHECK(pass);
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_GPUBufferBinding binding = {.buffer=buffer};
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
        SDL_GPUTextureSamplerBinding maskBinding = {.texture=mask, .sampler=sampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &maskBinding, 1);
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(pass);
        copy = SDL_BeginGPUCopyPass(cmd); CHECK(copy);
        SDL_GPUTextureRegion region = {.texture=depth, .w=8, .h=8, .d=1};
        SDL_GPUTextureTransferInfo readback = {.transfer_buffer=download};
        SDL_DownloadFromGPUTexture(copy, &region, &readback);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd); CHECK(fence);
        CHECK(SDL_WaitForGPUFences(device, true, &fence, 1));
        mapped = SDL_MapGPUTransferBuffer(device, download, false); CHECK(mapped);
        memcpy(results[mode], mapped, sizeof(results[mode]));
        SDL_UnmapGPUTransferBuffer(device, download);
        SDL_ReleaseGPUFence(device, fence);
    }
    CHECK(memcmp(results[1], results[2], sizeof(results[1])) == 0);
    unsigned changed = 0;
    for (unsigned mode = 0; mode < 3; ++mode) {
        unsigned written = 0;
        for (unsigned pixel = 0; pixel < 64; ++pixel) {
            CHECK(results[mode][pixel] == 0.5f || results[mode][pixel] == 1.0f);
            written += results[mode][pixel] == 0.5f;
            if (mode == 1) changed += results[mode][pixel] != results[0][pixel];
        }
        CHECK(written == 32);
    }
    CHECK(changed == 64);
    SDL_ReleaseGPUTransferBuffer(device, upload); SDL_ReleaseGPUTransferBuffer(device, download);
    SDL_ReleaseGPUBuffer(device, buffer); SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseGPUTexture(device, mask); SDL_ReleaseGPUTexture(device, depth);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUShader(device, vs); SDL_ReleaseGPUShader(device, fs);
    SDL_DestroyGPUDevice(device); SDL_Quit();
    puts("Masked shadow scroll: depth pixels match CPU UV expansion and differ from unscrolled control");
    return 0;
}
