/*
 * What the retail data actually is, byte for byte.
 *
 * host_state.c is the retail data segment transcribed into C, and it is being
 * untangled: strings written as hexadecimal are becoming strings, arrays that
 * swallowed their neighbours are being cut apart and named, and the whole file
 * is being split up by subsystem. Meaningful data has to retain its bytes;
 * padding and dead retail pointer tables are deliberately dropped.
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

#include "game/menu_types.h"
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

extern char g_MsgInsertController[20];
extern char g_MsgControllerError[20];
extern char g_MsgNegconUntwistedLine1[36];
extern char g_MsgNegconUntwistedLine2[36];
extern char g_MsgNegconSteerPlay[12];
extern char g_MsgNegconMaxTwist[];
extern char g_FmtRound[16];
extern char g_CaptionPrizeMoney2[8];
extern char g_FmtPrize1st[12];
extern char g_FmtPrize2nd[12];
extern char g_FmtPrize3rd[12];
extern char g_CaptionBestTotalTime[8];
extern char g_CaptionBestLapTime[];
extern char g_TextResult[8];
extern char g_FmtClassGrandPrix[24];
extern char g_FmtRoundIn[12];
extern char g_CaptionRanking[8];
extern char g_CaptionTotalTime[8];
extern char g_CaptionLapTime[8];
extern char g_CaptionPrizeMoney[8];
extern char g_FmtMoney[8];
extern char g_CaptionTotalMoney[8];
extern char g_CaptionPromotionBonus[];
extern char g_CaptionLostRace[24];
extern char g_TextTryAgain[12];
extern char g_TextEndRace[12];
extern char g_TextChance[8];
extern char g_TextPressStart[20];
extern char g_FmtLapTime[16];
extern char g_TextTimeAttack[12];
extern char g_TextCourseIn[];
extern char g_CaptionLapTime2[8];
extern char g_CaptionRanking2[8];
extern char g_FmtRecordName[8];
extern char g_FmtCarName[8];
extern char g_CaptionTotalTime2[8];
extern unsigned char g_NameEntryCharset[42];
extern char g_TextNowLoading[];
extern unsigned char g_MsgGame0Ok[12];
extern const MenuLightBurstBand g_MenuLightBurstBandX;
extern const MenuLightBurstBand g_MenuLightBurstBandY;
extern const char g_MsgOrdinalSt[4];
extern const char g_MsgOrdinalNd[4];
extern const char g_MsgOrdinalRd[4];
extern const char g_MsgOrdinalTh[8];
extern PaintColorTable g_PaintColorTable;
extern SVec g_CourseCardVerts[4];
extern Vec4 g_MenuCarPivotOffset;
extern const Vec4 g_TeamNameCharScale;
extern const char g_FormatDecimal[4];
extern DesignModeCellMask g_DesignModeCellMask;
extern unsigned char g_CarSoundVolumeScales[128];
extern const char g_MsgVabOpenHeadError[21];
extern const char g_MsgVabTransBodyError[22];
extern unsigned char g_IndexedEffects[36];
extern unsigned char g_SoundModes[96];
extern unsigned char g_MsgTooManyVoices[16];
extern const char g_MsgSeqVabOpenHeadError[21];
extern const char g_MsgSeqVabTransBodyError[22];
extern unsigned char g_SaveDefaults[104];
extern unsigned char g_DrawModeEnv[8];
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
extern unsigned char g_CarMirrorBadgeStyles[16];
extern unsigned char g_MirrorBadgeTexU[10];
extern unsigned char g_MirrorBadgeTexV[10];
extern unsigned char g_MirrorBadgeWidths[10];
extern unsigned char g_RoundScreenFadeDelays[8];
extern unsigned char g_TeamNameFontGlyphs[2688];
extern unsigned char g_TeamNameBlankTile[192];
extern unsigned char g_ResultPlaceSprites[10];
extern unsigned char g_ResultPlaceCluts[8];
extern unsigned char g_ResultPanelCluts[10];
extern unsigned char g_ClassPlaceBarSizes[8];
extern char g_ChanceDigits[6][2];
extern OptionHintCaption g_OptionHintCaptions[MENU_OPTION_HINT_COUNT];
extern DVec g_ClassRecordCellPoints[CLASS_RECORD_COUNT];
extern ClassRecordSprite g_ClassRecordCellSprites[CLASS_RECORD_COUNT];
extern Rgb g_ClassRecordNameSprites[CLASS_RECORD_COUNT + 1];
extern unsigned char g_AttractTitleDelays[8];
extern unsigned char g_SpriteFontCells[192];
extern unsigned char g_SpriteFontWidth[96];
extern int32_t g_RoadGrade;
extern ContentCarPoint g_PlayerHullPoints[6];
extern ContentCarPoint g_OpponentHullCorners[4];
extern ContentCarPoint g_CarCornerOffsets[4];
extern ContentLaunchSpeedThreshold g_LaunchSpeedThresholds[5];
extern unsigned char g_LaunchEnergyThresholds[12];
extern unsigned char g_TachoNeedleSprite[20];
extern unsigned char g_CountdownGlyphTable[256];
extern char g_ClockTextCells[8];
extern char g_RaceOptionMarquee[4][40];
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
extern int32_t g_CdCommandPending;
extern unsigned char g_MenuOverlayPatternTable[584];
extern TeamLogoCoordinate g_TeamLogoCursorX;
extern TeamLogoCoordinate g_TeamLogoViewX;
extern TeamLogoColorIndex g_TeamLogoPenColor;
extern uint16_t g_TeamLogoBlankClut[16];
extern unsigned char g_SmallFontGlyphs[184];
extern unsigned char g_LargeFontGlyphs[196];
extern Vec4 g_MenuViewScale;
extern int32_t g_CarPriceTable[32];
extern int32_t g_CarTuneUpPriceTable[31];
extern unsigned char g_SoundSlotTone[24];
extern unsigned char g_McSlotCursor[4];
extern unsigned char g_CameraMatrixSaved[32];
extern unsigned char g_SectorTimes[12];
extern ContentSVec g_RaceIntroCameraDelta;
extern unsigned char g_CdTrackElapsedLoc[4];

typedef struct HostStateBlob {
    const char *name;
    const unsigned char *bytes;
    unsigned long size;
} HostStateBlob;

#define BYTES(value) ((const unsigned char *)(value))

static const HostStateBlob s_blobs[] = {
    {"g_MsgInsertController", BYTES(g_MsgInsertController), 20},
    {"g_MsgControllerError", BYTES(g_MsgControllerError), 20},
    {"g_MsgNegconUntwistedLine1", BYTES(g_MsgNegconUntwistedLine1), 36},
    {"g_MsgNegconUntwistedLine2", BYTES(g_MsgNegconUntwistedLine2), 36},
    {"g_MsgNegconSteerPlay", BYTES(g_MsgNegconSteerPlay), 12},
    {"g_MsgNegconMaxTwist", BYTES(g_MsgNegconMaxTwist), 15},
    {"g_FmtRound", BYTES(g_FmtRound), 16},
    {"g_CaptionPrizeMoney2", BYTES(g_CaptionPrizeMoney2), 8},
    {"g_FmtPrize1st", BYTES(g_FmtPrize1st), 12},
    {"g_FmtPrize2nd", BYTES(g_FmtPrize2nd), 12},
    {"g_FmtPrize3rd", BYTES(g_FmtPrize3rd), 12},
    {"g_CaptionBestTotalTime", BYTES(g_CaptionBestTotalTime), 8},
    {"g_CaptionBestLapTime", BYTES(g_CaptionBestLapTime), 5},
    {"g_TextResult", BYTES(g_TextResult), 8},
    {"g_FmtClassGrandPrix", BYTES(g_FmtClassGrandPrix), 24},
    {"g_FmtRoundIn", BYTES(g_FmtRoundIn), 12},
    {"g_CaptionRanking", BYTES(g_CaptionRanking), 8},
    {"g_CaptionTotalTime", BYTES(g_CaptionTotalTime), 8},
    {"g_CaptionLapTime", BYTES(g_CaptionLapTime), 8},
    {"g_CaptionPrizeMoney", BYTES(g_CaptionPrizeMoney), 8},
    {"g_FmtMoney", BYTES(g_FmtMoney), 8},
    {"g_CaptionTotalMoney", BYTES(g_CaptionTotalMoney), 8},
    {"g_CaptionPromotionBonus", BYTES(g_CaptionPromotionBonus), 4},
    {"g_CaptionLostRace", BYTES(g_CaptionLostRace), 24},
    {"g_TextTryAgain", BYTES(g_TextTryAgain), 12},
    {"g_TextEndRace", BYTES(g_TextEndRace), 12},
    {"g_TextChance", BYTES(g_TextChance), 8},
    {"g_TextPressStart", BYTES(g_TextPressStart), 20},
    {"g_FmtLapTime", BYTES(g_FmtLapTime), 16},
    {"g_TextTimeAttack", BYTES(g_TextTimeAttack), 12},
    {"g_TextCourseIn", BYTES(g_TextCourseIn), 10},
    {"g_CaptionLapTime2", BYTES(g_CaptionLapTime2), 8},
    {"g_CaptionRanking2", BYTES(g_CaptionRanking2), 8},
    {"g_FmtRecordName", BYTES(g_FmtRecordName), 8},
    {"g_FmtCarName", BYTES(g_FmtCarName), 8},
    {"g_CaptionTotalTime2", BYTES(g_CaptionTotalTime2), 8},
    {"g_NameEntryCharset", g_NameEntryCharset, 42},
    {"g_TextNowLoading", BYTES(g_TextNowLoading), 12},
    {"g_MsgGame0Ok", g_MsgGame0Ok, 12},
    {"g_MenuLightBurstBandX",
     (const unsigned char *)&g_MenuLightBurstBandX, 66},
    {"g_MenuLightBurstBandY",
     (const unsigned char *)&g_MenuLightBurstBandY, 66},
    {"g_MsgOrdinalSt", (const unsigned char *)g_MsgOrdinalSt, 4},
    {"g_MsgOrdinalNd", (const unsigned char *)g_MsgOrdinalNd, 4},
    {"g_MsgOrdinalRd", (const unsigned char *)g_MsgOrdinalRd, 4},
    {"g_MsgOrdinalTh", (const unsigned char *)g_MsgOrdinalTh, 8},
    {"g_PaintColorTable", (const unsigned char *)&g_PaintColorTable, 54},
    {"g_CourseCardVerts", (const unsigned char *)g_CourseCardVerts, 32},
    {"g_MenuCarPivotOffset", (const unsigned char *)&g_MenuCarPivotOffset, 16},
    {"g_TeamNameCharScale", (const unsigned char *)&g_TeamNameCharScale, 16},
    {"g_FormatDecimal", (const unsigned char *)g_FormatDecimal, 4},
    {"g_DesignModeCellMask", (const unsigned char *)&g_DesignModeCellMask, 36},
    {"g_CarSoundVolumeScales", g_CarSoundVolumeScales, 128},
    {"g_MsgVabOpenHeadError",
     (const unsigned char *)g_MsgVabOpenHeadError, 21},
    {"g_MsgVabTransBodyError",
     (const unsigned char *)g_MsgVabTransBodyError, 22},
    {"g_IndexedEffects", g_IndexedEffects, 36},
    {"g_SoundModes", g_SoundModes, 96},
    {"g_MsgTooManyVoices", g_MsgTooManyVoices, 16},
    {"g_MsgSeqVabOpenHeadError",
     (const unsigned char *)g_MsgSeqVabOpenHeadError, 21},
    {"g_MsgSeqVabTransBodyError",
     (const unsigned char *)g_MsgSeqVabTransBodyError, 22},
    {"g_SaveDefaults", g_SaveDefaults, 104},
    {"g_DrawModeEnv", g_DrawModeEnv, 8},
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
    {"g_MirrorBadgeTexU", g_MirrorBadgeTexU, 10},
    {"g_MirrorBadgeTexV", g_MirrorBadgeTexV, 10},
    {"g_MirrorBadgeWidths", g_MirrorBadgeWidths, 10},
    {"g_RoundScreenFadeDelays", g_RoundScreenFadeDelays, 8},
    {"g_TeamNameFontGlyphs", g_TeamNameFontGlyphs, 2688},
    {"g_TeamNameBlankTile", g_TeamNameBlankTile, 192},
    {"g_ResultPlaceSprites", g_ResultPlaceSprites, 10},
    {"g_ResultPlaceCluts", g_ResultPlaceCluts, 8},
    {"g_ResultPanelCluts", g_ResultPanelCluts, 10},
    {"g_ClassPlaceBarSizes", g_ClassPlaceBarSizes, 8},
    {"g_ChanceDigits", BYTES(g_ChanceDigits), 12},
    {"g_OptionHintCaptions", (const unsigned char *)g_OptionHintCaptions, 28},
    {"g_ClassRecordCellPoints", (const unsigned char *)g_ClassRecordCellPoints,
     44},
    {"g_ClassRecordCellSprites",
     (const unsigned char *)g_ClassRecordCellSprites, 132},
    {"g_ClassRecordNameSprites",
     (const unsigned char *)g_ClassRecordNameSprites, 36},
    {"g_AttractTitleDelays", g_AttractTitleDelays, 8},
    {"g_SpriteFontCells", g_SpriteFontCells, 192},
    {"g_SpriteFontWidth", g_SpriteFontWidth, 96},
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
    {"g_TachoNeedleSprite", g_TachoNeedleSprite, 20},
    {"g_CountdownGlyphTable", g_CountdownGlyphTable, 256},
    {"g_ClockTextCells", BYTES(g_ClockTextCells), 8},
    {"g_RaceOptionMarquee", BYTES(g_RaceOptionMarquee), 160},
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
    {"g_CdCommandPending", (const unsigned char *)&g_CdCommandPending,
     sizeof(g_CdCommandPending)},
    {"g_MenuOverlayPatternTable", g_MenuOverlayPatternTable, 584},
    {"g_TeamLogoCursorX", (const unsigned char *)&g_TeamLogoCursorX, 4},
    {"g_TeamLogoViewX", (const unsigned char *)&g_TeamLogoViewX, 4},
    {"g_TeamLogoPenColor", (const unsigned char *)&g_TeamLogoPenColor, 4},
    {"g_TeamLogoBlankClut", (const unsigned char *)g_TeamLogoBlankClut, 32},
    {"g_SmallFontGlyphs", g_SmallFontGlyphs, 184},
    {"g_LargeFontGlyphs", g_LargeFontGlyphs, 196},
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
    {"g_CdTrackElapsedLoc", g_CdTrackElapsedLoc, 4},
};

int main(void) {
    /* Folded from the canonical host constants alone. */
    const unsigned long expected = 1658594987UL;
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
