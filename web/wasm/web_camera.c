/* Retail race cameras (track/camera_chase.c and friends). See web_camera.h. */
#include "web_camera.h"

#include <math.h>
#include <string.h>

#include "game/angle.h"
#include "game/integer.h"
#include "rage/chase_camera.h"
#include "render/render_projection.h"
#include "scene_matrix.h"
#include "web_sky.h"

/* 20 degrees vertically on the wide mirror target (render_world_game.c's
 * GameRenderWorldPublishCurrentCamera). */
#define MIRROR_FOV_DEGREES 20.0f
/* PAL geom screen 320 at 240 lines. */
#define RACE_FOV_DEGREES 41.112f

/* Chase camera, mode 1. The yaw settling is the retail integer code
 * verbatim. The eye/look-at geometry uses the same offsets, matrix order and
 * angle formulas, evaluated in floats instead of GTE fixed point: the camera
 * is presentation only and never feeds back into the simulation. */

static s32 SquareRootInt(s32 value) {
    uint32_t x = value > 0 ? (uint32_t)value : 0u, root = 0, bit = 1u << 30;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= root + bit) { x -= root + bit; root = (root >> 1) + bit; }
        else root >>= 1;
        bit >>= 2;
    }
    return (s32)root;
}

static void SettleChaseYaw(WebChase *chase, s32 stepLimit, s32 acceleratedStep, int negative) {
    if (stepLimit < acceleratedStep) {
        chase->yawLag = negative ? -stepLimit : stepLimit;
        if (negative) chase->rampNeg = SquareRootInt(WrapSigned32((int64_t)stepLimit * chase->damping));
        else chase->rampPos = SquareRootInt(WrapSigned32((int64_t)stepLimit * chase->damping));
    } else {
        chase->yawLag = negative ? -acceleratedStep : acceleratedStep;
    }
}

static void AdvanceChaseYawRamp(WebChase *chase, s32 stepLimit, int negative) {
    s32 ramp, acceleratedStep;
    if (stepLimit > 0x40) stepLimit = 0x40;
    chase->stepLimit = stepLimit;
    ramp = WrapSigned32((int64_t)(negative ? chase->rampNeg : chase->rampPos) + 8);
    acceleratedStep = WrapSigned32((int64_t)ramp * ramp) / chase->damping;
    if (negative) { chase->rampPos = 0; chase->rampNeg = WrapSigned32((int64_t)chase->rampNeg + 8); }
    else { chase->rampNeg = 0; chase->rampPos = WrapSigned32((int64_t)chase->rampPos + 8); }
    chase->step = acceleratedStep;
    SettleChaseYaw(chase, stepLimit, acceleratedStep, negative);
}

static s32 ChaseYawDamping(s32 carSpeed) {
    s32 difference = WrapSigned32((int64_t)0x4E2 - carSpeed), damping;
    if (carSpeed >= 0x321) {
        if (difference < 6) difference = 6;
        return ((((difference * 8) / 50) + 8) / 10) + 1;
    }
    damping = WrapSigned32((int64_t)difference * 6);
    damping = WrapSigned32((int64_t)damping * difference) / 2500;
    damping = WrapSigned32((int64_t)damping - WrapSigned32((int64_t)difference * 0x46) / 50);
    damping = WrapSigned32((int64_t)damping + 0xE0) / 10;
    return damping > 0 ? damping : 1;
}

static void UpdateChaseYawStep(WebChase *chase, s32 targetYaw, s32 previousYaw) {
    s32 error = WrapSigned32((int64_t)targetYaw - previousYaw);
    if (error >= 5) {
        if (error >= 0x800) AdvanceChaseYawRamp(chase, (((0x1000 - error) / 17) * 2) & ANGLE_MASK, 1);
        else AdvanceChaseYawRamp(chase, ((error / 17) * 2) & ANGLE_MASK, 0);
    } else if (error < -4) {
        if (error < -0x7FF) AdvanceChaseYawRamp(chase, (((0x1000 + error) / 17) * 2) & ANGLE_MASK, 0);
        else AdvanceChaseYawRamp(chase, ((WrapSigned32(-(int64_t)error) / 17) * 2) & ANGLE_MASK, 1);
    } else {
        chase->yawLag = chase->rampNeg = chase->rampPos = 0;
    }
}

