#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include "psyq/snd_types.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/state.h"
#include "game/work_buffer.h"
#include "game/sound.h"
#include "game/audio.h"
#include "game/cd_internal.h"
#include "game/memcard.h"

GameRenderState g_RenderState;
CarTrackWork g_CarTrackWork;

GameFrameContext g_FrameContexts[2];
NativeModelBank g_ModelBanks[GAME_MODEL_BANK_LIMIT];
const void *g_NativeTerrainCells[GAME_TERRAIN_CELL_LIMIT];
NativeCourseModel g_NativeCourseModels[GAME_COURSE_MODEL_LIMIT];
CarModelAsset *g_CarModelSlots[CAR_ASSET_SLOT_COUNT];
GameCdLoadEntry g_AssetCdEntries[GAME_ASSET_COUNT];
Matrix g_SceneColorMatrix;
Matrix g_SceneLightMatrix;
Matrix g_TrackColorMatrix;
Matrix g_TrackLightMatrix;
Matrix g_DefaultColorMatrix;
Matrix g_DefaultLightMatrix;
Matrix g_MenuColorMatrix;
Matrix g_MenuLightMatrix;
/* The symbol map only describes the first 0x30 bytes of g_Cars and the first
 * word of g_PlayerCar.  They are complete 0x19c-byte runtime records in the
 * game, so byte-array storage generated from those annotations corrupts the
 * following native globals as soon as BuildStartingGrid initializes them. */
GameCarRuntime g_Cars[11];
PlayerCarRuntime g_PlayerCar;
GameRaceProgress g_GrandPrixSave;
GameRaceProgress g_ExtraGrandPrixSave;
GameRaceProgress g_TimeAttackSave;
SeqStruct g_SndTableArea[2];

struct SeqStruct *GetSndTableArea(void) {
    return g_SndTableArea;
}

GameSpriteDesc g_RaceHudSpriteDescsGp[GRAND_PRIX_HUD_SPRITE_COUNT];
GameSpriteDesc
    g_RaceHudSpriteDescsTimeTrial[TIME_ATTACK_HUD_SPRITE_COUNT];
RaceGridSlot g_RaceGridSlots[RACE_GRID_STORAGE_COUNT];
RaceGridSlot g_AttractGridSlots[RACE_GRID_STORAGE_COUNT];
RECT g_CarImageRect;
RenderBufferAddress g_TileStripBuffers[2];
u8 g_TileStripStorage[2 * 512 * sizeof(TILE)];
u32 g_CountdownDigitPatterns[16];
CVec g_CountdownCellColors[4];
char g_TimeTextBuffer[12];
GameWorkBuffer g_ReplayFrameBuffer;
RaceRecord g_RankingRecords[2][4][5];
RaceRecord g_TimeRecords[2][4][5];
_Static_assert(sizeof(RaceRecord) == 16, "RaceRecord must retain the game ABI");
_Static_assert(sizeof(g_RankingRecords) == 640, "ranking table ABI changed");
_Static_assert(sizeof(g_TimeRecords) == 640, "time table ABI changed");
Rect g_TrackTextureRowRect;
u8 g_TrackTextureShadowPage[256];
u16 g_PadButtonMapping[16];
u16 g_PadButtonPresets[CONTROLLER_MAPPING_COUNT]
                      [CONTROLLER_MAPPING_BUTTON_COUNT];
u16 g_NegconButtonPresets[CONTROLLER_MAPPING_COUNT]
                         [CONTROLLER_MAPPING_BUTTON_COUNT];
/* InitPAD writes two 0x28-byte BIOS packets. Retail's byte symbols at +1..+3
 * are aliases into the first packet, not independent objects. */
u8 g_PadBuffers[0x50];
PadState g_PadState;
u8 g_PadType;
u16 g_PadPrevHeld;
u16 g_PadHeld;
u16 g_PadPressed;
u16 g_PadPressedRepeat;
u8 g_PadRepeatTimer;
s16 g_NegconAnalogI;
s16 g_NegconAnalogII;
s16 g_NegconAnalogL;
s16 g_NegconSteer;
SoundScale g_SoundScale;
SoundCueParams g_SoundCueParams[MAIN_SOUND_CUE_COUNT];
SoundCueParams g_SoundCueParams2[RACE_SOUND_CUE_COUNT];
EffectCueBank g_EffectCueTable[EFFECT_CUE_BANK_COUNT];
s32 g_SpecialVoiceBits[SPECIAL_VOICE_BIT_COUNT];
s32 g_VabSpuAddress[AUDIO_SLOT_COUNT];
CdlLOC g_CdTrackLocs[CD_TRACK_LOCATION_COUNT];
CdlLOC *g_CdBgmTrackLocs = &g_CdTrackLocs[2];
CdlLOC g_CdTrackLoopPoint[CD_TRACK_LOCATION_COUNT];
_Static_assert(sizeof(GameCarRuntime) == 0x19c,
               "GameCarRuntime must retain its retail stride");
