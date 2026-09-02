#ifndef GAME_ASSET_H
#define GAME_ASSET_H

#include <stddef.h>

#include "common.h"
#include "game/visibility.h"
#include "psyq/cd_location.h"
#include "psyq/gpu.h"

struct CarImageData;
struct ModelBankHeader;
struct CourseModelAssetHeader;
struct SceneryMotionData;
struct RaceIntroCameraScript;
struct PathSceneryPositionData;
struct TrackEventData;
struct TrackPointTable;
struct EnvironmentPalette;
struct GameEnvironmentScript;
struct TrackRenderTable;
struct CourseObjectTable;
struct OptionScreenAsset;
struct CarModelAsset;
struct SerializedCarModelAssetHeader;
struct SVec;

typedef enum AssetRequestType {
    ASSET_REQUEST_INVALID = -1,
    ASSET_REQUEST_IDLE = 0,
    ASSET_REQUEST_BOOT,
    ASSET_REQUEST_SAVE_SCREEN,
    ASSET_REQUEST_SELECT_BGM,
    ASSET_REQUEST_CAR_SELECT,
    ASSET_REQUEST_CAR_MODEL,
    ASSET_REQUEST_UPGRADED_CAR_MODEL,
    ASSET_REQUEST_OPTION_SCREEN,
    ASSET_REQUEST_ROUND_SCREEN,
    ASSET_REQUEST_RACE,
    ASSET_REQUEST_GRAND_PRIX_SCREEN,
    ASSET_REQUEST_COURSE,
    ASSET_REQUEST_TRACK_DATA
} AssetRequestType;

extern AssetRequestType g_AssetRequestType;

/* Asset-load state machine phase (0 idle; 1..6 drive LoadAsset loads). */
extern s32 g_AssetLoadState;

/* The asset sub-block currently being installed: `assetBase + <header offset>`,
 * then handed to UploadImageAsset / UploadImageEntry. */
extern u8 *g_AssetBlockPtr;

/* Its companion, the third pointer of the sub-block triple. */
extern u8 *g_AssetBlockPtr2;

/* Scratch image buffer: whatever the last screen load left free, handed to
 * LoadAsset and UploadImageAsset by the round / save / attract screens. */
extern u8 *g_ImageBlockBuffer;

/*
 * Index of the first entry of each variable-size family in that table. Read off
 * the retail path table and cross-checked against the 135-entry RAGE.BIN index
 * on the PAL disc.
 *
 * ROUND_SCREEN: [0x4A] = "\DATA\GP0.TMS". Six screens per series, the sixth
 * being GP10 / GP11, so LoadGrandPrixScreen wants base + series * 6 + class.
 *
 * TRACK_1ST / TRACK_2ND: [0x57] = "\PACK\BIG1.1ST", [0x58] its ".2ND" sibling.
 * Four courses (BIG, MID, HI, OVAL) x two packs = eight entries per class, so
 * both are indexed base + class * 8 + course * 2. Six classes fill [0x57..0x86],
 * which is exactly the end of the table.
 */
#define ASSET_ROUND_SCREEN_BASE 0x4A
#define ASSET_TIME_ATTACK_ROUND_SCREEN 0x55
#define ASSET_VOICE_BANK        0x56
#define ASSET_TRACK_1ST_BASE    0x57
#define ASSET_TRACK_2ND_BASE    0x58

enum {
    TRACK_ASSETS_PER_CLASS = 8,
    TRACK_ASSETS_PER_COURSE = 2,
};

static inline s32 TrackCourseAssetIndex(s32 base, s32 classIndex,
                                        s32 courseIndex) {
    return base + classIndex * TRACK_ASSETS_PER_CLASS +
           courseIndex * TRACK_ASSETS_PER_COURSE;
}
enum {
    ASSET_BOOT_LOGO = 0,
    ASSET_TITLE_SCREEN = 1,
    ASSET_BOOT_AUDIO_HEADER = 2,
    ASSET_BOOT_AUDIO_BODY = 3,
    ASSET_BOOT_RESOURCES = 4,
    ASSET_BOOT_CAR_SCREEN = 5,
    ASSET_SAVE_SCREEN = 6,
    ASSET_SELECT_BGM = 7,
    ASSET_CAR_SELECT_SCREEN = 8,
    ASSET_OPTION_SCREEN = 9
};

