#include "classic_motion.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CLASSIC_HASH_SLOTS (RAGE_CAPTURE_MAX_PACKETS * 2)
static uint64_t s_keys[CLASSIC_HASH_SLOTS];
static int s_slots[CLASSIC_HASH_SLOTS];
static int s_matches[RAGE_CAPTURE_MAX_PACKETS];
static int s_matchOwners[RAGE_CAPTURE_MAX_PACKETS];
static int s_drawMap[RAGE_CAPTURE_MAX_DRAWS];
static int s_reverseDrawMap[RAGE_CAPTURE_MAX_DRAWS];
static const RageSceneSnapshot *s_previous, *s_current;
static uint32_t s_previousFrame, s_currentFrame;
static int s_matchCount;

/* A partially matched surface must never mix two geometry times. Weld the
 * previous packet stream at shared projected vertices, then hold an entire
 * connected surface if a child cannot be matched or a shared vertex would
 * receive conflicting motion. This also connects differently subdivided
 * neighbours through their common edge endpoints. */
#define CLASSIC_VERTEX_SLOTS (RAGE_CAPTURE_MAX_PACKETS * 8)
static uint64_t s_vertexKeys[CLASSIC_VERTEX_SLOTS];
static int s_vertexOwners[CLASSIC_VERTEX_SLOTS];
static int16_t s_vertexMotion[CLASSIC_VERTEX_SLOTS][2];
static int s_component[RAGE_CAPTURE_MAX_PACKETS];
static uint8_t s_componentHeld[RAGE_CAPTURE_MAX_PACKETS];

static int ComponentRoot(int packet) {
    while (s_component[packet] != packet) {
        s_component[packet] = s_component[s_component[packet]];
        packet = s_component[packet];
    }
    return packet;
}

void ClassicMotionReset(void) {
    s_previous = s_current = NULL;
    s_matchCount = 0;
}

static int PolygonCoordinates(const RageCapturePacket *p, int xy[4][2]) {
    unsigned command = p->words[0] >> 24;
    if ((command & 0xe0) != 0x20) return 0;
    int count = command & 8 ? 4 : 3, cursor = 1;
    for (int v = 0; v < count; ++v) {
        if (v && (command & 16)) ++cursor;
        if (cursor >= p->size) return 0;
        uint32_t word = p->words[cursor++];
        xy[v][0] = (int)((word & 2047) ^ 1024) - 1024;
        xy[v][1] = (int)(((word >> 16) & 2047) ^ 1024) - 1024;
        if (command & 4) {
            if (cursor >= p->size) return 0;
            ++cursor;
        }
    }
    return count;
}

static uint64_t Hash(uint64_t h, const void *data, size_t size) {
    const uint8_t *bytes = data;
    for (size_t i = 0; i < size; ++i) h = (h ^ bytes[i]) * UINT64_C(1099511628211);
    return h;
}

/* Compare instances in world space, so camera motion cannot pair one car
 * with the car behind it. Mutual nearest matches reject entering/leaving
 * instances and ambiguous repeated models. */
static void DrawOrigin(const RageSceneSnapshot *s, int draw, double p[3]) {
    const RageCaptureMatrix *m = &s->draws[draw].gte.rot;
    for (int axis = 0; axis < 3; ++axis) {
        p[axis] = s->viewPosition[axis];
        for (int row = 0; row < 3; ++row)
            p[axis] += (double)s->viewMatrix.m[row][axis] * m->t[row] / 16384.0;
    }
}

static int NearestDraw(const RageSceneSnapshot *a, int i,
                       const RageSceneSnapshot *b) {
    const RageCaptureModelDraw *draw = &a->draws[i];
    double p[3], best = DBL_MAX, second = DBL_MAX;
    int match = -1;
    DrawOrigin(a, i, p);
    for (int j = 0; j < b->drawCount; ++j) {
        const RageCaptureModelDraw *candidate = &b->draws[j];
        if (draw->kind != candidate->kind || draw->modelIndex != candidate->modelIndex ||
            draw->bankId != candidate->bankId || draw->mirror != candidate->mirror ||
            draw->table != candidate->table || draw->gte.h != candidate->gte.h) continue;
        double q[3], distance = 0;
        DrawOrigin(b, j, q);
        for (int axis = 0; axis < 3; ++axis) {
            double delta = p[axis] - q[axis];
            distance += delta * delta;
        }
        if (distance < best) { second = best; best = distance; match = j; }
        else if (distance < second) second = distance;
    }
    /* A generous one-tick movement bound, but no ties or near-ties. */
    return best <= 4096.0 * 4096.0 && second > best * 4.0 + 1.0 ? match : -1;
}