static SceneMat3 CarRotation(const PlayerCarRuntime *car) {
    return SceneMat3Multiply(SceneRotationZ(car->bodyRoll),
                             SceneMat3Multiply(SceneRotationX(car->bodyPitch),
                                               SceneRotationY(car->bodyYaw)));
}

/* Eye position and PS1 view angles, as CameraViewFromChaseCamera leaves them
 * for chase preset 0 (eye 0x3A up, 0x118 back). */
static void RetailChaseView(WebChase *chase, const PlayerCarRuntime *car, Vec3 *eye,
                            s32 *pitch, s32 *yaw, s32 *roll) {
    s32 target = car->bodyYaw & ANGLE_MASK, settled, lag;
    SceneMat3 cameraRotation, object, inverseObject, work;
    Vec3 focus, eyeWorld;
    s32 ex, ey, ez, distance, angleX;

    if (chase->active) {
        chase->previousYaw &= ANGLE_MASK;
        chase->rampNeg &= ANGLE_MASK;
        chase->rampPos &= ANGLE_MASK;
    } else {
        chase->previousYaw = target;
        chase->rampNeg = chase->rampPos = 0;
        chase->active = 1;
    }
    chase->damping = ChaseYawDamping(car->speed);
    UpdateChaseYawStep(chase, target, chase->previousYaw);
    settled = WrapSigned32((int64_t)chase->previousYaw + chase->yawLag) & ANGLE_MASK;
    lag = WrapSigned32((int64_t)target - settled);
    if (target < settled) { if (lag < -0x7FF) lag = WrapSigned32((int64_t)lag + 0x1000); }
    else if (lag >= 0x800) lag = WrapSigned32((int64_t)lag - 0x1000);
    chase->yawLag = lag;
    chase->previousYaw = settled;

    cameraRotation = SceneMat3Multiply(SceneRotationX(-0x80), SceneRotationY(-lag));
    object = CarRotation(car);
    inverseObject = SceneMat3Transpose(object);
    work = SceneMat3Transpose(SceneMat3Multiply(cameraRotation, object));

    focus = SceneRotatePoint(inverseObject, 0.0f, -0x3C, 0x32);
    eyeWorld = SceneRotatePoint(work, 0.0f, (float)ChaseCameraHeight(0x3A),
                           (float)ChaseCameraDistance(0x118));
    eye->x = (float)car->x + focus.x - eyeWorld.x;
    eye->y = (float)car->y + focus.y - eyeWorld.y;
    eye->z = (float)car->z + focus.z - eyeWorld.z;

    ex = (s32)lroundf(eyeWorld.x);
    ey = (s32)lroundf(eyeWorld.y);
    ez = (s32)lroundf(eyeWorld.z);
    distance = SquareRootInt(WrapSigned32((int64_t)ex * ex + (int64_t)ez * ez));
    angleX = 0x400 - (Atan2(WrapSigned32((int64_t)ey + 0x28), distance) & ANGLE_MASK);
    *yaw = 0x400 - (Atan2(ex, ez) & ANGLE_MASK) + ChaseCameraYawOffset(car->steeringAngle);
    *roll = WrapSigned32((int64_t)car->bodyRoll - car->bodyRollVelocity);
    *pitch = angleX - 0x90 + ChaseCameraPitchOffset();
}

/* Mode 0, CameraViewFromCarBlock: the car's own pose, lifted along its up
 * axis and pitched by its tilt counter. */
static void RetailCarView(const PlayerCarRuntime *car, Vec3 *eye, s32 *pitch, s32 *yaw, s32 *roll) {
    const Vec3 lift = SceneRotatePoint(SceneMat3Transpose(CarRotation(car)), 0.0f, -0x1C0 / 16.0f, 0.0f);
    eye->x = (float)car->x + lift.x;
    eye->y = (float)car->y + lift.y;
    eye->z = (float)car->z + lift.z;
    *pitch = WrapSigned32((int64_t)car->bodyPitch + car->tiltCounter);
    *yaw = car->bodyYaw;
    *roll = car->bodyRoll;
}

