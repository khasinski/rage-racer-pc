#include <libgpu.h>
#include <libgte.h>
#include <psyz/gte.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/render_internal.h"
#include "game/race.h"
#include "game/state.h"

#include "scene_capture.h"
#include "modern_renderer.h"
#include "../runtime_config.h"

static RageSceneSnapshot s_snapshots[2];
static int s_current;
static int s_traceInitialized;
static FILE *s_trace;

/* Byte ranges of the primitive buffer written by 3D submissions this frame;
 * the end-of-frame ordering-table walk skips packets inside them. */
typedef struct RageCaptureRange {
    const uint8_t *begin;
    const uint8_t *end;
} RageCaptureRange;

#define RAGE_CAPTURE_MAX_RANGES 4096
static RageCaptureRange s_ranges[RAGE_CAPTURE_MAX_RANGES];
static int s_rangeCount;
static int s_rangeOverflow;
static const uint8_t *s_scopeStart;
static int s_scopeHasFaceOwner;

int CaptureActive(void) {
    if (!s_traceInitialized) {
        const char *trace = RuntimeConfigGet("diagnostics.scene_trace");
        s_traceInitialized = 1;
        if (trace != NULL && *trace != '\0' && strcmp(trace, "1") != 0) {
            s_trace = fopen(trace, "w");
            if (s_trace == NULL) {
                fprintf(stderr, "rage-port: cannot open scene trace %s\n",
                        trace);
            }
        } else if (trace != NULL) {
            s_trace = stderr;
        }
    }
    return ModernIsEnabled() || s_trace != NULL;
}

static void CaptureMatrixFromRegs(RageCaptureMatrix *out, unsigned base) {
    uint32_t r0 = Psyz_GteCtrlRead(base + 0);
    uint32_t r1 = Psyz_GteCtrlRead(base + 1);
    uint32_t r2 = Psyz_GteCtrlRead(base + 2);
    uint32_t r3 = Psyz_GteCtrlRead(base + 3);
    uint32_t r4 = Psyz_GteCtrlRead(base + 4);
    out->m[0][0] = (int16_t)(r0 & 0xffff);
    out->m[0][1] = (int16_t)(r0 >> 16);
    out->m[0][2] = (int16_t)(r1 & 0xffff);
    out->m[1][0] = (int16_t)(r1 >> 16);
    out->m[1][1] = (int16_t)(r2 & 0xffff);
    out->m[1][2] = (int16_t)(r2 >> 16);
    out->m[2][0] = (int16_t)(r3 & 0xffff);
    out->m[2][1] = (int16_t)(r3 >> 16);
    out->m[2][2] = (int16_t)(r4 & 0xffff);
    out->t[0] = (int32_t)Psyz_GteCtrlRead(base + 5);
    out->t[1] = (int32_t)Psyz_GteCtrlRead(base + 6);
    out->t[2] = (int32_t)Psyz_GteCtrlRead(base + 7);
}

static void CaptureGte(RageCaptureGteState *out) {
    CaptureMatrixFromRegs(&out->rot, 0);
    CaptureMatrixFromRegs(&out->light, 8);
    CaptureMatrixFromRegs(&out->color, 16);
    out->ofx = (int32_t)Psyz_GteCtrlRead(24) >> 16;
    out->ofy = (int32_t)Psyz_GteCtrlRead(25) >> 16;
    out->h = (int32_t)Psyz_GteCtrlRead(26);
    out->dqa = (int32_t)Psyz_GteCtrlRead(27);
    out->dqb = (int32_t)Psyz_GteCtrlRead(28);
}

static GameFrameContext *CaptureFrameContext(void) {
    return g_DrawBuffer;
}

/* Locate the active OT base within the frame's two tables so the showroom
 * +30-entry bias and the mirror pass's OT[1] both come through as data. */
static void CaptureOtBase(uint8_t *table, int32_t *bias) {
    GameFrameContext *frame = CaptureFrameContext();
    GameOrderingTableEntry *base = RENDER_OT_BASE;
    uintptr_t baseAddress = (uintptr_t)base;
    int t;
    *table = 0;
    *bias = 0;
    if (frame == NULL || base == NULL) return;
    for (t = 0; t < 2; t++) {
        GameOrderingTableEntry *start = frame->layout.orderingTables[t];
        uintptr_t startAddress = (uintptr_t)start;
        uintptr_t endAddress = (uintptr_t)(start + GAME_FRAME_OT_LENGTH);
        if (baseAddress >= startAddress && baseAddress < endAddress &&
            (baseAddress - startAddress) % sizeof(*start) == 0) {
            *table = (uint8_t)t;
            *bias = (int32_t)((baseAddress - startAddress) / sizeof(*start));
            return;
        }
    }
}