static uint64_t PacketKey(const RageSceneSnapshot *s,
                          const RageClassicPacketSource *sources, int index,
                          int previous) {
    const RageCapturePacket *p = &s->packets[index];
    const RageClassicPacketSource *source = &sources[index];
    if (!(p->flags & RAGE_CAPTURE_PACKET_3D) || source->faceIndex < 0 ||
        source->faceIndex >= s->faceCount || source->bytes == 0) return 0;
    const RageCaptureFace *face = &s->faces[source->faceIndex];
    uint32_t owner;
    if (face->kind == RAGE_CAPTURE_KIND_TERRAIN) {
        if (face->drawIndex < 0 || face->drawIndex >= s->terrainCount) return 0;
        const RageCaptureTerrainBatch *batch = &s->terrain[face->drawIndex];
        if (face->cellSlot < 0 || face->cellSlot >= batch->cellCount) return 0;
        owner = UINT32_C(0x80000000) | ((uint32_t)batch->mirror << 24) |
                (uint32_t)batch->cells[face->cellSlot][3];
    } else {
        if (face->drawIndex < 0 || face->drawIndex >= s->drawCount) return 0;
        int draw = previous ? s_drawMap[face->drawIndex] : face->drawIndex;
        if (draw < 0) return 0;
        owner = (uint32_t)draw;
    }
    uint64_t h = Hash(UINT64_C(14695981039346656037), &owner, sizeof(owner));
    h = Hash(h, &face->kind, sizeof(face->kind));
    h = Hash(h, face->pos, sizeof(face->pos));
    h = Hash(h, &source->offset, sizeof(source->offset));
    h = Hash(h, &source->bytes, sizeof(source->bytes));
    h = Hash(h, &p->table, sizeof(p->table));
    unsigned command = p->words[0] >> 24;
    if ((command & 0xe0) != 0x20) return 0;
    h = Hash(h, &command, sizeof(command));
    int count = command & 8 ? 4 : 3, cursor = 1;
    for (int v = 0; v < count; ++v) {
        if (v && (command & 16)) ++cursor;
        if (cursor >= p->size) return 0;
        ++cursor;
        if (command & 4) {
            if (cursor >= p->size) return 0;
            /* Includes UVs and material: a different subdivision child or
             * texture animation holds for one tick instead of morphing. */
            h = Hash(h, &p->words[cursor++], sizeof(uint32_t));
        }
    }
    return h ? h : 1;
}

static unsigned FindSlot(uint64_t key) {
    unsigned slot = (unsigned)(key ^ (key >> 32)) & (CLASSIC_HASH_SLOTS - 1);
    while (s_keys[slot] && s_keys[slot] != key)
        slot = (slot + 1) & (CLASSIC_HASH_SLOTS - 1);
    return slot;
}

static void KeepConnectedSurfacesCoherent(const RageClassicPacketSource *sources) {
    memset(s_vertexKeys, 0, sizeof(s_vertexKeys));
    for (int i = 0; i < s_previous->packetCount; ++i) {
        s_component[i] = i;
        s_componentHeld[i] = s_matches[i] < 0;
    }
    for (int i = 0; i < s_previous->packetCount; ++i) {
        const RageCapturePacket *packet = &s_previous->packets[i];
        int xy[4][2], target[4][2];
        if (!(packet->flags & RAGE_CAPTURE_PACKET_3D)) continue;
        int count = PolygonCoordinates(packet, xy);
        if (!count) continue;
        uint32_t domain = 0;
        int faceIndex = sources[i].faceIndex;
        if (faceIndex >= 0 && faceIndex < s_previous->faceCount) {
            const RageCaptureFace *face = &s_previous->faces[faceIndex];
            /* Separate model instances; course/terrain share world space. */
            if (face->kind == RAGE_CAPTURE_KIND_MODEL && face->drawIndex >= 0)
                domain = (uint32_t)face->drawIndex + 1;
        }
        domain = domain * 2u + (packet->table != 0);
        int matched = s_matches[i] >= 0;
        if (matched) PolygonCoordinates(&s_current->packets[s_matches[i]], target);
        for (int v = 0; v < count; ++v) {
            uint64_t key = (((uint64_t)domain << 22) |
                ((uint32_t)(xy[v][0] + 1024) << 11) |
                (uint32_t)(xy[v][1] + 1024)) + 1;
            unsigned slot = (unsigned)((key ^ (key >> 32)) * UINT64_C(11400714819323198485))
                            & (CLASSIC_VERTEX_SLOTS - 1);
            while (s_vertexKeys[slot] && s_vertexKeys[slot] != key)
                slot = (slot + 1) & (CLASSIC_VERTEX_SLOTS - 1);
            int dx = matched ? target[v][0] - xy[v][0] : 0;
            int dy = matched ? target[v][1] - xy[v][1] : 0;
            if (!s_vertexKeys[slot]) {
                s_vertexKeys[slot] = key;
                s_vertexOwners[slot] = i;
                s_vertexMotion[slot][0] = (int16_t)dx;
                s_vertexMotion[slot][1] = (int16_t)dy;
            } else {
                int root = ComponentRoot(i);
                int other = ComponentRoot(s_vertexOwners[slot]);
                s_componentHeld[other] |= s_componentHeld[root] ||
                    dx != s_vertexMotion[slot][0] || dy != s_vertexMotion[slot][1];
                s_component[root] = other;
            }
        }
    }
    for (int i = 0; i < s_previous->packetCount; ++i) {
        if (s_matches[i] >= 0 && s_componentHeld[ComponentRoot(i)]) {
            s_matches[i] = -1;
            --s_matchCount;
        }
    }
}

