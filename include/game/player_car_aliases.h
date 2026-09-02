#ifndef GAME_PLAYER_CAR_ALIASES_H
#define GAME_PLAYER_CAR_ALIASES_H

#include "common.h"
#include "game/vector.h"

/* Translation units view the same storage as either PlayerCarRuntime or the
 * showroom union.  This neutral base keeps retail interior labels usable from
 * both views without introducing incompatible extern declarations. */
void *GetPlayerCarStorage(void);

#define g_PlayerSegmentWeight \
    (*(s32 *)((u8 *)GetPlayerCarStorage() + 0x38))
#define g_PlayerField3C \
    (*(s32 *)((u8 *)GetPlayerCarStorage() + 0x3C))
#define g_PlayerSteerAngle \
    (*(s32 *)((u8 *)GetPlayerCarStorage() + 0x44))
#define g_PlayerCarWheelAngle \
    (*(s32 *)((u8 *)GetPlayerCarStorage() + 0x48))
#define g_PlayerRenderRotation \
    (*(Vec4 *)((u8 *)GetPlayerCarStorage() + 0x50))
#define g_PlayerRenderY \
    (*(s32 *)((u8 *)GetPlayerCarStorage() + 0x60))
#define g_PlayerFacingBackwards \
    (*(s16 *)((u8 *)GetPlayerCarStorage() + 0xB8))
#define g_PlayerVelocity \
    (*(Vec4 (*)[2])((u8 *)GetPlayerCarStorage() + 0xC4))
/* Retail reuses +0xE4 as render depth in-race and as the selected tire
 * compound while the same storage backs the showroom car. */
#define g_PlayerTireCompound \
    (*(s32 *)((u8 *)GetPlayerCarStorage() + 0xE4))
#define g_PlayerTransmission \
    (*(s16 *)((u8 *)GetPlayerCarStorage() + 0x130))
#define g_PlayerTargetRpm \
    (*(s32 *)((u8 *)GetPlayerCarStorage() + 0x134))
#define g_RacePosition \
    (*(s16 *)((u8 *)GetPlayerCarStorage() + 0x160))
#define g_HudLapHighlightRow \
    (*(s16 *)((u8 *)GetPlayerCarStorage() + 0x162))

#endif
