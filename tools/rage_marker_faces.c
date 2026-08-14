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
            if (behind) continue;
            if (pointInTri(probeX, probeY, sx[0], sy[0], sx[1], sy[1], sx[2],
                           sy[2]) ||
                pointInTri(probeX, probeY, sx[1], sy[1], sx[3], sy[3], sx[2],
                           sy[2])) {
                printf("cover kind=%d klass=%d ot=%d bias=%d cell=%d "
                       "clut=%04x tpage=%04x z=%.0f..%.0f "
                       "sxy=%.0f,%.0f/%.0f,%.0f/%.0f,%.0f/%.0f,%.0f\n",
                       f->kind, f->klass, f->otDepth, f->bias, f->cellSlot,
                       f->clut, f->tpage, view[0][2], view[3][2], sx[0],
                       sy[0], sx[1], sy[1], sx[2], sy[2], sx[3], sy[3]);
            }
        }
    }
    return 0;
}
