#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

typedef struct AnimatedSceneryTransform {
    Vec4 position;
    Matrix objectMatrix;
    Matrix worldMatrix;
} AnimatedSceneryTransform;

enum {
    ANIMATED_SCENERY_INSTANCE_COUNT = 2,
};

static Vec4 AnimatedSceneryPosition(s32 instance) {
    Vec4 position = g_AnimSceneryPos[instance];

    if (SeriesCourseIndex() == 3) {
        position.z += 0x5000;
    }
    return position;
}

static void BuildAnimatedSceneryTransform(AnimatedSceneryTransform *transform,
                                          s32 instance) {
    Matrix pitchMatrix;

    BuildRotMatrixY(&transform->objectMatrix, transform->position.w);
    BuildRotMatrixX(&pitchMatrix, g_AnimSceneryPitch[instance]);
    MulMatrix(&transform->objectMatrix, &pitchMatrix);
    transform->worldMatrix = transform->objectMatrix;
    MulMatrix2(&g_RenderState.matrix, &transform->objectMatrix);
}

static void SubmitAnimatedSceneryLayer(AnimatedSceneryTransform *transform,
                                       s32 entity, s32 modelId, s32 tint) {
    Vec4 *position = &transform->position;

    modelId = ModelOrFallback(modelId, g_CourseModelCount);
    SetGteObjectMatrix(&g_ObjectMatrixWork, AsPosition(position),
                       &transform->objectMatrix);
    g_RenderState.envMode4 = tint;
    GameRenderWorldSubmitDynamicCourseOverlay(
        entity, modelId, position->x, position->y, position->z,
        transform->worldMatrix.m, 0, 0);
    SubmitCourseModel(&g_RenderState, modelId);
}

void DrawAnimatedScenery(s32 timer, s32 instance) {
    AnimatedSceneryTransform transform;
    s32 primaryModel;
    s32 secondaryModel;

    if (instance < 0 || instance >= ANIMATED_SCENERY_INSTANCE_COUNT) {
        return;
    }

    transform.position = AnimatedSceneryPosition(instance);
    if (g_GrandPrixClass == GRAND_PRIX_FINAL_CLASS_INDEX ||
        !TrackCellVisible(transform.position.x, transform.position.z)) {
        return;
    }

    g_AnimSceneryFrame = (timer / 4) % 16;
    if (g_AnimSceneryFrame == 0 && timer % 8 == 0 && g_RacePaused == 0) {
        g_AnimSceneryTint = 0;
        g_AnimSceneryRacePosition = g_PlayerCar.drive.racePosition;
        g_AnimSceneryVariant = (Random15() & 7) / 3;
        if (g_AnimSceneryRacePosition >= 4) {
            g_AnimSceneryRacePosition = 0;
        }
    }

    BuildAnimatedSceneryTransform(&transform, instance);
    if (g_GrandPrixMode == 0) {
        return;
    }

    g_AnimSceneryTint = ((timer >> 3) & 3) << 16;
    if (g_AnimSceneryRacePosition != 0) {
        primaryModel = g_AnimSceneryFrame < 13
            ? g_AnimSceneryFrame + 10
            : g_AnimSceneryRacePosition;
        secondaryModel = g_AnimSceneryVariant + 4;
    } else {
        primaryModel = g_AnimSceneryFrame + 0x18;
        secondaryModel = g_AnimSceneryVariant + 7;
    }

    SubmitAnimatedSceneryLayer(&transform, 0x20 + instance * 2,
                               primaryModel, 0);
    SubmitAnimatedSceneryLayer(&transform, 0x21 + instance * 2,
                               secondaryModel, g_AnimSceneryTint);
}

void DrawPresentationAnimatedScenery(s32 timer, s32 instance, s32 isReplay,
                                     s32 animate) {
    AnimatedSceneryTransform transform;
    s32 primaryModel;
    s32 secondaryModel;

    if (instance < 0 || instance >= ANIMATED_SCENERY_INSTANCE_COUNT ||
        g_GrandPrixMode == 0 ||
        g_GrandPrixClass == GRAND_PRIX_FINAL_CLASS_INDEX) {
        return;
    }

    transform.position = AnimatedSceneryPosition(instance);
    if (!TrackCellVisible(transform.position.x, transform.position.z)) {
        return;
    }

    g_PresentationSceneryFrame = (timer / 4) % 16;
    if (g_PresentationSceneryFrame == 0 && timer % 8 == 0 && animate == 1) {
        g_PresentationSceneryTint = 0;
        g_PresentationSceneryVariant = (Random15() & 7) / 3;
    }

    BuildAnimatedSceneryTransform(&transform, instance);
    g_PresentationSceneryTint = ((timer >> 3) & 3) << 16;
    primaryModel = g_PresentationSceneryFrame +
                   (isReplay != 0 ? 0xA : 0x18);
    secondaryModel = g_PresentationSceneryVariant +
                     (isReplay != 0 ? 4 : 7);

    SubmitAnimatedSceneryLayer(&transform, 0x30 + instance * 2,
                               primaryModel, 0);
    SubmitAnimatedSceneryLayer(&transform, 0x31 + instance * 2,
                               secondaryModel, g_PresentationSceneryTint);
}
