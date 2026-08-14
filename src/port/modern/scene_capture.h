#ifndef RAGE_SCENE_CAPTURE_H
#define RAGE_SCENE_CAPTURE_H

/* Semantic scene capture for the modern renderer (phase R1 of
 * docs/modern_renderer_plan.md). The compat submission path always runs;
 * this module records what it submits, without feeding anything back into
 * game state. Records are written into a double-buffered RageSceneSnapshot
 * so a later phase can interpolate between two logic frames.
 *
 * 3D submissions (models, course models, terrain) are recorded semantically
 * at the native_geometry entry points, together with the GTE state in effect.
 * Everything else linked into the frame ordering tables (sky, HUD, text,
 * menus) is captured packet-by-packet at end of frame by walking the same
 * chains DrawOTag consumes, skipping the byte ranges the 3D submissions
 * emitted. */

#include <stdint.h>

enum {
    RAGE_CAPTURE_KIND_MODEL = 0,
    RAGE_CAPTURE_KIND_COURSE = 1,
};

typedef struct RageCaptureMatrix {
    int16_t m[3][3]; /* 4.12 fixed point */
    int32_t t[3];
} RageCaptureMatrix;

typedef struct RageCaptureGteState {
    RageCaptureMatrix rot;   /* composed rotation + translation (RT/TR) */
    RageCaptureMatrix light; /* light matrix + background colour */
    RageCaptureMatrix color; /* colour matrix + far colour */
    int32_t ofx, ofy;        /* screen offset, pixels */
    int32_t h;               /* projection distance (SetGeomScreen) */
    int32_t dqa, dqb;        /* depth-cue coefficients */
} RageCaptureGteState;

typedef struct RageCaptureModelDraw {
    uint8_t kind;       /* RAGE_CAPTURE_KIND_* */
    uint8_t mirror;     /* SCRATCH_MIRROR at submission time */
    uint8_t table;      /* ordering table index the draw targets (0/1) */
    uint8_t fogged;
    int32_t modelIndex;
    int32_t otBaseBias; /* SCRATCH_OT_BASE offset from the table start */
    uint32_t renderMode; /* g_ScratchRenderMode (CLUT row select) */
    uint64_t bankId;     /* identity of the active model/course bank */
    RageCaptureGteState gte;
} RageCaptureModelDraw;

#define RAGE_CAPTURE_MAX_CELLS 64

typedef struct RageCaptureTerrainBatch {
    uint8_t mirror;
    uint8_t envMode4;
    int16_t cellCount;
    /* Visible-cell records: translated x, y, z and the cell index. */
    int32_t cells[RAGE_CAPTURE_MAX_CELLS][4];
    RageCaptureGteState gte;
} RageCaptureTerrainBatch;

#define RAGE_CAPTURE_PACKET_WORDS 16

typedef struct RageCapturePacket {
    uint16_t bucket; /* ordering-table entry the packet is linked under */
    uint8_t table;   /* 0 = main, 1 = mirror/overlay */
    uint8_t size;    /* canonical GP0 payload length in words */
    uint32_t words[RAGE_CAPTURE_PACKET_WORDS];
} RageCapturePacket;

#define RAGE_CAPTURE_MAX_DRAWS 2048
#define RAGE_CAPTURE_MAX_TERRAIN 8
#define RAGE_CAPTURE_MAX_PACKETS 32768

typedef struct RageSceneSnapshot {
    uint32_t frameCounter;
    int32_t sceneId;
    int32_t sceneTimer;
    RageCaptureMatrix viewMatrix; /* SCRATCH_VIEW_MATRIX_GTE at frame end */
    int32_t viewPosition[3];
    int32_t drawCount, terrainCount, packetCount;
    int32_t drawOverflow, packetOverflow, oversizedPackets;
    int32_t skipped3DPackets;
    RageCaptureModelDraw draws[RAGE_CAPTURE_MAX_DRAWS];
    RageCaptureTerrainBatch terrain[RAGE_CAPTURE_MAX_TERRAIN];
    RageCapturePacket packets[RAGE_CAPTURE_MAX_PACKETS];
} RageSceneSnapshot;

/* Capture runs when the modern renderer is enabled or RAGE_PORT_SCENE_TRACE
 * is set. Checked lazily on the first frame. */
int RageCaptureActive(void);

/* Frame hooks, called from the port scene-handler hooks in the main loop. */
void RageCaptureFrameBegin(void);
void RageCaptureFrameEnd(void);

/* Submission hooks, called by native_geometry around each 3D submission. */
void RageCaptureModelBegin(int kind, int index, int fogged);
void RageCaptureTerrainBegin(const void *cells, int count);
void RageCaptureSubmitEnd(void);

/* The snapshot of the frame captured last / the one before it. */
const RageSceneSnapshot *RageCaptureCurrent(void);
const RageSceneSnapshot *RageCapturePrevious(void);

uint64_t RageCaptureSnapshotHash(const RageSceneSnapshot *snapshot);

#endif
