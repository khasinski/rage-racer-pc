#ifndef GAME_CAR_H
#define GAME_CAR_H

#include "common.h"
#include "game/vector.h"
#include "game/car_types.h"

typedef struct CarControlCommand CarControlCommand;

extern CarHullPoint g_PlayerHullPoints[6];
extern CarHullPoint g_OpponentHullCorners[4];
extern CarHullPoint g_CarCornerOffsets[4];
extern CarCollisionPoint g_CarCollisionCorners[4];

/* Per-car runtime state, player in slot 0. Individual slots and single fields
 * also have their own split symbols. */
extern GameCarRuntime g_Cars[11];
extern GameCarRuntime g_CameraCar;

/* The four contenders ordered by race progress (`progressA + progressB`), best
 * first; re-sorted every frame by RankContenders to rubber-band the AI. */
extern GameCarRuntime *g_RankedCars[4];

/* Active car-entry table; repointed at one of the three 13-entry tables below
 * per title-menu row, so it is a pointer rather than a fixed array. */
extern CarEntry *g_CarTable;

/* The three saved car-entry tables, one per title-menu race row (0 GRAND PRIX,
 * 1 EXTRA GRAND PRIX, 2 TIME ATTACK); save block +0x50 / +0xC0 / +0x128. The
 * shops raise the Time Attack row to the best spec reached in either GP file. */
extern CarEntry g_GrandPrixCars[];
extern CarEntry g_ExtraGrandPrixCars[];
extern CarEntry g_TimeAttackCars[];

/* g_Cars index the replay / attract camera is following. */
extern s32 g_CameraCarIndex;

/* Index into g_CarTable of the car the player drives; selects the model and
 * texture pack to install. Distinct from g_CarListCursor. */
extern s32 g_PlayerCarIndex;

/* Cursor of the car list being browsed in the shop; steps to the next entry
 * with `enabled == 0`. Buying it copies it into g_PlayerCarIndex. */
extern s32 g_CarListCursor;

/* Index of each car model's first grade in the 32-entry asset list; thirteen
 * entries, one per model. GetCarAssetIndex adds the owned grade to it. */
extern u8 g_CarModelBaseIndex[];

/* Per-model base of the progress level a purchase requires; the level needed is
 * this plus the grade being bought (GetCarUnlockLevel). */
extern u8 g_CarModelUnlockBase[];
extern GameCarSpec *g_CarSpec;
extern GearCurveRow g_GearTorqueCurve[];

/*
 * The car pipeline.
 */
/* Race-entry init for the player object: start pose plus the speed/gear lookup
 * tables g_GearTorqueCurve / g_TorqueBandEnd / g_TorqueLossBandEnd. Logs "init_car" .. "init_ok". */
/* race_scene.c passes a bare void *; an empty parameter list keeps both
 * units' spellings.  The body reads a GameCarRuntime *. */
void InitPlayerCar(PlayerCarRuntime *car);
/* Non-clamping twin of UpdateCarTrackState: recomputes the track-relative placement
 * and writes the reference triple at +0x50, for the init/reset paths only. */
void ResetCarTrackState(GameCarRuntime *car);
/* The two variants of the rival-car driver over GameCarRuntime[11]. Race runs
 * only while `g_RacePhase >= 2 && g_GrandPrixMode`, adds three race-only passes
 * and time-slices cars 4..10; attract has no player so every car runs. */
void UpdateRaceCars(void);
void UpdateAttractCars(void);
/* Player-vs-field collision (detection, response and the crash cue), called
 * only from UpdatePlayerCar; returns the struck sub-quad 1..4 or 0. */
/* update_player_car.c spells the argument void *; the body reads
 * GameCarRuntime *. */
s32 CollidePlayerWithCars(PlayerCarRuntime *car);
/* One row of the AI pairwise sweep: car[index] against car[index + 1 .. 10],
 * push-apart only - no sound, no damage globals, no mode gate. */
