#include "modern_native_gpu.h"
#include "../runtime_config.h"

#include "modern_assets.h"
#include "render/render_mesh_build.h"
#include "render/render_shadow.h"
#include "render/texture_mipmap.h"
#include "rage/track_asset_identity.h"

#include "shaders/native_color_frag_spv.h"
#include "shaders/native_sky_frag_spv.h"
#include "shaders/native_sky_vert_spv.h"
#include "shaders/native_texture_frag_spv.h"
#include "shaders/native_vert_spv.h"
#include "shaders/native_shadow_frag_spv.h"
#include "shaders/native_shadow_masked_frag_spv.h"
#include "shaders/native_shadow_vert_spv.h"

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
    MODERN_NATIVE_TEXTURE_HASH_SIZE = 4096,
};

typedef struct ModernNativeCameraUniform {
    float position[4];
    float viewRow0[4];
    float viewRow1[4];
    float viewRow2[4];
    float projection[4];
} ModernNativeCameraUniform;

typedef struct ModernNativeSkyUniform {
    float top[4];
    float middle[4];
    float horizon[4];
    float bottom[4];
} ModernNativeSkyUniform;

typedef struct ModernNativeLightUniform {
    float direction[4];
    float ambient[4];
    float diffuse[4];
    float skyTop[4];
    float skyHorizon[4];
    float skyBottom[4];
} ModernNativeLightUniform;

typedef struct ModernNativeMaterialUniform {
    float baseColor[4];
    float emissiveAndShading[4];
    float surface[4];
} ModernNativeMaterialUniform;

typedef struct ModernNativeTexture {
    uint32_t assetKey;
    uint32_t material;
    uint8_t materialVariant;
    uint8_t hasCarPaint;
    uint8_t carPaintColor1;
    uint8_t carPaintColor2;
    RageRenderAssetSet assetSet;
    RageRenderMaterial definition;
    int transparent;
    SDL_GPUTexture *texture;
} ModernNativeTexture;

static const char MODERN_NATIVE_MSL[] =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct NativeIn { float3 pos [[attribute(0)]]; float2 uv [[attribute(1)]]; uchar4 color [[attribute(2)]]; float3 normal [[attribute(3)]]; float4 fog [[attribute(4)]]; float lighting [[attribute(5)]]; float depthBias [[attribute(6)]]; float3 environmentLight [[attribute(7)]]; float shadowReception [[attribute(8)]]; };\n"
    "struct NativeCamera { float4 position; float4 viewRow0; float4 viewRow1; float4 viewRow2; float4 projection; };\n"
    "struct NativeOut { float4 pos [[position]]; float2 uv; float4 color; float3 normal; float4 fog; float lighting; float3 environmentLight; float3 shadowCoord; float shadowReception; float3 viewDirection; };\n"
    "struct NativeSkyOut { float4 pos [[position]]; float3 direction; };\n"
    "struct NativeSkyColors { float4 top; float4 middle; float4 horizon; float4 bottom; };\n"
    "struct NativeSceneLight { float4 direction; float4 ambient; float4 diffuse; float4 skyTop; float4 skyHorizon; float4 skyBottom; };\n"
    "struct NativeMaterial { float4 baseColor; float4 emissiveAndShading; float4 surface; };\n"
    "struct ShadowOut { float4 pos [[position]]; float2 uv; };\n"
    "vertex NativeOut vs_native(NativeIn in [[stage_in]], constant NativeCamera &camera [[buffer(0)]], constant NativeCamera &shadow [[buffer(1)]]) { NativeOut o; float3 p=in.pos-camera.position.xyz; float3 v=float3(dot(camera.viewRow0.xyz,p),dot(camera.viewRow1.xyz,p),dot(camera.viewRow2.xyz,p)); float depth=-v.z; float z=depth*camera.projection.z+camera.projection.w+(in.depthBias/1048576.0)*depth; o.pos=float4(v.x*camera.projection.x,v.y*camera.projection.y,z,depth); o.uv=in.uv; o.color=float4(in.color)/255.0; o.normal=in.normal; o.fog=in.fog; o.lighting=in.lighting; o.environmentLight=in.environmentLight; o.shadowReception=in.shadowReception; o.viewDirection=camera.position.xyz-in.pos; float3 sp=in.pos-shadow.position.xyz; float sx=dot(shadow.viewRow0.xyz,sp)*shadow.projection.x; float sy=dot(shadow.viewRow1.xyz,sp)*shadow.projection.y; float sd=-dot(shadow.viewRow2.xyz,sp); o.shadowCoord=float3(sx*0.5+0.5,0.5-sy*0.5,sd*shadow.projection.z+shadow.projection.w); return o; }\n"
    "vertex NativeSkyOut vs_native_sky(uint vertexID [[vertex_id]], constant NativeCamera &camera [[buffer(0)]]) { NativeSkyOut o; float2 corner=float2((vertexID<<1)&2,vertexID&2); float2 clip=corner*2.0-1.0; float3 v=float3(clip.x/camera.projection.x,clip.y/camera.projection.y,-1.0); o.direction=camera.viewRow0.xyz*v.x+camera.viewRow1.xyz*v.y+camera.viewRow2.xyz*v.z; o.pos=float4(clip,1.0,1.0); return o; }\n"
    "fragment float4 fs_native_sky(NativeSkyOut in [[stage_in]], texture2d<float> panorama [[texture(0)]], sampler skySampler [[sampler(0)]], constant NativeSkyColors &sky [[buffer(0)]]) { float3 d=normalize(in.direction); d.y=-d.y; float h=d.y; float horizontalLength=max(length(d.xz),0.001); float verticalSlope=h/horizontalLength; float3 c; if(h>=0.0)c=mix(sky.middle.rgb,sky.top.rgb,smoothstep(0.143,0.165,h)); else if(h>=-0.18)c=mix(sky.middle.rgb,sky.horizon.rgb,smoothstep(0.0,0.18,-h)); else c=mix(sky.horizon.rgb,sky.bottom.rgb,smoothstep(0.18,0.65,-h)); float bandCoordinate=1.0-verticalSlope*2.5; float band=floor(bandCoordinate); float bandOffset=fmod(band,2.0); if(bandOffset<0.0)bandOffset+=2.0; bandOffset*=0.5; float2 uv=float2(fract(atan2(d.z,d.x)*0.6366197724+0.25+bandOffset),fract(bandCoordinate)); float4 authored=panorama.sample(skySampler,uv); float upperHemisphereCoverage=smoothstep(0.0,0.08,verticalSlope); float cylinderCoverage=upperHemisphereCoverage*(1.0-smoothstep(0.9,1.25,verticalSlope)); c=mix(c,authored.rgb,authored.a*sky.bottom.a*cylinderCoverage); return float4(c,1.0); }\n"
    "vertex ShadowOut vs_shadow(NativeIn in [[stage_in]], constant NativeCamera &shadow [[buffer(0)]]) { ShadowOut o; float3 p=in.pos-shadow.position.xyz; float depth=-dot(shadow.viewRow2.xyz,p); o.pos=float4(dot(shadow.viewRow0.xyz,p)*shadow.projection.x,dot(shadow.viewRow1.xyz,p)*shadow.projection.y,depth*shadow.projection.z+shadow.projection.w,1.0); o.uv=in.uv; return o; }\n"
    "fragment void fs_shadow() {}\n"
    "fragment void fs_shadow_masked(ShadowOut in [[stage_in]], texture2d<float> textureImage [[texture(0)]], sampler smp [[sampler(0)]]) { if(textureImage.sample(smp,in.uv).a<=0.5) discard_fragment(); }\n"
    "static float native_shadow(NativeOut in, float3 n, depth2d<float> shadowMap, sampler shadowSampler, constant NativeSceneLight &sceneLight) { if(in.shadowCoord.x<=0.0||in.shadowCoord.x>=1.0||in.shadowCoord.y<=0.0||in.shadowCoord.y>=1.0||in.shadowCoord.z<=0.0||in.shadowCoord.z>=1.0)return 1.0; float facing=max(dot(n,normalize(sceneLight.direction.xyz)),0.0); float bias=mix(0.00025,0.00008,facing); float2 texel=1.0/float2(shadowMap.get_width(),shadowMap.get_height()); float visible=0.0; for(int y=0;y<2;y++){for(int x=0;x<2;x++){float stored=shadowMap.sample(shadowSampler,in.shadowCoord.xy+(float2(x,y)-0.5)*texel);visible+=in.shadowCoord.z-bias<=stored?1.0:0.0;}}return visible*0.25; }\n"
    "static float3 reflected_sky(float3 d, constant NativeSceneLight &sceneLight) { if(d.y>=0.0)return mix(sceneLight.skyHorizon.rgb,sceneLight.skyTop.rgb,smoothstep(0.0,0.8,d.y)); return mix(sceneLight.skyHorizon.rgb,sceneLight.skyBottom.rgb,smoothstep(0.0,0.55,-d.y)); }\n"
    "fragment float4 fs_native(NativeOut in [[stage_in]], texture2d<float> textureImage [[texture(0)]], sampler smp [[sampler(0)]], depth2d<float> shadowMap [[texture(1)]], sampler shadowSampler [[sampler(1)]], constant NativeSceneLight &sceneLight [[buffer(0)]], constant NativeMaterial &material [[buffer(1)]]) { float4 t=textureImage.sample(smp,in.uv); if((material.surface.z>1.5&&material.surface.z<2.5&&t.a<0.5)||t.a<=0.001) discard_fragment(); t.rgb/=t.a; float n2=dot(in.normal,in.normal); float3 n=n2>0.000001 ? in.normal*rsqrt(n2) : float3(0.0,1.0,0.0); float3 v=normalize(in.viewDirection); float3 ld=normalize(sceneLight.direction.xyz); float ndl=max(dot(n,ld),0.0); float materialLighting=in.lighting; if(material.emissiveAndShading.w>=0.0)materialLighting=material.emissiveAndShading.w; float3 light=mix(float3(1.0),in.environmentLight*(sceneLight.ambient.rgb+sceneLight.diffuse.rgb*ndl),materialLighting); float visibility=materialLighting>0.001&&in.shadowReception>0.5?native_shadow(in,n,shadowMap,shadowSampler,sceneLight):1.0; float shade=mix(0.62,1.0,visibility); light*=mix(shade,1.0,in.fog.a); float3 fogged=mix(in.color.rgb,in.fog.rgb,in.fog.a); float3 modulation=min(fogged*2.0,float3(1.0)); float3 base=t.rgb*modulation*light*material.baseColor.rgb; float roughness=clamp(material.surface.x,0.0,1.0); float metallic=clamp(material.surface.y,0.0,1.0); float gloss=1.0-roughness; float coat=smoothstep(0.18,0.55,gloss)*material.surface.w; float3 h=normalize(ld+v); float ndv=max(dot(n,v),0.001); float ndh=max(dot(n,h),0.0); float vdh=max(dot(v,h),0.0); float3 materialColor=t.rgb*material.baseColor.rgb; float3 f0=mix(float3(0.06),materialColor,metallic); float3 fresnel=f0+(float3(1.0)-f0)*pow(1.0-vdh,5.0); float alpha=max(roughness*roughness,0.025); float alpha2=alpha*alpha; float dd=ndh*ndh*(alpha2-1.0)+1.0; float distribution=alpha2/max(3.14159265*dd*dd,0.0001); float k=(roughness+1.0)*(roughness+1.0)*0.125; float gv=ndv/(ndv*(1.0-k)+k); float gl=ndl/(ndl*(1.0-k)+k); float3 directSpecular=sceneLight.diffuse.rgb*fresnel*distribution*gv*gl*ndl/max(4.0*ndv*ndl,0.001); float rim=pow(1.0-ndv,5.0); float zoneReflection=smoothstep(0.30,0.85,min(in.environmentLight.r,min(in.environmentLight.g,in.environmentLight.b))); float reflectionStrength=coat*zoneReflection*mix(0.10,0.55,rim)*mix(0.85,1.15,metallic); float3 reflected=reflected_sky(reflect(-v,n),sceneLight); float reflectedLuminance=dot(reflected,float3(0.2126,0.7152,0.0722)); reflected=mix(float3(reflectedLuminance),reflected,0.65); float3 environmentSpecular=reflected*reflectionStrength; directSpecular*=coat*zoneReflection; float3 specular=(environmentSpecular+directSpecular)*step(0.001,materialLighting); float3 emissive=t.rgb*material.emissiveAndShading.rgb; float4 c=float4(base+specular+emissive,t.a*in.color.a*material.baseColor.a); if(c.a<=0.001) discard_fragment(); return c; }\n"
    "fragment float4 fs_native_color(NativeOut in [[stage_in]], depth2d<float> shadowMap [[texture(0)]], sampler shadowSampler [[sampler(0)]], constant NativeSceneLight &sceneLight [[buffer(0)]]) { float n2=dot(in.normal,in.normal); float3 n=n2>0.000001 ? in.normal*rsqrt(n2) : float3(0.0,1.0,0.0); float ndl=max(dot(n,normalize(sceneLight.direction.xyz)),0.0); float3 light=mix(float3(1.0),in.environmentLight*(sceneLight.ambient.rgb+sceneLight.diffuse.rgb*ndl),in.lighting); float visibility=in.shadowReception>0.5?native_shadow(in,n,shadowMap,shadowSampler,sceneLight):1.0; float shade=mix(0.62,1.0,visibility); light*=mix(shade,1.0,in.fog.a); float3 fogged=mix(in.color.rgb,in.fog.rgb,in.fog.a); return float4(fogged*light,in.color.a); }\n";

