/*
 * What the retail data actually is, byte for byte.
 *
 * host_state.c is the retail data segment transcribed into C, and it is being
 * untangled: strings written as hexadecimal are becoming strings, arrays that
 * swallowed their neighbours are being cut apart and named, and the whole file
 * is being split up by subsystem. Every one of those changes has to leave the
 * bytes exactly as they were, and the ABI manifest cannot say so: it pins the
 * names and the sizes of the raw blobs, not what is in them.
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

extern unsigned char g_MsgInsertController[20];
extern unsigned char g_MsgControllerError[20];
extern unsigned char g_MsgNegconUntwistedLine1[36];
extern unsigned char g_MsgNegconUntwistedLine2[36];
extern unsigned char g_MsgNegconSteerPlay[12];
extern unsigned char g_MsgNegconMaxTwist[2640];
extern unsigned char g_FmtRound[16];
extern unsigned char g_CaptionPrizeMoney2[8];
extern unsigned char g_FmtPrize1st[12];
extern unsigned char g_FmtPrize2nd[12];
extern unsigned char g_FmtPrize3rd[12];
extern unsigned char g_CaptionBestTotalTime[8];
extern unsigned char g_CaptionBestLapTime[188];
extern unsigned char g_TextResult[8];
extern unsigned char g_FmtClassGrandPrix[24];
extern unsigned char g_FmtRoundIn[12];
extern unsigned char g_CaptionRanking[8];
extern unsigned char g_CaptionTotalTime[8];
extern unsigned char g_CaptionLapTime[8];
extern unsigned char g_CaptionPrizeMoney[8];
extern unsigned char g_FmtMoney[8];
extern unsigned char g_CaptionTotalMoney[8];
extern unsigned char g_CaptionPromotionBonus[40];
extern unsigned char g_CaptionLostRace[24];
extern unsigned char g_TextTryAgain[12];
extern unsigned char g_TextEndRace[12];
extern unsigned char g_TextChance[8];
extern unsigned char g_TextPressStart[20];
extern unsigned char g_FmtLapTime[16];
extern unsigned char g_TextTimeAttack[12];
extern unsigned char g_TextCourseIn[192];
extern unsigned char g_CaptionLapTime2[8];
extern unsigned char g_CaptionRanking2[8];
extern unsigned char g_FmtRecordName[8];
extern unsigned char g_FmtCarName[8];
extern unsigned char g_CaptionTotalTime2[8];
extern unsigned char g_NameEntryCharset[96];
extern unsigned char g_TextNowLoading[436];
extern unsigned char g_MsgResOk[8];
extern unsigned char g_MsgEventOk[12];
extern unsigned char g_MsgGameExit[12];
extern unsigned char g_MsgGame0Ok[12];
extern unsigned char g_MenuLightBurstBandX[68];
extern unsigned char g_MenuLightBurstBandY[68];
extern unsigned char g_MsgOrdinalSt[8];
extern unsigned char g_MsgOrdinalNd[8];
extern unsigned char g_MsgOrdinalRd[8];
extern unsigned char g_MsgOrdinalTh[116];
extern unsigned char g_PaintColorTable[168];
extern unsigned char g_CourseCardVerts[108];
extern unsigned char g_MenuCarPivotOffset[16];
extern unsigned char g_TeamNameCharScale[152];
extern unsigned char g_FormatDecimal[68];
extern unsigned char g_MenuBlankCaption[52];
extern unsigned char g_DesignModeCellMask[160];
extern unsigned char g_CarSoundVolumeScales[128];
extern unsigned char g_MsgVabOpenHeadError[24];
extern unsigned char g_MsgVabTransBodyError[24];
extern unsigned char g_IndexedEffects[36];
extern unsigned char g_SoundModes[96];
extern unsigned char g_MsgTooManyVoices[16];
extern unsigned char g_MsgSeqVabOpenHeadError[24];
extern unsigned char g_MsgSeqVabTransBodyError[44];
extern unsigned char g_FmtString[8];
extern unsigned char g_MsgSaveChecksumOk[8];
extern unsigned char g_FmtSaveChecksum[20];
extern unsigned char g_SaveDefaults[104];
extern unsigned char g_DrawModeEnv[8];
extern unsigned char g_TeamLogoClutRect[8];
extern unsigned char g_TeamLogoRect[8];
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
extern unsigned char g_CarModelBaseIndex[16];
extern unsigned char g_CarModelUnlockBase[16];
extern unsigned char g_AssetRequestType[8];
extern unsigned char g_TrackTextureRect[8];
extern unsigned char g_TeamLogoClutLoadRect[8];
extern unsigned char g_TeamLogoClutMoveRect[8];
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
extern unsigned char g_ChanceDigits[12];
extern unsigned char g_PlaceSuffixNames[20];
extern unsigned char g_CarNames[52];
extern unsigned char g_CarClassNames[52];
extern unsigned char g_OptionHintCaptions[28];
extern unsigned char g_ClassRecordCellPoints[44];
extern unsigned char g_ClassRecordCellSprites[132];
extern unsigned char g_ClassRecordNameSprites[36];
extern unsigned char g_BgmSelectSteps[20];
extern unsigned char g_AttractTitleDelays[8];
extern unsigned char g_SpriteFontCells[192];
extern unsigned char g_SpriteFontWidth[288];
extern int32_t g_RoadGrade;
extern ContentCarPoint g_PlayerHullPoints[6];
extern ContentCarPoint g_OpponentHullCorners[4];
extern ContentCarPoint g_CarCornerOffsets[4];
extern ContentLaunchSpeedThreshold g_LaunchSpeedThresholds[5];
extern unsigned char g_LaunchEnergyThresholds[12];
extern unsigned char g_TachoNeedleSprite[20];
extern unsigned char g_CountdownGlyphTable[256];
extern unsigned char g_ClockTextCells[8];
extern unsigned char g_RaceOptionMarquee[160];
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
extern unsigned char g_CdCommandPending[8];
extern unsigned char g_MenuOverlayPatternTable[584];
extern unsigned char g_TeamLogoCursorX[8];
extern unsigned char g_TeamLogoViewX[8];
extern unsigned char g_TeamLogoPenColor[8];
extern unsigned char g_TeamLogoBlankClut[32];
extern unsigned char g_SmallFontGlyphs[184];
extern unsigned char g_LargeFontGlyphs[196];
extern unsigned char g_TimeAttackPlateProgress[7252];
extern unsigned char g_RankingPanelScript[60];
extern unsigned char g_CustomizeMenuScriptGp[156];
extern unsigned char g_CustomizeMenuScriptTimeAttack[132];
extern unsigned char g_DesignModeScript[192];
extern unsigned char g_TeamLogoScreenScript[144];
extern unsigned char g_LogoSampleScreenScript[144];
extern unsigned char g_TeamNameScreenScript[732];
extern unsigned char g_PaintColorScreenScript[180];
extern unsigned char g_CarShopScreenScript[108];
extern unsigned char g_EngineerShopScreenScript[816];
extern unsigned char g_MenuDialogPanelUpperScript[48];
extern unsigned char g_MenuDialogPanelLowerScript[96];
extern unsigned char g_CourseSelectSavePromptScript[48];
extern unsigned char g_MenuRow0MarkerScript[48];
extern unsigned char g_MenuRow1MarkerScript[192];
extern unsigned char g_RankingMenuScript[108];
extern unsigned char g_CourseSelectSavePromptBanner[24];
extern unsigned char g_TransmissionUnavailableScript[48];
extern unsigned char g_TeamLogoScreenScript2[24];
extern unsigned char g_CarShopUnavailableScript[24];
extern unsigned char g_EngineerShopUnavailableScript[36];
extern unsigned char g_EngineerShopNoFundsScript[24];
extern unsigned char g_CarShopNoFundsScript[60];
extern unsigned char g_DesignModeDeniedScript[24];
extern unsigned char g_CarShopBuyPromptScript2[84];
extern unsigned char g_CarShopBuyPromptScript1[84];
extern unsigned char g_CarShopBuyPromptScript3[84];
extern unsigned char g_CarShopBuyPromptScript4[84];
extern unsigned char g_EngineerShopTuneUpPromptScript[60];
extern unsigned char g_MenuViewScale[16];
extern unsigned char g_CarPriceTable[128];
extern unsigned char g_CarTuneUpPriceTable[124];
extern unsigned char g_SoundSlotTone[24];
extern unsigned char g_McSlotCursor[4];
extern unsigned char g_McModeLabels[32];
extern unsigned char g_CameraMatrixSaved[32];
extern unsigned char g_SectorTimes[12];
extern ContentSVec g_RaceIntroCameraDelta;
extern unsigned char g_CdTrackElapsedLoc[8];

typedef struct HostStateBlob {
    const char *name;
    const unsigned char *bytes;
    unsigned long size;
} HostStateBlob;

static const HostStateBlob s_blobs[] = {
    {"g_MsgInsertController", g_MsgInsertController, 20},
    {"g_MsgControllerError", g_MsgControllerError, 20},
    {"g_MsgNegconUntwistedLine1", g_MsgNegconUntwistedLine1, 36},
    {"g_MsgNegconUntwistedLine2", g_MsgNegconUntwistedLine2, 36},
    {"g_MsgNegconSteerPlay", g_MsgNegconSteerPlay, 12},
    {"g_MsgNegconMaxTwist", g_MsgNegconMaxTwist, 2640},
    {"g_FmtRound", g_FmtRound, 16},
    {"g_CaptionPrizeMoney2", g_CaptionPrizeMoney2, 8},
    {"g_FmtPrize1st", g_FmtPrize1st, 12},
    {"g_FmtPrize2nd", g_FmtPrize2nd, 12},
    {"g_FmtPrize3rd", g_FmtPrize3rd, 12},
    {"g_CaptionBestTotalTime", g_CaptionBestTotalTime, 8},
    {"g_CaptionBestLapTime", g_CaptionBestLapTime, 188},
    {"g_TextResult", g_TextResult, 8},
    {"g_FmtClassGrandPrix", g_FmtClassGrandPrix, 24},
    {"g_FmtRoundIn", g_FmtRoundIn, 12},
    {"g_CaptionRanking", g_CaptionRanking, 8},
    {"g_CaptionTotalTime", g_CaptionTotalTime, 8},
    {"g_CaptionLapTime", g_CaptionLapTime, 8},
    {"g_CaptionPrizeMoney", g_CaptionPrizeMoney, 8},
    {"g_FmtMoney", g_FmtMoney, 8},
    {"g_CaptionTotalMoney", g_CaptionTotalMoney, 8},
    {"g_CaptionPromotionBonus", g_CaptionPromotionBonus, 40},
    {"g_CaptionLostRace", g_CaptionLostRace, 24},
    {"g_TextTryAgain", g_TextTryAgain, 12},
    {"g_TextEndRace", g_TextEndRace, 12},
    {"g_TextChance", g_TextChance, 8},
    {"g_TextPressStart", g_TextPressStart, 20},
    {"g_FmtLapTime", g_FmtLapTime, 16},
    {"g_TextTimeAttack", g_TextTimeAttack, 12},
    {"g_TextCourseIn", g_TextCourseIn, 192},
    {"g_CaptionLapTime2", g_CaptionLapTime2, 8},
    {"g_CaptionRanking2", g_CaptionRanking2, 8},
    {"g_FmtRecordName", g_FmtRecordName, 8},
    {"g_FmtCarName", g_FmtCarName, 8},
    {"g_CaptionTotalTime2", g_CaptionTotalTime2, 8},
    {"g_NameEntryCharset", g_NameEntryCharset, 96},
    {"g_TextNowLoading", g_TextNowLoading, 436},
    {"g_MsgResOk", g_MsgResOk, 8},
    {"g_MsgEventOk", g_MsgEventOk, 12},
    {"g_MsgGameExit", g_MsgGameExit, 12},
    {"g_MsgGame0Ok", g_MsgGame0Ok, 12},
    {"g_MenuLightBurstBandX", g_MenuLightBurstBandX, 68},
    {"g_MenuLightBurstBandY", g_MenuLightBurstBandY, 68},
    {"g_MsgOrdinalSt", g_MsgOrdinalSt, 8},
    {"g_MsgOrdinalNd", g_MsgOrdinalNd, 8},
    {"g_MsgOrdinalRd", g_MsgOrdinalRd, 8},
    {"g_MsgOrdinalTh", g_MsgOrdinalTh, 116},
    {"g_PaintColorTable", g_PaintColorTable, 168},
    {"g_CourseCardVerts", g_CourseCardVerts, 108},
    {"g_MenuCarPivotOffset", g_MenuCarPivotOffset, 16},
    {"g_TeamNameCharScale", g_TeamNameCharScale, 152},
    {"g_FormatDecimal", g_FormatDecimal, 68},
    {"g_MenuBlankCaption", g_MenuBlankCaption, 52},
    {"g_DesignModeCellMask", g_DesignModeCellMask, 160},
    {"g_CarSoundVolumeScales", g_CarSoundVolumeScales, 128},
    {"g_MsgVabOpenHeadError", g_MsgVabOpenHeadError, 24},
    {"g_MsgVabTransBodyError", g_MsgVabTransBodyError, 24},
    {"g_IndexedEffects", g_IndexedEffects, 36},
    {"g_SoundModes", g_SoundModes, 96},
    {"g_MsgTooManyVoices", g_MsgTooManyVoices, 16},
    {"g_MsgSeqVabOpenHeadError", g_MsgSeqVabOpenHeadError, 24},
    {"g_MsgSeqVabTransBodyError", g_MsgSeqVabTransBodyError, 44},
    {"g_FmtString", g_FmtString, 8},
    {"g_MsgSaveChecksumOk", g_MsgSaveChecksumOk, 8},
    {"g_FmtSaveChecksum", g_FmtSaveChecksum, 20},
    {"g_SaveDefaults", g_SaveDefaults, 104},
    {"g_DrawModeEnv", g_DrawModeEnv, 8},
    {"g_TeamLogoClutRect", g_TeamLogoClutRect, 8},
    {"g_TeamLogoRect", g_TeamLogoRect, 8},
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
    {"g_CarModelBaseIndex", g_CarModelBaseIndex, 16},
    {"g_CarModelUnlockBase", g_CarModelUnlockBase, 16},
    {"g_AssetRequestType", g_AssetRequestType, 8},
    {"g_TrackTextureRect", g_TrackTextureRect, 8},
    {"g_TeamLogoClutLoadRect", g_TeamLogoClutLoadRect, 8},
    {"g_TeamLogoClutMoveRect", g_TeamLogoClutMoveRect, 8},
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
    {"g_ChanceDigits", g_ChanceDigits, 12},
    {"g_PlaceSuffixNames", g_PlaceSuffixNames, 20},
    {"g_CarNames", g_CarNames, 52},
    {"g_CarClassNames", g_CarClassNames, 52},
    {"g_OptionHintCaptions", g_OptionHintCaptions, 28},
    {"g_ClassRecordCellPoints", g_ClassRecordCellPoints, 44},
    {"g_ClassRecordCellSprites", g_ClassRecordCellSprites, 132},
    {"g_ClassRecordNameSprites", g_ClassRecordNameSprites, 36},
    {"g_BgmSelectSteps", g_BgmSelectSteps, 20},
    {"g_AttractTitleDelays", g_AttractTitleDelays, 8},
    {"g_SpriteFontCells", g_SpriteFontCells, 192},
    {"g_SpriteFontWidth", g_SpriteFontWidth, 288},
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
    {"g_ClockTextCells", g_ClockTextCells, 8},
    {"g_RaceOptionMarquee", g_RaceOptionMarquee, 160},
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
    {"g_CdCommandPending", g_CdCommandPending, 8},
    {"g_MenuOverlayPatternTable", g_MenuOverlayPatternTable, 584},
    {"g_TeamLogoCursorX", g_TeamLogoCursorX, 8},
    {"g_TeamLogoViewX", g_TeamLogoViewX, 8},
    {"g_TeamLogoPenColor", g_TeamLogoPenColor, 8},
    {"g_TeamLogoBlankClut", g_TeamLogoBlankClut, 32},
    {"g_SmallFontGlyphs", g_SmallFontGlyphs, 184},
    {"g_LargeFontGlyphs", g_LargeFontGlyphs, 196},
    {"g_TimeAttackPlateProgress", g_TimeAttackPlateProgress, 7252},
    {"g_RankingPanelScript", g_RankingPanelScript, 60},
    {"g_CustomizeMenuScriptGp", g_CustomizeMenuScriptGp, 156},
    {"g_CustomizeMenuScriptTimeAttack", g_CustomizeMenuScriptTimeAttack, 132},
    {"g_DesignModeScript", g_DesignModeScript, 192},
    {"g_TeamLogoScreenScript", g_TeamLogoScreenScript, 144},
    {"g_LogoSampleScreenScript", g_LogoSampleScreenScript, 144},
    {"g_TeamNameScreenScript", g_TeamNameScreenScript, 732},
    {"g_PaintColorScreenScript", g_PaintColorScreenScript, 180},
    {"g_CarShopScreenScript", g_CarShopScreenScript, 108},
    {"g_EngineerShopScreenScript", g_EngineerShopScreenScript, 816},
    {"g_MenuDialogPanelUpperScript", g_MenuDialogPanelUpperScript, 48},
    {"g_MenuDialogPanelLowerScript", g_MenuDialogPanelLowerScript, 96},
    {"g_CourseSelectSavePromptScript", g_CourseSelectSavePromptScript, 48},
    {"g_MenuRow0MarkerScript", g_MenuRow0MarkerScript, 48},
    {"g_MenuRow1MarkerScript", g_MenuRow1MarkerScript, 192},
    {"g_RankingMenuScript", g_RankingMenuScript, 108},
    {"g_CourseSelectSavePromptBanner", g_CourseSelectSavePromptBanner, 24},
    {"g_TransmissionUnavailableScript", g_TransmissionUnavailableScript, 48},
    {"g_TeamLogoScreenScript2", g_TeamLogoScreenScript2, 24},
    {"g_CarShopUnavailableScript", g_CarShopUnavailableScript, 24},
    {"g_EngineerShopUnavailableScript", g_EngineerShopUnavailableScript, 36},
    {"g_EngineerShopNoFundsScript", g_EngineerShopNoFundsScript, 24},
    {"g_CarShopNoFundsScript", g_CarShopNoFundsScript, 60},
    {"g_DesignModeDeniedScript", g_DesignModeDeniedScript, 24},
    {"g_CarShopBuyPromptScript2", g_CarShopBuyPromptScript2, 84},
    {"g_CarShopBuyPromptScript1", g_CarShopBuyPromptScript1, 84},
    {"g_CarShopBuyPromptScript3", g_CarShopBuyPromptScript3, 84},
    {"g_CarShopBuyPromptScript4", g_CarShopBuyPromptScript4, 84},
    {"g_EngineerShopTuneUpPromptScript", g_EngineerShopTuneUpPromptScript, 60},
    {"g_MenuViewScale", g_MenuViewScale, 16},
    {"g_CarPriceTable", g_CarPriceTable, 128},
    {"g_CarTuneUpPriceTable", g_CarTuneUpPriceTable, 124},
    {"g_SoundSlotTone", g_SoundSlotTone, 24},
    {"g_McSlotCursor", g_McSlotCursor, 4},
    {"g_McModeLabels", g_McModeLabels, 32},
    {"g_CameraMatrixSaved", g_CameraMatrixSaved, 32},
    {"g_SectorTimes", g_SectorTimes, 12},
    {"g_RaceIntroCameraDelta",
     (const unsigned char *)&g_RaceIntroCameraDelta,
     sizeof(g_RaceIntroCameraDelta)},
    {"g_CdTrackElapsedLoc", g_CdTrackElapsedLoc, 8},
};

int main(void) {
    /* Folded from the bytes alone; see the note above on why. */
    const unsigned long expected = 3977169411UL;
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
        printf("FAIL the retail data changed: %lu bytes across %lu blobs "
               "digest to %lu, expected %lu\n", bytes,
               (unsigned long)(sizeof(s_blobs) / sizeof(s_blobs[0])), digest,
               expected);
        return 1;
    }
    printf("the retail data is the same %lu bytes it always was\n", bytes);
    return 0;
}
