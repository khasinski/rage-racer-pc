/*
 * Lock every branch of UpdateCamera.
 *
 * UpdateCamera picks one of six ways to place the camera, and which one it
 * picks comes from the track's own camera node rather than from the caller.
 * Twenty thousand frames of attract mode and a full race between them reach
 * three of the six; the other three only turn up when a node asks for them,
 * which is what this does. That is the whole reason the test exists: half the
 * function is not reachable by playing.
 *
 * The expected values are the ones the shipped game produces, so a deliberate
 * change to what a camera does means updating them on purpose.
 */

#include "common.h"
#include "game/render.h"
#include "game/track_internal.h"
#include "game/track_camera_internal.h"
#include "game/player_car_internal.h"
#include "game/render_state.h"
#include "rage/chase_camera.h"

#include <stdio.h>
#include <string.h>

void UpdateCamera(CameraViewMode cameraModeSel, GameRenderObject *car);

/*
 * The globals the camera reads are the game's own, out of host_state.c, so a
 * value that is wrong there is wrong here too. Only the calls out of the
 * camera into the rest of the renderer are stood in for.
 */
/* The two the camera writes through, which the port allocates alongside the
 * renderer rather than in host state. */
GameRenderState g_RenderState;
PlayerCarRuntime g_PlayerCar;

static GameTrackCameraNode s_nodes[2];
static s32 s_nearestCamera;

/* The camera asks the track which node is nearest; the test says which. */
s32 FindNearestTrackCamera(GameRenderObject *car) {
    (void)car;
    return s_nearestCamera;
}

void DrawPlayerCarModel(GameRenderObject *obj) { (void)obj; }
void SelectModelBank(s32 bank) { (void)bank; }
int ChaseCameraYawOffset(int steeringAngle) {
    (void)steeringAngle;
    return 0;
}

/* Reached only from SetCameraRotMatrix, which builds the mirror view rather
 * than the camera this test reads. */
MATRIX *MulMatrix0(MATRIX *a, MATRIX *b, MATRIX *out) {
    (void)a;
    (void)b;
    return out;
}
void GameRenderWorldSetCamera(s32 x, s32 y, s32 z, s32 pitch, s32 yaw,
                              s32 roll) {
    (void)x; (void)y; (void)z; (void)pitch; (void)yaw; (void)roll;
}

static int s_failures;

static void Check(const char *what, const s32 *got, const s32 *wanted) {
    int i;
    for (i = 0; i < 6; i++) {
        if (got[i] == wanted[i]) continue;
        printf("FAIL %s: got (%d,%d,%d) angle (%d,%d,%d), "
               "expected (%d,%d,%d) angle (%d,%d,%d)\n",
               what, got[0], got[1], got[2], got[3], got[4], got[5],
               wanted[0], wanted[1], wanted[2], wanted[3], wanted[4],
               wanted[5]);
        s_failures++;
        return;
    }
}

/*
 * Put the car somewhere with a pose that is not level, so a branch that drops
 * a rotation or mixes up two offsets moves the camera rather than landing on
 * the same answer by symmetry.
 */
static void PlaceCar(GameRenderObject *car) {
    memset(car, 0, sizeof(*car));
    car->x = 0x4000;
    car->y = 0x1000;
    car->z = 0x8000;
    car->bodyYaw = 0x300;
    car->bodyPitch = 0x40;
    car->bodyRoll = 0x60;
}

/* Drive one branch and read the view back out of the render state. */
static void Run(CameraViewMode selector, s32 *view) {
    GameRenderObject car;

    PlaceCar(&car);
    g_CameraNodeIndex = 0;
    s_nearestCamera = 0;
    g_CameraModePrev = 0;
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    UpdateCamera(selector, &car);
    view[0] = g_RenderState.viewX;
    view[1] = g_RenderState.viewY;
    view[2] = g_RenderState.viewZ;
    view[3] = g_RenderState.viewAngleX;
    view[4] = g_RenderState.viewAngleY;
    view[5] = g_RenderState.viewAngleZ;
}

/*
 * Drive the chase camera one frame with the yaw already lagging behind the car
 * by `yawError`, and report how far it advanced. g_CameraModePrev has to say 1
 * or the branch snaps the yaw to the target instead of chasing it. The lag
 * itself is no good to read back: the branch overwrites it further down with
 * the error that is left.
 */
static s32 ChaseAdvance(s32 yawError, s32 speed) {
    GameRenderObject car;
    s32 startYaw = (0x800 - yawError) & 0xFFF;

    PlaceCar(&car);
    car.bodyYaw = 0x800;
    car.speed = speed;
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_CameraNodeIndex = 0;
    g_CameraModePrev = 1;
    g_ChaseYawPrev = startYaw;
    g_ChaseYawRampPos = 0;
    g_ChaseYawRampNeg = 0;
    g_ChaseYawLag = 0;
    g_ChaseCameraPreset = 0;
    UpdateCamera(1, &car);
    return ((g_ChaseYaw - startYaw) + 0x800) % 0x1000 - 0x800;
}

