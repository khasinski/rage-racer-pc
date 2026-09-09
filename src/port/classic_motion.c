#include "classic_motion.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CLASSIC_HASH_SLOTS (RAGE_CAPTURE_MAX_PACKETS * 2)
/* Open-addressed slot calculation below uses a power-of-two table. */
#define CLASSIC_FACE_HASH_SLOTS 131072
static uint64_t s_keys[CLASSIC_HASH_SLOTS];
static int s_slots[CLASSIC_HASH_SLOTS];
static uint64_t s_faceKeys[CLASSIC_FACE_HASH_SLOTS];
static int s_faceSlots[CLASSIC_FACE_HASH_SLOTS];
static int s_matches[RAGE_CAPTURE_MAX_PACKETS];
static int s_packetFaces[RAGE_CAPTURE_MAX_PACKETS];
static uint8_t s_packetFaceMotion[RAGE_CAPTURE_MAX_PACKETS];
static uint8_t s_packetHeld[RAGE_CAPTURE_MAX_PACKETS];
static int s_faceMatches[RAGE_CAPTURE_MAX_FACES];
static uint8_t s_faceProjectable[RAGE_CAPTURE_MAX_FACES];
static int s_matchOwners[RAGE_CAPTURE_MAX_PACKETS];
static int s_drawMap[RAGE_CAPTURE_MAX_DRAWS];
static int s_reverseDrawMap[RAGE_CAPTURE_MAX_DRAWS];
static const RageSceneSnapshot *s_previous, *s_current;
static uint32_t s_previousFrame, s_currentFrame;
static int s_matchCount;
static int s_faceMotionCount;
static ClassicMotionStats s_stats;

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

static int PacketHasMotion(int packet) {
    return s_matches[packet] >= 0 ||
        s_packetFaceMotion[packet];
}

