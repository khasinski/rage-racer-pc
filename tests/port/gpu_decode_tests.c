#include <psyz/gpu_decode.h>

#include <stdio.h>

static int failures;
#define CHECK(value) do { if (!(value)) { failures++; \
    fprintf(stderr, "line %d: %s\n", __LINE__, #value); } } while (0)

static void CheckTexturedQuad(void) {
    const unsigned words[] = {
        0x2f302010, 0xfff00002, 0x12340807,
        0x00506040, 0x00560030, 0x00a00080,
        0x0b0a0908, 0x00e000c0, 0x0d0c0b0a,
    };
    PsyzGpuPrimitive p;
    CHECK(Psyz_GpuDecodePrimitive(words, 9, 7, 3, -2, &p) == 1);
    CHECK(p.type == PSYZ_GPU_PRIMITIVE_POLYGON && p.count == 4 &&
          p.word_count == 9);
    CHECK((p.flags & (PSYZ_GPU_TEXTURED | PSYZ_GPU_QUAD |
                      PSYZ_GPU_SEMITRANSPARENT | PSYZ_GPU_RAW_TEXTURE)) ==
          (PSYZ_GPU_TEXTURED | PSYZ_GPU_QUAD |
           PSYZ_GPU_SEMITRANSPARENT | PSYZ_GPU_RAW_TEXTURE));
    CHECK(p.points[0].x == 5 && p.points[0].y == -18);
    CHECK(p.points[0].u == 7 && p.points[0].v == 8);
    CHECK(p.points[0].color == 0x808080 && p.clut == 0x1234);
    CHECK(p.tpage == 0x56);
}

static void CheckRectangle(void) {
    const unsigned words[] = {0x64030201, 0x0014000a, 0x45670605, 0x00080010};
    const unsigned long native[] = {
        0x64030201, 0x0014000a, 0x45670605, 0x00080010};
    PsyzGpuPrimitive p;
    CHECK(Psyz_GpuDecodePrimitive(words, 4, 0x44, -1, 2, &p) == 1);
    CHECK(p.type == PSYZ_GPU_PRIMITIVE_RECTANGLE && p.count == 4 &&
          p.word_count == 4);
    CHECK(p.points[0].x == 9 && p.points[0].y == 22);
    CHECK(p.points[3].x == 25 && p.points[3].y == 30);
    CHECK(p.points[3].u == 21 && p.points[3].v == 14);
    CHECK(p.tpage == 0x44 && p.clut == 0x4567);
    CHECK(Psyz_GpuDecodePrimitiveNative(native, 4, 0x44, -1, 2, &p) == 1);
    CHECK(p.points[3].x == 25 && p.points[3].y == 30 && p.word_count == 4);
}

static void CheckLine(void) {
    const unsigned words[] = {
        0x50030201, 0x00020001, 0x00060504, 0x00040003,
    };
    PsyzGpuPrimitive p;
    CHECK(Psyz_GpuDecodePrimitive(words, 4, 0, 10, 20, &p) == 1);
    CHECK(p.type == PSYZ_GPU_PRIMITIVE_LINE && p.count == 2 &&
          p.word_count == 4);
    CHECK(p.points[0].x == 11 && p.points[0].y == 22);
    CHECK(p.points[1].x == 13 && p.points[1].y == 24);
    CHECK(p.points[1].color == 0x060504);
}

int main(void) {
    PsyzGpuPrimitive p;
    const unsigned invalid[] = {0x2cffffff};
    CheckTexturedQuad();
    CheckRectangle();
    CheckLine();
    CHECK(Psyz_GpuDecodePrimitive(invalid, 1, 0, 0, 0, &p) == -1);
    CHECK(Psyz_GpuDecodePrimitive(NULL, 0, 0, 0, 0, &p) == -1);
    return failures != 0;
}
