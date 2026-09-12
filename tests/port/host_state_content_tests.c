/*
 * What the retail data actually is, byte for byte.
 *
 * The host_state_*.c files are the retail data segment transcribed into C.
 * Strings written as hexadecimal became strings, arrays that swallowed their
 * neighbours were cut apart and named, and the original monolith was split by
 * subsystem. Meaningful data has to retain its bytes; padding and dead retail
 * pointer tables are deliberately dropped.
 *
 * So this folds the contents themselves. Only the bytes are folded, never the
 * names or the sizes, which is deliberate: cutting one array into two named
 * halves leaves the same bytes in the same order and must therefore leave
 * this number alone. The order below is the canonical one and does not follow
 * whichever file a symbol ends up being defined in.
 *
 * Buffers are left out. Two hundred and thirty-three of the arrays carry no
 * initialiser and so have nothing to preserve; their sizes are the manifest's
 * business.
 *
 * Generated once from the arrays that had initialisers. Add an entry by hand
 * when data is added, which the manifest will have told you about first.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "game/car_render_rules.h"
#include "game/cd.h"
#include "game/menu_types.h"
#include "game/race_hud_internal.h"
#include "game/result_screen_types.h"
#include "game/render_internal.h"
#include "game/team_logo.h"
#include "game/visible_cell_scan.h"

typedef struct ContentCarPoint {
    int16_t x;
    int16_t z;
} ContentCarPoint;

typedef struct ContentLaunchSpeedThreshold {
    int16_t initial;
    int16_t sustain;
} ContentLaunchSpeedThreshold;

typedef struct ContentSVec {
    int16_t vx;
    int16_t vy;
    int16_t vz;
    int16_t pad;
} ContentSVec;

typedef struct SceneryPlacement {
    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    } position;
    int32_t yaw;
} SceneryPlacement;

typedef struct StaticSceneryState {
    SceneryPlacement standard;
    SceneryPlacement highClass;
} StaticSceneryState;

extern unsigned char g_NameEntryCharset[42];
extern char g_TextNowLoading[];
extern const s16 g_MenuLightBurstBandX[MENU_LIGHT_BURST_RAY_COUNT];
extern const s16 g_MenuLightBurstBandY[MENU_LIGHT_BURST_RAY_COUNT];
extern Rgb g_PaintColorTable[MENU_PAINT_COLOR_COUNT];
extern SVec g_CourseCardVerts[4];
extern Vec4 g_MenuCarPivotOffset;
extern const Vec4 g_TeamNameCharScale;
extern u8 g_DesignModeCellMask[6][6];
extern unsigned char g_CarSoundVolumeScales[128];
extern unsigned char g_IndexedEffects[36];
extern unsigned char g_SoundModes[96];
extern unsigned char g_SaveDefaults[104];
extern Rect g_DrawModeEnv;
extern unsigned char g_PromotionBonusTable[20];
extern unsigned char g_NegconSteerRange[8];
extern unsigned char g_NegconSteerDeadZone[16];
extern unsigned char g_NegconPlayScale[16];
extern unsigned char g_PadLabelSlots[24];
extern unsigned char g_PadCalloutLabelPoints[24];
extern unsigned char g_PadCalloutButtonPoints[64];
extern unsigned char g_PadConfigLabelRows[40];
extern unsigned char g_PadConfigButtonRows[40];
extern unsigned char g_NegconConfigLabelRows[40];
extern unsigned char g_NegconConfigButtonRows[40];
extern unsigned char g_NegconPlayPercent[8];
extern unsigned char g_WordFontCells[40];
extern unsigned char g_HighFontCell[4];
extern unsigned char g_CarModelBaseIndex[13];
extern unsigned char g_CarModelUnlockBase[13];
extern Rect g_TrackTextureRect;
extern Rect g_TeamLogoClutLoadRect;
extern GpuRectPacked g_TeamLogoClutMoveRect;
extern MirrorBadgeStyle
    g_CarMirrorBadgeStyles[MIRROR_BADGE_STYLE_STORAGE_COUNT];
extern MirrorBadgeSprite g_MirrorBadgeSprites[MIRROR_BADGE_STYLE_COUNT];
extern unsigned char g_RoundScreenFadeDelays[8];
extern unsigned char g_TeamNameFontGlyphs
    [TEAM_NAME_FONT_GLYPH_COUNT * TEAM_NAME_FONT_GLYPH_BYTES];
extern unsigned char g_TeamNameBlankTile[192];
extern unsigned char g_ResultPlaceCluts[8];
extern char g_ChanceDigits[6][2];
extern OptionHintCaption g_OptionHintCaptions[MENU_OPTION_HINT_COUNT];
extern DVec g_ClassRecordCellPoints[CLASS_RECORD_COUNT];
extern ClassRecordSprite g_ClassRecordCellSprites[CLASS_RECORD_COUNT];
extern Rgb g_ClassRecordNameSprites[CLASS_RECORD_COUNT + 1];
extern unsigned char g_AttractTitleDelays[8];
extern int32_t g_RoadGrade;
extern ContentCarPoint g_PlayerHullPoints[6];
extern ContentCarPoint g_OpponentHullCorners[4];
extern ContentCarPoint g_CarCornerOffsets[4];
extern ContentLaunchSpeedThreshold g_LaunchSpeedThresholds[5];
extern unsigned char g_LaunchEnergyThresholds[12];
extern GameSpriteDesc g_TachoNeedleSprite;
extern char g_ClockTextCells[8];
extern ContentCarPoint g_CarCollisionCorners[4];
typedef struct StartGridSceneryStep {
    int16_t x;
    int16_t y;
} StartGridSceneryStep;
extern StartGridSceneryStep g_StartGridSceneryStep[2];
extern unsigned char g_StartGridSceneryPos[32];
extern unsigned char g_StartGridSceneryAngle[8];
extern unsigned char g_AnimSceneryPos[32];
extern unsigned char g_AnimSceneryPitch[8];
extern unsigned char g_SpinningSceneryPlacements[64];
extern unsigned char g_SpinningSceneryAngle[8];
extern unsigned char g_SpinningSceneryRate[8];
extern StaticSceneryState g_StaticSceneryState;
typedef struct ShuttlePath {
    struct {
        int32_t x;
        int32_t y;
        int32_t z;
        int32_t w;
    } endpoint[2];
} ShuttlePath;
extern ShuttlePath g_ShuttlePathPoints[3];
extern unsigned char g_ShuttlePathAngles[24];
extern unsigned char g_ShuttlePathTravelMax[8];
extern unsigned char g_ShuttlePathDwellMax[124];
extern unsigned char g_TeamNameChars[16];
extern int16_t g_SkyTileMap[5][16];
extern unsigned char g_SkyTileUV[64];
extern unsigned char g_CdMixPresets[8];
extern Cd g_Cd;
extern MenuOverlayPatternFrame
    g_MenuOverlayPatternTable[MENU_OVERLAY_PATTERN_FRAME_COUNT];
extern s32 g_TeamLogoCursorX;
extern s32 g_TeamLogoViewX;
extern s32 g_TeamLogoPenColor;
extern uint16_t g_TeamLogoBlankClut[16];
extern FontGlyph g_SmallFontGlyphs[SMALL_FONT_GLYPH_COUNT];
extern FontGlyph g_LargeFontGlyphs[LARGE_FONT_GLYPH_COUNT];
extern Vec4 g_MenuViewScale;
extern int32_t g_CarPriceTable[32];
extern int32_t g_CarTuneUpPriceTable[31];
extern unsigned char g_SoundSlotTone[24];
extern unsigned char g_McSlotCursor[4];
extern unsigned char g_CameraMatrixSaved[32];
extern unsigned char g_SectorTimes[12];
extern ContentSVec g_RaceIntroCameraDelta;

typedef struct HostStateBlob {
    const char *name;
    const unsigned char *bytes;
    unsigned long size;
} HostStateBlob;

#define BYTES(value) ((const unsigned char *)(value))

static const HostStateBlob s_blobs[] = {
    {"g_NameEntryCharset", g_NameEntryCharset, 42},
    {"g_TextNowLoading", BYTES(g_TextNowLoading), 12},
    {"g_MenuLightBurstBandX",
     (const unsigned char *)&g_MenuLightBurstBandX, 66},
    {"g_MenuLightBurstBandY",
     (const unsigned char *)&g_MenuLightBurstBandY, 66},
    {"g_PaintColorTable", (const unsigned char *)&g_PaintColorTable, 54},
    {"g_CourseCardVerts", (const unsigned char *)g_CourseCardVerts, 32},
    {"g_MenuCarPivotOffset", (const unsigned char *)&g_MenuCarPivotOffset, 16},
    {"g_TeamNameCharScale", (const unsigned char *)&g_TeamNameCharScale, 16},
    {"g_DesignModeCellMask", (const unsigned char *)&g_DesignModeCellMask, 36},
    {"g_CarSoundVolumeScales", g_CarSoundVolumeScales, 128},
    {"g_IndexedEffects", g_IndexedEffects, 36},
    {"g_SoundModes", g_SoundModes, 96},
    {"g_SaveDefaults", g_SaveDefaults, 104},
    {"g_DrawModeEnv", (const unsigned char *)&g_DrawModeEnv,
     sizeof(g_DrawModeEnv)},
    {"g_TeamLogoClutRect", (const unsigned char *)&g_TeamLogoClutRect, 8},
    {"g_TeamLogoRect", (const unsigned char *)&g_TeamLogoRect, 8},
    {"g_PromotionBonusTable", g_PromotionBonusTable, 20},
    {"g_NegconSteerRange", g_NegconSteerRange, 8},
    {"g_NegconSteerDeadZone", g_NegconSteerDeadZone, 16},
    {"g_NegconPlayScale", g_NegconPlayScale, 16},
    {"g_PadLabelSlots", g_PadLabelSlots, 24},
    {"g_PadCalloutLabelPoints", g_PadCalloutLabelPoints, 24},
    {"g_PadCalloutButtonPoints", g_PadCalloutButtonPoints, 64},
    {"g_PadConfigLabelRows", g_PadConfigLabelRows, 40},
    {"g_PadConfigButtonRows", g_PadConfigButtonRows, 40},
    {"g_NegconConfigLabelRows", g_NegconConfigLabelRows, 40},
    {"g_NegconConfigButtonRows", g_NegconConfigButtonRows, 40},
    {"g_NegconPlayPercent", g_NegconPlayPercent, 8},
    {"g_WordFontCells", g_WordFontCells, 40},
    {"g_HighFontCell", g_HighFontCell, 4},
    {"g_CarModelBaseIndex", g_CarModelBaseIndex, 13},
    {"g_CarModelUnlockBase", g_CarModelUnlockBase, 13},
    {"g_TrackTextureRect", (const unsigned char *)&g_TrackTextureRect, 8},
    {"g_TeamLogoClutLoadRect",
     (const unsigned char *)&g_TeamLogoClutLoadRect, 8},
    {"g_TeamLogoClutMoveRect",
     (const unsigned char *)&g_TeamLogoClutMoveRect, 8},
    {"g_CarMirrorBadgeStyles", g_CarMirrorBadgeStyles, 16},
    {"g_MirrorBadgeSprites", BYTES(g_MirrorBadgeSprites),
     sizeof(g_MirrorBadgeSprites)},
    {"g_RoundScreenFadeDelays", g_RoundScreenFadeDelays, 8},
    {"g_TeamNameFontGlyphs", g_TeamNameFontGlyphs,
     sizeof(g_TeamNameFontGlyphs)},
    {"g_TeamNameBlankTile", g_TeamNameBlankTile, 192},
    {"g_ResultPlaceSprites", BYTES(&g_ResultPlaceSprites),
     sizeof(g_ResultPlaceSprites)},
    {"g_ResultPlaceCluts", g_ResultPlaceCluts, 8},
    {"g_ResultPanelCluts", BYTES(&g_ResultPanelCluts),
     sizeof(g_ResultPanelCluts)},
    {"g_ClassPlaceBarSizes", BYTES(&g_ClassPlaceBarSizes),
     sizeof(g_ClassPlaceBarSizes)},
    {"g_ChanceDigits", BYTES(g_ChanceDigits), 12},
    {"g_OptionHintCaptions", (const unsigned char *)g_OptionHintCaptions, 24},
    {"g_ClassRecordCellPoints", (const unsigned char *)g_ClassRecordCellPoints,
     44},
    {"g_ClassRecordCellSprites",
     (const unsigned char *)g_ClassRecordCellSprites, 132},
    {"g_ClassRecordNameSprites",
     (const unsigned char *)g_ClassRecordNameSprites, 36},
    {"g_AttractTitleDelays", g_AttractTitleDelays, 8},
    {"g_SpriteFontCells", BYTES(g_SpriteFontCells),
     sizeof(g_SpriteFontCells)},
    {"g_SpriteFontWidth", g_SpriteFontWidth,
     sizeof(g_SpriteFontWidth)},
    {"g_RoadGrade", (const unsigned char *)&g_RoadGrade,
     sizeof(g_RoadGrade)},
    {"g_PlayerHullPoints", (const unsigned char *)g_PlayerHullPoints,
     sizeof(g_PlayerHullPoints)},
    {"g_OpponentHullCorners", (const unsigned char *)g_OpponentHullCorners,
     sizeof(g_OpponentHullCorners)},
    {"g_CarCornerOffsets", (const unsigned char *)g_CarCornerOffsets,
     sizeof(g_CarCornerOffsets)},
    {"g_LaunchSpeedThresholds",
     (const unsigned char *)g_LaunchSpeedThresholds,
     sizeof(g_LaunchSpeedThresholds)},
    {"g_LaunchEnergyThresholds", g_LaunchEnergyThresholds, 12},
    {"g_TachoNeedleSprite",
     (const unsigned char *)&g_TachoNeedleSprite,
     sizeof(g_TachoNeedleSprite)},
    {"g_CountdownGlyphTable", BYTES(g_CountdownGlyphTable),
     sizeof(g_CountdownGlyphTable)},
    {"g_ClockTextCells", BYTES(g_ClockTextCells), 8},
    {"g_CarCollisionCorners", (const unsigned char *)g_CarCollisionCorners,
     sizeof(g_CarCollisionCorners)},
    {"g_StartGridSceneryStep",
     (const unsigned char *)g_StartGridSceneryStep, 8},
    {"g_StartGridSceneryPos", g_StartGridSceneryPos, 32},
    {"g_StartGridSceneryAngle", g_StartGridSceneryAngle, 8},
    {"g_AnimSceneryPos", g_AnimSceneryPos, 32},
    {"g_AnimSceneryPitch", g_AnimSceneryPitch, 8},
    {"g_SpinningSceneryPlacements", g_SpinningSceneryPlacements, 64},
    {"g_SpinningSceneryAngle", g_SpinningSceneryAngle, 8},
    {"g_SpinningSceneryRate", g_SpinningSceneryRate, 8},
    {"g_StaticSceneryState", (const unsigned char *)&g_StaticSceneryState, 32},
    {"g_ShuttlePathPoints",
     (const unsigned char *)g_ShuttlePathPoints, 96},
    {"g_ShuttlePathAngles", g_ShuttlePathAngles, 24},
    {"g_ShuttlePathTravelMax", g_ShuttlePathTravelMax, 8},
    {"g_ShuttlePathDwellMax", g_ShuttlePathDwellMax, 124},
    {"g_CellScanOffsets", (const unsigned char *)g_CellScanOffsets.flat,
     sizeof(g_CellScanOffsets)},
    {"g_TeamNameChars", g_TeamNameChars, 16},
    {"g_SkyTileMap", (const unsigned char *)g_SkyTileMap, 160},
    {"g_SkyTileUV", g_SkyTileUV, 64},
    {"g_CdMixPresets", g_CdMixPresets, 8},
    {"g_Cd.pendingCommand", (const unsigned char *)&g_Cd.pendingCommand,
     sizeof(g_Cd.pendingCommand)},
    {"g_MenuOverlayPatternTable", BYTES(g_MenuOverlayPatternTable),
     sizeof(g_MenuOverlayPatternTable)},
    {"g_TeamLogoCursorX", (const unsigned char *)&g_TeamLogoCursorX, 4},
    {"g_TeamLogoViewX", (const unsigned char *)&g_TeamLogoViewX, 4},
    {"g_TeamLogoPenColor", (const unsigned char *)&g_TeamLogoPenColor, 4},
    {"g_TeamLogoBlankClut", (const unsigned char *)g_TeamLogoBlankClut, 32},
    {"g_SmallFontGlyphs", BYTES(g_SmallFontGlyphs),
     sizeof(g_SmallFontGlyphs)},
    {"g_LargeFontGlyphs", BYTES(g_LargeFontGlyphs),
     sizeof(g_LargeFontGlyphs)},
    {"g_MenuViewScale", (const unsigned char *)&g_MenuViewScale, 16},
    {"g_CarPriceTable", (const unsigned char *)g_CarPriceTable, 128},
    {"g_CarTuneUpPriceTable",
     (const unsigned char *)g_CarTuneUpPriceTable, 124},
    {"g_SoundSlotTone", g_SoundSlotTone, 24},
    {"g_McSlotCursor", g_McSlotCursor, 4},
    {"g_CameraMatrixSaved", g_CameraMatrixSaved, 32},
    {"g_SectorTimes", g_SectorTimes, 12},
    {"g_RaceIntroCameraDelta",
     (const unsigned char *)&g_RaceIntroCameraDelta,
     sizeof(g_RaceIntroCameraDelta)},
    {"g_Cd.elapsed", (const unsigned char *)&g_Cd.elapsed, 4},
};

int main(void) {
    /* Folded from the canonical host constants alone. */
    const unsigned long expected = 56180094UL;
    unsigned long digest = 2166136261UL;
    unsigned long bytes = 0;
    const char *trace = getenv("RAGE_HOST_STATE_TRACE");
    FILE *out = trace != NULL ? fopen(trace, "w") : NULL;
    size_t i;

    for (i = 0; i < sizeof(s_blobs) / sizeof(s_blobs[0]); i++) {
        unsigned long j;
        for (j = 0; j < s_blobs[i].size; j++) {
            digest ^= s_blobs[i].bytes[j];
            digest = (digest * 16777619UL) & 0xFFFFFFFFUL;
        }
        bytes += s_blobs[i].size;
        if (out != NULL) {
            fprintf(out, "%s %lu %lu\n", s_blobs[i].name, s_blobs[i].size,
                    digest);
        }
    }
    if (out != NULL) {
        fclose(out);
    }
    if (digest != expected) {
        printf("FAIL the host constants changed: %lu bytes across %lu blobs "
               "digest to %lu, expected %lu\n", bytes,
               (unsigned long)(sizeof(s_blobs) / sizeof(s_blobs[0])), digest,
               expected);
        return 1;
    }
    printf("the host constants are stable across %lu bytes\n", bytes);
    return 0;
}