int main(void) {
    s32 view[6];

    g_TrackCameras = s_nodes;
    memset(s_nodes, 0, sizeof(s_nodes));

    /* Mode 0 takes the camera straight off the car's own block and lifts it
     * by a fixed amount along the car's up axis. */
    {
        static const s32 wanted[6] = {16384, 4068, 32772, 64, 768, 96};
        Run(0, view);
        Check("mode 0, car block", view, wanted);
    }

    /* Unknown authored modes use the same stable fallback as a missing
     * camera table instead of retaining an unrelated previous view. */
    {
        static const s32 wanted[6] = {16384, 4068, 32772, 64, 768, 96};

        s_nodes[0].mode = 99;
        Run(2, view);
        Check("unknown track camera mode", view, wanted);
    }

    /* A missing authored camera falls back to the car-block view. */
    {
        static const s32 wanted[6] = {16384, 4068, 32772, 64, 768, 96};
        GameRenderObject car;

        PlaceCar(&car);
        s_nearestCamera = -1;
        memset(&g_RenderState, 0, sizeof(g_RenderState));
        UpdateCamera(2, &car);
        view[0] = g_RenderState.viewX;
        view[1] = g_RenderState.viewY;
        view[2] = g_RenderState.viewZ;
        view[3] = g_RenderState.viewAngleX;
        view[4] = g_RenderState.viewAngleY;
        view[5] = g_RenderState.viewAngleZ;
        Check("missing track camera", view, wanted);
        s_nearestCamera = 0;
    }

    /* Mode 1 is the chase camera. Preset 2 is the furthest of the three. */
    {
        static const s32 wanted[6] = {16046, 3931, 32649, 131, 785, 96};
        g_ChaseCameraPreset = 2;
        Run(1, view);
        Check("mode 1, chase preset 2", view, wanted);
    }

    /*
     * An unrecognised preset is the trap in that branch: the shipped code
     * builds both the look-at offset and the eye offset in one place, so a
     * preset outside 0..2 leaves the eye sitting on the look-at offset. It has
     * to keep doing that.
     */
    {
        s32 fallback[6];
        s32 explicitEye[6];
        g_ChaseCameraPreset = 99;
        Run(1, fallback);
        /* The same camera, with the fallback offset asked for by name. */
        g_ChaseCameraPreset = 0;
        s_nodes[0].mode = 0;
        Run(1, explicitEye);
        if (memcmp(fallback, explicitEye, sizeof(fallback)) == 0) {
            printf("FAIL unknown preset: fell through to preset 0\n");
            s_failures++;
        }
        g_ChaseCameraPreset = 2;
    }

    /* Mode 2 is a node that watches the car from a fixed spot and is dragged
     * towards it by its own blend. */
    {
        static const s32 wanted[6] = {13193, 3320, 26223, 75, 306, 0};
        s_nodes[0].mode = 2;
        s_nodes[0].data.world.x = 0x2000;
        s_nodes[0].data.world.y = 0x800;
        s_nodes[0].data.world.z = 0x4000;
        s_nodes[0].data.world.blend = 6000;
        s_nodes[0].offset[0] = 0x20;
        s_nodes[0].offset[1] = 0x40;
        s_nodes[0].offset[2] = 0x60;
        Run(2, view);
        Check("mode 2, blended node", view, wanted);
        if (g_RenderState.reserved14 != s_nodes[0].data.world.blend) {
            printf("FAIL mode 2 node metadata: got %d, expected %d\n",
                   g_RenderState.reserved14, s_nodes[0].data.world.blend);
            s_failures++;
        }
    }

    /* Mode 3 is the scripted cam path: an offset that slides across the node's
     * duration, an eye pushed back by the path's distance, and a roll read off
     * the camera's own right-hand axis. It is the only branch that sets a
     * roll from anything but the car. */
    {
        static const s32 wanted[6] = {16468, 5204, 38693, -113, -2054, 23};
        s_nodes[0].mode = 3;
        s_nodes[0].duration = 60;
        g_CamPathFrame = 30;
        g_CamPathNode = 0;
        g_CamPathOffsetStart[0] = 0x10;
        g_CamPathOffsetStart[1] = 0x20;
        g_CamPathOffsetStart[2] = 0x30;
        g_CamPathOffsetDelta[0] = 0x40;
        g_CamPathOffsetDelta[1] = 0x50;
        g_CamPathOffsetDelta[2] = 0x60;
        g_CamPathAngle[3] = 0x200;
        Run(2, view);
        Check("mode 3, cam path", view, wanted);
    }

    /* Mode 4 slides the camera to the node's own position across the node's
     * duration, then looks back at the car. */
    {
        static const s32 wanted[6] = {8192, 2048, 16384, 86, 304, 0};
        s_nodes[0].mode = 4;
        s_nodes[0].duration = 60;
        s_nodes[0].offset[0] = 0x2000;
        s_nodes[0].offset[1] = 0x800;
        s_nodes[0].offset[2] = 0x4000;
        s_nodes[0].data.orientation.distance = 0x180;
        g_CamPathFrame = 30;
        Run(2, view);
        Check("mode 4, sliding node", view, wanted);
        if (g_RenderState.reserved14 !=
            s_nodes[0].data.orientation.distance) {
            printf("FAIL mode 4 node metadata: got %d, expected %d\n",
                   g_RenderState.reserved14,
                   s_nodes[0].data.orientation.distance);
            s_failures++;
        }
    }

    /* A malformed zero duration is treated as one frame in both modes that
     * divide by it. This keeps corrupt or partially imported camera data from
     * taking down the race scene. */
    {
        s32 zeroDuration[6];
        s32 oneFrame[6];

        s_nodes[0].mode = 3;
        s_nodes[0].duration = 0;
        Run(2, zeroDuration);
        s_nodes[0].duration = 1;
        Run(2, oneFrame);
        Check("mode 3, zero duration", zeroDuration, oneFrame);

        s_nodes[0].mode = 4;
        s_nodes[0].duration = 0;
        Run(2, zeroDuration);
        s_nodes[0].duration = 1;
        Run(2, oneFrame);
        Check("mode 4, zero duration", zeroDuration, oneFrame);
    }

    /* Mode 5 orbits behind the car at a set distance. Its pitch comes off the
     * orbit distance rather than the flattened eye vector, which is a shade
     * different from a true look-at and is the view the game shipped. */
    {
        static const s32 wanted[6] = {16161, 4004, 32519, 94, 512, 96};
        s_nodes[0].mode = 5;
        g_OrbitCameraDistance = 0x180;
        g_OrbitCameraYaw = 0x100;
        Run(2, view);
        Check("mode 5, orbit behind", view, wanted);
    }

    /*
     * The chase camera has to lean into a turn. Four paths settle the yaw: the
     * error can point either way, and either way can be the short way round or
     * the long way across the wrap. A refactor that lets one of them fall
     * through to the "no error" arm throws away the swing it just worked out,
     * and the camera stops leading the car at all. Each of the four has to
     * move the yaw, and move it towards the car rather than away.
     */
    {
        static const struct {
            s32 error;
            s32 speed;
            const char *what;
        } cases[] = {
            {0x40, 0x400, "behind, short way"},
            {0x900, 0x400, "behind, across the wrap"},
            {-0x40, 0x400, "ahead, short way"},
            {-0x900, 0x400, "ahead, across the wrap"},
        };
        size_t i;
        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            s32 advance = ChaseAdvance(cases[i].error, cases[i].speed);
            /* The camera takes the shorter way round the circle, so past
             * half a turn it leans the other way. */
            s32 wanted = cases[i].error;
            if (wanted > 0x7FF) wanted -= 0x1000;
            if (wanted < -0x800) wanted += 0x1000;
            if (advance == 0 || (advance > 0) != (wanted > 0)) {
                printf("FAIL chase yaw %s: moved %d, wanted a step towards "
                       "%d\n", cases[i].what, advance, wanted);
                s_failures++;
            }
        }
    }

    /* Damping has separate low- and high-speed curves, including a clamp
     * once the car reaches the top of the table. Keep representative points
     * on both sides of that branch. */
    {
        static const struct {
            s32 speed;
            s32 damping;
        } cases[] = {
            {0, 222},
            {0x320, 8},
            {0x321, 8},
            {0x4E2, 1},
        };
        size_t i;
        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            ChaseAdvance(0x40, cases[i].speed);
            if (g_ChaseYawDamping != cases[i].damping) {
                printf("FAIL chase damping at speed %d: got %d, wanted %d\n",
                       cases[i].speed, g_ChaseYawDamping, cases[i].damping);
                s_failures++;
            }
        }
    }

    /* Errors within four angle units are the camera's settled dead zone. */
    if (ChaseAdvance(4, 0x400) != 0 || ChaseAdvance(-4, 0x400) != 0) {
        printf("FAIL chase yaw moved inside its dead zone\n");
        s_failures++;
    }

    if (s_failures != 0) {
        printf("%d camera branches moved\n", s_failures);
        return 1;
    }
    printf("every UpdateCamera branch places the camera where it always did\n");
    return 0;
}
