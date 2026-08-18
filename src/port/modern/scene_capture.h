#ifndef RAGE_SCENE_CAPTURE_H
#define RAGE_SCENE_CAPTURE_H

/* Adapter from the recovered classic submission path to RageRenderScene.
 * It observes submissions but never feeds data back into gameplay. */
#include "../render/render_scene.h"

typedef struct RageCaptureFaceInput {
    int kind, klass;
    int semi, raw, fogged;
    int bias;
    int otDepth;
    int fog;
    int cellSlot;
    const void *v[4];
    const uint8_t *uv;
    uint8_t uvStorage[8];
    uint16_t clut, tpage;
    uint32_t textureWindow;
    const uint8_t *colors;
    int colorCount;
} RageCaptureFaceInput;

int RageModernCullMarginX(void);
int RageModernDepthLimit(void);
int RageCaptureActive(void);
void RageCaptureFrameBegin(void);
void RageCaptureFrameEnd(void);
void RageCaptureModelBegin(int kind, int index, int fogged);
void RageCaptureTerrainBegin(const void *cells, int count);
void RageCaptureFace3D(const RageCaptureFaceInput *input);
void RageCaptureSubmitEnd(void);
const RageRenderScene *RageCaptureCurrent(void);
const RageRenderScene *RageCapturePrevious(void);

#define RageCaptureSnapshotHash RageRenderSceneHash

#endif