_Static_assert(sizeof(PlayerCarRuntime) == 0x19c,
               "PlayerCarRuntime must retain its retail stride");

TimedDrawCommand g_CourseSelectGpScript[10];
TimedDrawCommand g_CourseSelectTimeAttackScript[10];
TimedDrawCommand g_UiChromeScript[16];
TimedDrawCommand g_MenuRowScript[FADING_MENU_ROW_COUNT + 1];
TimedDrawCommand g_UiEmptyScript[1];
TimedDrawCommand g_MenuHintBarScript[61];
TimedDrawCommand g_CarSelectMenuScriptGp[18];
TimedDrawCommand g_CarSelectMenuScriptTimeAttack[12];
TimedDrawCommand g_UiChromeScript2[9];
#define RAGE_DEFINE_NATIVE_UI_SCRIPT(name, count) \
    TimedDrawCommand g_Native##name[count]
RAGE_DEFINE_NATIVE_UI_SCRIPT(RankingPanelScript, 5);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CustomizeMenuScriptGp, 13);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CustomizeMenuScriptTimeAttack, 11);
RAGE_DEFINE_NATIVE_UI_SCRIPT(DesignModeScript, 16);
RAGE_DEFINE_NATIVE_UI_SCRIPT(TeamLogoScreenScript, 12);
RAGE_DEFINE_NATIVE_UI_SCRIPT(LogoSampleScreenScript, 12);
RAGE_DEFINE_NATIVE_UI_SCRIPT(TeamNameScreenScript, 61);
RAGE_DEFINE_NATIVE_UI_SCRIPT(PaintColorScreenScript, 15);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CarShopScreenScript, 9);
RAGE_DEFINE_NATIVE_UI_SCRIPT(EngineerShopScreenScript, 68);
RAGE_DEFINE_NATIVE_UI_SCRIPT(MenuDialogPanelUpperScript, 4);
RAGE_DEFINE_NATIVE_UI_SCRIPT(MenuDialogPanelLowerScript, 8);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CourseSelectSavePromptScript, 4);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CourseSelectSavePromptBanner, 2);
RAGE_DEFINE_NATIVE_UI_SCRIPT(MenuRow0MarkerScript, 4);
RAGE_DEFINE_NATIVE_UI_SCRIPT(MenuRow1MarkerScript, 16);
RAGE_DEFINE_NATIVE_UI_SCRIPT(RankingMenuScript, 9);
RAGE_DEFINE_NATIVE_UI_SCRIPT(TransmissionUnavailableScript, 4);
RAGE_DEFINE_NATIVE_UI_SCRIPT(TeamLogoScreenScript2, 2);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CarShopUnavailableScript, 2);
RAGE_DEFINE_NATIVE_UI_SCRIPT(EngineerShopUnavailableScript, 3);
RAGE_DEFINE_NATIVE_UI_SCRIPT(EngineerShopNoFundsScript, 2);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CarShopNoFundsScript, 5);
RAGE_DEFINE_NATIVE_UI_SCRIPT(DesignModeDeniedScript, 2);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript2, 7);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript1, 7);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript3, 7);
RAGE_DEFINE_NATIVE_UI_SCRIPT(CarShopBuyPromptScript4, 7);
RAGE_DEFINE_NATIVE_UI_SCRIPT(EngineerShopTuneUpPromptScript, 5);
#undef RAGE_DEFINE_NATIVE_UI_SCRIPT
char g_FmtBgmNumber[] = "%d";
static char g_BgmName00[] = "\"RANDOM PLAY\"";
static char g_BgmName01[] = "\"RAGE RACER\"";
static char g_BgmName02[] = "\"MATHEMABEAT\"";
static char g_BgmName03[] = "\"LIGHTNING LUGE\"";
static char g_BgmName04[] = "\"INDUSTRIA\"";
static char g_BgmName05[] = "\"HURRICANE HUB\"";
static char g_BgmName06[] = "\"MECH MONSTER\"";
static char g_BgmName07[] = "\"SILVER STREAM\"";
static char g_BgmName08[] = "\"STIMULATION\"";
static char g_BgmName09[] = "\"VOLCANO VEHICLE\"";
static char g_BgmName10[] = "\"DEEP DRIVE\"";
char *g_BgmTrackNames[11] = {
    g_BgmName00, g_BgmName01, g_BgmName02, g_BgmName03, g_BgmName04,
    g_BgmName05, g_BgmName06, g_BgmName07, g_BgmName08, g_BgmName09,
    g_BgmName10
};
static char g_GrandPrixName00[] = "CALME";
static char g_GrandPrixName01[] = "BRISE";
static char g_GrandPrixName02[] = "RAFALE";
static char g_GrandPrixName03[] = "MISTRAL";
static char g_GrandPrixName04[] = "TEMPETE";
static char g_GrandPrixName05[] = "DIABLE";
static char g_GrandPrixName06[] = "AISANCE";
static char g_GrandPrixName07[] = "AGITATION";
static char g_GrandPrixName08[] = "IRRITATION";
static char g_GrandPrixName09[] = "COLERE";
static char g_GrandPrixName10[] = "RAGE";
char *g_GrandPrixNames[11] = {
    g_GrandPrixName00, g_GrandPrixName01, g_GrandPrixName02,
    g_GrandPrixName03, g_GrandPrixName04, g_GrandPrixName05,
    g_GrandPrixName06, g_GrandPrixName07, g_GrandPrixName08,
    g_GrandPrixName09, g_GrandPrixName10
};
static char g_CourseName00[] = "MYTHICAL COAST";
static char g_CourseName01[] = "OVER PASS CITY";
static char g_CourseName02[] = "LAKESIDE GATE";
static char g_CourseName03[] = "THE EXTREME OVAL";
char *g_CourseNames[4] = {
    g_CourseName00, g_CourseName01, g_CourseName02, g_CourseName03
};
static u8 g_PlaceSuffix1st[] = "1ST";
static u8 g_PlaceSuffix2nd[] = "2ND";
static u8 g_PlaceSuffix3rd[] = "3RD";
static u8 g_PlaceSuffix4th[] = "4TH";
static u8 g_PlaceSuffix5th[] = "5TH";
u8 *g_NativePlaceSuffixNames[5] = {
    g_PlaceSuffix1st, g_PlaceSuffix2nd, g_PlaceSuffix3rd,
    g_PlaceSuffix4th, g_PlaceSuffix5th
};
static char g_CarNameSqualdon[] = "SQUALDON";
static char g_CarNameBulshade[] = "BULSHADE";
static char g_CarNameVainqure[] = "VAINQURE";
static char g_CarNameGhepardo[] = "GHEPARDO";
static char g_CarNameIstante[] = "ISTANTE";
static char g_CarNameFatalita[] = "FATALITA";
static char g_CarNameHijack[] = "HIJACK";
static char g_CarNameBayonet[] = "BAYONET";
static char g_CarNameAcceron[] = "ACCERON";
static char g_CarNameEsperanza[] = "ESPERANZA";
static char g_CarNamePegase[] = "PEGASE";
static char g_CarNameAbeille[] = "ABEILLE";
static char g_CarNameErriso[] = "ERRISO";
/* Retail g_CarNames stores pointers from the last adjacent string back to the
 * first. The model index order therefore runs ERRISO..SQUALDON; using the
 * declaration order made record tables label every car as another model. */