/* Load asset assetIndex into dst; returns loaded size/status. */
s32 LoadAsset(s32 assetIndex, void *dst);

/* Phase of LoadAsset's own CD state machine, 0..6 (seek, SetLoc, CdRead,
 * wait, success, failure). Sequences one transfer, unlike g_AssetLoadState. */
extern s16 g_CdLoadPhase;

typedef union GameCdPosition {
    u32 sectorOffset;
    CdlLOC location;
} GameCdPosition;

typedef struct GameCdLoadEntry {
    GameCdPosition position;
    u32 size;
} GameCdLoadEntry;

/* Disc location + size of every asset, read from the "\RAGE.BIN;1" index by
 * LoadDiscArchiveIndex and rebased onto its LBA. */
extern GameCdLoadEntry g_AssetCdEntries[];

/* The same for the 11 streams in "\RAGE.STR;1"; BeginClassFmv picks
 * `1 + class` in the Grand Prix and `5 + class` in the Extra GP. */
extern GameCdLoadEntry g_StreamCdEntries[];

/* The selected stream and its playback bounds, set by BeginIntroFmv and
 * friends and handed to the FMV streamer. The limit intentionally exceeds
 * the recorded size: the original player uses two passes for normal films
 * and four for the ending. */
extern GameCdLoadEntry *g_StreamLoc;
extern u32 g_StreamSectorCount;
extern u32 g_StreamSectorLimit;

/* Boot CD scratch buffer: the RAGE.BIN index first, then asset 0. */
extern s32 g_LoadBuffer[];

typedef s32 TrackTextureShadowRow[0xE0];

typedef union AssetAddress {
    void *pointer;
    struct CarImageData *carImage;
    struct ModelBankHeader *modelBank;
} AssetAddress;

static inline s32 *GetAssetWords(void *data) {
    return (s32 *)data;
}

static inline u8 *GetAssetBytes(void *data) {
    return (u8 *)data;
}

static inline TrackTextureShadowRow *GetTrackTextureShadowRows(void *data) {
    return (TrackTextureShadowRow *)data;
}

static inline u16 *GetAssetHalfwords(void *data) {
    return (u16 *)data;
}

static inline struct CourseModelAssetHeader *GetCourseModelAssetHeader(
    void *data) {
    return (struct CourseModelAssetHeader *)data;
}

static inline struct ModelBankHeader *GetModelBankHeader(void *data) {
    return (struct ModelBankHeader *)data;
}

static inline void *ResolveAssetAddress(void *base, s32 offset) {
    return (u8 *)base + offset;
}

typedef struct CarModelAsset {
    s16 modelOffsetX;
    s16 modelOffsetY;
    s16 modelOffsetZ;
    s16 horizon;
    u8 transmissionAvailable;
    u8 gearCount;
    u8 upgradesAvailable;
    u8 performanceRatings[3];
    u8 reserved0E[2];
    s16 maxPower;
    s16 maxPowerRpm;
    u8 maxTorqueFraction;
    u8 maxTorqueWhole;
    s16 maxTorqueRpm;
    s32 serializedModelSize;
    u8 reserved1C[4];
    AssetAddress modelData;
    AssetAddress imageData;
} CarModelAsset;

#define SERIALIZED_CAR_MODEL_HEADER_SIZE 0x28

typedef struct SerializedCarModelAssetHeader {
    u8 metadata[0x20];
    s32 modelOffset;
    s32 imageOffset;
} SerializedCarModelAssetHeader;

_Static_assert(offsetof(CarModelAsset, serializedModelSize) == 0x18,
               "serialized car model size must remain at +0x18");
_Static_assert(offsetof(CarModelAsset, modelData) == 0x20,
               "serialized car model offset must remain at +0x20");
_Static_assert(sizeof(SerializedCarModelAssetHeader) ==
                   SERIALIZED_CAR_MODEL_HEADER_SIZE,
               "serialized car model header must remain 0x28 bytes");

static inline SerializedCarModelAssetHeader *GetSerializedCarModelAssetHeader(
    void *data) {
    return (SerializedCarModelAssetHeader *)data;
}