void CaptureFrameBegin(void) {
    RageSceneSnapshot *snapshot;
    if (!CaptureActive()) return;
    s_current ^= 1;
    snapshot = &s_snapshots[s_current];
    snapshot->drawCount = 0;
    snapshot->terrainCount = 0;
    snapshot->packetCount = 0;
    snapshot->faceCount = 0;
    snapshot->drawOverflow = 0;
    snapshot->packetOverflow = 0;
    snapshot->oversizedPackets = 0;
    snapshot->faceOverflow = 0;
    snapshot->skipped3DPackets = 0;
    s_rangeCount = 0;
    s_rangeOverflow = 0;
    s_scopeStart = NULL;
    s_scopeHasFaceOwner = 0;
}

void CaptureModelBegin(int kind, int index, int fogged) {
    RageSceneSnapshot *snapshot;
    RageCaptureModelDraw *draw;
    if (!CaptureActive()) return;
    snapshot = &s_snapshots[s_current];
    s_scopeStart = RENDER_PRIM_CURSOR_AS(uint8_t);
    s_scopeHasFaceOwner = 0;
    if (snapshot->drawCount >= RAGE_CAPTURE_MAX_DRAWS) {
        snapshot->drawOverflow++;
        return;
    }
    draw = &snapshot->draws[snapshot->drawCount++];
    s_scopeHasFaceOwner = 1;
    memset(draw, 0, sizeof(*draw));
    draw->kind = (uint8_t)kind;
    draw->mirror = g_RenderState.orderingFlag != 0;
    draw->fogged = (uint8_t)fogged;
    draw->otShift = (uint8_t)g_RenderState.otShift;
    draw->modelIndex = index;
    draw->renderMode = (uint32_t)g_RenderState.envMode4;
    draw->bankId = (uint64_t)(uintptr_t)(kind == RAGE_CAPTURE_KIND_MODEL
                                             ? g_RenderState.modelModels
                                             : g_RenderState.courseBank);
    CaptureOtBase(&draw->table, &draw->otBaseBias);
    CaptureGte(&draw->gte);
}

void CaptureTerrainBegin(const void *cells, int count) {
    RageSceneSnapshot *snapshot;
    RageCaptureTerrainBatch *batch;
    const int32_t *records = (const int32_t *)cells;
    int i;
    if (!CaptureActive()) return;
    snapshot = &s_snapshots[s_current];
    s_scopeStart = RENDER_PRIM_CURSOR_AS(uint8_t);
    s_scopeHasFaceOwner = 0;
    if (snapshot->terrainCount >= RAGE_CAPTURE_MAX_TERRAIN || count < 0 ||
        (count != 0 && cells == NULL)) return;
    batch = &snapshot->terrain[snapshot->terrainCount++];
    s_scopeHasFaceOwner = 1;
    memset(batch, 0, sizeof(*batch));
    batch->mirror = g_RenderState.orderingFlag != 0;
    batch->envMode4 = g_RenderState.envMode4 != 0;
    batch->otShift = (uint8_t)g_RenderState.otShift;
    if (count > RAGE_CAPTURE_MAX_CELLS) count = RAGE_CAPTURE_MAX_CELLS;
    batch->cellCount = (int16_t)count;
    for (i = 0; i < count * 4; i++) {
        batch->cells[i / 4][i % 4] = records[i];
    }
    CaptureGte(&batch->gte);
}

