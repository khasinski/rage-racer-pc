#ifndef GAME_SCENE_RUNTIME_H
#define GAME_SCENE_RUNTIME_H

#include "game/asset.h"
#include "game/bgm_select_state.h"
#include "game/memcard_types.h"
#include "game/memcard_state.h"
#include "game/record_entry_state.h"
#include "game/prologue_state.h"
#include "game/scene.h"
#include "game/scene_state.h"

/*
 * Port-owned scene lifetime.  Retail handlers still use their recovered
 * globals, but cross-scene resources are attached to this generation rather
 * than inferred from whichever globals the previous handler happened to use.
 */
typedef union SceneState {
    BootLogo bootLogo;
    MemoryCardSession memoryCard;
    BgmSelect bgmSelect;
    RecordEntry recordEntry;
    Prologue prologue;
    AttractDemo attractDemo;
    LostRace lostRace;
    RaceScene race;
} SceneState;

typedef struct SceneRuntime {
    s32 scene;
    u32 generation;
    u32 assetGeneration;
    SceneState state;
} SceneRuntime;

void SceneRuntimeBeforeDispatch(s32 scene);
const SceneRuntime *SceneRuntimeCurrent(void);
BootLogo *SceneRuntimeBootLogo(void);
MemoryCardSession *SceneRuntimeMemoryCard(void);
BgmSelect *SceneRuntimeBgmSelect(void);
RecordEntry *SceneRuntimeRecordEntry(void);
Prologue *SceneRuntimePrologue(void);
AttractDemo *SceneRuntimeAttractDemo(void);
LostRace *SceneRuntimeLostRace(void);
RaceScene *SceneRuntimeRace(void);
/* A result belongs to a scene only when it was the active transaction at the
 * point that scene began. */
const AssetLoadTransaction *SceneRuntimeAssetResult(AssetRequestType request);
const AssetLoadTransaction *SceneRuntimeActiveAssetResult(void);

#endif
