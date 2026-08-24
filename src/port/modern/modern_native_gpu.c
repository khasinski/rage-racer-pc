#include "modern_native_gpu.h"

#include "modern_assets.h"
#include "render/render_mesh_build.h"

#include "shaders/native_color_frag_spv.h"
#include "shaders/native_texture_frag_spv.h"
#include "shaders/native_vert_spv.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    MODERN_NATIVE_MAX_VERTICES = 1000000,
    MODERN_NATIVE_MAX_SPANS = 32768,
    MODERN_NATIVE_MAX_TEXTURES = 2048,
};

typedef struct ModernNativeCameraUniform {
    float position[4];
    float viewRow0[4];
    float viewRow1[4];
    float viewRow2[4];
    float projection[4];
} ModernNativeCameraUniform;

typedef struct ModernNativeTexture {
    uint32_t assetKey;
    uint32_t material;
    uint32_t entity;
    RageRenderAssetSet assetSet;
    int transparent;
    SDL_GPUTexture *texture;
    SDL_GPUTransferBuffer *transfer;
} ModernNativeTexture;

static const char MODERN_NATIVE_MSL[] =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct NativeIn { float3 pos [[attribute(0)]]; float2 uv [[attribute(1)]]; uchar4 color [[attribute(2)]]; float3 normal [[attribute(3)]]; };\n"
    "struct NativeCamera { float4 position; float4 viewRow0; float4 viewRow1; float4 viewRow2; float4 projection; };\n"
    "struct NativeOut { float4 pos [[position]]; float2 uv; float4 color; float3 normal; };\n"
    "vertex NativeOut vs_native(NativeIn in [[stage_in]], constant NativeCamera &camera [[buffer(0)]]) { NativeOut o; float3 p=in.pos-camera.position.xyz; float3 v=float3(dot(camera.viewRow0.xyz,p),dot(camera.viewRow1.xyz,p),dot(camera.viewRow2.xyz,p)); float z=(v.z-camera.projection.z)*camera.projection.w; o.pos=float4(v.x*camera.projection.x,v.y*camera.projection.y,z*v.z,v.z); o.uv=in.uv; o.color=float4(in.color)/255.0; o.normal=in.normal; return o; }\n"
    "fragment float4 fs_native(NativeOut in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) { float4 t=tex.sample(smp,in.uv); float n2=dot(in.normal,in.normal); float3 n=n2>0.000001 ? in.normal*rsqrt(n2) : float3(0.0,1.0,0.0); float ndl=max(dot(n,normalize(float3(-0.4,0.7,0.5))),0.0); float3 light=float3(0.28+0.72*ndl); float4 c=float4(t.rgb*in.color.rgb*light,t.a*in.color.a); if(c.a<=0.001) discard_fragment(); return c; }\n"
    "fragment float4 fs_native_color(NativeOut in [[stage_in]]) { float n2=dot(in.normal,in.normal); float3 n=n2>0.000001 ? in.normal*rsqrt(n2) : float3(0.0,1.0,0.0); float ndl=max(dot(n,normalize(float3(-0.4,0.7,0.5))),0.0); return float4(in.color.rgb*(0.28+0.72*ndl),in.color.a); }\n";

static SDL_GPUDevice *s_device;
static SDL_GPUGraphicsPipeline *s_texturedOpaque;
static SDL_GPUGraphicsPipeline *s_texturedTransparent;
static SDL_GPUGraphicsPipeline *s_colorOpaque;
static SDL_GPUBuffer *s_vertexBuffer;
static SDL_GPUTransferBuffer *s_vertexTransfer;
static SDL_GPUSampler *s_sampler;
static RageNativeDrawVertex *s_vertices;
static RageNativeDrawSpan *s_spans;
static uint32_t s_vertexCount;
static uint32_t s_spanCount;
static uint64_t s_worldFrame = UINT64_MAX;
static const RageRenderWorld *s_world;
static float s_aspect = 4.0f / 3.0f;
static int s_completeWorld;
static ModernNativeTexture s_textures[MODERN_NATIVE_MAX_TEXTURES];
static uint32_t s_textureCount;

