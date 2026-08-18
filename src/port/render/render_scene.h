#ifndef RAGE_RENDER_SCENE_H
#define RAGE_RENDER_SCENE_H

#include <stdint.h>

/* Renderer-neutral frame description. The classic backend remains the
 * authoritative producer; modern consumes this data without reaching back
 * into mutable gameplay state. */
enum {
    RAGE_CAPTURE_KIND_MODEL = 0,
    RAGE_CAPTURE_KIND_COURSE = 1,
    RAGE_CAPTURE_KIND_TERRAIN = 2
};

typedef struct RageRenderMatrix {
    int16_t m[3][3];
    int32_t t[3];
} RageRenderMatrix;

typedef struct RageRenderTransformState {
    RageRenderMatrix rot;
    RageRenderMatrix light;
    RageRenderMatrix color;
    int32_t ofx, ofy;
    int32_t h;
    int32_t dqa, dqb;
} RageRenderTransformState;

typedef struct RageRenderModelDraw {
    uint8_t kind;
    uint8_t mirror;
    uint8_t table;
    uint8_t fogged;
    uint8_t otShift;
    uint8_t pad[3];
    int32_t modelIndex;
    int32_t otBaseBias;
    uint32_t renderMode;
    uint64_t bankId;
    RageRenderTransformState gte;
} RageRenderModelDraw;

#define RAGE_CAPTURE_MAX_CELLS 64
typedef struct RageRenderTerrainBatch {
    uint8_t mirror;
    uint8_t envMode4;
    uint8_t otShift;
    uint8_t pad;
    int16_t cellCount;
    int32_t cells[RAGE_CAPTURE_MAX_CELLS][4];
    RageRenderTransformState gte;
} RageRenderTerrainBatch;

#define RAGE_CAPTURE_PACKET_WORDS 16
typedef struct RageRenderUiPacket {
    uint16_t bucket;
    uint8_t table;
    uint8_t size;
    uint32_t words[RAGE_CAPTURE_PACKET_WORDS];
} RageRenderUiPacket;

typedef struct RageRenderFace {
    uint8_t kind;
    uint8_t klass;
    uint8_t flags;
    int8_t bias;
    int16_t drawIndex;
    int16_t cellSlot;
    int32_t otDepth;
    int32_t fog;
    uint16_t clut, tpage;
    uint32_t textureWindow;
    int16_t pos[4][4];
    uint8_t uv[4][2];
    uint8_t color[4][4];
} RageRenderFace;

#define RAGE_CAPTURE_FACE_SEMI 0x01
#define RAGE_CAPTURE_FACE_RAW 0x02
#define RAGE_CAPTURE_FACE_FOGGED 0x04
#define RAGE_CAPTURE_MAX_DRAWS 2048
#define RAGE_CAPTURE_MAX_TERRAIN 16
#define RAGE_CAPTURE_MAX_PACKETS 32768
#define RAGE_CAPTURE_MAX_FACES 49152

typedef struct RageRenderScene {
    uint32_t frameCounter;
    int32_t sceneId;
    int32_t sceneTimer;
    int32_t displayHeight;
    int32_t displayPageY;
    int32_t courseMirror;
    RageRenderMatrix viewMatrix;
    int32_t viewPosition[3];
    int32_t drawCount, terrainCount, packetCount, faceCount;
    int32_t drawOverflow, packetOverflow, oversizedPackets, faceOverflow;
    int32_t skipped3DPackets;
    RageRenderModelDraw draws[RAGE_CAPTURE_MAX_DRAWS];
    RageRenderTerrainBatch terrain[RAGE_CAPTURE_MAX_TERRAIN];
    RageRenderUiPacket packets[RAGE_CAPTURE_MAX_PACKETS];
    RageRenderFace faces[RAGE_CAPTURE_MAX_FACES];
} RageRenderScene;

uint64_t RageRenderSceneHash(const RageRenderScene *scene);

/* Compatibility names keep call sites bisectable while modules migrate to
 * renderer-neutral vocabulary. */
typedef RageRenderMatrix RageCaptureMatrix;
typedef RageRenderTransformState RageCaptureGteState;
typedef RageRenderModelDraw RageCaptureModelDraw;
typedef RageRenderTerrainBatch RageCaptureTerrainBatch;
typedef RageRenderUiPacket RageCapturePacket;
typedef RageRenderFace RageCaptureFace;
typedef RageRenderScene RageSceneSnapshot;

#endif
