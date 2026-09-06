#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render/render_mesh_build.h"
#include "render/render_native_vertex.h"
#include "native_vert_spv.h"
#include "native_vert_msl.h"
#include "fog_probe_frag_spv.h"
#include "fog_probe_frag_msl.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s: %s\n", \
    __LINE__, #x, SDL_GetError()); return 1; } } while (0)

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 77;
    SDL_GPUDevice *device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (!device) { fprintf(stderr, "%s\n", SDL_GetError()); SDL_Quit(); return 77; }
    bool spirv = (SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV) != 0;
    SDL_GPUShaderCreateInfo shader = {0};
    shader.format = spirv ? SDL_GPU_SHADERFORMAT_SPIRV : SDL_GPU_SHADERFORMAT_MSL;
    shader.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader.code = spirv ? native_vert_spv : native_vert_msl;
    shader.code_size = spirv ? native_vert_spv_len : native_vert_msl_len;
    shader.entrypoint = spirv ? "main" : "vs_native";
    shader.num_uniform_buffers = 3;
    SDL_GPUShader *vs = SDL_CreateGPUShader(device, &shader); CHECK(vs);
    shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader.code = spirv ? fog_probe_frag_spv : fog_probe_frag_msl;
    shader.code_size = spirv ? fog_probe_frag_spv_len : fog_probe_frag_msl_len;
    shader.entrypoint = spirv ? "main" : "fs_fog_probe";
    shader.num_uniform_buffers = 0;
    SDL_GPUShader *fs = SDL_CreateGPUShader(device, &shader); CHECK(fs);
    SDL_GPUVertexBufferDescription description = {0};
    description.pitch = sizeof(RageNativeGpuVertex);
    description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