const char *g_NativeCarNames[GAME_CAR_COUNT] = {
    g_CarNameErriso, g_CarNameAbeille, g_CarNamePegase,
    g_CarNameEsperanza, g_CarNameAcceron, g_CarNameBayonet,
    g_CarNameHijack, g_CarNameFatalita, g_CarNameIstante,
    g_CarNameGhepardo, g_CarNameVainqure, g_CarNameBulshade,
    g_CarNameSqualdon
};
static char g_CarClassAge[] = "AGE";
static char g_CarClassGnade[] = "GNADE";
static char g_CarClassLizard[] = "LIZARD";
static char g_CarClassAssolute[] = "ASSOLUTE";
const char *g_NativeCarClassNames[GAME_CAR_COUNT] = {
    g_CarClassAge, g_CarClassAge, g_CarClassAge, g_CarClassGnade,
    g_CarClassLizard, g_CarClassLizard, g_CarClassLizard,
    g_CarClassAssolute, g_CarClassAssolute, g_CarClassAssolute,
    g_CarClassAge, g_CarClassLizard, g_CarClassAssolute
};
static char g_CarManufacturerAge[] = "AGE";
static char g_CarManufacturerGnade[] = "GNADE";
static char g_CarManufacturerLeizard[] = "LEIZARD";
static char g_CarManufacturerAssoluto[] = "ASSOLUTO";
const char *g_NativeCarManufacturerNames[GAME_CAR_COUNT] = {
    g_CarManufacturerAge, g_CarManufacturerAge, g_CarManufacturerAge,
    g_CarManufacturerGnade, g_CarManufacturerLeizard,
    g_CarManufacturerLeizard, g_CarManufacturerLeizard,
    g_CarManufacturerAssoluto, g_CarManufacturerAssoluto,
    g_CarManufacturerAssoluto, g_CarManufacturerAge,
    g_CarManufacturerLeizard, g_CarManufacturerAssoluto
};
static char g_EngineSpecLabel00[] = "CAR-0";
static char g_EngineSpecLabel01[] = "CAR-1";
static char g_EngineSpecLabel02[] = "CAR-2";
static char g_EngineSpecLabel03[] = "CAR-3";
static char g_EngineSpecLabel04[] = "CAR-4";
static char g_EngineSpecLabel05[] = "CAR-5";
static char g_EngineSpecLabel06[] = "CAR-6";
static char g_EngineSpecLabel07[] = "CAR-7";
static char g_EngineSpecLabel08[] = "CAR-8";
static char g_EngineSpecLabel09[] = "CAR-9";
static char g_EngineSpecLabel10[] = "CAR-A";
static char g_EngineSpecLabel11[] = "CAR-B";
static char g_EngineSpecLabel12[] = "CAR-C";
const char *g_NativeEngineSpecLabels[GAME_CAR_COUNT] = {
    g_EngineSpecLabel00, g_EngineSpecLabel01, g_EngineSpecLabel02,
    g_EngineSpecLabel03, g_EngineSpecLabel04, g_EngineSpecLabel05,
    g_EngineSpecLabel06, g_EngineSpecLabel07, g_EngineSpecLabel08,
    g_EngineSpecLabel09, g_EngineSpecLabel10, g_EngineSpecLabel11,
    g_EngineSpecLabel12
};
char g_SaveTitleSjis[212];
char g_SaveFilePath[80];
char g_FmtCardDevice[12];
char g_FmtCardWildcard[12];
char g_FmtPlayTime[14];
char g_FmtSaveRow[6];
char g_FmtSaveRowTail[3];
char g_FmtSaveRowEmpty[15];
u8 g_SaveNameCharset[44];
static char g_McMessage00[] = "Select file to save.";
static char g_McMessage01[] = "Select file to load.";
static char g_McMessage02[] = "No Memory card.";
static char g_McMessage03[] = "Please insert a Memory card.";
static char g_McMessage04[] = "Memory card full.";
static char g_McMessage05[] = "Please delete a file.";
static char g_McMessage06[] = "No data in Memory card.";
static char g_McMessage07[] = "Please insert another card.";
static char g_McMessage08[] = "New Memory card.";
static char g_McMessage09[] = "Format Memory card?";
static char g_McMessage10[] = "                      Yes       No";
static char g_McMessage11[] = "Overwrite old file?";
static char g_McMessage17[] = "Now accessing Memory card.";
static char g_McMessage18[] = "Memory card error.";
static char g_McMessage19[] = "LOAD DATA OK!";
static char g_McMessage20[] = "SAVE DATA OK!";
static char g_McMessage21[] = "FORMAT DATA OK!";
static char g_McMessage22[] = "No file found.";
static char g_McMessage23[] = "Choose another file.";
#define MC_MESSAGE_ROW(message_, flags_) \
    {.text = (message_), .column = (flags_), .reserved = {0, 0, 0}}