static SDL_GPUShader *ModernNativeCreateShader(
    const unsigned char *spirv, size_t spirvSize, const char *entry,
    SDL_GPUShaderStage stage, uint32_t samplers, uint32_t uniforms) {
    SDL_GPUShaderCreateInfo info = {0};
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(s_device);
    info.stage = stage;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniforms;
    if ((formats & SDL_GPU_SHADERFORMAT_SPIRV) != 0) {
        info.code = spirv;
        info.code_size = spirvSize;
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if ((formats & SDL_GPU_SHADERFORMAT_MSL) != 0) {
        info.code = (const Uint8 *)MODERN_NATIVE_MSL;
        info.code_size = sizeof(MODERN_NATIVE_MSL);
        info.entrypoint = entry;
        info.format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        return NULL;
    }
    return SDL_CreateGPUShader(s_device, &info);
}

static SDL_GPUGraphicsPipeline *ModernNativeCreatePipeline(
    SDL_GPUShader *vertex, SDL_GPUShader *fragment, int transparent) {
    const SDL_GPUVertexBufferDescription buffer = {
        .slot = 0,
        .pitch = sizeof(RageNativeDrawVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    const SDL_GPUVertexAttribute attributes[] = {
        {.location = 0, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
         .offset = offsetof(RageNativeDrawVertex, position)},
        {.location = 1, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
         .offset = offsetof(RageNativeDrawVertex, uv)},
        {.location = 2, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4,
         .offset = offsetof(RageNativeDrawVertex, color)},
        {.location = 3, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
         .offset = offsetof(RageNativeDrawVertex, normal)},
    };
    SDL_GPUColorTargetDescription color = {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    };
    SDL_GPUGraphicsPipelineCreateInfo info = {0};
    if (transparent) {
        color.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        color.blend_state.dst_color_blendfactor =
            SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        color.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        color.blend_state.dst_alpha_blendfactor =
            SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        color.blend_state.enable_blend = true;
    }
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attributes;
    info.vertex_input_state.num_vertex_attributes = 4;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = !transparent;
    info.target_info.color_target_descriptions = &color;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    return SDL_CreateGPUGraphicsPipeline(s_device, &info);
}

static void ModernNativeRotate(float out[3], const float in[3],
                               const RageRenderCamera *camera) {
    float x = in[0], y = in[1], z = in[2];
    if (camera->transform.hasOrientation) {
        const RageRenderQuaternion *q = &camera->transform.orientation;
        float length = sqrtf(q->x * q->x + q->y * q->y +
                             q->z * q->z + q->w * q->w);
        if (length > 0.0f) {
            float qx = -q->x / length, qy = -q->y / length;
            float qz = -q->z / length, qw = q->w / length;
            float xx = qx * qx, yy = qy * qy, zz = qz * qz;
            float xy = qx * qy, xz = qx * qz, yz = qy * qz;
            float wx = qw * qx, wy = qw * qy, wz = qw * qz;
            out[0] = (1.0f - 2.0f * (yy + zz)) * x +
                     2.0f * (xy - wz) * y + 2.0f * (xz + wy) * z;
            out[1] = 2.0f * (xy + wz) * x +
                     (1.0f - 2.0f * (xx + zz)) * y +
                     2.0f * (yz - wx) * z;
            out[2] = 2.0f * (xz - wy) * x + 2.0f * (yz + wx) * y +
                     (1.0f - 2.0f * (xx + yy)) * z;
            return;
        }
    }
    {
        float rx = -camera->transform.rotation.x * 0.017453292519943295f;
        float ry = -camera->transform.rotation.y * 0.017453292519943295f;
        float rz = -camera->transform.rotation.z * 0.017453292519943295f;
        float c = cosf(rz), s = sinf(rz), next;
        next = x * c - y * s; y = x * s + y * c; x = next;
        c = cosf(ry); s = sinf(ry);
        next = x * c + z * s; z = -x * s + z * c; x = next;
        c = cosf(rx); s = sinf(rx);
        next = y * c - z * s; z = y * s + z * c; y = next;
    }
    out[0] = x; out[1] = y; out[2] = z;
}

static void ModernNativeBuildCamera(const RageRenderCamera *camera,
                                    ModernNativeCameraUniform *out) {
    static const float axes[3][3] = {
        {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    float columns[3][3];
    float fovScale = 1.0f / tanf(camera->verticalFovDegrees *
                                 0.008726646259971648f);
    int axis;
    memset(out, 0, sizeof(*out));
    out->position[0] = camera->transform.position.x;
    out->position[1] = camera->transform.position.y;
    out->position[2] = camera->transform.position.z;
    for (axis = 0; axis < 3; axis++) {
        ModernNativeRotate(columns[axis], axes[axis], camera);
        out->viewRow0[axis] = columns[axis][0];
        out->viewRow1[axis] = columns[axis][1];
        out->viewRow2[axis] = columns[axis][2];
    }
    out->projection[0] = fovScale;
    out->projection[1] = fovScale;
    out->projection[2] = camera->nearPlane;
    out->projection[3] = 1.0f / (camera->farPlane - camera->nearPlane);
}

int ModernNativeGpuInit(SDL_GPUDevice *device) {
    SDL_GPUShader *vertex = NULL, *textureFragment = NULL, *colorFragment = NULL;
    SDL_GPUBufferCreateInfo buffer = {0};
    SDL_GPUTransferBufferCreateInfo transfer = {0};
    SDL_GPUSamplerCreateInfo sampler = {0};
    if (!ModernAssetsReady()) return 1;
    s_device = device;
    vertex = ModernNativeCreateShader(
        native_vert_spv, native_vert_spv_len, "vs_native",
        SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    textureFragment = ModernNativeCreateShader(
        native_texture_frag_spv, native_texture_frag_spv_len, "fs_native",
        SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
    colorFragment = ModernNativeCreateShader(
        native_color_frag_spv, native_color_frag_spv_len, "fs_native_color",
        SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    if (vertex != NULL && textureFragment != NULL) {
        s_texturedOpaque = ModernNativeCreatePipeline(vertex, textureFragment, 0);
        s_texturedTransparent = ModernNativeCreatePipeline(
            vertex, textureFragment, 1);
    }
    if (vertex != NULL && colorFragment != NULL)
        s_colorOpaque = ModernNativeCreatePipeline(vertex, colorFragment, 0);
    if (vertex != NULL) SDL_ReleaseGPUShader(s_device, vertex);
    if (textureFragment != NULL)
        SDL_ReleaseGPUShader(s_device, textureFragment);
    if (colorFragment != NULL) SDL_ReleaseGPUShader(s_device, colorFragment);

    buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer.size = MODERN_NATIVE_MAX_VERTICES * sizeof(RageNativeDrawVertex);
    s_vertexBuffer = SDL_CreateGPUBuffer(s_device, &buffer);
    transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer.size = buffer.size;
    s_vertexTransfer = SDL_CreateGPUTransferBuffer(s_device, &transfer);
    sampler.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    s_sampler = SDL_CreateGPUSampler(s_device, &sampler);
    s_vertices = malloc(MODERN_NATIVE_MAX_VERTICES * sizeof(*s_vertices));
    s_spans = malloc(MODERN_NATIVE_MAX_SPANS * sizeof(*s_spans));
    if (s_texturedOpaque == NULL || s_texturedTransparent == NULL ||
        s_colorOpaque == NULL || s_vertexBuffer == NULL ||
        s_vertexTransfer == NULL || s_sampler == NULL || s_vertices == NULL ||
        s_spans == NULL) {
        fprintf(stderr, "rage-port: native GPU setup failed: %s\n", SDL_GetError());
        ModernNativeGpuShutdown();
        return 0;
    }
    fprintf(stderr, "rage-port: native GPU pipeline ready\n");
    return 1;
}

void ModernNativeGpuPrepare(const RageRenderWorld *world, float aspect) {
    uint32_t instance;
    if (s_vertices == NULL || s_spans == NULL || world == NULL ||
        world->frame == s_worldFrame) return;
    ModernAssetsWarmWorld(world);
    s_vertexCount = RageRenderBuildNativeDraws(
        world, aspect, ModernAssetsMeshLookup, NULL, s_vertices,
        MODERN_NATIVE_MAX_VERTICES, s_spans, MODERN_NATIVE_MAX_SPANS,
        &s_spanCount);
    s_world = world;
    s_worldFrame = world->frame;
    s_aspect = aspect;
    s_completeWorld = world->instanceCount != 0 && world->overflowCount == 0;
    for (instance = 0; instance < world->instanceCount; instance++) {
        if (ModernAssetsFind(&world->instances[instance]) == NULL) {
            s_completeWorld = 0;
            break;
        }
    }
    if (getenv("RAGE_PORT_MODERN_ASSET_TRACE") != NULL) {
        fprintf(stderr,
                "rage-port: native world frame=%llu camera=%u instances=%u "
                "cached=%u vertices=%u spans=%u\n",
                (unsigned long long)world->frame, (unsigned)world->hasCamera,
                world->instanceCount, ModernAssetsCachedMeshCount(),
                s_vertexCount, s_spanCount);
    }
}

int ModernNativeGpuHasDraws(void) {
    return s_vertexCount != 0 && s_world != NULL && s_world->hasCamera;
}

int ModernNativeGpuCanReplaceWorld(void) {
    return ModernNativeGpuHasDraws() && s_completeWorld;
}

static ModernNativeTexture *ModernNativeFindTexture(
    const RageNativeDrawSpan *span) {
    uint32_t index;
    for (index = 0; index < s_textureCount; index++) {
        ModernNativeTexture *entry = &s_textures[index];
        if (entry->assetKey == span->assetKey &&
            entry->assetSet == span->assetSet &&
            entry->material == span->material && entry->entity == span->entity)
            return entry;
    }
    return NULL;
}

static ModernNativeTexture *ModernNativeLoadTexture(
    SDL_GPUCommandBuffer *command, const RageNativeDrawSpan *span) {
    ModernNativeTexture *entry = ModernNativeFindTexture(span);
    RageRenderMeshInstance instance = {0};
    const void *pixels;
    size_t size, byte;
    if (entry != NULL || span->material == UINT32_MAX) return entry;
    if (s_textureCount == MODERN_NATIVE_MAX_TEXTURES) return NULL;
    instance.assetKey = span->assetKey;
    instance.assetSet = span->assetSet;
    if (!ModernAssetsLoadMaterialPixels(&instance, span->material,
                                        &pixels, &size)) return NULL;
    entry = &s_textures[s_textureCount];
    {
        SDL_GPUTextureCreateInfo texture = {0};
        SDL_GPUTransferBufferCreateInfo transfer = {0};
        texture.type = SDL_GPU_TEXTURETYPE_2D;
        texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texture.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texture.width = 256;
        texture.height = 256;
        texture.layer_count_or_depth = 1;
        texture.num_levels = 9;
        entry->texture = SDL_CreateGPUTexture(s_device, &texture);
        transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer.size = (Uint32)size;
        entry->transfer = SDL_CreateGPUTransferBuffer(s_device, &transfer);
    }
    if (entry->texture == NULL || entry->transfer == NULL) goto fail;
    {
        void *mapped = SDL_MapGPUTransferBuffer(s_device, entry->transfer, false);
        SDL_GPUTextureTransferInfo source = {
            .transfer_buffer = entry->transfer,
            .pixels_per_row = 256,
            .rows_per_layer = 256,
        };
        SDL_GPUTextureRegion destination = {
            .texture = entry->texture, .w = 256, .h = 256, .d = 1,
        };
        SDL_GPUCopyPass *copy;
        if (mapped == NULL) goto fail;
        memcpy(mapped, pixels, size);
        SDL_UnmapGPUTransferBuffer(s_device, entry->transfer);
        copy = SDL_BeginGPUCopyPass(command);
        SDL_UploadToGPUTexture(copy, &source, &destination, false);
        SDL_EndGPUCopyPass(copy);
        SDL_GenerateMipmapsForGPUTexture(command, entry->texture);
    }
    entry->transparent = 0;
    for (byte = 3; byte < size; byte += 4) {
        uint8_t alpha = ((const uint8_t *)pixels)[byte];
        if (alpha != 0 && alpha != 255) {
            entry->transparent = 1;
            break;
        }
    }
    ModernAssetsFreeMaterialPixels(pixels);
    entry->assetKey = span->assetKey;
    entry->assetSet = span->assetSet;
    entry->material = span->material;
    entry->entity = span->entity;
    s_textureCount++;
    return entry;
fail:
    ModernAssetsFreeMaterialPixels(pixels);
    if (entry->texture != NULL) SDL_ReleaseGPUTexture(s_device, entry->texture);
    if (entry->transfer != NULL)
        SDL_ReleaseGPUTransferBuffer(s_device, entry->transfer);
    memset(entry, 0, sizeof(*entry));
    return NULL;
}

void ModernNativeGpuDraw(SDL_GPUCommandBuffer *command,
                         SDL_GPUTexture *colorTarget,
                         SDL_GPUTexture *depthTarget,
                         int clearColor) {
    SDL_GPUColorTargetInfo color = {
        .texture = colorTarget,
        .clear_color = {0.0f, 0.0f, 0.0f, 1.0f},
        .load_op = clearColor ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPUDepthStencilTargetInfo depth = {
        .texture = depthTarget,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
    };
    ModernNativeCameraUniform camera;
    SDL_GPURenderPass *pass;
    uint32_t spanIndex;
    uint32_t drawCount = 0;
    if (!ModernNativeGpuHasDraws()) return;
    for (spanIndex = 0; spanIndex < s_spanCount; spanIndex++)
        (void)ModernNativeLoadTexture(command, &s_spans[spanIndex]);
    {
        void *mapped = SDL_MapGPUTransferBuffer(s_device, s_vertexTransfer, true);
        SDL_GPUCopyPass *copy;
        SDL_GPUTransferBufferLocation source = {
            .transfer_buffer = s_vertexTransfer, .offset = 0};
        SDL_GPUBufferRegion destination = {
            .buffer = s_vertexBuffer, .offset = 0,
            .size = s_vertexCount * sizeof(*s_vertices)};
        if (mapped == NULL) return;
        memcpy(mapped, s_vertices, destination.size);
        SDL_UnmapGPUTransferBuffer(s_device, s_vertexTransfer);
        copy = SDL_BeginGPUCopyPass(command);
        SDL_UploadToGPUBuffer(copy, &source, &destination, true);
        SDL_EndGPUCopyPass(copy);
    }
    pass = SDL_BeginGPURenderPass(command, &color, 1, &depth);
    if (pass == NULL) return;
    ModernNativeBuildCamera(&s_world->camera, &camera);
    camera.projection[0] /= s_aspect;
    SDL_PushGPUVertexUniformData(command, 0, &camera, sizeof(camera));
    {
        SDL_GPUBufferBinding vertex = {.buffer = s_vertexBuffer, .offset = 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vertex, 1);
    }
    for (int transparent = 0; transparent < 2; transparent++) {
        SDL_GPUGraphicsPipeline *boundPipeline = NULL;
        SDL_GPUTexture *boundTexture = NULL;
        for (spanIndex = 0; spanIndex < s_spanCount; spanIndex++) {
            const RageNativeDrawSpan *span = &s_spans[spanIndex];
            ModernNativeTexture *texture;
            SDL_GPUGraphicsPipeline *pipeline;
            if (span->pass != RAGE_RENDER_PASS_MAIN || span->vertexCount == 0)
                continue;
            if (span->material == UINT32_MAX) {
                if (transparent) continue;
                pipeline = s_colorOpaque;
                texture = NULL;
            } else {
                texture = ModernNativeFindTexture(span);
                if (texture == NULL || texture->transparent != transparent)
                    continue;
                pipeline = transparent ? s_texturedTransparent : s_texturedOpaque;
            }
            if (pipeline != boundPipeline) {
                SDL_BindGPUGraphicsPipeline(pass, pipeline);
                boundPipeline = pipeline;
            }
            if (texture != NULL && texture->texture != boundTexture) {
                SDL_GPUTextureSamplerBinding binding = {
                    .texture = texture->texture, .sampler = s_sampler};
                SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
                boundTexture = texture->texture;
            }
            SDL_DrawGPUPrimitives(pass, span->vertexCount, 1,
                                  span->firstVertex, 0);
            drawCount++;
        }
    }
    SDL_EndGPURenderPass(pass);
    if (drawCount != 0 && getenv("RAGE_PORT_MODERN_ASSET_TRACE") != NULL) {
        fprintf(stderr, "rage-port: native draws frame=%llu draws=%u vertices=%u\n",
                (unsigned long long)s_worldFrame, drawCount, s_vertexCount);
    }
}

void ModernNativeGpuShutdown(void) {
    uint32_t index;
    if (s_device != NULL) {
        for (index = 0; index < s_textureCount; index++) {
            if (s_textures[index].texture != NULL)
                SDL_ReleaseGPUTexture(s_device, s_textures[index].texture);
            if (s_textures[index].transfer != NULL)
                SDL_ReleaseGPUTransferBuffer(s_device, s_textures[index].transfer);
        }
        if (s_texturedOpaque != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_texturedOpaque);
        if (s_texturedTransparent != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_texturedTransparent);
        if (s_colorOpaque != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_colorOpaque);
        if (s_vertexBuffer != NULL) SDL_ReleaseGPUBuffer(s_device, s_vertexBuffer);
        if (s_vertexTransfer != NULL)
            SDL_ReleaseGPUTransferBuffer(s_device, s_vertexTransfer);
        if (s_sampler != NULL) SDL_ReleaseGPUSampler(s_device, s_sampler);
    }
    free(s_vertices);
    free(s_spans);
    memset(s_textures, 0, sizeof(s_textures));
    s_device = NULL;
    s_texturedOpaque = NULL;
    s_texturedTransparent = NULL;
    s_colorOpaque = NULL;
    s_vertexBuffer = NULL;
    s_vertexTransfer = NULL;
    s_sampler = NULL;
    s_vertices = NULL;
    s_spans = NULL;
    s_vertexCount = 0;
    s_spanCount = 0;
    s_worldFrame = UINT64_MAX;
    s_world = NULL;
    s_aspect = 4.0f / 3.0f;
    s_completeWorld = 0;
    s_textureCount = 0;
}
