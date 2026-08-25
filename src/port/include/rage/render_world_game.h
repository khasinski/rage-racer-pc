#ifndef RAGE_PORT_RENDER_WORLD_GAME_H
#define RAGE_PORT_RENDER_WORLD_GAME_H

#include <stdint.h>

struct GameRenderObject;
struct RageRenderWorld;

typedef enum RageGameCarRenderDetail {
    RAGE_GAME_CAR_RENDER_CLOSE = 0,
    RAGE_GAME_CAR_RENDER_FAR = 1,
} RageGameCarRenderDetail;

/* Game-state producer for Render World.  It receives a GameRenderObject
 * before the classic GTE path mutates or projects it. */
void RageGameRenderWorldBeginFrame(uint64_t frame);
void RageGameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z,
                                  int32_t pitch, int32_t yaw, int32_t roll);
void RageGameRenderWorldPublishCurrentCamera(void);
void RageGameRenderWorldSubmitCourseObject(uint32_t entity, int32_t mesh,
                                           int32_t x, int32_t y, int32_t z,
                                           int32_t yaw, int fogged,
                                           int mirror_pass);
/* Dynamic course props publish their authored world rotation before the
 * classic path multiplies it by the view matrix. */
void RageGameRenderWorldSubmitDynamicCourseObject(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass);
void RageGameRenderWorldSubmitTerrainCell(uint32_t grid_x, uint32_t grid_z,
                                          int32_t mesh, int mirror_pass);
void RageGameRenderWorldPublishTerrainGrid(void);
void RageGameRenderWorldPublishCourseObjects(void);
/* LoadRaceAssets owns the palette pack actually resident in VRAM.  Menu state
 * may change while an intro frame is being assembled, so rendering must not
 * infer this identity again from g_PlayerCarIndex. */
void RageGameRenderWorldSetTrackCarAsset(int asset);
void RageGameRenderWorldSubmitCar(const struct GameRenderObject *object,
                                  int mirror_pass,
                                  RageGameCarRenderDetail detail);
/* The player's selected model is a separately loaded bank.  It does not use
 * the course-specific opponent lookup used by RageGameRenderWorldSubmitCar. */
void RageGameRenderWorldSubmitPlayerCar(const struct GameRenderObject *object,
                                        int mirror_pass);
const struct RageRenderWorld *RageGameRenderWorldCurrent(void);
const struct RageRenderWorld *RageGameRenderWorldPrevious(void);
/* Build the previous->current presentation state used by unlocked rendering.
 * The returned storage is owned by the adapter until the next call. */
const struct RageRenderWorld *RageGameRenderWorldPresentation(float t);

#endif
