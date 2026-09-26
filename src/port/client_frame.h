#ifndef PORT_CLIENT_FRAME_H
#define PORT_CLIENT_FRAME_H
#include "client_race.h"
#include "render/render_world_snapshot.h"
#include "modern/modern_prepared_meshes.h"

/* Owns renderer input and retains its resource owner. Do not copy by value.
 * Palette/page are captured values, not the race's later environment state. */
typedef struct ClientFrame {
    ClientRace *race;
    RenderWorldSnapshot scene;
    ModernPreparedMeshes meshes;
    u16 palette[16];
    int page;
} ClientFrame;
/* Only prepared owned main-pass assets are accepted. Failure leaves callers
 * and existing frames unchanged. Lifetime operations run on the main thread. */
ClientFrame *CaptureClientFrame(ClientRace *race, const RenderWorld *world, int page);
void FreeClientFrame(ClientFrame *frame);
const RageRuntimeMesh *ClientFrameMeshLookup(void *context, const RenderMeshInstance *instance);
int DecodeFrameMaterial(const ClientFrame *frame, const RenderMeshInstance *instance,
                         u32 material, u8 *rgba, size_t size);
#endif