s32 CollideRivalCars(GameCarRuntime *car, s32 index);
/* Draws one car, from the DrawCars loop; two LOD tiers plus the mirrored
 * wheel pass, submitted through SubmitModel. */
/* Selects model bank 1 and calls DrawCar for each of the 11 runtime cars
 * whose activeFlag != -1 and aiEnabled == 1. */
void DrawCars(void);
/* Car motion-state handler for CAR_MOTION_TAKEOFF: the one-frame jump takeoff,
 * which hands over to the airborne handler UpdateCarAirborne. */
void UpdateCarLaunch(PlayerCarRuntime *car);

/*
 * The player's own 0x19C-byte car object.
 */

/* Declared identically by 58 translation units before this
 * header carried them. */

extern s32 g_DriveBoostTimer;
extern s32 g_EngineRpmSnapshot;
extern s32 g_AutoShiftCooldown;
extern u8 *g_CarModelBuffer;
extern s16 g_DragScale;
extern s32 g_EngineRpm;
extern s32 g_EngineRpmJitter;
extern s16 g_GripLossTimer;
extern u16 g_HudGlyphClut;
extern s16 g_PeakOutputRpm;
extern s16 g_PeakOutputValue;
typedef struct RaceIntroCameraScript RaceIntroCameraScript;
extern RaceIntroCameraScript *volatile g_RaceIntroCameraScript;
extern s32 g_RoadGrade;
extern s32 g_SharedAssetWord0;
extern s32 g_ShiftSoundLevel;
extern s32 g_ShiftTargetRpm;
extern s32 g_StandingStartSpin;
extern s16 g_SteerHoldFrames;
extern s16 g_TachoNeedleQuad[4][2];
extern s16 g_TrackZoneDark;

/* (model, owned grade) -> index of the CAR_xx asset pair, 0..31. */
s32 GetCarAssetIndex(s32 model, s32 grade);
/* Progress level needed to buy this model's next grade. */
s32 GetCarUnlockLevel(s32 model);
void SetCarImageSlot(CarImageData *asset, s32 index);
struct CarModelAsset;
void SetCarModelSlot(struct CarModelAsset *asset, s32 index);
/* Which of the two showroom model slots is live, 0 or 1. */
extern u32 g_CarModelSlot;
/* Point g_CarModelAsset at g_CarModelSlots[index]. */
void SelectCarModelSlot(s32 index);
struct CarModelAsset *GetSerializedCarModelAsset(struct CarModelAsset *nativeAsset);
/* Repaint the loaded car's texture block in the two body colours. */
void ApplyBodyColor1(u32 colour, CarImageData *imageData);
void ApplyBodyColor2(u32 colour, CarImageData *imageData);
s32 SmoothTrackAngle(s32 pointIndex, s32 weight);
void UpdateRivalRubberBand(void);

/* Declared identically by 77 translation units before this
 * header carried them. */

extern char g_MsgInitCar[];
extern char g_MsgHTbl[];
extern char g_MsgInit0[];
extern char g_MsgInit1[];
extern char g_MsgInit1b[];
extern char g_FmtDecimalLine[];
extern char g_MsgInit2[];
extern char g_MsgInit4[];
extern char g_MsgInit5[];
extern char g_MsgInit6[];
extern char g_FmtLongLine[];
extern char g_MsgInitOk[];
extern s16 g_LaunchEnergyThresholds[];
extern s16 g_RedlineToPeakRpmHalf;
extern s16 g_PeakToRevLimitRpmHalf;
extern s16 g_StandingStartState;
extern RaceGridSlot g_AttractGridSlots[];
extern u16 g_BodyColorPrimary[];
extern u16 g_BodyColorSecondary[];
extern s16 g_NegconAccelMask;
extern s16 g_NegconAnalogI;
extern s16 g_NegconAnalogII;
extern s16 g_NegconAnalogL;
extern s16 g_NegconBrakeMask;
extern s16 g_PadAccelMask;
extern s16 g_PadBrakeMask;
extern s16 g_PadShiftMasks[2][8];
extern volatile u16 g_PaintBlendShades[3];
#define g_PaintBlendShade0 g_PaintBlendShades[0]
#define g_PaintBlendShade1 g_PaintBlendShades[1]
#define g_PaintBlendShade2 g_PaintBlendShades[2]
extern volatile u16 g_PaintSlots3StopA[];
extern volatile u16 g_PaintSlots3StopB[];
extern volatile u16 g_PaintSlots4Stop[];
extern RaceGridSlot g_RaceGridSlots[];
/*
 * The race-intro camera's offset from the keyframe it is easing away from:
 * the three halfwords at 0x8009AFBC.  All three writers take them from one
 * keyframe's f0/f4/f8 in x/y/z order and the easing reads them back in the
 * same order, one per camera axis.  Retail's codegen does not discriminate
 * here -- no component is touched twice in a block -- so this is a layout
 * claim, not a proof.
 */
