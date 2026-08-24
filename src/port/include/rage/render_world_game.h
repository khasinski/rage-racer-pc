#ifndef RAGE_PORT_RENDER_WORLD_GAME_H
#define RAGE_PORT_RENDER_WORLD_GAME_H

#include <stdint.h>

struct GameRenderObject;
struct RageRenderWorld;

/* Game-state producer for Render World.  It receives a GameRenderObject
 * before the classic GTE path mutates or projects it. */
void RageGameRenderWorldBeginFrame(uint64_t frame);
void RageGameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z,
                                  int32_t pitch, int32_t yaw, int32_t roll);
void RageGameRenderWorldSubmitCourseObject(uint32_t entity, int32_t mesh,
                                           int32_t x, int32_t y, int32_t z,
                                           int32_t yaw, int transparent,
                                           int mirror_pass);
void RageGameRenderWorldSubmitTerrainCell(uint32_t grid_x, uint32_t grid_z,
                                          int32_t mesh, int mirror_pass);
void RageGameRenderWorldPublishTerrainGrid(void);
void RageGameRenderWorldPublishCourseObjects(void);
void RageGameRenderWorldSubmitCar(const struct GameRenderObject *object,
                                  int mirror_pass);
/* The player's selected model is a separately loaded bank.  It does not use
 * the course-specific opponent lookup used by RageGameRenderWorldSubmitCar. */
void RageGameRenderWorldSubmitPlayerCar(const struct GameRenderObject *object,
                                        int mirror_pass);
const struct RageRenderWorld *RageGameRenderWorldCurrent(void);
const struct RageRenderWorld *RageGameRenderWorldPrevious(void);

#endif