static inline CarModelAsset *GetCarModelAsset(void *data) {
    return (CarModelAsset *)data;
}

extern CarModelAsset *g_CarModelAsset;

/* One VRAM upload record inside an image entry. UploadImageAsset walks the
 * outer entry chain; UploadImageEntry uploads its optional CLUT and pixels. */
typedef struct GameImageBlock {
    u32 size;   /* +0x00 block size in bytes, rounded down to a word */
    u16 x;      /* +0x04 VRAM destination */
    u16 y;      /* +0x06 */
    u16 w;      /* +0x08 in 16-bit words */
    u16 h;      /* +0x0A */
    u8 pixels[4]; /* +0x0C */
} GameImageBlock;

typedef union GameImageAssetHeaderWord {
    s32 size;
    s32 flags;
} GameImageAssetHeaderWord;

typedef struct GameImageEntryHeader {
    s32 reserved;
    u32 flags;
} GameImageEntryHeader;

enum {
    GAME_IMAGE_ENTRY_HAS_CLUT = 1 << 3
};

static inline GameImageAssetHeaderWord *GetImageAssetHeaderWords(
    void *data) {
    return (GameImageAssetHeaderWord *)data;
}

static inline GameImageEntryHeader *GetImageEntryHeader(void *data) {
    return (GameImageEntryHeader *)data;
}

/* The offset table every asset pack starts with; sub-blocks live at
 * base + offsets[n]. Some packs only ever use the first three. */
typedef struct GameSceneAssetHeader {
    s32 offsets[11];
} GameSceneAssetHeader;

typedef struct VoiceBankAssetHeader {
    s32 sharedHeaderSize;
    s32 audioHeaderOffset;
    s32 audioBodyOffset;
} VoiceBankAssetHeader;

static inline GameSceneAssetHeader *GetSceneAssetHeader(void *data) {
    return (GameSceneAssetHeader *)data;
}

static inline VoiceBankAssetHeader *GetVoiceBankAssetHeader(void *data) {
    return (VoiceBankAssetHeader *)data;
}

static inline void *GetSceneAssetAddress(GameSceneAssetHeader *header,
                                         s32 offset) {
    return (u8 *)header + offset;
}

/*
 * Asset-region pointers. All three address the load region in bytes: they are
 * advanced by byte counts (a load's returned size, TRACK_TEXTURE_SHADOW_SIZE,
 * g_SharedAssetWord0) and by offsets read out of the pack that happens to sit
 * there, so u8 * is the correct type. A pack header is a
 * view taken through GetSceneAssetHeader/GetVoiceBankAssetHeader at the asset
 * boundary; consumers then operate on a named serialized layout.
 */

/*
 * Byte size of the track-texture shadow copy StoreTeamLogoImage leaves at the
 * cursor, and hence what the cursor must skip before the .2ND pack is loaded
 * behind the .1ST. It is g_TrackTextureRect measured out: that rect is
 * {x 0x240, y 0x100, w 0x1C0, h 0x100}, and 0x1C0 * 0x100 16-bit pixels * 2
 * bytes = 0x38000 exactly.
 * Not a pack size - the largest .1ST on the disc is 0xB5830.
 */
#define TRACK_TEXTURE_SHADOW_SIZE 0x38000

/*
 * The showroom's double-buffered car-model slot. LoadCarModel /
 * LoadUpgradedCarModel load into g_CarModelBuffer or that plus one stride, so
 * the incoming model never lands on the one still being drawn; the buffer is
 * therefore two strides long, which is where g_ImageBlockBuffer starts.
 * The stride is generous rather than tight: the largest CAR_xx.1ST on the
 * retail PAL disc is 0xD4A0.
 */
#define CAR_MODEL_SLOT_SIZE   0x20000
#define CAR_MODEL_BUFFER_SIZE 0x40000

/* Base of the resident asset block; sub-block n is base + base->offsets[n]. */
extern u8 *g_AssetBase;
/* Load destination, advanced past each pack as it is loaded. */
extern u8 *g_AssetLoadCursor;