#define ATTRIBUTE(n, field, type) { .location = n, .buffer_slot = 0, \
    .format = SDL_GPU_VERTEXELEMENTFORMAT_##type, .offset = offsetof(RageNativeGpuVertex, field) }
    SDL_GPUVertexAttribute attributes[] = {
        ATTRIBUTE(0, position, FLOAT3), ATTRIBUTE(1, uv, FLOAT2),
        ATTRIBUTE(2, color, UBYTE4), ATTRIBUTE(3, normal, FLOAT3),
        ATTRIBUTE(4, fog, FLOAT4), ATTRIBUTE(6, depthBias, FLOAT)};
    SDL_GPUColorTargetDescription targetDescription = {0};
    targetDescription.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.vertex_shader = vs; pipelineInfo.fragment_shader = fs;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &description;
    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_attributes = attributes;
    pipelineInfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(attributes);
    SDL_GPUColorTargetDescription targetDescriptions[2] = {targetDescription, targetDescription};
    pipelineInfo.target_info.num_color_targets = 2;
    pipelineInfo.target_info.color_target_descriptions = targetDescriptions;
    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    CHECK(pipeline);
    SDL_GPUTextureCreateInfo texture = {0};
    texture.type = SDL_GPU_TEXTURETYPE_2D; texture.format = targetDescription.format;
    texture.width = texture.height = 8; texture.layer_count_or_depth = texture.num_levels = 1;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    SDL_GPUTexture *target = SDL_CreateGPUTexture(device, &texture); CHECK(target);
    SDL_GPUTexture *instanceTarget = SDL_CreateGPUTexture(device, &texture); CHECK(instanceTarget);
    RageNativeDrawVertex vertices[3] = {0};
    RageNativeGpuVertex packed[3];
    SDL_GPUBufferCreateInfo bufferInfo = {0};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX; bufferInfo.size = sizeof(packed);
    SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(device, &bufferInfo); CHECK(buffer);
    SDL_GPUTransferBufferCreateInfo transferInfo = {0};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(packed);
    SDL_GPUTransferBuffer *upload = SDL_CreateGPUTransferBuffer(device, &transferInfo); CHECK(upload);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD; transferInfo.size = 2 * 8 * 8 * 4;
    SDL_GPUTransferBuffer *download = SDL_CreateGPUTransferBuffer(device, &transferInfo); CHECK(download);
    const float depths[] = {0, 4, 5, 6, 10, 19, 20, 30};
    unsigned cases = 0;
    for (unsigned reference = 0; reference < 2; ++reference)
    for (unsigned view = 0; view < 2; ++view)
    for (unsigned range = 0; range < 3; ++range)
    for (unsigned enabled = 0; enabled < 2; ++enabled)
    for (unsigned sample = 0; sample < SDL_arraysize(depths); ++sample) {
        RageRenderCamera camera = {0};
        camera.transform.position.z = view ? 8.0f : -8.0f;
        camera.fogNear = range == 1 ? 0.0f : 5.0f;
        camera.fogFar = range == 2 ? 4.0f : 20.0f;
        /* Native camera ABI: position, view rows, projection, fog colour/range. */
        float uniform[7][4] = {{0}}, shadow[5][4] = {{0}};
        uniform[0][2] = camera.transform.position.z;
        uniform[1][0] = uniform[2][1] = uniform[3][2] = 1.0f;
        uniform[4][0] = uniform[4][1] = 1.0f; uniform[4][3] = 0.5f;
        uniform[5][0] = view ? 0.75f : 0.25f;
        uniform[5][1] = 0.5f; uniform[5][2] = 0.125f;
        uniform[5][3] = (float)reference;
        if (range == 0) {
            uniform[6][0] = camera.fogNear; uniform[6][1] = camera.fogFar;
            uniform[6][2] = 1.0f / camera.fogNear;
            uniform[6][3] = uniform[6][2] - 1.0f / camera.fogFar;
        }
        RageRenderVec3 original = {0, 0, camera.transform.position.z - depths[sample]};
        float expected = enabled ? RenderFogFactor(&camera, &original) : 0.0f;
        for (unsigned i = 0; i < 3; ++i) {
            vertices[i].position[0] = i == 1 ? 3.0f : -1.0f;
            vertices[i].position[1] = i == 2 ? 3.0f : -1.0f;
            vertices[i].position[2] = camera.transform.position.z - 1.0f;
            vertices[i].fog[0] = vertices[i].fog[1] = 0.0f;
            vertices[i].fog[2] = original.z; vertices[i].fog[3] = (float)enabled;
            if (reference) {
                memcpy(vertices[i].fog, uniform[5], 3 * sizeof(float));
                vertices[i].fog[3] = expected;
            }
        }
        void *mapped = SDL_MapGPUTransferBuffer(device, upload, true); CHECK(mapped);
        for (unsigned i = 0; i < 3; ++i) packed[i] = RenderPackNativeGpuVertex(&vertices[i]);
        memcpy(mapped, packed, sizeof(packed)); SDL_UnmapGPUTransferBuffer(device, upload);
        SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device); CHECK(cmd);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd); CHECK(copy);
        SDL_GPUTransferBufferLocation source = {.transfer_buffer = upload};
        SDL_GPUBufferRegion destination = {.buffer = buffer, .size = sizeof(packed)};
        SDL_UploadToGPUBuffer(copy, &source, &destination, true); SDL_EndGPUCopyPass(copy);
        SDL_PushGPUVertexUniformData(cmd, 0, uniform, sizeof(uniform));
        SDL_PushGPUVertexUniformData(cmd, 1, shadow, sizeof(shadow));
        const float instance[2][4] = {
            {view ? 0.25f : 0.75f, (float)range * 0.25f, 1, 0},
            {enabled ? 0.5f : 1, (float)(sample % 2), 0, 0}};
        SDL_PushGPUVertexUniformData(cmd, 2, instance, sizeof(instance));
        SDL_GPUColorTargetInfo color = {0}; color.texture = target;
        color.load_op = SDL_GPU_LOADOP_CLEAR; color.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPUColorTargetInfo colors[2] = {color, color};
        colors[1].texture = instanceTarget;
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, colors, 2, NULL); CHECK(pass);
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_GPUBufferBinding binding = {.buffer = buffer};
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0); SDL_EndGPURenderPass(pass);
        copy = SDL_BeginGPUCopyPass(cmd); CHECK(copy);
        SDL_GPUTextureRegion region = {.texture = target, .w = 8, .h = 8, .d = 1};
        SDL_GPUTextureTransferInfo readback = {.transfer_buffer = download};
        SDL_DownloadFromGPUTexture(copy, &region, &readback);
        region.texture = instanceTarget;
        readback.offset = 8 * 8 * 4;
        SDL_DownloadFromGPUTexture(copy, &region, &readback); SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd); CHECK(fence);
        CHECK(SDL_WaitForGPUFences(device, true, &fence, 1));
        const unsigned char *pixels = SDL_MapGPUTransferBuffer(device, download, false); CHECK(pixels);
        for (unsigned pixel = 0; pixel < 64; ++pixel) {
            for (unsigned channel = 0; channel < 4; ++channel) {
                float value = channel == 3 ? expected : uniform[5][channel];
                int wanted = (int)lroundf(value * 255.0f);
                if (abs((int)pixels[pixel * 4 + channel] - wanted) > 1) {
                    fprintf(stderr, "view=%u range=%u enabled=%u depth=%g pixel=%u channel=%u expected=%d got=%u\n",
                        view, range, enabled, (double)depths[sample], pixel, channel, wanted, pixels[pixel * 4 + channel]);
                    return 1;
                }
                float stateValue = channel == 3 ? instance[1][1]
                    : instance[0][channel] * instance[1][0];
                int stateWanted = (int)lroundf(stateValue * 255.0f);
                if (abs((int)pixels[256 + pixel * 4 + channel] - stateWanted) > 1) {
                    fprintf(stderr, "instance state mismatch: case=%u channel=%u expected=%d got=%u\n",
                        cases, channel, stateWanted, pixels[256 + pixel * 4 + channel]);
                    return 1;
                }
            }
        }
        SDL_UnmapGPUTransferBuffer(device, download); SDL_ReleaseGPUFence(device, fence);
        ++cases;
    }
    SDL_ReleaseGPUTransferBuffer(device, upload); SDL_ReleaseGPUTransferBuffer(device, download);
    SDL_ReleaseGPUBuffer(device, buffer); SDL_ReleaseGPUTexture(device, target);
    SDL_ReleaseGPUTexture(device, instanceTarget);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUShader(device, vs); SDL_ReleaseGPUShader(device, fs);
    SDL_DestroyGPUDevice(device); SDL_Quit();
    printf("Native GPU fog and instance state: %u cases match CPU within one UNORM step\n", cases);
    return 0;
}