/* CameraViewFromLookBehind: the orbit camera turned round behind the car. */
static void RetailLookBehindView(const PlayerCarRuntime *car, Vec3 *eye, s32 *pitch, s32 *yaw,
                                 s32 *roll) {
    enum { LOOK_BEHIND_YAW = 0x800, LOOK_BEHIND_DISTANCE = 0xE0, LOOK_BEHIND_HEIGHT = 0x50 };
    const SceneMat3 object = CarRotation(car);
    const SceneMat3 cameraToWorld =
        SceneMat3Transpose(SceneMat3Multiply(SceneRotationY(-LOOK_BEHIND_YAW), object));
    const Vec3 focus = SceneRotatePoint(SceneMat3Transpose(object), 0.0f, 0.0f, 0x32);
    const Vec3 eyeWorld = SceneRotatePoint(cameraToWorld, 0.0f, LOOK_BEHIND_HEIGHT, LOOK_BEHIND_DISTANCE);
    eye->x = (float)car->x + focus.x - eyeWorld.x;
    eye->y = (float)car->y + focus.y - 0x28 - eyeWorld.y;
    eye->z = (float)car->z + focus.z - eyeWorld.z;
    *pitch = 0x400 - (Atan2((s32)lroundf(eyeWorld.y), LOOK_BEHIND_DISTANCE) & ANGLE_MASK);
    *yaw = 0x400 - (Atan2((s32)lroundf(eyeWorld.x), (s32)lroundf(eyeWorld.z)) & ANGLE_MASK);
    *roll = car->bodyRoll;
}

/* render_world_game.c's GameRenderWorldBuildCamera: near 1, the verified
 * race depth limit. A rear-facing (mirror) camera pre-rotates the view basis
 * by 180 degrees in its own local space, like an attached camera rig. */
RenderCamera WebCameraFromView(const ClientRace *race, Vec3 eye, s32 pitch, s32 yaw, s32 roll,
                                   float verticalFovDegrees, int rearFacing) {
    RenderCamera camera;
    SceneMat3 view, converted;

    memset(&camera, 0, sizeof(camera));
    camera.transform.position = (Vec3){eye.x, -eye.y, -eye.z};
    view = SceneMat3Multiply(SceneMat3Multiply(SceneRotationZ(roll), SceneRotationX(pitch)),
                             SceneRotationY(yaw));
    if (rearFacing) view = SceneMat3Multiply(SceneRotationY(0x800), view);
    RenderConvertPsxMatrix(view.m, converted.m);
    camera.transform.orientation = SceneQuaternion(SceneMat3Transpose(converted));
    camera.transform.hasOrientation = 1;
    camera.transform.rotation = (Vec3){-AngleToDegrees(pitch), -AngleToDegrees(yaw),
                                       -AngleToDegrees(roll)};
    camera.transform.scale = (Vec3){1.0f, 1.0f, 1.0f};
    camera.verticalFovDegrees = verticalFovDegrees;
    camera.nearPlane = 1.0f;
    camera.farPlane = 16384.0f;
    WebSkySetCamera(&camera, race, (s32)lroundf(eye.y), pitch, yaw, roll, rearFacing);
    return camera;
}

RenderCamera WebRaceCamera(WebChase *chase, const ClientRace *race, const PlayerCarRuntime *car,
                           WebView selected, RenderCamera *mirror) {
    Vec3 eye;
    s32 pitch, yaw, roll;

    if (selected == WEB_VIEW_CHASE) {
        RetailChaseView(chase, car, &eye, &pitch, &yaw, &roll);
    } else {
        chase->active = 0;
        if (selected == WEB_VIEW_LOOK_BEHIND) RetailLookBehindView(car, &eye, &pitch, &yaw, &roll);
        else RetailCarView(car, &eye, &pitch, &yaw, &roll);
    }
    *mirror = WebCameraFromView(race, eye, pitch, yaw, roll, MIRROR_FOV_DEGREES, 1);
    return WebCameraFromView(race, eye, pitch, yaw, roll, RACE_FOV_DEGREES, 0);
}