/*
 * One numbered block out of the scene asset the loader has just read.
 *
 * Every installer wants the same three steps: find the header, look the
 * block's offset up in it, turn that into an address. Twenty-nine call sites
 * wrote all three out, and each also left the address in g_AssetBlockPtr,
 * which is kept because the loader's own bookkeeping reads it.
 */
static inline void *SceneAssetBlock(s32 slot) {
    GameSceneAssetHeader *header = GetSceneAssetHeader(g_AssetLoadCursor);

    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[slot]);
    return g_AssetBlockPtr;
}
/* Second sub-block cursor: base + header->offsets[n + 1]. */
extern u8 *g_AssetSubBlockPtr;

/*
 * Asset-load state machine. ServiceAssetLoad runs once per frame and
 * dispatches g_AssetRequestType 1..12 to the GameLoad*Assets step below; each step
 * advances g_AssetLoadState until it reaches 0. A screen starts a load with the
 * matching GameRequest* (which sets g_AssetRequestType and returns 1 while busy) and
 * polls the same GameRequest* until it returns 0. Asset indices are documented
 * with the constants near the top of this header.
 */
void ServiceAssetLoad(void);
/* Cancel an in-flight load: aborts a running CdRead and clears all three
 * state words (g_CdLoadPhase / g_AssetLoadState / g_AssetRequestType). */
void ResetAssetLoader(void);
/* Spin on LoadAsset until the transfer completes. */
void LoadAssetBlocking(s32 assetIndex, void *dst);
/* Boot: read the "\RAGE.BIN;1" first sector into g_AssetCdEntries (135 entries)
 * and rebase the 11 "\RAGE.STR;1" stream entries. Prints "Now Searching [%s]". */
void LoadDiscArchiveIndex(void);
/* LoadDiscArchiveIndex, then blocking-load asset 0 (LOGO.TMS) and upload it. */
void InitAssetSystem(void);
/* Switch the drive to CD-DA mode (CdlSetmode 0x07 = report|autopause|CDDA);
 * the last step of every track load. */
s32 EnableCdAudioMode(void);

/* Phase 1: TITLE.TMS, RG3.VH + RG3.VB (the main VAB), RES.DAT, CAR.TMS. */
s32 RequestBootAssets(void);
void LoadBootAssets(void);
/* Phase 2: SAVE.TMS (memory-card screen). */
s32 RequestSaveScreenAssets(void);
void LoadSaveScreenAssets(void);
/* Phase 3: SELBGM.BIN, split into its SEQ / VH / VB sub-blocks. */
s32 RequestSelectBgmAssets(void);
s32 RequestSelectBgmAssetsKeepAudioSlots(void);
void LoadSelectBgmAssets(void);
/* Phase 4: upload the SELBGM bank, load SELECT.BIN and the player's CAR_xx.1ST. */
s32 RequestCarSelectAssets(void);
void LoadCarSelectAssets(void);
/* Phase 5/6: one car's CAR_xx.1ST pack into the double-buffered showroom slot;
 * the "Upgraded" pair asks for modelVariant + 1, i.e. the next grade's body.
 * The *Now wrappers request and then pump ServiceAssetLoad until idle. */
/* Phase 7: OPTION.BIN. */
void LoadOptionScreenAssets(void);
/* Phase 8: the GP*.TMS round screen (series * 6 + class + 0x4A) plus VOICE.BIN.
 * The request also rolls a random class when g_GrandPrixMode is 0. */
s32 RequestRoundAssets(void);
void LoadRoundAssets(void);
/* Phase 9: the whole race load - VOICE bank, the player's CAR_xx.2ND, then the
 * course's <COURSE>n.1ST and <COURSE>n.2ND packs. */
s32 RequestRaceAssets(void);
void LoadRaceAssets(void);
/* Phase 12: <COURSE>n.2ND, handing its 11 sub-blocks to the track subsystems. */
void LoadTrackDataAssets(void);
/* Unpack the already-resident <COURSE>n.1ST pack out of g_AssetBase (the same
 * work LoadRaceAssets does in its step 5). */
void InstallCourseAssets(void);
/* Copy the live car model into g_AssetBase and re-register its bank there. */
void RelocateCarModel(void);

typedef struct ModelBankHeader {
    u32 modelCount;
    s32 tableOffset;
    s32 normalsOffset;
    s32 modelOffsets[1];
} ModelBankHeader;

