#include "client_frame.h"
#include "modern/modern_native_source.h"
#include <string.h>
#include <stdio.h>

/* Headless sink for testing the real frame adapter's provider contract.
 * This does not simulate GPU rendering or cache invalidation. */
static ModernNativeSource retained;
void ModernNativeGpuPrepareSource(const RenderWorld *world, float aspect,
                                  const ModernNativeSource *source) {
    (void)world;
    (void)aspect;
    ModernNativeSource next = *source;
    next.context = source->retain(source->context);
    if (retained.context) retained.release(retained.context);
    retained = next;
}
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "source line %d: %s\n", __LINE__, #c); return 0; } } while (0)
int CheckFrameSource(ClientFrame *first, ClientFrame *second);
int CheckFrameSource(ClientFrame *first, ClientFrame *second) {
    for (unsigned i = 0; i < 2; ++i) {
        ClientFrame *frame = i ? second : first;
        const unsigned references = frame->references;
        PrepareClientFrameGpu(frame, 16.0f / 9.0f);
        CHECK(frame->references == references + 1);
        CHECK(retained.context == frame && retained.owner == frame->race);
        /* GPU preparation uses its own copied instances, not frame addresses. */
        RenderMeshInstance instance = frame->scene.world.instances[0];
        CHECK(retained.mesh(retained.context, &instance));
        RageRenderMaterial definition;
        ModernAssetImage image = {0};
        CHECK(retained.material(retained.context, &instance, 0, &definition, &image));
        CHECK(ModernAssetImageValidRGBA(&image));
        retained.freeImage(&image);
        CHECK(!image.pixels && !image.size);
        instance.assetKey = UINT32_MAX;
        CHECK(!retained.mesh(retained.context, &instance));
        const ModernAssetImage saved = image;
        const RageRenderMaterial savedDefinition = definition;
        CHECK(!retained.material(retained.context, &instance, 0, &definition, &image));
        CHECK(memcmp(&image, &saved, sizeof(image)) == 0);
        CHECK(memcmp(&definition, &savedDefinition, sizeof(definition)) == 0);
        CHECK(retained.sky(retained.context, &image));
        CHECK(ModernAssetImageValidRGBA(&image) && image.width == 512);
        retained.freeImage(&image);
        PrepareClientFrameGpu(frame, 16.0f / 9.0f);
        CHECK(frame->references == references + 1);
    }
    CHECK(first->references == 1 && second->references == 2);
    retained.release(retained.context);
    retained = (ModernNativeSource){0};
    CHECK(first->references == 1 && second->references == 1);
    return 1;
}