static MemoryCardMessageRow g_RageMcMessageRowStorage[24] = {
    MC_MESSAGE_ROW(g_McMessage00, 0), MC_MESSAGE_ROW(g_McMessage01, 0), MC_MESSAGE_ROW(g_McMessage02, 2),
    MC_MESSAGE_ROW(g_McMessage03, 0), MC_MESSAGE_ROW(g_McMessage04, 2), MC_MESSAGE_ROW(g_McMessage05, 0),
    MC_MESSAGE_ROW(g_McMessage06, 2), MC_MESSAGE_ROW(g_McMessage07, 0), MC_MESSAGE_ROW(g_McMessage08, 0),
    MC_MESSAGE_ROW(g_McMessage09, 2), MC_MESSAGE_ROW(g_McMessage10, 0), MC_MESSAGE_ROW(g_McMessage11, 2),
    MC_MESSAGE_ROW(g_McMessage10, 0), MC_MESSAGE_ROW(g_McMessage11, 2), MC_MESSAGE_ROW(g_McMessage10, 0),
    MC_MESSAGE_ROW(g_McMessage11, 2), MC_MESSAGE_ROW(g_McMessage10, 0), MC_MESSAGE_ROW(g_McMessage17, 0),
    MC_MESSAGE_ROW(g_McMessage18, 0), MC_MESSAGE_ROW(g_McMessage19, 0), MC_MESSAGE_ROW(g_McMessage20, 0),
    MC_MESSAGE_ROW(g_McMessage21, 0), MC_MESSAGE_ROW(g_McMessage22, 2), MC_MESSAGE_ROW(g_McMessage23, 0)
};
#undef MC_MESSAGE_ROW
MemoryCardMessageRow *g_McMessageRows[MEMORY_CARD_MESSAGE_COUNT] = {
    &g_RageMcMessageRowStorage[0], &g_RageMcMessageRowStorage[1],
    &g_RageMcMessageRowStorage[2], &g_RageMcMessageRowStorage[4],
    &g_RageMcMessageRowStorage[6], &g_RageMcMessageRowStorage[8],
    &g_RageMcMessageRowStorage[9], &g_RageMcMessageRowStorage[9],
    &g_RageMcMessageRowStorage[11], &g_RageMcMessageRowStorage[11],
    &g_RageMcMessageRowStorage[13], &g_RageMcMessageRowStorage[13],
    &g_RageMcMessageRowStorage[15], &g_RageMcMessageRowStorage[15],
    &g_RageMcMessageRowStorage[17], &g_RageMcMessageRowStorage[18],
    &g_RageMcMessageRowStorage[19], &g_RageMcMessageRowStorage[20],
    &g_RageMcMessageRowStorage[21], &g_RageMcMessageRowStorage[22]
};
s16 g_McMessageColumnX[MEMORY_CARD_MESSAGE_COLUMN_COUNT];
char g_McSlotLabels[9];
char g_McSlotLabelNoFile[8];
char g_McSlotLabelError[11];
static char g_CdAudioName00[] = "\\CDDA\\DA02PRO.DA;1";
static char g_CdAudioName01[] = "\\CDDA\\DA03TECH.DA;1";
static char g_CdAudioName02[] = "\\CDDA\\DA04HC.DA;1";
static char g_CdAudioName03[] = "\\CDDA\\DA05JNGL.DA;1";
static char g_CdAudioName04[] = "\\CDDA\\DA06JGL2.DA;1";
static char g_CdAudioName05[] = "\\CDDA\\DA07DNCE.DA;1";
static char g_CdAudioName06[] = "\\CDDA\\DA08GTR.DA;1";
static char g_CdAudioName07[] = "\\CDDA\\DA09JGL1.DA;1";
static char g_CdAudioName08[] = "\\CDDA\\DA10TRNC.DA;1";
static char g_CdAudioName09[] = "\\CDDA\\DA11VOCD.DA;1";
static char g_CdAudioName10[] = "\\CDDA\\DA12RPLY.DA;1";
static char g_CdAudioName11[] = "\\CDDA\\DA13RPTM.DA;1";
static char g_CdAudioName12[] = "\\CDDA\\DA14NAME.DA;1";
static char g_CdAudioName13[] = "\\CDDA\\DA15OVER.DA;1";
static char g_CdAudioName14[] = "\\CDDA\\DA16END1.DA;1";
static char g_CdAudioName15[] = "\\CDDA\\DA17EXTR.DA;1";
char *g_CdAudioFileNames[16] = {
    g_CdAudioName00, g_CdAudioName01, g_CdAudioName02, g_CdAudioName03,
    g_CdAudioName04, g_CdAudioName05, g_CdAudioName06, g_CdAudioName07,
    g_CdAudioName08, g_CdAudioName09, g_CdAudioName10, g_CdAudioName11,
    g_CdAudioName12, g_CdAudioName13, g_CdAudioName14, g_CdAudioName15
};
s32 g_DefaultLapTimes[8];
s32 g_DefaultTotalTimes[8];
s16 g_AtanTable[1026];

