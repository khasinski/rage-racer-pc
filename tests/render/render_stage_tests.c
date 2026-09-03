/*
 * Where a stage puts its camera.
 *
 * Everything the stage does that can be wrong without a GPU noticing is here:
 * the orbit has to land the camera at the stated distance, facing the target,
 * on the side the caller asked for. The sign of the elevation is the trap:
 * "thirty degrees above the subject" is a negative camera pitch, because the
 * camera is looking down. Getting it backwards renders every car from
 * underneath and makes terrain vanish outright, since its authored quads are
 * one-sided.
 */

#include "render/render_stage.h"
#include "render/render_projection.h"

#include <math.h>
#include <stdio.h>

static int failures;

static void Expect(const char *what, float got, float want, float tolerance) {
    if (fabsf(got - want) <= tolerance) return;
    printf("FAIL %s: expected %.4f, got %.4f\n", what, want, got);
    failures++;
}

static void ExpectTrue(const char *what, int condition) {
    if (condition) return;
    printf("FAIL %s\n", what);
    failures++;
}

static float Distance(RageRenderVec3 a, RageRenderVec3 b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* Where the target lands in the camera's own space. A camera looking at its
 * target puts it straight ahead, which is negative Z and nothing else. */
static RageRenderVec3 TargetInView(const RageRenderStage *stage) {
    RageRenderCamera camera;
    RageRenderVec3 view;
    RenderStageCamera(stage, &camera);
    RenderWorldToView(&camera, &stage->target, &view);
    return view;
}

static void OrbitTests(void) {
    RageRenderStage stage;
    RageRenderCamera camera;
    int azimuth;
    int elevation;

    RenderStageDefaults(&stage);
    stage.target.x = 1500.0f;
    stage.target.y = -800.0f;
    stage.target.z = 32000.0f;
    stage.distance = 4000.0f;

    /* Wherever it stands, the camera is one orbit radius from the target and
     * looking straight at it. */
    for (azimuth = 0; azimuth < 360; azimuth += 30) {
        for (elevation = -60; elevation <= 60; elevation += 30) {
            RageRenderVec3 view;
            stage.azimuthDegrees = (float)azimuth;
            stage.elevationDegrees = (float)elevation;
            RenderStageCamera(&stage, &camera);
            Expect("orbit radius",
                   Distance(camera.transform.position, stage.target),
                   stage.distance, 0.5f);
            view = TargetInView(&stage);
            Expect("target is centred sideways", view.x, 0.0f, 0.5f);
            Expect("target is centred vertically", view.y, 0.0f, 0.5f);
            Expect("target is straight ahead", -view.z, stage.distance, 0.5f);
        }
    }
}

static void ElevationTests(void) {
    RageRenderStage stage;
    RageRenderCamera above, below, level;

    RenderStageDefaults(&stage);
    stage.distance = 1000.0f;
    stage.azimuthDegrees = 0.0f;

    stage.elevationDegrees = 0.0f;
    RenderStageCamera(&stage, &level);
    stage.elevationDegrees = 45.0f;
    RenderStageCamera(&stage, &above);
    stage.elevationDegrees = -45.0f;
    RenderStageCamera(&stage, &below);

    ExpectTrue("a positive elevation stands above the subject",
               above.transform.position.y > level.transform.position.y);
    ExpectTrue("a negative elevation stands below it",
               below.transform.position.y < level.transform.position.y);
    ExpectTrue("looking down is a negative pitch",
               above.transform.rotation.x < 0.0f);
    Expect("level with the subject is level", level.transform.position.y,
           0.0f, 0.001f);
    /* The two are mirror images of each other about the subject. */
    Expect("above and below are the same climb",
           above.transform.position.y, -below.transform.position.y, 0.001f);
    Expect("and the same distance out", above.transform.position.z,
           below.transform.position.z, 0.001f);
}

static void AzimuthTests(void) {
    RageRenderStage stage;
    RageRenderCamera front, quarter, side, back;

    RenderStageDefaults(&stage);
    stage.distance = 1000.0f;
    stage.elevationDegrees = 0.0f;

    stage.azimuthDegrees = 0.0f;
    RenderStageCamera(&stage, &front);
    stage.azimuthDegrees = 90.0f;
    RenderStageCamera(&stage, &side);
    stage.azimuthDegrees = 180.0f;
    RenderStageCamera(&stage, &back);
    stage.azimuthDegrees = 45.0f;
    RenderStageCamera(&stage, &quarter);

    /* Azimuth zero stands on the subject's near side and turns from there. */
    Expect("azimuth zero is on one axis", front.transform.position.x, 0.0f,
           0.001f);
    Expect("azimuth ninety is on the other", side.transform.position.z, 0.0f,
           0.001f);
    ExpectTrue("half a turn is the opposite side",
               fabsf(back.transform.position.z + front.transform.position.z) <
                   0.001f);
    ExpectTrue("a quarter turn is between the two",
               fabsf(quarter.transform.position.x) > 1.0f &&
                   fabsf(quarter.transform.position.z) > 1.0f);
    Expect("the camera turns with the orbit", quarter.transform.rotation.y,
           45.0f, 0.001f);
}

/* Composing has to place every pose asked for, and say so when it cannot. */
static void ComposeTests(void) {
    RageRenderStage stage;
    RageRenderPose poses[3];
    RageRenderMeshInstance instances[3];
    RageRenderWorld world;
    uint32_t index;

    RenderStageDefaults(&stage);
    for (index = 0; index < 3; index++) {
        RenderPoseDefaults(&poses[index]);
        poses[index].assetKey = 10 + index;
        poses[index].mesh = index;
        poses[index].position.x = (float)(index * 2048);
        poses[index].rotationDegrees.y = (float)(index * 90);
    }

    Expect("every pose is placed",
           (float)RenderStageCompose(&world, instances, 3, &stage, poses, 3),
           3.0f, 0.0f);
    Expect("the world counts them", (float)world.instanceCount, 3.0f, 0.0f);
    Expect("nothing overflowed", (float)world.overflowCount, 0.0f, 0.0f);
    ExpectTrue("the world has a camera", world.hasCamera != 0);
    for (index = 0; index < 3; index++) {
        Expect("the key is carried", (float)instances[index].assetKey,
               (float)(10 + index), 0.0f);
        Expect("the mesh is carried", (float)instances[index].mesh,
               (float)index, 0.0f);
        Expect("the position is carried",
               instances[index].transform.position.x,
               (float)(index * 2048), 0.0f);
        Expect("the rotation is carried",
               instances[index].transform.rotation.y, (float)(index * 90),
               0.0f);
        /* An unscaled instance renders at its authored size. */
        Expect("the scale is one", instances[index].transform.scale.x, 1.0f,
               0.0f);
        ExpectTrue("entities are distinct", instances[index].entity != 0);
    }

    /* More poses than room keeps the ones that fit and reports the rest. */
    Expect("a full stage places what it can",
           (float)RenderStageCompose(&world, instances, 2, &stage, poses, 3),
           2.0f, 0.0f);
    Expect("and counts what it could not", (float)world.overflowCount, 1.0f,
           0.0f);

    /* An empty stage needs no backing array. Invalid non-empty inputs fail
     * without dereferencing absent pose or instance storage. */
    Expect("an empty stage needs no storage",
           (float)RenderStageCompose(&world, NULL, 0, &stage, NULL, 0),
           0.0f, 0.0f);
    ExpectTrue("the empty stage still owns a camera", world.hasCamera != 0);
    Expect("poses require storage",
           (float)RenderStageCompose(&world, NULL, 1, &stage, poses, 1),
           0.0f, 0.0f);
    Expect("a non-empty stage requires poses",
           (float)RenderStageCompose(&world, instances, 1, &stage, NULL, 1),
           0.0f, 0.0f);
}

int main(void) {
    OrbitTests();
    ElevationTests();
    AzimuthTests();
    ComposeTests();

    if (failures != 0) {
        printf("%d stage assertion(s) failed\n", failures);
        return 1;
    }
    printf("the stage puts its camera where it says it does\n");
    return 0;
}
