#ifndef PORT_CLIENT_FRAME_H
#define PORT_CLIENT_FRAME_H
#include "client_race.h"
#include "render/render_world_snapshot.h"
#include "modern/modern_prepared_meshes.h"

/* Owns renderer input and retains its resource owner. Do not copy by value.
 * Palette/page are captured values, not the race's later environment state. */
typedef struct ClientFrame {
    unsigned references;
    ClientRace *race;
    RenderWorldSnapshot scene;
    ModernPreparedMeshes meshes;
    u16 palette[16];
    int page;
} ClientFrame;
/* Camera pose/clipping come from world; environment colors, fog and retail
 * sky layout come from race. Only prepared owned main-pass assets are accepted.
 * Failure leaves callers and existing frames unchanged. Lifetime operations run on the main thread. */
ClientFrame *CaptureClientFrame(ClientRace *race, const RenderWorld *world, int page);
ClientFrame *RetainClientFrame(ClientFrame *frame);
void FreeClientFrame(ClientFrame *frame);
/* GPU retains this immutable frame until another prepare or shutdown. */
void PrepareClientFrameGpu(ClientFrame *frame, float aspect);
const RageRuntimeMesh *ClientFrameMeshLookup(void *context, const RenderMeshInstance *instance);
int DecodeFrameMaterial(const ClientFrame *frame, const RenderMeshInstance *instance,
                         u32 material, u8 *rgba, size_t size);
/* Uses the captured camera layout and palette. No legacy-layout fallback.
 * Caller owns 512x256 RGBA storage; failure preserves its contents. */
int DecodeFrameSky(const ClientFrame *frame, u8 *rgba, size_t size);
#endif