static SDL_GPUDevice *s_device;
static SDL_GPUGraphicsPipeline *s_texturedOpaque;
static SDL_GPUGraphicsPipeline *s_texturedTransparent;
static SDL_GPUGraphicsPipeline *s_texturedOpaqueDecal;
static SDL_GPUGraphicsPipeline *s_texturedTransparentDecal;
static SDL_GPUGraphicsPipeline *s_colorOpaque;
static SDL_GPUGraphicsPipeline *s_colorOpaqueDecal;
static SDL_GPUGraphicsPipeline *s_sky;
static SDL_GPUTexture *s_skyTexture;
static SDL_GPUSampler *s_skySampler;
static uint32_t s_skyAssetKey = UINT32_MAX;
static int s_skyHasPanorama;
static uint32_t s_skyRetryFrames;
static SDL_GPUGraphicsPipeline *s_shadowDepth;
static SDL_GPUGraphicsPipeline *s_shadowMasked;
static SDL_GPUBuffer *s_vertexBuffer;
static SDL_GPUTransferBuffer *s_vertexTransfer;
static SDL_GPUSampler *s_sampler;
static SDL_GPUTexture *s_shadowTexture;
static SDL_GPUSampler *s_shadowSampler;
static RageNativeDrawVertex *s_vertices;
static RageNativeDrawSpan *s_spans;
static RageNativeDrawSpan *s_mirrorSpans;
static uint32_t s_vertexCount;
static uint32_t s_spanCount;
static uint32_t s_mirrorVertexCount;
static uint32_t s_mirrorSpanCount;
static uint64_t s_worldFrame = UINT64_MAX;
static const RageRenderWorld *s_world;
static float s_aspect = 4.0f / 3.0f;
static float s_mirrorAspect = 148.0f / 36.0f;
static int s_completeWorld;
static ModernNativeTexture s_textures[MODERN_NATIVE_MAX_TEXTURES];

/*
 * Transfer buffers a texture upload has been recorded from, but whose command
 * buffer this module does not submit: the caller does, after it has finished
 * with the frame. Releasing one before that submission leaves the copy
 * reading memory that has been handed back, which Metal tolerates and Vulkan
 * does not, and which shows up as textures that are corrupt from the moment
 * they are first uploaded. They are released a frame later, by which time the
 * work that reads them has certainly been submitted.
 */
enum { MODERN_NATIVE_MAX_PENDING_UPLOADS = 256 };
static SDL_GPUTransferBuffer
    *s_pendingUploads[MODERN_NATIVE_MAX_PENDING_UPLOADS];
static uint32_t s_pendingUploadCount;

static void ModernNativeReleasePendingUploads(void) {
    uint32_t index;
    for (index = 0; index < s_pendingUploadCount; index++) {
        if (s_device != NULL && s_pendingUploads[index] != NULL)
            SDL_ReleaseGPUTransferBuffer(s_device, s_pendingUploads[index]);
        s_pendingUploads[index] = NULL;
    }
    s_pendingUploadCount = 0;
}

/* Hand a transfer buffer over to be released once the frame it belongs to has
 * been submitted. A full list means this frame uploaded more textures than
 * the list holds, so the oldest is waited out rather than leaked. */
static void ModernNativeRetireUpload(SDL_GPUTransferBuffer *upload) {
    if (upload == NULL) return;
    if (s_pendingUploadCount == MODERN_NATIVE_MAX_PENDING_UPLOADS) {
        if (s_device != NULL) {
            SDL_WaitForGPUIdle(s_device);
            ModernNativeReleasePendingUploads();
        }
    }
    if (s_pendingUploadCount < MODERN_NATIVE_MAX_PENDING_UPLOADS)
        s_pendingUploads[s_pendingUploadCount++] = upload;
}
static uint16_t s_textureHash[MODERN_NATIVE_TEXTURE_HASH_SIZE];
static uint32_t s_textureCount;
static uint64_t s_trackAssetRevision = UINT64_MAX;
static RageRenderShadowMap s_shadowMap;
static int s_haveShadowMap;

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
        {.location = 4, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
         .offset = offsetof(RageNativeDrawVertex, fog)},
        {.location = 5, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
         .offset = offsetof(RageNativeDrawVertex, lighting)},
        {.location = 6, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
         .offset = offsetof(RageNativeDrawVertex, depthBias)},
        {.location = 7, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
         .offset = offsetof(RageNativeDrawVertex, environmentLight)},
        {.location = 8, .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
         .offset = offsetof(RageNativeDrawVertex, shadowReception)},
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
    info.vertex_input_state.num_vertex_attributes =
        sizeof(attributes) / sizeof(attributes[0]);
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    /* PS1 assets contain deliberately mixed and double-sided face winding,
     * including thin sign supports, so native culling would remove geometry
     * that the original GPU draws. */
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    /* Semantic decals are already real, slightly lifted world geometry. They
     * use the same depth test as every other opaque surface. */
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    info.depth_stencil_state.enable_depth_test = true;
    /* Every opaque surface must participate in the Z buffer, including the
     * later coplanar-detail phase. Imported terrain OT bias is not a semantic
     * decal marker: tunnel and road faces can carry it too. Leaving that
     * phase read-only lets a later, farther face overwrite nearer geometry. */
    info.depth_stencil_state.enable_depth_write = !transparent;
    info.target_info.color_target_descriptions = &color;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    return SDL_CreateGPUGraphicsPipeline(s_device, &info);
}