#define GAME_MODEL_BANK_LIMIT 16
#define GAME_MODEL_PER_BANK_LIMIT 256
typedef struct NativeModelBank {
    s32 modelCount;
    void *table;
    void *normals;
    void *models[GAME_MODEL_PER_BANK_LIMIT];
} NativeModelBank;

typedef struct OptionScreenAsset {
    s32 imageOffset;
    ModelBankHeader modelBank;
} OptionScreenAsset;

static inline OptionScreenAsset *GetOptionScreenAsset(void *data) {
    return (OptionScreenAsset *)data;
}

typedef struct TerrainCellAssetHeader {
    s32 cellCount;
    s32 facesOffset;
    s32 cellOffsets[1];
} TerrainCellAssetHeader;

#define GAME_TERRAIN_CELL_LIMIT 2048
extern void *g_NativeTerrainCells[GAME_TERRAIN_CELL_LIMIT];

typedef struct TerrainCellAsset {
    u16 grid[32][32];
    CellVisibilityRow visibility[32];
    TerrainCellAssetHeader header;
} TerrainCellAsset;

typedef struct CourseModelAssetEntry {
    s32 geometryOffset;
    s32 vertexCount;
    s32 modelOffset;
} CourseModelAssetEntry;

typedef struct CourseModelAssetHeader {
    s32 modelCount;
    CourseModelAssetEntry models[1];
} CourseModelAssetHeader;

#define GAME_COURSE_MODEL_LIMIT 256
typedef struct NativeCourseModel {
    void *geometry;
    s32 vertexCount;
    void *model;
} NativeCourseModel;
extern NativeCourseModel g_NativeCourseModels[GAME_COURSE_MODEL_LIMIT];

/* Asset-installation helpers. RegisterModelBank/RegisterCourseModels rebase a
 * pack's internal offsets to absolute addresses. The Set*Slot pair only
 * records a pointer in a small registry that Select/Upload reads. */
void UploadCarImage(s32 slot);

/* Declared identically by 42 translation units before this
 * header carried them. */

extern s32 g_PendingCarModelIndex;
extern TrackTextureShadowRow *g_TrackTextureShadow;

void InstallTerrainCellData(void *data);
void InstallCarModelAsset(CarModelAsset *asset, s32 slot, s32 carIndex);
void InstallTrackTextureAssetPack(u8 *base);
void InstallTrackEventData(struct TrackEventData *eventData);
void InstallTrackPoints(struct TrackPointTable *trackData);
void InstallTrackRuntimeAssetPack(s32 assetIndex, s32 useSeriesCamera);
void LoadCourseAssets(void);
void LoadGrandPrixScreen(void);
void RegisterModelBank(ModelBankHeader *base, s32 index);
void RegisterCourseModels(CourseModelAssetHeader *base);
s32 RequestRaceStart(void);
s32 RequestTrackLoad(void);
void RequestCarModel(s32 carIndex);
void RequestOptionScreenAssets(void);
void RequestUpgradedCarModel(s32 carIndex);
void ResetTrackTextureSwap(void);
void SetEnvironmentScript(struct GameEnvironmentScript *script);
void StoreTeamLogoImage(void* dst);
void UploadImageAsset(GameImageAssetHeaderWord *asset);
void UploadImageEntry(GameImageEntryHeader *entry);
void UploadLoadBufferImage(void);
s32 RequestTrackDataAssets(void);
s32 GetCarAssetIndex(s32 model, s32 grade);

/* Declared identically by 19 translation units before this
 * header carried them. */

extern s32 g_TerrainCellCount;
extern RECT g_CarImageRect;
struct CarImageData;
extern struct CarImageData *g_CarImageSlots[];
extern CarModelAsset *g_CarModelSlots[];
extern NativeModelBank g_ModelBanks[GAME_MODEL_BANK_LIMIT];
void LoadCarModel(s32);
void LoadUpgradedCarModel(s32);

/* Declared identically by 3 translation units before this
 * header carried them. */

extern Rect g_TeamLogoClutLoadRect;
extern GpuRectPacked g_TeamLogoClutMoveRect;
extern Rect g_TrackTextureRect;

#endif