typedef struct RageSerializedTimedDrawCommand {
    s16 time;
    s16 type;
    u32 shapeAddress;
    u32 motionAddress;
} RageSerializedTimedDrawCommand;

typedef struct NativeTimedDrawScript {
    TimedDrawCommand *commands;
    size_t commandCount;
    u32 retailAddress;
} NativeTimedDrawScript;

enum {
    RAGE_UI_SCRIPT_DATA_ADDRESS = 0x8007fb50u,
    RAGE_UI_SCRIPT_DATA_SIZE = 0x321cu
};
extern const unsigned char g_UiScriptData[RAGE_UI_SCRIPT_DATA_SIZE];

static void *ResolveUiDataAddress(u32 address, size_t size) {
    size_t offset;

    if (address < RAGE_UI_SCRIPT_DATA_ADDRESS) return NULL;
    offset = (size_t)(address - RAGE_UI_SCRIPT_DATA_ADDRESS);
    if (offset > RAGE_UI_SCRIPT_DATA_SIZE ||
        size > RAGE_UI_SCRIPT_DATA_SIZE - offset) {
        return NULL;
    }
    return (void *)(g_UiScriptData + offset);
}

static int LoadTimedDrawScript(
    TimedDrawCommand *destination, size_t count, u32 retailAddress) {
    const RageSerializedTimedDrawCommand *source;
    size_t i;

    if (count > RAGE_UI_SCRIPT_DATA_SIZE /
                    sizeof(RageSerializedTimedDrawCommand)) {
        return 0;
    }
    source = ResolveUiDataAddress(
        retailAddress, count * sizeof(RageSerializedTimedDrawCommand));
    if (source == NULL) return 0;
    for (i = 0; i < count; i++) {
        destination[i].time = source[i].time;
        destination[i].type = source[i].type;
        if (source[i].time < 0) {
            destination[i].shape.value = (s32)source[i].shapeAddress;
            destination[i].motion.value = (s32)source[i].motionAddress;
        } else {
            destination[i].shape.pointer =
                ResolveUiDataAddress(source[i].shapeAddress, 1);
            destination[i].motion.pointer =
                ResolveUiDataAddress(source[i].motionAddress, 1);
            if (destination[i].shape.pointer == NULL ||
                destination[i].motion.pointer == NULL) {
                return 0;
            }
        }
    }
    return 1;
}

