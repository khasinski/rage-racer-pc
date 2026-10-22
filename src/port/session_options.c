#include "session_options.h"

#include "analog_pad.h"
#include "force_feedback_device.h"
#include "game/audio.h"
#include "rage/chase_camera.h"
#include "rage/hud_config.h"

void PortSessionOptionsRead(PortSessionOptions *options) {
    if (options == NULL) return;
    options->analog = PortAnalogEnabled();
    options->wheel = PortWheelEnabled();
    options->steeringLinearity = PortSteeringLinearity();
    options->forceFeedback = PortForceFeedbackEnabled();
    options->forceFeedbackGain = PortForceFeedbackGainPercent();
    options->music = (int)ClampAudioSetting(g_BgmVolumeSetting);
    options->effects = (int)ClampAudioSetting(g_SfxVolumeSetting);
    options->hudEdges = HudAnchorEdges();
    options->showLapTimes = HudShowLapTimes();
    options->showTimeLimit = HudShowTimeLimit();
    options->chaseDistance = ChaseCameraDistanceScale();
    options->chaseHeight = ChaseCameraHeightScale();
    options->chaseLookahead = ChaseCameraLookahead();
}

void PortSessionOptionsApply(const PortSessionOptions *options) {
    int music;
    int effects;

    if (options == NULL) return;
    PortSetAnalogEnabled(options->analog);
    PortSetWheelEnabled(options->wheel);
    PortSetSteeringLinearity(options->steeringLinearity);
    PortSetForceFeedbackEnabled(options->forceFeedback);
    PortSetForceFeedbackGainPercent(options->forceFeedbackGain);
    music = (int)ClampAudioSetting(options->music);
    effects = (int)ClampAudioSetting(options->effects);
    if (music != g_BgmVolumeSetting || effects != g_SfxVolumeSetting) {
        g_BgmVolumeSetting = music;
        g_SfxVolumeSetting = effects;
        ApplyAudioSettings();
    }
    HudSetAnchorEdges(options->hudEdges);
    HudSetShowLapTimes(options->showLapTimes);
    HudSetShowTimeLimit(options->showTimeLimit);
    ChaseCameraSetDistanceScale(options->chaseDistance);
    ChaseCameraSetHeightScale(options->chaseHeight);
    ChaseCameraSetLookahead(options->chaseLookahead);
}