void ClassicMotionReset(void) {
    s_previous = s_current = NULL;
    s_matchCount = 0;
    s_faceMotionCount = 0;
    memset(&s_stats, 0, sizeof(s_stats));
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

/* A subdivision child has no stable allocation offset when the renderer
 * changes how it splits its parent quad. The face itself does: identify it in
 * world space, then use its exact captured parent projections to move the old child.
 * This keeps a complete old surface coherent until the next logic frame
 * replaces it, rather than freezing its neighbours at 25/30 Hz. */
static uint64_t FaceKey(const RageSceneSnapshot *s, int index, int previous) {
    if (index < 0 || index >= s->faceCount) return 0;
    const RageCaptureFace *face = &s->faces[index];
    uint32_t owner;
    if (face->kind == RAGE_CAPTURE_KIND_TERRAIN) {
        if (face->drawIndex < 0 || face->drawIndex >= s->terrainCount ||
            face->cellSlot < 0 || face->cellSlot >= s->terrain[face->drawIndex].cellCount)
            return 0;
        owner = UINT32_C(0x80000000) |
            ((uint32_t)s->terrain[face->drawIndex].mirror << 24) |
            (uint32_t)s->terrain[face->drawIndex].cells[face->cellSlot][3];
    } else {
        if (face->drawIndex < 0 || face->drawIndex >= s->drawCount) return 0;
        int draw = previous ? s_drawMap[face->drawIndex] : face->drawIndex;
        if (draw < 0) return 0;
        owner = (uint32_t)draw;
    }
    uint64_t h = Hash(UINT64_C(14695981039346656037), &owner, sizeof(owner));
    h = Hash(h, &face->kind, sizeof(face->kind));
    h = Hash(h, &face->klass, sizeof(face->klass));
    h = Hash(h, face->pos, sizeof(face->pos));
    return h ? h : 1;
}

static unsigned FindSlot(uint64_t key) {
    unsigned slot = (unsigned)(key ^ (key >> 32)) & (CLASSIC_HASH_SLOTS - 1);
    while (s_keys[slot] && s_keys[slot] != key)
        slot = (slot + 1) & (CLASSIC_HASH_SLOTS - 1);
    return slot;
}

static unsigned FindFaceSlot(uint64_t key) {
    unsigned slot = (unsigned)(key ^ (key >> 32)) & (CLASSIC_FACE_HASH_SLOTS - 1);
    while (s_faceKeys[slot] && s_faceKeys[slot] != key)
        slot = (slot + 1) & (CLASSIC_FACE_HASH_SLOTS - 1);
    return slot;
}

static int Barycentric(const float triangle[3][2], float x, float y,
                       float weight[3]) {
    float ax = triangle[1][0] - triangle[0][0];
    float ay = triangle[1][1] - triangle[0][1];
    float bx = triangle[2][0] - triangle[0][0];
    float by = triangle[2][1] - triangle[0][1];
    float determinant = ax * by - ay * bx;
    if (fabsf(determinant) < 0.001f) return 0;
    float px = x - triangle[0][0], py = y - triangle[0][1];
    weight[1] = (px * by - py * bx) / determinant;
    weight[2] = (ax * py - ay * px) / determinant;
    weight[0] = 1.0f - weight[1] - weight[2];
    return weight[0] >= -0.01f && weight[1] >= -0.01f && weight[2] >= -0.01f;
}

static int FaceMotionCoordinates(int faceIndex, float fraction,
                                 float x[4], float y[4]) {
    if (!s_previous || !s_current || faceIndex < 0 ||
        faceIndex >= s_previous->faceCount) return 0;
    int currentFace = s_faceMatches[faceIndex];
    if (currentFace < 0 || !s_faceProjectable[faceIndex]) return 0;
    float from[4][2], to[4][2];
    for (int vertex = 0; vertex < 4; ++vertex) {
        from[vertex][0] = s_previous->faces[faceIndex].screen[vertex][0];
        from[vertex][1] = s_previous->faces[faceIndex].screen[vertex][1];
        to[vertex][0] = s_current->faces[currentFace].screen[vertex][0];
        to[vertex][1] = s_current->faces[currentFace].screen[vertex][1];
    }
    for (int vertex = 0; vertex < 4; ++vertex) {
        float previousTriangle[3][2], currentTriangle[3][2], weight[3];
        int indices[3] = {0, 1, 2};
        for (int corner = 0; corner < 3; ++corner) {
            previousTriangle[corner][0] = from[indices[corner]][0];
            previousTriangle[corner][1] = from[indices[corner]][1];
            currentTriangle[corner][0] = to[indices[corner]][0];
            currentTriangle[corner][1] = to[indices[corner]][1];
        }
        if (!Barycentric(previousTriangle, x[vertex], y[vertex], weight)) {
            indices[0] = 1; indices[1] = 2; indices[2] = 3;
            for (int corner = 0; corner < 3; ++corner) {
                previousTriangle[corner][0] = from[indices[corner]][0];
                previousTriangle[corner][1] = from[indices[corner]][1];
                currentTriangle[corner][0] = to[indices[corner]][0];
                currentTriangle[corner][1] = to[indices[corner]][1];
            }
            if (!Barycentric(previousTriangle, x[vertex], y[vertex], weight)) return 0;
        }
        float targetX = 0, targetY = 0;
        for (int corner = 0; corner < 3; ++corner) {
            targetX += weight[corner] * currentTriangle[corner][0];
            targetY += weight[corner] * currentTriangle[corner][1];
        }
        x[vertex] += fraction * (targetX - x[vertex]);
        y[vertex] += fraction * (targetY - y[vertex]);
    }
    return 1;
}

static void KeepConnectedSurfacesCoherent(const RageClassicPacketSource *sources) {
    memset(s_vertexKeys, 0, sizeof(s_vertexKeys));
    for (int i = 0; i < s_previous->packetCount; ++i) {
        s_component[i] = i;
        s_componentHeld[i] = !PacketHasMotion(i);
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
        if (s_componentHeld[ComponentRoot(i)]) {
            if (s_matches[i] >= 0) --s_matchCount;
            s_matches[i] = -1;
            s_packetHeld[i] = 1;
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
    memset(s_faceMatches, 0xff, sizeof(s_faceMatches));
    memset(s_faceProjectable, 0, sizeof(s_faceProjectable));
    memset(s_packetFaces, 0xff, sizeof(s_packetFaces));
    memset(s_packetFaceMotion, 0, sizeof(s_packetFaceMotion));
    memset(s_packetHeld, 0, sizeof(s_packetHeld));
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
    memset(s_faceKeys, 0, sizeof(s_faceKeys));
    for (int i = 0; i < current->faceCount; ++i) {
        uint64_t key = FaceKey(current, i, 0);
        if (!key) continue;
        unsigned slot = FindFaceSlot(key);
        s_faceSlots[slot] = s_faceKeys[slot] ? -1 : i;
        s_faceKeys[slot] = key;
    }
    for (int i = 0; i < previous->faceCount; ++i) {
        uint64_t key = FaceKey(previous, i, 1);
        if (!key) continue;
        unsigned slot = FindFaceSlot(key);
        if (s_faceKeys[slot] == key && s_faceSlots[slot] >= 0) {
            s_faceMatches[i] = s_faceSlots[slot];
            s_faceProjectable[i] = 1;
            ++s_faceMotionCount;
        }
    }
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
        s_packetFaces[i] = previousSources[i].faceIndex;
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
    s_stats.candidates = s_matchCount;
    s_stats.faceParents = s_faceMotionCount;
    for (int i = 0; i < previous->packetCount; ++i) {
        int coordinates[4][2];
        float x[4], y[4];
        int face = s_packetFaces[i];
        if (!(previous->packets[i].flags & RAGE_CAPTURE_PACKET_3D) ||
            face < 0 || face >= previous->faceCount || !s_faceProjectable[face] ||
            !PolygonCoordinates(&previous->packets[i], coordinates)) continue;
        for (int vertex = 0; vertex < 4; ++vertex) {
            x[vertex] = (float)coordinates[vertex][0];
            y[vertex] = (float)coordinates[vertex][1];
        }
        /* Verify every child coordinate belongs to the captured parent before
         * it is allowed to keep a connected surface moving. If a packet uses
         * a different coordinate space, the existing hold path is safer. */
        s_packetFaceMotion[i] = FaceMotionCoordinates(face, 0.0f, x, y);
    }
    /* Packet matches are exact. Parent-face matches cover changed subdivision
     * layouts, so only hold a component when neither representation exists. */
    KeepConnectedSurfacesCoherent(previousSources);
    for (int i = 0; i < previous->packetCount; ++i) {
        int xy[4][2];
        if (!(previous->packets[i].flags & RAGE_CAPTURE_PACKET_3D) ||
            !PolygonCoordinates(&previous->packets[i], xy)) continue;
        ++s_stats.polygons;
        int faceIndex = previousSources[i].faceIndex;
        int course = faceIndex >= 0 && faceIndex < previous->faceCount &&
            (previous->faces[faceIndex].kind == RAGE_CAPTURE_KIND_COURSE ||
             previous->faces[faceIndex].kind == RAGE_CAPTURE_KIND_TERRAIN);
        s_stats.coursePolygons += course;
        if (s_matches[i] >= 0 || s_packetFaceMotion[i]) {
            ++s_stats.moving;
            s_stats.courseMoving += course;
        }
    }
}

int ClassicMotionCoordinates(int packetIndex, float fraction,
                             float x[4], float y[4]) {
    if (!s_previous || !s_current || packetIndex < 0 ||
        packetIndex >= s_previous->packetCount || s_packetHeld[packetIndex] ||
        !isfinite(fraction)) return 0;
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;
    if (s_matches[packetIndex] < 0 && s_packetFaceMotion[packetIndex])
        return FaceMotionCoordinates(s_packetFaces[packetIndex], fraction, x, y);
    if (s_matches[packetIndex] < 0) return 0;
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
ClassicMotionStats ClassicMotionGetStats(void) { return s_stats; }