extern SVec g_RaceIntroCameraDelta;
extern s32 g_RaceIntroCameraTimer;
extern s32 g_ShiftTargetSpeed;
extern s32 g_TachoNeedleFlash;
extern s16 g_TorqueBandStart;
extern s16 g_TorqueLossBandStart;

void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex);
void BlendPaintColor(u32 color0, u32 color1);
void BlendPaintColorQuarters(u32 color0, u32 color1);
void BlendPaintColorThirds(u32 color0, u32 color1);
void BuildTachoNeedleQuad(void);
void ClampCarLateralOffset(GameCarRuntime *car, s32 carIndex);
s32 GetCarCrestTrigger(GameCarRuntime* car);
s32 GetTrackSurfaceHeight(CarSurfaceSampleView *sample);
void InitRivalCar(GameCarRuntime* ent, s32 pos, RaceGridSlot* slots);
void InitRivalCarAi(GameCarRuntime* ent, s32 pos, RaceGridSlot* slots);
void RankContenders(void);
void SeedCarRouteMarkers(void);
void SlowRivalAhead(GameCarRuntime *car, s32 carIndex);
void SteerCarToTrackLine(PlayerCarRuntime *car);
void SteerCarAlongRoute(GameCarRuntime *car);
void TransformCollisionVector(const s16 *input, s32 *output);
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 gear);
void UpdateCarDrivetrain(PlayerCarRuntime *car,
                         const CarControlCommand *command);
void UpdateCarDriving(PlayerCarRuntime *car);
void UpdateCarStandingStart(PlayerCarRuntime *car);
void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex);
void AccumulateLapProgress(GameCarRuntime *car);
void AdvanceCarPosition(GameCarRuntime *car);
void ApplyCarKnockback(GameCarRuntime *car);
void ClearCarMotionState(GameCarRuntime *car);
s32 FindTrackSegment(GameCarRuntime *car, s32 startIndex);
s32 IsCarFacingBackwards(PlayerCarRuntime *car);
s32 IsPointInQuad(s32 p0, s32 p1, s32 p2, s32 p3, s32 point);
void SeedCarLapProgress(GameCarRuntime *car, s32 mode);
void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode);
void StartCarBodyKick(s32 strength, GameCarRuntime *car);
void UpdateCarAirborne(PlayerCarRuntime *car);
void UpdateCarBodyKick(GameCarRuntime *car);
void UpdateCarBodyRoll(PlayerCarRuntime *car,
                       const CarControlCommand *command);
void UpdateCarCrestHop(GameCarRuntime *car);
void UpdateCarTiltCounter(GameCarRuntime *car);
s32 UpdateCarTrackState(GameCarRuntime *car, s32 trackPointIndex,
                        CarTrackLimits *limits);
void DrawTachometer(s32 rpm, s32 flash, s32 type, s32 amt);
void DrawPlayerTachometer(void);
void BeginCarStandingStart(PlayerCarRuntime *car, s32 sceneTimer);
void RunRaceIntroCamera(PlayerCarRuntime *car, s32 mode);
void UpdatePlayerCar(PlayerCarRuntime *car);

#endif