static SDL_GPUGraphicsPipeline *ModernNativeCreateShadowPipeline(
    SDL_GPUShader *vertex, SDL_GPUShader *fragment) {
    const SDL_GPUVertexBufferDescription buffer = {
        .slot = 0,
        .pitch = sizeof(RageNativeDrawVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    const SDL_GPUVertexAttribute attributes[] = {
        {.location = 0,
         .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
         .offset = offsetof(RageNativeDrawVertex, position)},
        {.location = 1,
         .buffer_slot = 0,
         .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
         .offset = offsetof(RageNativeDrawVertex, uv)},
    };
    SDL_GPUGraphicsPipelineCreateInfo info = {0};
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attributes;
    info.vertex_input_state.num_vertex_attributes =
        sizeof(attributes) / sizeof(attributes[0]);
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    return SDL_CreateGPUGraphicsPipeline(s_device, &info);
}

static SDL_GPUGraphicsPipeline *ModernNativeCreateSkyPipeline(
    SDL_GPUShader *vertex, SDL_GPUShader *fragment) {
    SDL_GPUColorTargetDescription color = {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    };
    SDL_GPUGraphicsPipelineCreateInfo info = {0};
    info.vertex_shader = vertex;
    info.fragment_shader = fragment;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
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
    RenderPerspectiveDepthTerms(camera, &out->projection[2],
                                    &out->projection[3]);
}

static void ModernNativeBuildSky(const RageRenderCamera *camera,
                                 ModernNativeSkyUniform *out) {
    memset(out, 0, sizeof(*out));
    out->top[0] = camera->skyTopColor.x;
    out->top[1] = camera->skyTopColor.y;
    out->top[2] = camera->skyTopColor.z;
    out->top[3] = 1.0f;
    out->middle[0] = camera->skyColor.x;
    out->middle[1] = camera->skyColor.y;
    out->middle[2] = camera->skyColor.z;
    out->middle[3] = 1.0f;
    out->horizon[0] = camera->skyHorizonColor.x;
    out->horizon[1] = camera->skyHorizonColor.y;
    out->horizon[2] = camera->skyHorizonColor.z;
    out->horizon[3] = 1.0f;
    out->bottom[0] = camera->skyBottomColor.x;
    out->bottom[1] = camera->skyBottomColor.y;
    out->bottom[2] = camera->skyBottomColor.z;
    /* Alpha is backend metadata here: it enables the optional imported
     * panorama while RGB remains the authored lower sky band. */
    /* Alpha enables the imported panorama. Until the flip above, the
     * coverage term it is multiplied by was always zero, so no panorama has
     * ever been shown; with the axis right it would appear everywhere above
     * the horizon, and the game's own sky has no cloud there. Where its band
     * really belongs is a question for whoever brings it back. */
    out->bottom[3] = 0.0f;
}

static void ModernNativeBuildLight(const RageRenderDirectionalLight *light,
                                   const RageRenderCamera *camera,
                                   ModernNativeLightUniform *out) {
    memset(out, 0, sizeof(*out));
    out->direction[0] = light->direction.x;
    out->direction[1] = light->direction.y;
    out->direction[2] = light->direction.z;
    out->ambient[0] = light->ambientColor.x;
    out->ambient[1] = light->ambientColor.y;
    out->ambient[2] = light->ambientColor.z;
    out->diffuse[0] = light->diffuseColor.x;
    out->diffuse[1] = light->diffuseColor.y;
    out->diffuse[2] = light->diffuseColor.z;
    out->skyTop[0] = camera->skyTopColor.x;
    out->skyTop[1] = camera->skyTopColor.y;
    out->skyTop[2] = camera->skyTopColor.z;
    out->skyHorizon[0] = camera->skyHorizonColor.x;
    out->skyHorizon[1] = camera->skyHorizonColor.y;
    out->skyHorizon[2] = camera->skyHorizonColor.z;
    out->skyBottom[0] = camera->skyBottomColor.x;
    out->skyBottom[1] = camera->skyBottomColor.y;
    out->skyBottom[2] = camera->skyBottomColor.z;
}

static void ModernNativeBuildMaterial(const RageRenderMaterial *material,
                                      int allowClearcoat,
                                      ModernNativeMaterialUniform *out) {
    memset(out, 0, sizeof(*out));
    memcpy(out->baseColor, material->baseColorFactor,
           sizeof(out->baseColor));
    memcpy(out->emissiveAndShading, material->emissiveFactor,
           sizeof(material->emissiveFactor));
    out->emissiveAndShading[3] = -1.0f;
    if (material->shading == RAGE_RENDER_MATERIAL_SHADING_LIT)
        out->emissiveAndShading[3] = 1.0f;
    else if (material->shading == RAGE_RENDER_MATERIAL_SHADING_UNLIT)
        out->emissiveAndShading[3] = 0.0f;
    out->surface[0] = material->roughness;
    out->surface[1] = material->metallic;
    out->surface[2] = (float)material->alphaMode;
    out->surface[3] = allowClearcoat ? 1.0f : 0.0f;
}

static void ModernNativeBuildShadowCamera(
    const RageRenderShadowMap *shadow, ModernNativeCameraUniform *out) {
    memset(out, 0, sizeof(*out));
    out->position[0] = shadow->position.x;
    out->position[1] = shadow->position.y;
    out->position[2] = shadow->position.z;
    out->viewRow0[0] = shadow->row0.x;
    out->viewRow0[1] = shadow->row0.y;
    out->viewRow0[2] = shadow->row0.z;
    out->viewRow1[0] = shadow->row1.x;
    out->viewRow1[1] = shadow->row1.y;
    out->viewRow1[2] = shadow->row1.z;
    out->viewRow2[0] = shadow->row2.x;
    out->viewRow2[1] = shadow->row2.y;
    out->viewRow2[2] = shadow->row2.z;
    out->projection[0] = shadow->scaleX;
    out->projection[1] = shadow->scaleY;
    out->projection[2] = shadow->depthScale;
    out->projection[3] = shadow->depthOffset;
}

int ModernNativeGpuInit(SDL_GPUDevice *device) {
    SDL_GPUShader *vertex = NULL, *skyVertex = NULL, *shadowVertex = NULL;
    SDL_GPUShader *skyFragment = NULL;
    SDL_GPUShader *shadowFragment = NULL, *shadowMaskedFragment = NULL;
    SDL_GPUShader *textureFragment = NULL, *colorFragment = NULL;
    SDL_GPUBufferCreateInfo buffer = {0};
    SDL_GPUTransferBufferCreateInfo transfer = {0};
    SDL_GPUSamplerCreateInfo sampler = {0};
    if (!ModernAssetsReady()) return 0;
    s_device = device;
    vertex = ModernNativeCreateShader(
        native_vert_spv, native_vert_spv_len, "vs_native",
        SDL_GPU_SHADERSTAGE_VERTEX, 0, 2);
    skyVertex = ModernNativeCreateShader(
        native_sky_vert_spv, native_sky_vert_spv_len, "vs_native_sky",
        SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    skyFragment = ModernNativeCreateShader(
        native_sky_frag_spv, native_sky_frag_spv_len, "fs_native_sky",
        SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
    shadowVertex = ModernNativeCreateShader(
        native_shadow_vert_spv, native_shadow_vert_spv_len, "vs_shadow",
        SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    shadowFragment = ModernNativeCreateShader(
        native_shadow_frag_spv, native_shadow_frag_spv_len, "fs_shadow",
        SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    shadowMaskedFragment = ModernNativeCreateShader(
        native_shadow_masked_frag_spv, native_shadow_masked_frag_spv_len,
        "fs_shadow_masked", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
    textureFragment = ModernNativeCreateShader(
        native_texture_frag_spv, native_texture_frag_spv_len, "fs_native",
        SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 2);
    colorFragment = ModernNativeCreateShader(
        native_color_frag_spv, native_color_frag_spv_len, "fs_native_color",
        SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
    if (vertex != NULL && textureFragment != NULL) {
        s_texturedOpaque = ModernNativeCreatePipeline(
            vertex, textureFragment, 0);
        s_texturedTransparent = ModernNativeCreatePipeline(
            vertex, textureFragment, 1);
        s_texturedOpaqueDecal = ModernNativeCreatePipeline(
            vertex, textureFragment, 0);
        s_texturedTransparentDecal = ModernNativeCreatePipeline(
            vertex, textureFragment, 1);
    }
    if (vertex != NULL && colorFragment != NULL) {
        s_colorOpaque = ModernNativeCreatePipeline(vertex, colorFragment, 0);
        s_colorOpaqueDecal = ModernNativeCreatePipeline(
            vertex, colorFragment, 0);
    }
    if (skyVertex != NULL && skyFragment != NULL)
        s_sky = ModernNativeCreateSkyPipeline(skyVertex, skyFragment);
    if (shadowVertex != NULL && shadowFragment != NULL)
        s_shadowDepth = ModernNativeCreateShadowPipeline(
            shadowVertex, shadowFragment);
    if (shadowVertex != NULL && shadowMaskedFragment != NULL)
        s_shadowMasked = ModernNativeCreateShadowPipeline(
            shadowVertex, shadowMaskedFragment);
    if (vertex != NULL) SDL_ReleaseGPUShader(s_device, vertex);
    if (skyVertex != NULL) SDL_ReleaseGPUShader(s_device, skyVertex);
    if (skyFragment != NULL) SDL_ReleaseGPUShader(s_device, skyFragment);
    if (shadowVertex != NULL) SDL_ReleaseGPUShader(s_device, shadowVertex);
    if (shadowFragment != NULL)
        SDL_ReleaseGPUShader(s_device, shadowFragment);
    if (shadowMaskedFragment != NULL)
        SDL_ReleaseGPUShader(s_device, shadowMaskedFragment);
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
    /* The CPU supplies a deliberately bounded atlas-safe mip chain. Blend
     * adjacent levels to avoid visible transitions on long road surfaces. */
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.max_anisotropy = 8.0f;
    sampler.enable_anisotropy = true;
    s_sampler = SDL_CreateGPUSampler(s_device, &sampler);
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.max_anisotropy = 1.0f;
    sampler.enable_anisotropy = false;
    s_skySampler = SDL_CreateGPUSampler(s_device, &sampler);
    {
        SDL_GPUTextureCreateInfo texture = {0};
        SDL_GPUSamplerCreateInfo shadowSampler = {0};
        texture.type = SDL_GPU_TEXTURETYPE_2D;
        texture.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        texture.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET |
                        SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texture.width = RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION;
        texture.height = RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION;
        texture.layer_count_or_depth = 1;
        texture.num_levels = 1;
        s_shadowTexture = SDL_CreateGPUTexture(s_device, &texture);
        shadowSampler.min_filter = SDL_GPU_FILTER_NEAREST;
        shadowSampler.mag_filter = SDL_GPU_FILTER_NEAREST;
        shadowSampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        shadowSampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        shadowSampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        shadowSampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        s_shadowSampler = SDL_CreateGPUSampler(s_device, &shadowSampler);
    }
    s_vertices = malloc(MODERN_NATIVE_MAX_VERTICES * sizeof(*s_vertices));
    s_spans = malloc(MODERN_NATIVE_MAX_SPANS * sizeof(*s_spans));
    s_mirrorSpans = malloc(MODERN_NATIVE_MAX_SPANS * sizeof(*s_mirrorSpans));
    if (s_texturedOpaque == NULL || s_texturedTransparent == NULL ||
        s_texturedOpaqueDecal == NULL ||
        s_texturedTransparentDecal == NULL ||
        s_colorOpaque == NULL ||
        s_colorOpaqueDecal == NULL ||
        s_sky == NULL ||
        s_shadowDepth == NULL || s_shadowMasked == NULL ||
        s_shadowTexture == NULL ||
        s_shadowSampler == NULL ||
        s_skySampler == NULL ||
        s_vertexBuffer == NULL ||
        s_vertexTransfer == NULL || s_sampler == NULL || s_vertices == NULL ||
        s_spans == NULL || s_mirrorSpans == NULL) {
        fprintf(stderr, "rage-port: native GPU setup failed: %s\n", SDL_GetError());
        ModernNativeGpuShutdown();
        return 0;
    }
    fprintf(stderr, "rage-port: native GPU pipeline ready\n");
    return 1;
}

static void ModernNativeGpuClearTextures(void) {
    uint32_t index;
    if (s_device != NULL) {
        for (index = 0; index < s_textureCount; index++) {
            if (s_textures[index].texture != NULL)
                SDL_ReleaseGPUTexture(s_device, s_textures[index].texture);
        }
    }
    /* The uploads still in flight read these textures. */
    if (s_device != NULL) SDL_WaitForGPUIdle(s_device);
    ModernNativeReleasePendingUploads();
    memset(s_textures, 0, sizeof(s_textures));
    memset(s_textureHash, 0, sizeof(s_textureHash));
    s_textureCount = 0;
}

static void ModernNativeReleaseSkyTexture(void) {
    if (s_device != NULL) {
        if (s_skyTexture != NULL)
            SDL_ReleaseGPUTexture(s_device, s_skyTexture);
    }
    s_skyTexture = NULL;
    s_skyAssetKey = UINT32_MAX;
    s_skyHasPanorama = 0;
    s_skyRetryFrames = 0;
}

static int ModernNativeEnsureSkyTexture(SDL_GPUCommandBuffer *command,
                                        uint32_t assetKey) {
    static const uint8_t transparent[4] = {0, 0, 0, 0};
    ModernAssetImage image = {0};
    SDL_GPUTextureCreateInfo texture = {0};
    SDL_GPUTransferBufferCreateInfo transfer = {0};
    SDL_GPUTextureTransferInfo source = {0};
    SDL_GPUTextureRegion destination = {0};
    SDL_GPUCopyPass *copy;
    SDL_GPUTransferBuffer *upload = NULL;
    const void *pixels = transparent;
    size_t size = sizeof(transparent);
    uint32_t width = 1, height = 1;
    void *mapped;
    int loaded;
    if (s_skyTexture != NULL && s_skyAssetKey == assetKey) {
        /* The first native present can occur while the game's sky atlas is
         * still being uploaded to VRAM. Do not retain that blank import for
         * the rest of the course: keep the gradient briefly, then retry. */
        if (s_skyHasPanorama) return 1;
        if (s_skyRetryFrames != 0) {
            s_skyRetryFrames--;
            return 1;
        }
    }
    ModernNativeReleaseSkyTexture();
    loaded = ModernAssetsLoadSkyImage(assetKey, &image);
    if (loaded) {
        pixels = image.pixels;
        size = image.size;
        width = image.width;
        height = image.height;
    }
    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture.width = width;
    texture.height = height;
    texture.layer_count_or_depth = 1;
    texture.num_levels = 1;
    s_skyTexture = SDL_CreateGPUTexture(s_device, &texture);
    transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer.size = (Uint32)size;
    upload = SDL_CreateGPUTransferBuffer(s_device, &transfer);
    if (s_skyTexture == NULL || upload == NULL) {
        ModernAssetsFreeMaterialImage(&image);
        if (upload != NULL) SDL_ReleaseGPUTransferBuffer(s_device, upload);
        ModernNativeReleaseSkyTexture();
        return 0;
    }
    mapped = SDL_MapGPUTransferBuffer(s_device, upload, true);
    if (mapped == NULL) {
        ModernAssetsFreeMaterialImage(&image);
        SDL_ReleaseGPUTransferBuffer(s_device, upload);
        ModernNativeReleaseSkyTexture();
        return 0;
    }
    memcpy(mapped, pixels, size);
    SDL_UnmapGPUTransferBuffer(s_device, upload);
    source.transfer_buffer = upload;
    source.pixels_per_row = width;
    source.rows_per_layer = height;
    destination.texture = s_skyTexture;
    destination.w = width;
    destination.h = height;
    destination.d = 1;
    copy = SDL_BeginGPUCopyPass(command);
    if (copy == NULL) {
        ModernAssetsFreeMaterialImage(&image);
        SDL_ReleaseGPUTransferBuffer(s_device, upload);
        ModernNativeReleaseSkyTexture();
        return 0;
    }
    SDL_UploadToGPUTexture(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    /* SDL retains resources referenced by an encoded command until the GPU
     * has finished with them. The upload staging copy is never used again,
     * so keeping one beside every resident texture only doubles memory use. */
    SDL_ReleaseGPUTransferBuffer(s_device, upload);
    s_skyAssetKey = assetKey;
    s_skyHasPanorama = loaded;
    s_skyRetryFrames = loaded ? 0 : 30;
    if (RuntimeConfigEnabled("diagnostics.modern_asset_trace")) {
        fprintf(stderr,
                "rage-port: native sky asset=%u panorama=%s %ux%u\n",
                assetKey, loaded ? "loaded" : "gradient", width, height);
    }
    ModernAssetsFreeMaterialImage(&image);
    return 1;
}

void ModernNativeGpuPrepare(const RageRenderWorld *world, float aspect) {
    uint32_t instance;
    uint32_t mirrorFirstVertex;
    RageRenderVec3 shadowCenter;
    uint64_t trackAssetRevision;
    if (s_vertices == NULL || s_spans == NULL || world == NULL ||
        world->frame == s_worldFrame) return;
    /* The frame these belong to has been submitted by now. */
    ModernNativeReleasePendingUploads();
    trackAssetRevision = TrackAssetIdentityRevision();
    if (trackAssetRevision != s_trackAssetRevision) {
        if (RuntimeConfigEnabled("diagnostics.modern_asset_trace") &&
            s_trackAssetRevision != UINT64_MAX) {
            fprintf(stderr,
                    "rage-port: native texture cache reset old=%llu new=%llu "
                    "textures=%u\n",
                    (unsigned long long)s_trackAssetRevision,
                    (unsigned long long)trackAssetRevision, s_textureCount);
        }
        ModernNativeGpuClearTextures();
        s_trackAssetRevision = trackAssetRevision;
    }
    ModernAssetsWarmWorld(world);
    shadowCenter = world->camera.transform.position;
    for (instance = 0; instance < world->instanceCount; instance++) {
        const RageRenderMeshInstance *candidate = &world->instances[instance];
        if (candidate->pass == RAGE_RENDER_PASS_MAIN &&
            candidate->entity == 11 && candidate->component == 0 &&
            candidate->assetSet == RAGE_RENDER_ASSET_MODEL_BANK) {
            shadowCenter = candidate->transform.position;
            break;
        }
    }
    s_haveShadowMap = RenderBuildDirectionalShadowMap(
        &shadowCenter, &world->light.direction,
        RAGE_RENDER_VEHICLE_SHADOW_EXTENT,
        RAGE_RENDER_VEHICLE_SHADOW_RESOLUTION,
        &s_shadowMap);
    s_vertexCount = RenderBuildNativePassDraws(
        world, RAGE_RENDER_PASS_MAIN, aspect, ModernAssetsMeshLookup, NULL,
        s_vertices,
        MODERN_NATIVE_MAX_VERTICES, s_spans, MODERN_NATIVE_MAX_SPANS,
        &s_spanCount);
    s_mirrorVertexCount = 0;
    s_mirrorSpanCount = 0;
    mirrorFirstVertex = s_vertexCount;
    if (world->mirrorActive && world->hasMirrorCamera &&
        mirrorFirstVertex < MODERN_NATIVE_MAX_VERTICES) {
        RageRenderWorld mirrorWorld = *world;
        uint32_t span;
        mirrorWorld.camera = world->mirrorCamera;
        s_mirrorVertexCount = RenderBuildNativePassDraws(
            &mirrorWorld, RAGE_RENDER_PASS_MAIN, s_mirrorAspect,
            ModernAssetsMeshLookup, NULL, s_vertices + mirrorFirstVertex,
            MODERN_NATIVE_MAX_VERTICES - mirrorFirstVertex, s_mirrorSpans,
            MODERN_NATIVE_MAX_SPANS, &s_mirrorSpanCount);
        for (span = 0; span < s_mirrorSpanCount; span++)
            s_mirrorSpans[span].firstVertex += mirrorFirstVertex;
    }
    s_world = world;
    s_worldFrame = world->frame;
    s_aspect = aspect;
    s_completeWorld = world->instanceCount != 0 && world->overflowCount == 0;
    for (instance = 0; instance < world->instanceCount; instance++) {
        /* Native rendering consumes the ordinary semantic scene for both
         * cameras. Missing assets in the deprecated PS1 mirror submission
         * must not disable replacement of that complete main scene. */
        if (world->instances[instance].pass != RAGE_RENDER_PASS_MAIN) continue;
        if (ModernAssetsFind(&world->instances[instance]) == NULL) {
            s_completeWorld = 0;
            break;
        }
    }
    if (RuntimeConfigEnabled("diagnostics.modern_asset_trace")) {
        uint32_t mirrorVehicleSpans = 0;
        uint32_t span;
        for (span = 0; span < s_mirrorSpanCount; span++) {
            if (s_mirrorSpans[span].assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
                s_mirrorSpans[span].assetSet ==
                    RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) {
                mirrorVehicleSpans++;
            }
        }
        fprintf(stderr,
                "rage-port: native world frame=%llu camera=%u instances=%u "
                "cached=%u textures=%u vertices=%u spans=%u mirror_vertices=%u "
                "mirror_spans=%u mirror_vehicle_spans=%u\n",
                (unsigned long long)world->frame, (unsigned)world->hasCamera,
                world->instanceCount, ModernAssetsCachedMeshCount(),
                s_textureCount,
                s_vertexCount, s_spanCount, s_mirrorVertexCount,
                s_mirrorSpanCount, mirrorVehicleSpans);
    }
}

const RageRenderWorld *ModernNativeGpuPreparedWorld(void) {
    return s_world;
}

static ModernNativeTexture *ModernNativeFindTexture(
    const RageNativeDrawSpan *span);

int ModernNativeGpuWriteDrawDump(FILE *file) {
    uint32_t spanIndex;
    if (file == NULL || s_world == NULL) return 0;
    fprintf(file,
            "world_frame=%llu aspect=%.9g vertices=%u spans=%u "
            "mirror_vertices=%u mirror_spans=%u complete=%d\n",
            (unsigned long long)s_worldFrame, s_aspect, s_vertexCount,
            s_spanCount, s_mirrorVertexCount, s_mirrorSpanCount,
            s_completeWorld);
    fprintf(file,
            "camera %.9g %.9g %.9g orientation %.9g %.9g %.9g %.9g "
            "fov %.9g near %.9g far %.9g\n",
            s_world->camera.transform.position.x,
            s_world->camera.transform.position.y,
            s_world->camera.transform.position.z,
            s_world->camera.transform.orientation.x,
            s_world->camera.transform.orientation.y,
            s_world->camera.transform.orientation.z,
            s_world->camera.transform.orientation.w,
            s_world->camera.verticalFovDegrees,
            s_world->camera.nearPlane, s_world->camera.farPlane);
    fprintf(file,
            "sky top %.9g %.9g %.9g horizon %.9g %.9g %.9g "
            "bottom %.9g %.9g %.9g\n",
            s_world->camera.skyTopColor.x,
            s_world->camera.skyTopColor.y,
            s_world->camera.skyTopColor.z,
            s_world->camera.skyHorizonColor.x,
            s_world->camera.skyHorizonColor.y,
            s_world->camera.skyHorizonColor.z,
            s_world->camera.skyBottomColor.x,
            s_world->camera.skyBottomColor.y,
            s_world->camera.skyBottomColor.z);
    fprintf(file,
            "span first count asset_set asset_key mesh source_entity entity "
            "material material_flags instance_flags pass decal variant "
            "paint paint1 paint2 component roughness metallic shading "
            "environment_r environment_g environment_b\n");
    for (spanIndex = 0; spanIndex < s_spanCount; spanIndex++) {
        const RageNativeDrawSpan *span = &s_spans[spanIndex];
        const ModernNativeTexture *texture = ModernNativeFindTexture(span);
        float roughness = texture != NULL ? texture->definition.roughness : 1.0f;
        float metallic = texture != NULL ? texture->definition.metallic : 0.0f;
        int shading = texture != NULL ? (int)texture->definition.shading : 0;
        const RageNativeDrawVertex *firstVertex =
            span->firstVertex < s_vertexCount
                ? &s_vertices[span->firstVertex] : NULL;
        uint32_t vertexIndex;
        fprintf(file,
                "s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
                "%.9g %.9g %d %.9g %.9g %.9g\n",
                span->firstVertex, span->vertexCount,
                (unsigned)span->assetSet, span->assetKey, span->mesh,
                span->sourceEntity, span->entity, span->material,
                span->materialFlags, span->instanceFlags,
                (unsigned)span->pass, (unsigned)span->depthDecal,
                (unsigned)span->materialVariant,
                (unsigned)span->hasCarPaint,
                (unsigned)span->carPaintColor1,
                (unsigned)span->carPaintColor2,
                (unsigned)span->component,
                roughness, metallic, shading,
                firstVertex != NULL ? firstVertex->environmentLight[0] : 1.0f,
                firstVertex != NULL ? firstVertex->environmentLight[1] : 1.0f,
                firstVertex != NULL ? firstVertex->environmentLight[2] : 1.0f);
        for (vertexIndex = span->firstVertex;
             vertexIndex < span->firstVertex + span->vertexCount &&
             vertexIndex < s_vertexCount;
             vertexIndex++) {
            const RageNativeDrawVertex *vertex = &s_vertices[vertexIndex];
            fprintf(file,
                    "v %u %.9g %.9g %.9g %.9g %.9g %u %u %u %u "
                    "%.9g %.9g %.9g %.9g %.9g\n",
                    vertexIndex, vertex->position[0], vertex->position[1],
                    vertex->position[2], vertex->uv[0], vertex->uv[1],
                    (unsigned)vertex->color[0],
                    (unsigned)vertex->color[1],
                    (unsigned)vertex->color[2],
                    (unsigned)vertex->color[3], vertex->depthBias,
                    vertex->normal[0], vertex->normal[1], vertex->normal[2],
                    vertex->shadowReception);
        }
    }
    return ferror(file) == 0;
}

typedef struct ModernNativeProbeVertex {
    RageRenderVec3 view;
    float depthBias;
} ModernNativeProbeVertex;

static uint32_t ModernNativeClipNear(
    const ModernNativeProbeVertex input[3], ModernNativeProbeVertex output[4],
    float nearPlane) {
    uint32_t inputIndex, count = 0;
    ModernNativeProbeVertex previous = input[2];
    int previousInside = -previous.view.z >= nearPlane;
    for (inputIndex = 0; inputIndex < 3; inputIndex++) {
        ModernNativeProbeVertex current = input[inputIndex];
        int currentInside = -current.view.z >= nearPlane;
        if (currentInside != previousInside) {
            float boundaryZ = -nearPlane;
            float t = (boundaryZ - previous.view.z) /
                      (current.view.z - previous.view.z);
            ModernNativeProbeVertex clipped;
            clipped.view.x = previous.view.x +
                             (current.view.x - previous.view.x) * t;
            clipped.view.y = previous.view.y +
                             (current.view.y - previous.view.y) * t;
            clipped.view.z = boundaryZ;
            clipped.depthBias = previous.depthBias +
                                (current.depthBias - previous.depthBias) * t;
            output[count++] = clipped;
        }
        if (currentInside) output[count++] = current;
        previous = current;
        previousInside = currentInside;
    }
    return count;
}

static int ModernNativeProbeTriangle(
    const ModernNativeProbeVertex triangle[3], float probeX, float probeY,
    int width, int height, float aspect, float fovScale,
    float depthScale, float depthOffset, float *depthOut) {
    float screenX[3], screenY[3], screenDepth[3];
    float denominator, a, b, c;
    int corner;
    for (corner = 0; corner < 3; corner++) {
        float depth = -triangle[corner].view.z;
        float ndcX = triangle[corner].view.x * fovScale / (depth * aspect);
        float ndcY = triangle[corner].view.y * fovScale / depth;
        screenX[corner] = (ndcX + 1.0f) * 0.5f * (float)width;
        screenY[corner] = (1.0f - ndcY) * 0.5f * (float)height;
        screenDepth[corner] = depthScale + depthOffset / depth +
                              triangle[corner].depthBias / 1048576.0f;
    }
    denominator = (screenY[1] - screenY[2]) *
                      (screenX[0] - screenX[2]) +
                  (screenX[2] - screenX[1]) *
                      (screenY[0] - screenY[2]);
    if (fabsf(denominator) < 0.000001f) return 0;
    a = ((screenY[1] - screenY[2]) * (probeX - screenX[2]) +
         (screenX[2] - screenX[1]) * (probeY - screenY[2])) / denominator;
    b = ((screenY[2] - screenY[0]) * (probeX - screenX[2]) +
         (screenX[0] - screenX[2]) * (probeY - screenY[2])) / denominator;
    c = 1.0f - a - b;
    if (a < -0.00001f || b < -0.00001f || c < -0.00001f) return 0;
    *depthOut = a * screenDepth[0] + b * screenDepth[1] +
                c * screenDepth[2];
    return 1;
}

int ModernNativeGpuWriteProbe(FILE *file, int x, int y,
                              int width, int height) {
    float aspect, fovScale, depthScale, depthOffset;
    uint32_t spanIndex;
    int hits = 0;
    if (file == NULL || s_world == NULL || width <= 0 || height <= 0 ||
        x < 0 || x >= width || y < 0 || y >= height) return 0;
    aspect = (float)width / (float)height;
    fovScale = 1.0f / tanf(s_world->camera.verticalFovDegrees *
                           0.008726646259971648f);
    if (!RenderPerspectiveDepthTerms(&s_world->camera, &depthScale,
                                         &depthOffset)) return 0;
    fprintf(file, "probe %d %d target %d %d\n", x, y, width, height);
    for (spanIndex = 0; spanIndex < s_spanCount; spanIndex++) {
        const RageNativeDrawSpan *span = &s_spans[spanIndex];
        uint32_t first;
        for (first = span->firstVertex;
             first + 2 < span->firstVertex + span->vertexCount &&
             first + 2 < s_vertexCount;
             first += 3) {
            ModernNativeProbeVertex input[3], clipped[4];
            uint32_t corner, clippedCount, piece;
            for (corner = 0; corner < 3; corner++) {
                const RageNativeDrawVertex *vertex = &s_vertices[first + corner];
                RageRenderVec3 position = {
                    vertex->position[0], vertex->position[1],
                    vertex->position[2]};
                RenderWorldToView(&s_world->camera, &position,
                                      &input[corner].view);
                input[corner].depthBias = vertex->depthBias;
            }
            clippedCount = ModernNativeClipNear(
                input, clipped, s_world->camera.nearPlane);
            for (piece = 1; piece + 1 < clippedCount; piece++) {
                ModernNativeProbeVertex triangle[3] = {
                    clipped[0], clipped[piece], clipped[piece + 1]};
                float depth;
                if (!ModernNativeProbeTriangle(
                        triangle, (float)x + 0.5f, (float)y + 0.5f,
                        width, height, aspect, fovScale,
                        depthScale, depthOffset, &depth)) continue;
                fprintf(file,
                        "hit=%d span=%u triangle=%u piece=%u depth=%.9g "
                        "view_depth=%.9g,%.9g,%.9g asset_set=%u "
                        "asset_key=%u mesh=%u source_entity=%u entity=%u "
                        "material=%u flags=%u decal=%u bias=%.9g\n",
                        hits++, spanIndex, (first - span->firstVertex) / 3,
                        piece - 1, depth, -input[0].view.z,
                        -input[1].view.z, -input[2].view.z,
                        (unsigned)span->assetSet, span->assetKey, span->mesh,
                        span->sourceEntity, span->entity, span->material,
                        span->instanceFlags, (unsigned)span->depthDecal,
                        s_vertices[first].depthBias);
            }
        }
    }
    fprintf(file, "hits=%d\n", hits);
    return ferror(file) == 0;
}

int ModernNativeGpuHasDraws(void) {
    return s_vertexCount != 0 && s_world != NULL && s_world->hasCamera;
}

int ModernNativeGpuWorldComplete(void) { return s_completeWorld; }

int ModernNativeGpuHasMirrorDraws(void) {
    return s_mirrorVertexCount != 0 && s_mirrorSpanCount != 0 &&
           s_world != NULL && s_world->mirrorActive &&
           s_world->hasMirrorCamera;
}

float ModernNativeGpuMirrorPanelY(void) {
    return s_world != NULL ? s_world->mirrorPanelY : -36.0f;
}

static ModernNativeTexture *ModernNativeFindTexture(
    const RageNativeDrawSpan *span) {
    uint32_t hash = span->assetKey * 0x9E3779B1u;
    uint32_t probe;
    hash ^= (uint32_t)span->assetSet * 0x85EBCA77u;
    hash ^= span->material * 0xC2B2AE3Du;
    hash ^= (uint32_t)span->materialVariant << 24;
    hash ^= (uint32_t)span->hasCarPaint << 23;
    hash ^= (uint32_t)span->carPaintColor1 << 8;
    hash ^= (uint32_t)span->carPaintColor2 << 16;
    hash ^= hash >> 16;
    for (probe = 0; probe < MODERN_NATIVE_TEXTURE_HASH_SIZE; probe++) {
        uint16_t stored =
            s_textureHash[(hash + probe) &
                          (MODERN_NATIVE_TEXTURE_HASH_SIZE - 1u)];
        ModernNativeTexture *entry;
        if (stored == 0) return NULL;
        entry = &s_textures[stored - 1u];
        if (entry->assetKey == span->assetKey &&
            entry->assetSet == span->assetSet &&
            entry->material == span->material &&
            entry->materialVariant == span->materialVariant &&
            entry->hasCarPaint == span->hasCarPaint &&
            entry->carPaintColor1 == span->carPaintColor1 &&
            entry->carPaintColor2 == span->carPaintColor2)
            return entry;
    }
    return NULL;
}

static void ModernNativeIndexTexture(uint32_t index) {
    const ModernNativeTexture *entry = &s_textures[index];
    uint32_t hash = entry->assetKey * 0x9E3779B1u;
    uint32_t probe;
    hash ^= (uint32_t)entry->assetSet * 0x85EBCA77u;
    hash ^= entry->material * 0xC2B2AE3Du;
    hash ^= (uint32_t)entry->materialVariant << 24;
    hash ^= (uint32_t)entry->hasCarPaint << 23;
    hash ^= (uint32_t)entry->carPaintColor1 << 8;
    hash ^= (uint32_t)entry->carPaintColor2 << 16;
    hash ^= hash >> 16;
    for (probe = 0; probe < MODERN_NATIVE_TEXTURE_HASH_SIZE; probe++) {
        uint32_t slot =
            (hash + probe) & (MODERN_NATIVE_TEXTURE_HASH_SIZE - 1u);
        if (s_textureHash[slot] == 0) {
            s_textureHash[slot] = (uint16_t)(index + 1u);
            return;
        }
    }
}

static ModernNativeTexture *ModernNativeLoadTexture(
    SDL_GPUCommandBuffer *command, const RageNativeDrawSpan *span) {
    ModernNativeTexture *entry = ModernNativeFindTexture(span);
    RageRenderMeshInstance instance = {0};
    RageRenderMaterial materialDefinition;
    ModernAssetImage image;
    uint8_t *mipChain = NULL;
    size_t mipSize;
    size_t byte;
    uint32_t mipLevels;
    SDL_GPUTransferBuffer *upload = NULL;
    if (entry != NULL || span->material == UINT32_MAX) return entry;
    if (s_textureCount == MODERN_NATIVE_MAX_TEXTURES) {
        /* Nothing evicts, so this is permanent for the rest of the course:
         * every material after it draws nothing at all. Say so once, because
         * the symptom is missing scenery rather than an error. */
        static int reported;
        if (!reported) {
            reported = 1;
            fprintf(stderr,
                    "rage-port: native texture cache full at %u; further "
                    "materials will not be drawn\n",
                    s_textureCount);
        }
        return NULL;
    }
    instance.assetKey = span->assetKey;
    instance.assetSet = span->assetSet;
    instance.hasCarPaint = span->hasCarPaint;
    instance.carPaintColor1 = span->carPaintColor1;
    instance.carPaintColor2 = span->carPaintColor2;
    if (!ModernAssetsLoadMaterial(&instance, span->material,
                                  span->materialVariant,
                                  &materialDefinition, &image)) return NULL;
    mipLevels = TextureMipLevelCount(
        image.width, image.height, RAGE_TEXTURE_ATLAS_MIP_LEVELS);
    mipSize = TextureMipChainSizeRGBA8(
        image.width, image.height, mipLevels);
    mipChain = malloc(mipSize);
    if (mipChain == NULL ||
        !TextureBuildMipChainRGBA8(
            image.pixels, image.width, image.height, mipLevels,
            mipChain, mipSize)) goto fail;
    entry = &s_textures[s_textureCount];
    {
        SDL_GPUTextureCreateInfo texture = {0};
        SDL_GPUTransferBufferCreateInfo transfer = {0};
        texture.type = SDL_GPU_TEXTURETYPE_2D;
        texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texture.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texture.width = image.width;
        texture.height = image.height;
        texture.layer_count_or_depth = 1;
        texture.num_levels = mipLevels;
        entry->texture = SDL_CreateGPUTexture(s_device, &texture);
        transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer.size = (Uint32)mipSize;
        upload = SDL_CreateGPUTransferBuffer(s_device, &transfer);
    }
    if (entry->texture == NULL || upload == NULL) goto fail;
    {
        void *mapped = SDL_MapGPUTransferBuffer(s_device, upload, false);
        SDL_GPUCopyPass *copy;
        uint32_t level;
        if (mapped == NULL) goto fail;
        memcpy(mapped, mipChain, mipSize);
        SDL_UnmapGPUTransferBuffer(s_device, upload);
        copy = SDL_BeginGPUCopyPass(command);
        if (copy == NULL) goto fail;
        for (level = 0; level < mipLevels; level++) {
            uint32_t width = image.width >> level;
            uint32_t height = image.height >> level;
            SDL_GPUTextureTransferInfo source = {
                .transfer_buffer = upload,
                .offset = (Uint32)TextureMipLevelOffsetRGBA8(
                    image.width, image.height, level),
                .pixels_per_row = width != 0 ? width : 1,
                .rows_per_layer = height != 0 ? height : 1,
            };
            SDL_GPUTextureRegion destination = {
                .texture = entry->texture,
                .mip_level = level,
                .w = width != 0 ? width : 1,
                .h = height != 0 ? height : 1,
                .d = 1,
            };
            SDL_UploadToGPUTexture(copy, &source, &destination, false);
        }
        SDL_EndGPUCopyPass(copy);
    }
    ModernNativeRetireUpload(upload);
    upload = NULL;
    entry->transparent = 0;
    for (byte = 3; byte < image.size; byte += 4) {
        uint8_t alpha = ((const uint8_t *)image.pixels)[byte];
        if (alpha != 0 && alpha != 255) {
            entry->transparent = 1;
            break;
        }
    }
    if (materialDefinition.alphaMode == RAGE_RENDER_MATERIAL_ALPHA_BLEND)
        entry->transparent = 1;
    else if (materialDefinition.alphaMode != RAGE_RENDER_MATERIAL_ALPHA_AUTO)
        entry->transparent = 0;
    free(mipChain);
    ModernAssetsFreeMaterialImage(&image);
    entry->assetKey = span->assetKey;
    entry->assetSet = span->assetSet;
    entry->material = span->material;
    entry->materialVariant = span->materialVariant;
    entry->hasCarPaint = span->hasCarPaint;
    entry->carPaintColor1 = span->carPaintColor1;
    entry->carPaintColor2 = span->carPaintColor2;
    materialDefinition.baseColorTexture = (RageRenderMaterialPath){0};
    materialDefinition.paintMask = (RageRenderMaterialPath){0};
    entry->definition = materialDefinition;
    ModernNativeIndexTexture(s_textureCount);
    s_textureCount++;
    return entry;
fail:
    /* The mip chain is allocated before a cache slot is claimed, so failing
     * to build it arrives here with no slot to release. Running out of
     * memory is exactly when this path is taken, which made it a crash in
     * the one situation it exists to survive. */
    free(mipChain);
    ModernAssetsFreeMaterialImage(&image);
    if (upload != NULL) SDL_ReleaseGPUTransferBuffer(s_device, upload);
    if (entry != NULL) {
        if (entry->texture != NULL)
            SDL_ReleaseGPUTexture(s_device, entry->texture);
        memset(entry, 0, sizeof(*entry));
    }
    return NULL;
}

static int ModernNativeUploadVertices(SDL_GPUCommandBuffer *command) {
    void *mapped;
    SDL_GPUCopyPass *copy;
    SDL_GPUTransferBufferLocation source = {
        .transfer_buffer = s_vertexTransfer, .offset = 0};
    SDL_GPUBufferRegion destination = {
        .buffer = s_vertexBuffer, .offset = 0,
        .size = (s_vertexCount + s_mirrorVertexCount) * sizeof(*s_vertices)};
    if (destination.size == 0) return 0;
    mapped = SDL_MapGPUTransferBuffer(s_device, s_vertexTransfer, true);
    if (mapped == NULL) return 0;
    memcpy(mapped, s_vertices, destination.size);
    SDL_UnmapGPUTransferBuffer(s_device, s_vertexTransfer);
    copy = SDL_BeginGPUCopyPass(command);
    if (copy == NULL) return 0;
    SDL_UploadToGPUBuffer(copy, &source, &destination, true);
    SDL_EndGPUCopyPass(copy);
    return 1;
}

static int ModernNativeSpanCastsShadow(const RageNativeDrawSpan *span) {
    return span->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
           span->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
}

static void ModernNativeDrawShadowMap(SDL_GPUCommandBuffer *command) {
    SDL_GPUDepthStencilTargetInfo depth = {
        .texture = s_shadowTexture,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    ModernNativeCameraUniform camera;
    SDL_GPURenderPass *pass;
    SDL_GPUBufferBinding vertex = {.buffer = s_vertexBuffer, .offset = 0};
    SDL_GPUGraphicsPipeline *boundPipeline = NULL;
    ModernNativeTexture *boundTexture = NULL;
    uint32_t spanIndex;
    uint32_t drawCount = 0;
    uint32_t maskedDrawCount = 0;
    if (!s_haveShadowMap) return;
    /* Texture uploads must finish before the render pass that samples them.
     * Every textured caster uses its alpha channel so empty atlas texels do
     * not turn the source mesh's triangles into visible shadow geometry. */
    for (spanIndex = 0; spanIndex < s_spanCount; spanIndex++) {
        const RageNativeDrawSpan *span = &s_spans[spanIndex];
        if (span->vertexCount != 0 && ModernNativeSpanCastsShadow(span) &&
            span->material != UINT32_MAX)
            (void)ModernNativeLoadTexture(command, span);
    }
    pass = SDL_BeginGPURenderPass(command, NULL, 0, &depth);
    if (pass == NULL) return;
    ModernNativeBuildShadowCamera(&s_shadowMap, &camera);
    SDL_PushGPUVertexUniformData(command, 0, &camera, sizeof(camera));
    SDL_BindGPUVertexBuffers(pass, 0, &vertex, 1);
    for (spanIndex = 0; spanIndex < s_spanCount; spanIndex++) {
        const RageNativeDrawSpan *span = &s_spans[spanIndex];
        SDL_GPUGraphicsPipeline *pipeline = s_shadowDepth;
        ModernNativeTexture *texture = NULL;
        if (span->vertexCount == 0 || !ModernNativeSpanCastsShadow(span))
            continue;
        if (span->material != UINT32_MAX) {
            texture = ModernNativeFindTexture(span);
            if (texture == NULL) continue;
            pipeline = s_shadowMasked;
            maskedDrawCount++;
        }
        if (pipeline != boundPipeline) {
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            boundPipeline = pipeline;
            boundTexture = NULL;
        }
        if (texture != NULL && texture != boundTexture) {
            SDL_GPUTextureSamplerBinding binding = {
                .texture = texture->texture,
                .sampler = s_sampler};
            SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
            boundTexture = texture;
        }
        SDL_DrawGPUPrimitives(pass, span->vertexCount, 1,
                              span->firstVertex, 0);
        drawCount++;
    }
    SDL_EndGPURenderPass(pass);
    if (RuntimeConfigEnabled("diagnostics.modern_asset_trace")) {
        fprintf(stderr,
                "rage-port: native shadow map frame=%llu draws=%u masked=%u\n",
                (unsigned long long)s_worldFrame, drawCount, maskedDrawCount);
    }
}

static void ModernNativeGpuDrawSet(
    SDL_GPUCommandBuffer *command,
    SDL_GPUTexture *colorTarget, SDL_GPUTexture *depthTarget, int clearColor,
    const RageRenderCamera *renderCamera, float aspect,
    const RageNativeDrawSpan *spans, uint32_t spanCount,
    uint32_t drawVertexCount, const char *viewName) {
    SDL_GPUColorTargetInfo color = {
        .texture = colorTarget,
        .clear_color = {renderCamera != NULL ? renderCamera->skyColor.x : 0.0f,
                        renderCamera != NULL ? renderCamera->skyColor.y : 0.0f,
                        renderCamera != NULL ? renderCamera->skyColor.z : 0.0f,
                        1.0f},
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
    ModernNativeCameraUniform shadowCamera;
    ModernNativeSkyUniform sky;
    ModernNativeLightUniform light;
    SDL_GPURenderPass *pass;
    uint32_t spanIndex;
    uint32_t drawCount = 0;
    if (renderCamera == NULL) return;
    for (spanIndex = 0; spanIndex < spanCount; spanIndex++)
        (void)ModernNativeLoadTexture(command, &spans[spanIndex]);
    if (!ModernNativeEnsureSkyTexture(command, renderCamera->skyAssetKey))
        return;
    pass = SDL_BeginGPURenderPass(command, &color, 1, &depth);
    if (pass == NULL) return;
    ModernNativeBuildCamera(renderCamera, &camera);
    camera.projection[0] /= aspect;
    SDL_PushGPUVertexUniformData(command, 0, &camera, sizeof(camera));
    ModernNativeBuildSky(renderCamera, &sky);
    SDL_PushGPUFragmentUniformData(command, 0, &sky, sizeof(sky));
    SDL_BindGPUGraphicsPipeline(pass, s_sky);
    {
        SDL_GPUTextureSamplerBinding binding = {
            .texture = s_skyTexture,
            .sampler = s_skySampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    }
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    ModernNativeBuildLight(&s_world->light, renderCamera, &light);
    SDL_PushGPUFragmentUniformData(command, 0, &light, sizeof(light));
    ModernNativeBuildShadowCamera(&s_shadowMap, &shadowCamera);
    SDL_PushGPUVertexUniformData(
        command, 1, &shadowCamera, sizeof(shadowCamera));
    {
        SDL_GPUBufferBinding vertex = {.buffer = s_vertexBuffer, .offset = 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vertex, 1);
    }
    /* Opaque scenery first, its surface overlays second, opaque vehicles
     * third, then transparent materials. Vehicles therefore remain in front
     * even when a road marking or animated sign needs a meaningful offset
     * from its imported support surface. */
    for (int phase = 0; phase < 4; phase++) {
        SDL_GPUGraphicsPipeline *boundPipeline = NULL;
        ModernNativeTexture *boundTexture = NULL;
        int boundAllowClearcoat = -1;
        for (spanIndex = 0; spanIndex < spanCount; spanIndex++) {
            const RageNativeDrawSpan *span = &spans[spanIndex];
            ModernNativeTexture *texture;
            SDL_GPUGraphicsPipeline *pipeline;
            int vehicle = span->assetSet == RAGE_RENDER_ASSET_MODEL_BANK ||
                          span->assetSet ==
                              RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
            int allowClearcoat = !vehicle || span->component == 0;
            if (span->vertexCount == 0) continue;
            if (span->material == UINT32_MAX) {
                if (phase != (span->depthDecal ? 1 : (vehicle ? 2 : 0)))
                    continue;
                pipeline = span->depthDecal
                    ? s_colorOpaqueDecal : s_colorOpaque;
                texture = NULL;
            } else {
                texture = ModernNativeFindTexture(span);
                if (texture == NULL) continue;
                if (texture->transparent) {
                    if (phase != 3) continue;
                } else if (span->depthDecal) {
                    if (phase != 1) continue;
                } else if (phase != (vehicle ? 2 : 0)) {
                    continue;
                }
                if (span->depthDecal) {
                    pipeline = texture->transparent
                        ? s_texturedTransparentDecal
                        : s_texturedOpaqueDecal;
                } else {
                    pipeline = texture->transparent
                        ? s_texturedTransparent
                        : s_texturedOpaque;
                }
            }
            if (pipeline != boundPipeline) {
                SDL_GPUTextureSamplerBinding shadowBinding = {
                    .texture = s_shadowTexture,
                    .sampler = s_shadowSampler};
                int textured = pipeline == s_texturedOpaque ||
                    pipeline == s_texturedTransparent ||
                    pipeline == s_texturedOpaqueDecal ||
                    pipeline == s_texturedTransparentDecal;
                SDL_BindGPUGraphicsPipeline(pass, pipeline);
                SDL_BindGPUFragmentSamplers(
                    pass, textured ? 1 : 0, &shadowBinding, 1);
                boundPipeline = pipeline;
                boundTexture = NULL;
                boundAllowClearcoat = -1;
            }
            if (texture != NULL &&
                (texture != boundTexture ||
                 allowClearcoat != boundAllowClearcoat)) {
                SDL_GPUTextureSamplerBinding binding = {
                    .texture = texture->texture,
                    .sampler = s_sampler};
                ModernNativeMaterialUniform material;
                ModernNativeBuildMaterial(
                    &texture->definition, allowClearcoat, &material);
                SDL_PushGPUFragmentUniformData(
                    command, 1, &material, sizeof(material));
                SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
                boundTexture = texture;
                boundAllowClearcoat = allowClearcoat;
            }
            SDL_DrawGPUPrimitives(pass, span->vertexCount, 1,
                                  span->firstVertex, 0);
            drawCount++;
        }
    }
    SDL_EndGPURenderPass(pass);
    if (drawCount != 0 && RuntimeConfigEnabled("diagnostics.modern_asset_trace")) {
        fprintf(stderr,
                "rage-port: native draws frame=%llu draws=%u vertices=%u "
                "view=%s\n",
                (unsigned long long)s_worldFrame, drawCount, drawVertexCount,
                viewName);
    }
}


void ModernNativeGpuDraw(SDL_GPUCommandBuffer *command,
                         SDL_GPUTexture *colorTarget,
                         SDL_GPUTexture *depthTarget,
                         int clearColor) {
    if (s_world == NULL || !s_world->hasCamera) return;
    if (ModernNativeGpuHasDraws()) {
        if (!ModernNativeUploadVertices(command)) return;
        ModernNativeDrawShadowMap(command);
    }
    ModernNativeGpuDrawSet(command, colorTarget, depthTarget, clearColor,
                           &s_world->camera, s_aspect, s_spans, s_spanCount,
                           s_vertexCount, "main");
}

void ModernNativeGpuDrawMirror(SDL_GPUCommandBuffer *command,
                               SDL_GPUTexture *colorTarget,
                               SDL_GPUTexture *depthTarget) {
    if (!ModernNativeGpuHasMirrorDraws()) return;
    ModernNativeGpuDrawSet(command, colorTarget, depthTarget, 1,
                           &s_world->mirrorCamera, s_mirrorAspect,
                           s_mirrorSpans, s_mirrorSpanCount,
                           s_mirrorVertexCount, "mirror");
}

void ModernNativeGpuShutdown(void) {
    ModernNativeGpuClearTextures();
    ModernNativeReleaseSkyTexture();
    if (s_device != NULL) {
        if (s_texturedOpaque != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_texturedOpaque);
        if (s_texturedTransparent != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_texturedTransparent);
        if (s_texturedOpaqueDecal != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_texturedOpaqueDecal);
        if (s_texturedTransparentDecal != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device,
                                           s_texturedTransparentDecal);
        if (s_colorOpaque != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_colorOpaque);
        if (s_colorOpaqueDecal != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_colorOpaqueDecal);
        if (s_sky != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_sky);
        if (s_shadowDepth != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_shadowDepth);
        if (s_shadowMasked != NULL)
            SDL_ReleaseGPUGraphicsPipeline(s_device, s_shadowMasked);
        if (s_vertexBuffer != NULL) SDL_ReleaseGPUBuffer(s_device, s_vertexBuffer);
        if (s_vertexTransfer != NULL)
            SDL_ReleaseGPUTransferBuffer(s_device, s_vertexTransfer);
        if (s_sampler != NULL) SDL_ReleaseGPUSampler(s_device, s_sampler);
        if (s_skySampler != NULL)
            SDL_ReleaseGPUSampler(s_device, s_skySampler);
        if (s_shadowTexture != NULL)
            SDL_ReleaseGPUTexture(s_device, s_shadowTexture);
        if (s_shadowSampler != NULL)
            SDL_ReleaseGPUSampler(s_device, s_shadowSampler);
    }
    free(s_vertices);
    free(s_spans);
    free(s_mirrorSpans);
    s_device = NULL;
    s_texturedOpaque = NULL;
    s_texturedTransparent = NULL;
    s_texturedOpaqueDecal = NULL;
    s_texturedTransparentDecal = NULL;
    s_colorOpaque = NULL;
    s_colorOpaqueDecal = NULL;
    s_sky = NULL;
    s_shadowDepth = NULL;
    s_shadowMasked = NULL;
    s_vertexBuffer = NULL;
    s_vertexTransfer = NULL;
    s_sampler = NULL;
    s_skySampler = NULL;
    s_shadowTexture = NULL;
    s_shadowSampler = NULL;
    s_vertices = NULL;
    s_spans = NULL;
    s_mirrorSpans = NULL;
    s_vertexCount = 0;
    s_spanCount = 0;
    s_mirrorVertexCount = 0;
    s_mirrorSpanCount = 0;
    s_worldFrame = UINT64_MAX;
    s_world = NULL;
    s_aspect = 4.0f / 3.0f;
    s_completeWorld = 0;
    ModernNativeReleasePendingUploads();
    s_textureCount = 0;
    /* The lookup index has to go with the textures it points into. Leaving it
     * behind left entries naming slots that the next run filled with
     * different textures, so a material found one that was never its own. */
    memset(s_textureHash, 0, sizeof(s_textureHash));
    s_trackAssetRevision = UINT64_MAX;
    s_haveShadowMap = 0;
}
