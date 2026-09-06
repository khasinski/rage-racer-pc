#define RAGE_MESH_OBJ_TEST
#include "../../tools/rage_mesh_obj.c"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); exit(1); } } while (0)

static const char *fixture =
    "v 1 2 3 1 0 0\nv 4 5 6 0 1 0\nv 7 8 9 0 0 1\n"
    "vt 0.25 0.75\nvt 0.5 0.5\nvt 0.75 0.25\n"
    "vn 0 4096 0\nusemtl rage_7\nf 1/1/1 2/2/1 3/3/1\n"
    "usemtl rage_4294967295\nf -1/-1/-1 -2/-2/-1 -3/-3/-1\n"
    "usemtl rage_552468483\nf 1/1/1 3/3/1 2/2/1\n";

static size_t ReadOutput(FILE *file, unsigned char *bytes, size_t capacity) {
    size_t n;
    CHECK(!fflush(file));
    rewind(file);
    n = fread(bytes, 1, capacity, file);
    CHECK(!ferror(file) && feof(file));
    return n;
}

static void Reject(const char *text) {
    FILE *in = tmpfile(), *out = tmpfile();
    CHECK(in && out);
    fputs(text, in);
    rewind(in);
    CHECK(!ObjImport(in, out));
    CHECK(ftell(out) == 0);
    fclose(in); fclose(out);
}

int main(void) {
    FILE *in = tmpfile(), *native = tmpfile(), *obj = tmpfile(), *again = tmpfile();
    unsigned char bytes[1024], other[1024];
    RageRuntimeMesh mesh;
    RageRuntimeVertex v;
    size_t size, second;
    unsigned int channel;
    for (channel = 0; channel < 256; channel++)
        CHECK(lroundf(ObjToLinear(ObjToSRGB((uint8_t)channel)) * 255.0f) == channel);
    CHECK(lroundf(ObjToLinear(0.2392f) * 255.0f) == 12);
    CHECK(in && native && obj && again);
    fputs(fixture, in); rewind(in);
    CHECK(ObjImport(in, native));
    size = ReadOutput(native, bytes, sizeof(bytes));
    CHECK(RuntimeMeshOpen(&mesh, bytes, size));
    CHECK(mesh.meshCount == 1 && mesh.indexCount == 9);
    CHECK(RuntimeMeshVertex(&mesh, 0, &v));
    CHECK(v.position[0] == 1 && v.position[1] == 2 && v.position[2] == 3);
    CHECK(v.normal[1] == 4096 && v.color[0] == 255 && v.color[1] == 0);
    CHECK(v.uv[0] == 0.25f && v.uv[1] == 0.25f && v.material == 7);
    CHECK(RuntimeMeshVertex(&mesh, 3, &v));
    CHECK(v.position[0] == 7 && v.color[2] == 255 && v.material == UINT32_MAX);
    CHECK(RuntimeMeshVertex(&mesh, 6, &v) && v.material == 552468483);
    CHECK(ObjExport(&mesh, 0, obj));
    CHECK(!ObjExport(&mesh, 1, obj));
    rewind(obj);
    CHECK(ObjImport(obj, again));
    second = ReadOutput(again, other, sizeof(other));
    CHECK(size == second && !memcmp(bytes, other, size));
    Reject("");
    Reject("v nan 0 0\n");
    Reject("vn 0 inf 0\n");
    Reject("vt 0 nan\n");
    Reject("v 0 0 0 2 0 0\n");
    Reject("usemtl rage_4294967296\n");
    Reject("usemtl rage_7suffix\n");
    Reject("usemtl Material\n");
    Reject("v 0 0 0\nvt 0 0\nvn 0 1 0\nf 0/1/1 1/1/1 1/1/1\n");
    Reject("v 0 0 0\nvt 0 0\nvn 0 1 0\nf -2/1/1 1/1/1 1/1/1\n");
    Reject("v 0 0 0\nvt 0 0\nvn 0 1 0\nf 1/1/1 1/1/1 1/1/1 1/1/1\n");
    Reject("v 0 0 0\nvt 0 0\nvn 0 1 0\nf 1//1 1//1 1//1\n");
    fclose(in); fclose(native); fclose(obj); fclose(again);
    puts("mesh OBJ round trip and rejection tests passed");
    return 0;
}