void CaptureFace3D(const RageCaptureFaceInput *input) {
    RageSceneSnapshot *snapshot;
    RageCaptureFace *face;
    int vertex;
    if (!CaptureActive() || input == NULL) return;
    snapshot = &s_snapshots[s_current];
    if (snapshot->faceCount >= RAGE_CAPTURE_MAX_FACES) {
        snapshot->faceOverflow++;
        return;
    }
    /* A face must belong to a recorded draw/batch; if the owner overflowed
     * the capture arrays, indexing terrain[-1]/draws[-1] would crash. */
    if (!s_scopeHasFaceOwner ||
        (input->kind == RAGE_CAPTURE_KIND_TERRAIN
            ? snapshot->terrainCount <= 0
            : snapshot->drawCount <= 0)) {
        snapshot->faceOverflow++;
        return;
    }
    if (input->kind < RAGE_CAPTURE_KIND_MODEL ||
        input->kind > RAGE_CAPTURE_KIND_TERRAIN ||
        input->klass < 0 || input->klass > 3 || input->colors == NULL ||
        (input->colorCount != 1 && input->colorCount != 4)) {
        snapshot->faceOverflow++;
        return;
    }
    if (input->kind == RAGE_CAPTURE_KIND_TERRAIN) {
        const RageCaptureTerrainBatch *batch =
            &snapshot->terrain[snapshot->terrainCount - 1];
        if (input->cellSlot < 0 || input->cellSlot >= batch->cellCount) {
            snapshot->faceOverflow++;
            return;
        }
    }
    for (vertex = 0; vertex < 4; vertex++) {
        if (input->v[vertex] == NULL) {
            snapshot->faceOverflow++;
            return;
        }
    }
    face = &snapshot->faces[snapshot->faceCount++];
    memset(face, 0, sizeof(*face));
    face->kind = (uint8_t)input->kind;
    face->klass = (uint8_t)input->klass;
    face->flags = (uint8_t)((input->semi ? RAGE_CAPTURE_FACE_SEMI : 0) |
                            (input->raw ? RAGE_CAPTURE_FACE_RAW : 0) |
                            (input->fogged ? RAGE_CAPTURE_FACE_FOGGED : 0));
    face->bias = (int8_t)input->bias;
    face->drawIndex = (int16_t)(input->kind == RAGE_CAPTURE_KIND_TERRAIN
                                    ? snapshot->terrainCount - 1
                                    : snapshot->drawCount - 1);
    face->cellSlot = (int16_t)input->cellSlot;
    face->otDepth = input->otDepth;
    face->fog = input->fog;
    face->clut = input->clut;
    face->tpage = input->tpage;
    face->textureWindow = input->textureWindow;
    for (vertex = 0; vertex < 4; vertex++) {
        const SVECTOR *v = (const SVECTOR *)input->v[vertex];
        const uint8_t *color = input->colorCount == 4
                                   ? input->colors + vertex * 4
                                   : input->colors;
        face->pos[vertex][0] = v->vx;
        face->pos[vertex][1] = v->vy;
        face->pos[vertex][2] = v->vz;
        if (input->uv != NULL) {
            face->uv[vertex][0] = input->uv[vertex * 2];
            face->uv[vertex][1] = input->uv[vertex * 2 + 1];
        }
        face->color[vertex][0] = color[0];
        face->color[vertex][1] = color[1];
        face->color[vertex][2] = color[2];
    }
}

void CaptureSubmitEnd(void) {
    const uint8_t *end;
    if (!CaptureActive() || s_scopeStart == NULL) return;
    end = RENDER_PRIM_CURSOR_AS(uint8_t);
    if (end > s_scopeStart) {
        if (s_rangeCount < RAGE_CAPTURE_MAX_RANGES) {
            s_ranges[s_rangeCount].begin = s_scopeStart;
            s_ranges[s_rangeCount].end = end;
            s_rangeCount++;
        } else {
            s_rangeOverflow++;
        }
    }
    s_scopeStart = NULL;
    s_scopeHasFaceOwner = 0;
}

static int CaptureIs3DPacket(const uint8_t *address) {
    uintptr_t candidate = (uintptr_t)address;
    int i;
    for (i = 0; i < s_rangeCount; i++) {
        if (candidate >= (uintptr_t)s_ranges[i].begin &&
            candidate < (uintptr_t)s_ranges[i].end) {
            return 1;
        }
    }
    return 0;
}

/* Fields the PS1 GPU ignores can carry uninitialized bytes (the FMV player
 * leaves stack garbage in the colour of raw-textured quads and in the unused
 * high halfwords of their trailing UV words). Zero them so captured packets
 * compare equal whenever they render equal. */
