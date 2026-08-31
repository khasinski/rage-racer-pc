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
void GameRenderWorldBeginFrame(uint64_t frame);
void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z,
                                  int32_t pitch, int32_t yaw, int32_t roll);
void GameRenderWorldPublishCurrentCamera(void);
void GameRenderWorldSubmitCourseObject(uint32_t entity, int32_t mesh,
                                           int32_t x, int32_t y, int32_t z,
                                           int32_t yaw, int fogged,
                                           int mirror_pass);
/* Dynamic course props publish their authored world rotation before the
 * classic path multiplies it by the view matrix. */
void GameRenderWorldSubmitDynamicCourseObject(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass);
/* Animated signs are layered directly onto an authored screen surface. */
void GameRenderWorldSubmitDynamicCourseOverlay(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass);
void GameRenderWorldSubmitTerrainCell(uint32_t grid_x, uint32_t grid_z,
                                          int32_t mesh, int mirror_pass);
void GameRenderWorldPublishTerrainGrid(void);
void GameRenderWorldPublishCourseObjects(void);
/* Publish active race traffic independently of what the classic main camera
 * happened to submit, so secondary native cameras see the complete field. */
void GameRenderWorldPublishRaceCars(void);
void GameRenderWorldDiscardLegacyMirror(void);
/* LoadRaceAssets owns the palette pack actually resident in VRAM.  Menu state
 * may change while an intro frame is being assembled, so rendering must not
 * infer this identity again from g_PlayerCarIndex. */
void GameRenderWorldSetTrackCarAsset(int asset);
void GameRenderWorldSubmitCar(const struct GameRenderObject *object,
                                  int mirror_pass,
                                  RageGameCarRenderDetail detail);
/* The player's selected model is a separately loaded bank.  It does not use
 * the course-specific opponent lookup used by GameRenderWorldSubmitCar. */
void GameRenderWorldSubmitPlayerCar(const struct GameRenderObject *object,
                                        int mirror_pass);
const struct RageRenderWorld *GameRenderWorldCurrent(void);
const struct RageRenderWorld *GameRenderWorldPrevious(void);
/* Build the previous->current presentation state used by unlocked rendering.
 * The returned storage is owned by the adapter until the next call. */
const struct RageRenderWorld *GameRenderWorldPresentation(float t);

/* The nine environment colours of the frame being drawn, for diagnostics
 * that need the palette and the picture to come from the same moment. */
void GameRenderWorldEnvironmentPalette(unsigned char out[9][3]);

#endif