#define NATIVE_UI_SCRIPT(commands_, address_)                              \
    {(commands_), sizeof(commands_) / sizeof((commands_)[0]), (address_)}

static const NativeTimedDrawScript s_nativeUiScripts[] = {
    NATIVE_UI_SCRIPT(g_CourseSelectGpScript, 0x800817a0u),
    NATIVE_UI_SCRIPT(g_CourseSelectTimeAttackScript, 0x80081818u),
    NATIVE_UI_SCRIPT(g_UiChromeScript, 0x80082460u),
    NATIVE_UI_SCRIPT(g_MenuRowScript, 0x80082520u),
    NATIVE_UI_SCRIPT(g_UiEmptyScript, 0x80082568u),
    NATIVE_UI_SCRIPT(g_MenuHintBarScript, 0x80082a90u),
    NATIVE_UI_SCRIPT(g_CarSelectMenuScriptGp, 0x800818ccu),
    NATIVE_UI_SCRIPT(g_CarSelectMenuScriptTimeAttack, 0x800819a4u),
    NATIVE_UI_SCRIPT(g_UiChromeScript2, 0x80082790u),
    NATIVE_UI_SCRIPT(g_NativeRankingPanelScript, 0x80081890u),
    NATIVE_UI_SCRIPT(g_NativeCustomizeMenuScriptGp, 0x80081a34u),
    NATIVE_UI_SCRIPT(g_NativeCustomizeMenuScriptTimeAttack, 0x80081ad0u),
    NATIVE_UI_SCRIPT(g_NativeDesignModeScript, 0x80081b54u),
    NATIVE_UI_SCRIPT(g_NativeTeamLogoScreenScript, 0x80081c14u),
    NATIVE_UI_SCRIPT(g_NativeLogoSampleScreenScript, 0x80081ca4u),
    NATIVE_UI_SCRIPT(g_NativeTeamNameScreenScript, 0x80081d34u),
    NATIVE_UI_SCRIPT(g_NativePaintColorScreenScript, 0x80082010u),
    NATIVE_UI_SCRIPT(g_NativeCarShopScreenScript, 0x800820c4u),
    NATIVE_UI_SCRIPT(g_NativeEngineerShopScreenScript, 0x80082130u),
    NATIVE_UI_SCRIPT(g_NativeMenuDialogPanelUpperScript, 0x80082574u),
    NATIVE_UI_SCRIPT(g_NativeMenuDialogPanelLowerScript, 0x800825a4u),
    NATIVE_UI_SCRIPT(g_NativeCourseSelectSavePromptScript, 0x80082604u),
    NATIVE_UI_SCRIPT(g_NativeCourseSelectSavePromptBanner, 0x800827fcu),
    NATIVE_UI_SCRIPT(g_NativeMenuRow0MarkerScript, 0x80082634u),
    NATIVE_UI_SCRIPT(g_NativeMenuRow1MarkerScript, 0x80082664u),
    NATIVE_UI_SCRIPT(g_NativeRankingMenuScript, 0x80082724u),
    NATIVE_UI_SCRIPT(g_NativeTransmissionUnavailableScript, 0x80082814u),
    NATIVE_UI_SCRIPT(g_NativeTeamLogoScreenScript2, 0x80082844u),
    NATIVE_UI_SCRIPT(g_NativeCarShopUnavailableScript, 0x8008285cu),
    NATIVE_UI_SCRIPT(g_NativeEngineerShopUnavailableScript, 0x80082874u),
    NATIVE_UI_SCRIPT(g_NativeEngineerShopNoFundsScript, 0x80082898u),
    NATIVE_UI_SCRIPT(g_NativeCarShopNoFundsScript, 0x800828b0u),
    NATIVE_UI_SCRIPT(g_NativeDesignModeDeniedScript, 0x800828ecu),
    NATIVE_UI_SCRIPT(g_NativeCarShopBuyPromptScript2, 0x80082904u),
    NATIVE_UI_SCRIPT(g_NativeCarShopBuyPromptScript1, 0x80082958u),
    NATIVE_UI_SCRIPT(g_NativeCarShopBuyPromptScript3, 0x800829acu),
    NATIVE_UI_SCRIPT(g_NativeCarShopBuyPromptScript4, 0x80082a00u),
    NATIVE_UI_SCRIPT(g_NativeEngineerShopTuneUpPromptScript, 0x80082a54u),
};