static void CaptureMaskIgnoredFields(uint32_t *words, int length) {
    unsigned command = words[0] >> 24;
    int isPoly = (command & 0xE0) == 0x20;
    int isLine = (command & 0xE0) == 0x40;
    int isRect = (command & 0xE0) == 0x60;
    int textured = (command & 0x04) != 0;
    int gouraud = (command & 0x10) != 0;
    int quad = (command & 0x08) != 0;
    int raw = (command & 0x01) != 0;
    if (isRect) {
        /* Raw-textured sprites ignore their colour word. */
        if (textured && raw) words[0] &= 0xFF000000u;
        return;
    }
    if (isLine) {
        /* LINE_G2's second colour word carries an unused top byte. */
        if (gouraud && length == 4) words[2] &= 0x00FFFFFFu;
        return;
    }
    if (!isPoly) return;
    if (gouraud) {
        /* Secondary shading-colour words only use their low 24 bits. */
        static const int flat3[] = {2, 4}, flat4[] = {2, 4, 6};
        static const int tex3[] = {3, 6}, tex4[] = {3, 6, 9};
        const int *slots = textured ? (quad ? tex4 : tex3)
                                    : (quad ? flat4 : flat3);
        int count = (quad ? 3 : 2);
        int i;
        for (i = 0; i < count; i++) {
            if (slots[i] < length) words[slots[i]] &= 0x00FFFFFFu;
        }
    }
    if (!textured) return;
    if (raw && !gouraud) words[0] &= 0xFF000000u;
    if (!gouraud) {
        if (length > 6) words[6] &= 0xFFFFu;
        if (quad && length > 8) words[8] &= 0xFFFFu;
    } else {
        if (length > 8) words[8] &= 0xFFFFu;
        if (quad && length > 11) words[11] &= 0xFFFFu;
    }
}

/* Mirror of PsyZ's CanonicalizePacket: drawing primitives keep their payload
 * as dense 32-bit words after O_TAG; environment packets use u_long slots. */
static int CapturePacketWords(const DR_ENV *packet, uint32_t *out, int cap) {
    int length = getlen(packet);
    int code;
    int i;
    if (length <= 0 || length > cap) return length;
    code = getcode(packet) & ~3;
    if (code >= 0x20 && code < 0x80) {
        const u32 *packed = (const u32 *)packet->code;
        for (i = 0; i < length; i++) out[i] = packed[i];
    } else {
        for (i = 0; i < length; i++) {
            out[i] = (uint32_t)(packet->code[i] & 0xFFFFFFFFu);
        }
    }
    CaptureMaskIgnoredFields(out, length);
    return length;
}

static void CaptureWalkTable(RageSceneSnapshot *snapshot, int tableIndex,
                             GameOrderingTableEntry *table) {
    const DR_ENV *node = (const DR_ENV *)&table[GAME_FRAME_OT_LENGTH - 1];
    int bucket = GAME_FRAME_OT_LENGTH - 1;
    while (1) {
        const uint8_t *address = (const uint8_t *)node;
        const uint8_t *tableBegin = (const uint8_t *)table;
        const uint8_t *tableEnd =
            (const uint8_t *)&table[GAME_FRAME_OT_LENGTH];
        uintptr_t addressValue = (uintptr_t)address;
        /* Mirror PsyZ's GPU_Enqueue defenses: a game/pause path can link a
         * packet with a garbage or misaligned tag; the compat renderer
         * truncates the chain gracefully there, so the capture must too. */
        if (addressValue < 4096 ||
            (addressValue & (sizeof(uint32_t) - 1)) != 0) {
            snapshot->oversizedPackets++;
            break;
        }
        if (addressValue >= (uintptr_t)tableBegin &&
            addressValue < (uintptr_t)tableEnd) {
            bucket = (int)((addressValue - (uintptr_t)tableBegin) /
                           sizeof(*table));
        } else if (CaptureIs3DPacket(address)) {
            snapshot->skipped3DPackets++;
        } else {
            int length = getlen(node);
            if (length > 0x100) {
                snapshot->oversizedPackets++;
                break;
            }
            if (length > RAGE_CAPTURE_PACKET_WORDS) {
                snapshot->oversizedPackets++;
            } else if (length > 0) {
                if (snapshot->packetCount >= RAGE_CAPTURE_MAX_PACKETS) {
                    snapshot->packetOverflow++;
                } else {
                    RageCapturePacket *out =
                        &snapshot->packets[snapshot->packetCount++];
                    memset(out, 0, sizeof(*out));
                    out->bucket = (uint16_t)bucket;
                    out->table = (uint8_t)tableIndex;
                    out->size = (uint8_t)CapturePacketWords(
                        node, out->words, RAGE_CAPTURE_PACKET_WORDS);
                }
            }
        }
        if (isendprim(node)) break;
        node = (const DR_ENV *)nextPrim(node);
    }
}

