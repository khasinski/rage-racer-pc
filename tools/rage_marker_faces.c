/* Offline analysis of a modern-renderer marker scene snapshot: projects
 * every captured 3D face with the renderer's math and reports the ones
 * covering a probed logical (4:3, 320x240) pixel, plus terrain/cell stats.
 *
 *   rage_marker_faces markers/marker-0-scene.bin 164 114
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/port/modern/scene_capture.h"

static const RageSceneSnapshot *snapshotFromFile(const char *path) {
    static RageSceneSnapshot snapshot;
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;
    if (fread(&snapshot, sizeof(snapshot), 1, file) != 1) {
        fclose(file);
        return NULL;
    }
    fclose(file);
    return &snapshot;
}

static void faceView(const RageSceneSnapshot *s, const RageCaptureFace *f,
                     float out[4][3]) {
    const RageCaptureGteState *gte;
    float tx, ty, tz;
    int vertex, row;
    if (f->kind == RAGE_CAPTURE_KIND_TERRAIN) {
        const RageCaptureTerrainBatch *batch = &s->terrain[f->drawIndex];
        const int32_t *cell = batch->cells[f->cellSlot];
        gte = &batch->gte;
        tx = (float)(batch->mirror ? -cell[0] : cell[0]);
        ty = (float)cell[1];
        tz = (float)cell[2];
    } else {
        gte = &s->draws[f->drawIndex].gte;
        tx = (float)gte->rot.t[0];
        ty = (float)gte->rot.t[1];
        tz = (float)gte->rot.t[2];
    }
    for (vertex = 0; vertex < 4; vertex++) {
        for (row = 0; row < 3; row++) {
            out[vertex][row] =
                (gte->rot.m[row][0] * (float)f->pos[vertex][0] +
                 gte->rot.m[row][1] * (float)f->pos[vertex][1] +
                 gte->rot.m[row][2] * (float)f->pos[vertex][2]) /
                4096.0f;
        }
        out[vertex][0] += tx;
        out[vertex][1] += ty;
        out[vertex][2] += tz;
    }
}

static int pointInTri(float px, float py, float x0, float y0, float x1,
                      float y1, float x2, float y2) {
    float d = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    float w0, w1, w2;
    if (d == 0.0f) return 0;
    w0 = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / d;
    w1 = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / d;
    w2 = 1.0f - w0 - w1;
    return w0 >= -0.001f && w1 >= -0.001f && w2 >= -0.001f;
}

int main(int argc, char **argv) {
    const RageSceneSnapshot *s;
    float probeX = -1000.0f, probeY = -1000.0f;
    int histogram[8] = {0};
    int terrainFaces = 0, courseFaces = 0, modelFaces = 0, extended = 0;
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s scene.bin [x4_3 y]\n", argv[0]);
        return 2;
    }
    s = snapshotFromFile(argv[1]);
    if (s == NULL) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 1;
    }
    if (argc >= 4) {
        probeX = (float)atof(argv[2]);
        probeY = (float)atof(argv[3]);
    }
    printf("frame=%u scene=%d timer=%d draws=%d terrain=%d faces=%d\n",
           s->frameCounter, s->sceneId, s->sceneTimer, s->drawCount,
           s->terrainCount, s->faceCount);
    for (i = 0; i < s->terrainCount; i++) {
        const RageCaptureTerrainBatch *batch = &s->terrain[i];
        int c, minZ = 0x7fffffff, maxZ = -0x7fffffff;
        for (c = 0; c < batch->cellCount; c++) {
            if (batch->cells[c][2] < minZ) minZ = batch->cells[c][2];
            if (batch->cells[c][2] > maxZ) maxZ = batch->cells[c][2];
        }
        printf("terrain[%d] mirror=%d cells=%d cellZ=%d..%d\n", i,
               batch->mirror, batch->cellCount, minZ, maxZ);
        if (getenv("MARKER_LIST_CELLS") != NULL) {
            for (c = 0; c < batch->cellCount; c++) {
                printf("  cell slot=%d index=%d xyz=%d,%d,%d\n", c,
                       batch->cells[c][3], batch->cells[c][0],
                       batch->cells[c][1], batch->cells[c][2]);
            }
        }
    }
    for (i = 0; i < s->faceCount; i++) {
        const RageCaptureFace *f = &s->faces[i];
        int bucket = f->otDepth;
        if (f->kind == RAGE_CAPTURE_KIND_TERRAIN) terrainFaces++;
        else if (f->kind == RAGE_CAPTURE_KIND_COURSE) courseFaces++;
        else modelFaces++;
        if (bucket >= 448) extended++;
        if (bucket < 0) bucket = 0;
        if (bucket > 895) bucket = 895;
        histogram[bucket / 128]++;
    }
    printf("faces: terrain=%d course=%d model=%d extended(>=448)=%d\n",
           terrainFaces, courseFaces, modelFaces, extended);
    if (getenv("MARKER_LIST_PACKETS") != NULL) {
        int p;
        for (p = 0; p < s->packetCount; p++) {
            const RageCapturePacket *packet = &s->packets[p];
            int word;
            printf("packet %d table=%d bucket=%d words=", p, packet->table,
                   packet->bucket);
            for (word = 0; word < packet->size; word++) {
                printf("%s%08x", word ? "," : "", packet->words[word]);
            }
            printf("\n");
        }
    }
    if (getenv("MARKER_CELL_STATS") != NULL) {
        static int slotCount[16][64];
        static float slotMinZ[16][64], slotMaxZ[16][64];
        int b, c;
        for (i = 0; i < s->faceCount; i++) {
            const RageCaptureFace *f = &s->faces[i];
            float view[4][3];
            int vertex;
            if (f->kind != RAGE_CAPTURE_KIND_TERRAIN) continue;
            if (f->drawIndex < 0 || f->drawIndex >= 16) continue;
            if (f->cellSlot < 0 || f->cellSlot >= 64) continue;
            faceView(s, f, view);
            for (vertex = 0; vertex < 4; vertex++) {
                float z = view[vertex][2];
                int *count = &slotCount[f->drawIndex][f->cellSlot];
                if (*count == 0 || z < slotMinZ[f->drawIndex][f->cellSlot])
                    slotMinZ[f->drawIndex][f->cellSlot] = z;
                if (*count == 0 || z > slotMaxZ[f->drawIndex][f->cellSlot])
                    slotMaxZ[f->drawIndex][f->cellSlot] = z;
            }
            slotCount[f->drawIndex][f->cellSlot]++;
        }
        for (b = 0; b < s->terrainCount; b++) {
            for (c = 0; c < s->terrain[b].cellCount; c++) {
                if (slotCount[b][c] == 0) {
                    if (s->terrain[b].cells[c][3] >= 0) {
                        printf("cellstat batch=%d slot=%d index=%d faces=0\n",
                               b, c, s->terrain[b].cells[c][3]);
                    }
                    continue;
                }
                printf("cellstat batch=%d slot=%d index=%d faces=%d "
                       "z=%.0f..%.0f\n",
                       b, c, s->terrain[b].cells[c][3], slotCount[b][c],
                       slotMinZ[b][c], slotMaxZ[b][c]);
            }
        }
    }
    if (getenv("MARKER_BAND") != NULL) {
        /* MARKER_BAND="x yMin yMax": every non-mirror face whose projected
         * quad crosses column x within [yMin,yMax], with exact view-space
         * corners, for crack diagnosis. */
        float bandX, bandY0, bandY1;
        if (sscanf(getenv("MARKER_BAND"), "%f %f %f", &bandX, &bandY0,
                   &bandY1) == 3) {
            for (i = 0; i < s->faceCount; i++) {
                const RageCaptureFace *f = &s->faces[i];
                const RageCaptureGteState *gte;
                float view[4][3];
                float sx[4], sy[4];
                int vertex, behind = 0, mirror;
                float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
                if (f->kind == RAGE_CAPTURE_KIND_TERRAIN) {
                    mirror = s->terrain[f->drawIndex].mirror;
                    gte = &s->terrain[f->drawIndex].gte;
                } else {
                    mirror = s->draws[f->drawIndex].mirror;
                    gte = &s->draws[f->drawIndex].gte;
                }
                if (mirror) continue;
                faceView(s, f, view);
                for (vertex = 0; vertex < 4; vertex++) {
                    float z = view[vertex][2];
                    if (z < 1.0f) { behind = 1; break; }
                    sx[vertex] = gte->ofx + view[vertex][0] * gte->h / z;
                    sy[vertex] = gte->ofy + view[vertex][1] * gte->h / z;
                    if (sx[vertex] < minX) minX = sx[vertex];
                    if (sx[vertex] > maxX) maxX = sx[vertex];
                    if (sy[vertex] < minY) minY = sy[vertex];
                    if (sy[vertex] > maxY) maxY = sy[vertex];
                }
                if (behind) continue;
                if (minX > bandX + 0.5f || maxX < bandX - 0.5f) continue;
                if (minY > bandY1 || maxY < bandY0) continue;
                printf("band face=%d kind=%d cell=%d draw=%d ot=%d "
                       "clut=%04x tpage=%04x\n",
                       i, f->kind, f->cellSlot, f->drawIndex, f->otDepth,
                       f->clut, f->tpage);
                for (vertex = 0; vertex < 4; vertex++) {
                    printf("  v%d local=%d,%d,%d view=%.2f,%.2f,%.2f "
                           "s=%.3f,%.3f\n",
                           vertex, f->pos[vertex][0], f->pos[vertex][1],
                           f->pos[vertex][2], view[vertex][0],
                           view[vertex][1], view[vertex][2], sx[vertex],
                           sy[vertex]);
                }
            }
        }
    }
    printf("bucket histogram (128 buckets/bin): ");
    for (i = 0; i < 8; i++) printf("%d ", histogram[i]);
    printf("\n");
    if (probeX > -999.0f) {
        for (i = 0; i < s->faceCount; i++) {
            const RageCaptureFace *f = &s->faces[i];
            const RageCaptureGteState *gte =
                f->kind == RAGE_CAPTURE_KIND_TERRAIN
                    ? &s->terrain[f->drawIndex].gte
                    : &s->draws[f->drawIndex].gte;
            float view[4][3];
            float sx[4], sy[4];
            int vertex, behind = 0;
            faceView(s, f, view);
            for (vertex = 0; vertex < 4; vertex++) {
                float z = view[vertex][2];
                if (z < 1.0f) {
                    behind = 1;
                    break;
                }
                sx[vertex] = gte->ofx + view[vertex][0] * gte->h / z;
                sy[vertex] = gte->ofy + view[vertex][1] * gte->h / z;
            }
            if (behind) {
                /* Near-plane clip each triangle in view space (like the
                 * GPU's homogeneous clip) and test the clipped polygon. */
                const int tris[2][3] = {{0, 1, 2}, {1, 3, 2}};
                int t, hit = 0;
                for (t = 0; t < 2 && !hit; t++) {
                    float poly[8][3];
                    int count = 0;
                    int a;
                    for (a = 0; a < 3; a++) {
                        const float *cur = view[tris[t][a]];
                        const float *nxt = view[tris[t][(a + 1) % 3]];
                        if (cur[2] >= 1.0f) {
                            memcpy(poly[count++], cur, sizeof(poly[0]));
                        }
                        if ((cur[2] >= 1.0f) != (nxt[2] >= 1.0f)) {
                            float span = nxt[2] - cur[2];
                            float k = span != 0.0f ? (1.0f - cur[2]) / span
                                                   : 0.0f;
                            poly[count][0] = cur[0] + k * (nxt[0] - cur[0]);
                            poly[count][1] = cur[1] + k * (nxt[1] - cur[1]);
                            poly[count][2] = 1.0f;
                            count++;
                        }
                    }
                    if (count >= 3) {
                        float px[8], py[8];
                        int e, inside = 1;
                        for (e = 0; e < count; e++) {
                            px[e] = gte->ofx +
                                    poly[e][0] * gte->h / poly[e][2];
                            py[e] = gte->ofy +
                                    poly[e][1] * gte->h / poly[e][2];
                        }
                        /* Fan containment test. */
                        inside = 0;
                        for (e = 1; e + 1 < count; e++) {
                            if (pointInTri(probeX, probeY, px[0], py[0],
                                           px[e], py[e], px[e + 1],
                                           py[e + 1])) {
                                inside = 1;
                                break;
                            }
                        }
                        if (inside) hit = 1;
                    }
                }
                if (hit) {
                    printf("cover-clipped kind=%d klass=%d flags=%02x ot=%d "
                           "bias=%d cell=%d draw=%d mirror=%d clut=%04x "
                           "tpage=%04x z=%.0f,%.0f,%.0f,%.0f\n",
                           f->kind, f->klass, f->flags, f->otDepth, f->bias,
                           f->cellSlot, f->drawIndex,
                           f->kind == RAGE_CAPTURE_KIND_TERRAIN
                               ? s->terrain[f->drawIndex].mirror
                               : s->draws[f->drawIndex].mirror,
                           f->clut, f->tpage, view[0][2], view[1][2],
                           view[2][2], view[3][2]);
                }
                continue;
            }
            if (pointInTri(probeX, probeY, sx[0], sy[0], sx[1], sy[1], sx[2],
                           sy[2]) ||
                pointInTri(probeX, probeY, sx[1], sy[1], sx[3], sy[3], sx[2],
                           sy[2])) {
                printf("cover kind=%d klass=%d flags=%02x ot=%d bias=%d "
                       "cell=%d draw=%d mirror=%d "
                       "clut=%04x tpage=%04x z=%.0f..%.0f "
                       "sxy=%.0f,%.0f/%.0f,%.0f/%.0f,%.0f/%.0f,%.0f\n",
                       f->kind, f->klass, f->flags, f->otDepth, f->bias,
                       f->cellSlot, f->drawIndex,
                       f->kind == RAGE_CAPTURE_KIND_TERRAIN
                           ? s->terrain[f->drawIndex].mirror
                           : s->draws[f->drawIndex].mirror,
                       f->clut, f->tpage, view[0][2], view[3][2], sx[0],
                       sy[0], sx[1], sy[1], sx[2], sy[2], sx[3], sy[3]);
                {
                    /* Perspective-correct UV at the probe (screen-space
                     * barycentric over 1/z, both triangles of the quad). */
                    const int tris[2][3] = {{0, 1, 2}, {1, 3, 2}};
                    int t;
                    for (t = 0; t < 2; t++) {
                        int a = tris[t][0], b = tris[t][1], c = tris[t][2];
                        float d = (sy[b] - sy[c]) * (sx[a] - sx[c]) +
                                  (sx[c] - sx[b]) * (sy[a] - sy[c]);
                        float w0, w1, w2, iz, u, v;
                        if (d == 0.0f) continue;
                        w0 = ((sy[b] - sy[c]) * (probeX - sx[c]) +
                              (sx[c] - sx[b]) * (probeY - sy[c])) / d;
                        w1 = ((sy[c] - sy[a]) * (probeX - sx[c]) +
                              (sx[a] - sx[c]) * (probeY - sy[c])) / d;
                        w2 = 1.0f - w0 - w1;
                        if (w0 < -0.001f || w1 < -0.001f || w2 < -0.001f)
                            continue;
                        iz = w0 / view[a][2] + w1 / view[b][2] +
                             w2 / view[c][2];
                        u = (w0 * f->uv[a][0] / view[a][2] +
                             w1 * f->uv[b][0] / view[b][2] +
                             w2 * f->uv[c][0] / view[c][2]) / iz;
                        v = (w0 * f->uv[a][1] / view[a][2] +
                             w1 * f->uv[b][1] / view[b][2] +
                             w2 * f->uv[c][1] / view[c][2]) / iz;
                        printf("  tri%d uv=%.3f,%.3f corners=%u,%u/%u,%u/"
                               "%u,%u/%u,%u twin=%05x\n",
                               t, u, v, f->uv[0][0], f->uv[0][1],
                               f->uv[1][0], f->uv[1][1], f->uv[2][0],
                               f->uv[2][1], f->uv[3][0], f->uv[3][1],
                               f->textureWindow);
                    }
                }
            }
        }
    }
    return 0;
}