void ClassicMotionPrepare(const RageSceneSnapshot *previous,
                          const RageClassicPacketSource *previousSources,
                          const RageSceneSnapshot *current,
                          const RageClassicPacketSource *currentSources) {
    if (previous == s_previous && current == s_current &&
        previous && current && previous->frameCounter == s_previousFrame &&
        current->frameCounter == s_currentFrame) return;
    ClassicMotionReset();
    memset(s_matches, 0xff, sizeof(s_matches));
    if (!previous || !current || !previousSources || !currentSources) return;
    s_previous = previous; s_current = current;
    s_previousFrame = previous->frameCounter; s_currentFrame = current->frameCounter;
    if (current->frameCounter - previous->frameCounter != 1 ||
        current->sceneId != previous->sceneId || current->sceneTimer <= previous->sceneTimer ||
        current->courseMirror != previous->courseMirror ||
        current->displayHeight != previous->displayHeight ||
        previous->packetOverflow || current->packetOverflow ||
        previous->faceOverflow || current->faceOverflow) return;
    for (int axis = 0; axis < 3; ++axis)
        if (fabs((double)current->viewPosition[axis] - previous->viewPosition[axis]) > 8192) return;
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            if (abs(current->viewMatrix.m[row][col] - previous->viewMatrix.m[row][col]) > 2048) return;
    for (int i = 0; i < previous->drawCount; ++i)
        s_drawMap[i] = NearestDraw(previous, i, current);
    for (int i = 0; i < current->drawCount; ++i)
        s_reverseDrawMap[i] = NearestDraw(current, i, previous);
    for (int i = 0; i < previous->drawCount; ++i)
        if (s_drawMap[i] >= 0 && s_reverseDrawMap[s_drawMap[i]] != i) s_drawMap[i] = -1;
    memset(s_keys, 0, sizeof(s_keys));
    memset(s_matchOwners, 0xff, sizeof(s_matchOwners));
    for (int i = 0; i < current->packetCount; ++i) {
        uint64_t key = PacketKey(current, currentSources, i, 0);
        if (!key) continue;
        unsigned slot = FindSlot(key);
        s_slots[slot] = s_keys[slot] ? -1 : i;
        s_keys[slot] = key;
    }
    for (int i = 0; i < previous->packetCount; ++i) {
        uint64_t key = PacketKey(previous, previousSources, i, 1);
        if (!key) continue;
        unsigned slot = FindSlot(key);
        if (s_keys[slot] != key || s_slots[slot] < 0) continue;
        int xy[4][2], target[4][2], match = s_slots[slot];
        int count = PolygonCoordinates(&previous->packets[i], xy);
        if (!count || PolygonCoordinates(&current->packets[match], target) != count) continue;
        int safe = 1;
        for (int v = 0; v < count; ++v)
            for (int axis = 0; axis < 2; ++axis)
                if (abs(target[v][axis] - xy[v][axis]) > 160 ||
                    abs(xy[v][axis]) >= 1000 || abs(target[v][axis]) >= 1000) safe = 0;
        if (safe) {
            if (s_matchOwners[match] == -1) {
                s_matches[i] = match;
                s_matchOwners[match] = i;
                ++s_matchCount;
            } else if (s_matchOwners[match] >= 0) {
                s_matches[s_matchOwners[match]] = -1;
                s_matchOwners[match] = -2;
                --s_matchCount;
            }
        }
    }
    KeepConnectedSurfacesCoherent(previousSources);
}

int ClassicMotionCoordinates(int packetIndex, float fraction,
                             float x[4], float y[4]) {
    if (!s_previous || !s_current || packetIndex < 0 ||
        packetIndex >= s_previous->packetCount || s_matches[packetIndex] < 0 ||
        !isfinite(fraction)) return 0;
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;
    int xy[4][2], target[4][2];
    int count = PolygonCoordinates(&s_previous->packets[packetIndex], xy);
    if (PolygonCoordinates(&s_current->packets[s_matches[packetIndex]], target) != count) return 0;
    for (int v = 0; v < count; ++v) {
        x[v] += fraction * (target[v][0] - xy[v][0]);
        y[v] += fraction * (target[v][1] - xy[v][1]);
    }
    return 1;
}

int ClassicMotionMatchCount(void) { return s_matchCount; }