#undef NATIVE_UI_SCRIPT

int InitNativeGameData(void) {
    size_t index;

    for (index = 0;
         index < sizeof(s_nativeUiScripts) / sizeof(s_nativeUiScripts[0]);
         index++) {
        const NativeTimedDrawScript *script = &s_nativeUiScripts[index];

        if (!LoadTimedDrawScript(script->commands, script->commandCount,
                                 script->retailAddress)) {
            return 0;
        }
    }
    return 1;
}
static const char g_RagePrologueText0[] = "NO ONE KNOWS HOW THE RACE BEGAN";
static const char g_RagePrologueText1[] = "OR HOW ITS DRIVERS BECAME KNOWN";
static const char g_RagePrologueText2[] = "AS RAGE RACERS.";
static const char g_RagePrologueText3[] = "THEY LIVE DANGEROUSLY CLOSE TO THE";
static const char g_RagePrologueText4[] = "EDGE; PUSHING BOTH THEIR CARS AND";
static const char g_RagePrologueText5[] = "THEMSELVES TO THE LIMIT. THEY FEED ON";
static const char g_RagePrologueText6[] = "THE BURNING FIRE OF COMPETITION;";
static const char g_RagePrologueText7[] = "ARE FUELED BY THE RUSH OF SPEED;";
static const char g_RagePrologueText8[] = "AND SHARE AN OVERWHELMING DESIRE";
static const char g_RagePrologueText9[] = "FOR VICTORY.";
static const char g_RagePrologueText10[] = "FOR THESE ELITE FEW, THE WHOLE";
static const char g_RagePrologueText11[] = "WORLD IS THEIR SPEEDWAY; BUT THERE";
static const char g_RagePrologueText12[] = "CAN ONLY BE ONE ULTIMATE...";
static const char g_RagePrologueText13[] = "RAGE RACER!";
PrologueLine g_PrologueLines[17] = {
    {36, 6, g_RagePrologueText0}, {36, 23, g_RagePrologueText1},
    {100, 40, g_RagePrologueText2}, {24, 66, g_RagePrologueText3},
    {28, 83, g_RagePrologueText4}, {12, 100, g_RagePrologueText5},
    {32, 116, g_RagePrologueText6}, {32, 133, g_RagePrologueText7},
    {32, 150, g_RagePrologueText8}, {112, 166, g_RagePrologueText9},
    {40, 193, g_RagePrologueText10}, {24, 210, g_RagePrologueText11},
    {52, 226, g_RagePrologueText12}, {116, 253, g_RagePrologueText13},
};
s32 g_PrologueLineCount = 14;
PrologueCameraCut g_PrologueCameraCuts[11] = {
    {155, 4}, {228, 5}, {299, 6}, {391, 7}, {516, 8}, {609, 0},
    {650, 1}, {800, 9}, {891, 10}, {931, 2}, {0, 0},
};
/* Where the eleven movies sit in RAGE.STR and how many frames of each the game
 * shows.  Retail had one disc and could compile the answer in; the port mounts
 * whichever disc the player owns and the American pressing puts the movies
 * elsewhere, so HostInitDisc fills this in: retail's PAL table from
 * g_RetailPalStreamTable, then the mounted disc's own table over the top. */