void CaptureFrameEnd(void) {
    RageSceneSnapshot *snapshot;
    GameFrameContext *frame;
    const Matrix *view;
    if (!CaptureActive()) return;
    snapshot = &s_snapshots[s_current];
    snapshot->frameCounter = (uint32_t)g_FrameCounter;
    snapshot->sceneId = g_SceneId;
    snapshot->courseMirror = g_MirrorMode != 0;
    snapshot->sceneTimer = g_SceneTimer;
    view = (&g_RenderState.matrix);
    memcpy(snapshot->viewMatrix.m, view->m, sizeof(snapshot->viewMatrix.m));
    snapshot->viewMatrix.t[0] = view->t[0];
    snapshot->viewMatrix.t[1] = view->t[1];
    snapshot->viewMatrix.t[2] = view->t[2];
    snapshot->viewPosition[0] = g_RenderState.viewX;
    snapshot->viewPosition[1] = g_RenderState.viewY;
    snapshot->viewPosition[2] = g_RenderState.viewZ;
    frame = CaptureFrameContext();
    if (frame != NULL) {
        snapshot->displayHeight = frame->layout.environment.draw.clip.h;
        snapshot->displayPageY = frame->layout.environment.draw.clip.y;
        CaptureWalkTable(snapshot, 0, frame->layout.orderingTables[0]);
        CaptureWalkTable(snapshot, 1, frame->layout.orderingTables[1]);
    }
    if (s_trace != NULL) {
        int cells = 0;
        int i;
        for (i = 0; i < snapshot->terrainCount; i++) {
            cells += snapshot->terrain[i].cellCount;
        }
        fprintf(s_trace,
                "scene-frame frame=%u scene=%d timer=%d draws=%d terrain=%d "
                "cells=%d packets=%d faces=%d skipped3d=%d "
                "overflow=%d,%d,%d,%d,%d hash=%016llx\n",
                snapshot->frameCounter, snapshot->sceneId,
                snapshot->sceneTimer, snapshot->drawCount,
                snapshot->terrainCount, cells, snapshot->packetCount,
                snapshot->faceCount, snapshot->skipped3DPackets,
                snapshot->drawOverflow, snapshot->packetOverflow,
                snapshot->oversizedPackets, snapshot->faceOverflow,
                s_rangeOverflow,
                (unsigned long long)CaptureSnapshotHash(snapshot));
        if (RuntimeConfigEnabled("diagnostics.scene_trace_verbose")) {
            for (i = 0; i < snapshot->packetCount; i++) {
                const RageCapturePacket *packet = &snapshot->packets[i];
                int word;
                fprintf(s_trace, "scene-packet frame=%u index=%d table=%d "
                        "bucket=%d words=", snapshot->frameCounter, i,
                        packet->table, packet->bucket);
                for (word = 0; word < packet->size; word++) {
                    fprintf(s_trace, "%s%08x", word ? "," : "",
                            packet->words[word]);
                }
                fputc('\n', s_trace);
            }
        }
        fflush(s_trace);
    }
}

const RageSceneSnapshot *CaptureCurrent(void) {
    return &s_snapshots[s_current];
}

const RageSceneSnapshot *CapturePrevious(void) {
    return &s_snapshots[s_current ^ 1];
}

static uint64_t HashBytes(uint64_t hash, const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t i;
    for (i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001B3ull;
    }
    return hash;
}

uint64_t CaptureSnapshotHash(const RageSceneSnapshot *snapshot) {
    uint64_t hash = 0xCBF29CE484222325ull;
    int i;
    hash = HashBytes(hash, &snapshot->sceneId, sizeof(snapshot->sceneId));
    hash = HashBytes(hash, &snapshot->sceneTimer,
                     sizeof(snapshot->sceneTimer));
    hash = HashBytes(hash, &snapshot->viewMatrix,
                     sizeof(snapshot->viewMatrix));
    hash = HashBytes(hash, snapshot->viewPosition,
                     sizeof(snapshot->viewPosition));
    for (i = 0; i < snapshot->drawCount; i++) {
        /* bankId is a live pointer, so it changes with ASLR; keep the hash
         * comparable between independent runs. */
        RageCaptureModelDraw draw = snapshot->draws[i];
        draw.bankId = 0;
        hash = HashBytes(hash, &draw, sizeof(draw));
    }
    for (i = 0; i < snapshot->terrainCount; i++) {
        hash = HashBytes(hash, &snapshot->terrain[i],
                         sizeof(snapshot->terrain[i]));
    }
    for (i = 0; i < snapshot->packetCount; i++) {
        hash = HashBytes(hash, &snapshot->packets[i],
                         sizeof(snapshot->packets[i]));
    }
    for (i = 0; i < snapshot->faceCount; i++) {
        hash = HashBytes(hash, &snapshot->faces[i],
                         sizeof(snapshot->faces[i]));
    }
    return hash;
}
