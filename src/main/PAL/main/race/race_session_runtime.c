#include "game/audio.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/race_session_runtime.h"
#include "game/player_car_internal.h"
#include "game/records_internal.h"
#include "game/render.h"
#include "game/save_internal.h"
#include "game/state.h"

void CaptureRaceSession(RaceSession *session, s32 retireCameraActive) {
    session->pause = (RacePauseState){
        g_SceneTimer, g_RacePaused, g_PauseDebounce, g_RacePhase,
        g_RaceOptionCursor, g_GrandPrixMode, g_RaceFadeTimer,
        g_CourseProgress->retriesRemaining, retireCameraActive};
    session->end = (RaceEndState){
        g_RacePhase, g_GrandPrixMode, g_RaceFadeTimer,
        g_CourseProgress->retriesRemaining};
}

void ApplyRaceSession(
    const RaceSession *session,
    const RaceSessionCommands *commands,
    s32 *retireCameraActive) {
    const RacePauseCommands *pause = &commands->pause;
    const RaceEndCommands *end = &commands->end;
    s32 i;

    g_SceneTimer = session->pause.sceneTimer;
    g_RacePaused = session->pause.paused;
    g_PauseDebounce = session->pause.debounce;
    g_RacePhase = session->pause.phase;
    g_RaceOptionCursor = session->pause.optionCursor;
    g_RaceFadeTimer = session->pause.fadeTimer;
    *retireCameraActive = session->pause.retireCameraActive;

    if (pause->pauseCd) PauseCdAudio();
    if (pause->resumeCd) ResumeCdAudio();
    if (pause->setEffectVoices)
        ForceAllEffectVoicesEnabled(pause->effectVoicesEnabled);
    for (i = 0; i < pause->soundCueCount; i++)
        PlaySoundCue(pause->soundCues[i]);
    if (pause->updateTimeAttackRecord) {
        g_BestLapTimes[ReadStableRaceSeries()][RageSeriesCourseIndex()][0] =
            g_RankingRecords[ReadStableRaceSeries()]
                            [RageSeriesCourseIndex()][0].raceTime;
    }
    if (pause->seedFinishCamera) SeedFinishCamera(&g_PlayerCar);
    if (pause->startCdFadeFrames)
        StartCdVolumeFade(pause->startCdFadeFrames);
    if (pause->exitRaceScene >= 0)
        ExitRaceScene(pause->exitRaceScene);

    if (end->drawEndBannerIntensity >= 0)
        DrawRaceEndBanner(end->drawEndBannerIntensity);
    if (end->drawLostCaptionIntensity >= 0)
        DrawLostRaceCaption(end->drawLostCaptionIntensity);
    if (end->drawFadeIntensity >= 0)
        DrawFullscreenFadeTile(end->drawFadeIntensity, 0x49);
    if (end->requestCdTrack >= 0) RequestCdTrack(end->requestCdTrack);
    if (end->startCd) StartCdAudio();
    if (end->exitScene >= 0) ExitRaceScene(end->exitScene);
    if (end->disableMirror) g_MirrorViewEnabled = 0;
}