GameCdLoadEntry g_StreamCdEntries[FMV_STREAM_COUNT];
/* Retail main.exe @ 0x8007C2F8: (u, v) cells for ASCII 0x20..0x7f. */
u8 g_Font8x8Cells[0xC0] = {
    0x00,0x00,0x05,0x01,0x06,0x01,0x07,0x01,0x08,0x01,0x09,0x01,
    0x0a,0x01,0x0b,0x01,0x0c,0x01,0x0d,0x01,0x0e,0x01,0x0f,0x01,
    0x10,0x01,0x11,0x01,0x12,0x01,0x13,0x01,0x00,0x00,0x01,0x00,
    0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00,0x07,0x00,
    0x08,0x00,0x09,0x00,0x14,0x01,0x15,0x01,0x16,0x01,0x17,0x01,
    0x18,0x01,0x19,0x01,0x04,0x01,0x0a,0x00,0x0b,0x00,0x0c,0x00,
    0x0d,0x00,0x0e,0x00,0x0f,0x00,0x10,0x00,0x11,0x00,0x12,0x00,
    0x13,0x00,0x14,0x00,0x15,0x00,0x16,0x00,0x17,0x00,0x18,0x00,
    0x19,0x00,0x1a,0x00,0x1b,0x00,0x1c,0x00,0x1d,0x00,0x1e,0x00,
    0x1f,0x00,0x00,0x01,0x01,0x01,0x02,0x01,0x03,0x01,0x1b,0x01,
    0x1c,0x01,0x1d,0x01,0x1e,0x01,0x1f,0x01,0x1a,0x01,0x00,0x03,
    0x01,0x03,0x02,0x03,0x03,0x03,0x0e,0x00,0x0f,0x00,0x10,0x00,
    0x11,0x00,0x12,0x00,0x13,0x00,0x14,0x00,0x15,0x00,0x16,0x00,
    0x17,0x00,0x18,0x00,0x19,0x00,0x1a,0x00,0x1b,0x00,0x1c,0x00,
    0x1d,0x00,0x1e,0x00,0x1f,0x00,0x00,0x01,0x01,0x01,0x02,0x01,
    0x03,0x01,0x1b,0x01,0x1c,0x01,0x1d,0x01,0x1e,0x01,0x1f,0x01,
};
BootLogoState g_BootLogoState = BOOT_LOGO_STATE_FADE_IN;
s32 g_BootLogoTimer = 0;
s32 g_BootLogoHoldTimer = 150;
