/* Race cameras for the browser (web_camera.c): retail's car view, chase
 * view and look-behind view, plus the rear-view mirror camera. */
#ifndef WEB_CAMERA_H
#define WEB_CAMERA_H
#include "client_race.h"
#include "game/car.h"
#include "render/render_world.h"

/* race_scene.c: the race starts in the car view; the camera button swaps it
 * with the chase view (chase preset 0, the only one retail selects), and
 * holding down in the chase view looks behind. */
typedef enum WebView { WEB_VIEW_CAR, WEB_VIEW_CHASE, WEB_VIEW_LOOK_BEHIND } WebView;

/* Retail chase camera state, settled once per game frame. */
typedef struct WebChase {
    s32 previousYaw, rampNeg, rampPos, yawLag, damping, stepLimit, step;
    int active;
} WebChase;

/* The race view with the PAL 320x240 projection, and through `mirror` the
 * rear-view mirror camera derived from the same eye. The chase camera only
 * settles while it is the view, as retail's previousMode check restarts it
 * otherwise. */
RenderCamera WebRaceCamera(WebChase *chase, const ClientRace *race, const PlayerCarRuntime *car,
                           WebView view, RenderCamera *mirror);
/* A camera at `eye` with PS1 view angles: near 1, the verified race depth
 * limit, and the course sky. A rear-facing (mirror) camera is turned round in
 * its own space. */
RenderCamera WebCameraFromView(const ClientRace *race, Vec3 eye, s32 pitch, s32 yaw, s32 roll,
                               float verticalFovDegrees, int rearFacing);
#endif
