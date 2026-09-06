/* Blender interchange without an interpreter dependency. Coordinates stay in
 * the runtime mesh's Y-up space. OBJ texture V is the inverse of runtime V.
 * Material names encode the native material slot, including UINT32_MAX for
 * untextured faces. Export from Blender with triangulation and vertex colors. */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/rmesh.h"

typedef struct ObjPosition { float xyz[3], rgb[3]; } ObjPosition;
typedef struct ObjNormal { float xyz[3]; } ObjNormal;
typedef struct ObjUV { float uv[2]; } ObjUV;
typedef struct ObjArray { void *data; size_t count, size, capacity; } ObjArray;

/* Blender's OBJ colors are sRGB; the native vertex multiplier is linear.
 * Without this conversion a dark 12/255 trim becomes 61/255 after export. */
static float ObjToLinear(float value) {
    return value <= 0.04045f ? value / 12.92f :
        powf((value + 0.055f) / 1.055f, 2.4f);
}

static float ObjToSRGB(uint8_t value) {
    float linear = value / 255.0f;
    return linear <= 0.0031308f ? linear * 12.92f :
        1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
}

static int ObjAppend(ObjArray *a, const void *value) {
    if (a->count == a->capacity) {
        size_t capacity = a->capacity ? a->capacity * 2 : 256;
        void *next;
        if (capacity < a->capacity || capacity > UINT32_MAX ||
            capacity > SIZE_MAX / a->size) return 0;
        next = realloc(a->data, capacity * a->size);
        if (!next) return 0;
        a->data = next;
        a->capacity = capacity;
    }
    memcpy((char *)a->data + a->count++ * a->size, value, a->size);
    return 1;
}

static int ObjU32(FILE *out, uint32_t value) {
    unsigned char b[4] = {(unsigned char)value, (unsigned char)(value >> 8),
                         (unsigned char)(value >> 16), (unsigned char)(value >> 24)};
    return fwrite(b, 1, 4, out) == 4;
}

static int ObjFloat(FILE *out, float value) {
    uint32_t bits;
    memcpy(&bits, &value, 4);
    return ObjU32(out, bits);
}

static int ObjWriteMesh(FILE *out, const ObjArray *vertices) {
    const RageRuntimeVertex *v = vertices->data;
    size_t i, j;
    if (vertices->count > UINT32_MAX ||
        fwrite("RRMESH1\0", 1, 8, out) != 8 ||
        !ObjU32(out, 1) || !ObjU32(out, 1) ||
        !ObjU32(out, (uint32_t)vertices->count) ||
        !ObjU32(out, (uint32_t)vertices->count) ||
        !ObjU32(out, 0) || !ObjU32(out, (uint32_t)vertices->count)) return 0;
    for (i = 0; i < vertices->count; i++) {
        for (j = 0; j < 3; j++) if (!ObjFloat(out, v[i].position[j])) return 0;
        for (j = 0; j < 3; j++) if (!ObjFloat(out, v[i].normal[j])) return 0;
        if (fwrite(v[i].color, 1, 4, out) != 4 ||
            !ObjFloat(out, v[i].uv[0]) || !ObjFloat(out, v[i].uv[1]) ||
            !ObjU32(out, v[i].material)) return 0;
    }
    for (i = 0; i < vertices->count; i++)
        if (!ObjU32(out, (uint32_t)i)) return 0;
    return !ferror(out);
}

static int ObjFinite(const float *v, size_t count) {
    size_t i;
    for (i = 0; i < count; i++) if (!isfinite(v[i])) return 0;
    return 1;
}

static int ObjIndex(const char **text, size_t count, size_t *index) {
    char *end;
    long value;
    errno = 0;
    value = strtol(*text, &end, 10);
    if (end == *text || errno || value == 0) return 0;
    if (value > 0) {
        if ((unsigned long)value > count) return 0;
        *index = (size_t)value - 1;
    } else {
        /* Avoid negating LONG_MIN. */
        unsigned long magnitude = (unsigned long)(-(value + 1)) + 1;
        if (magnitude > count) return 0;
        *index = count - magnitude;
    }
    *text = end;
    return 1;
}

static int ObjImport(FILE *in, FILE *out) {
    ObjArray positions = {NULL, 0, sizeof(ObjPosition), 0};
    ObjArray normals = {NULL, 0, sizeof(ObjNormal), 0};
    ObjArray uvs = {NULL, 0, sizeof(ObjUV), 0};
    ObjArray vertices = {NULL, 0, sizeof(RageRuntimeVertex), 0};
    char line[4096];
    uint32_t material = UINT32_MAX;
    size_t lineNumber = 0;
    int ok = 0;
    while (fgets(line, sizeof(line), in)) {
        const char *s = line;
        lineNumber++;
        if (!strchr(line, '\n') && !feof(in)) goto done;
        while (isspace((unsigned char)*s)) s++;
        if (!*s || *s == '#') continue;
        if (s[0] == 'v' && isspace((unsigned char)s[1])) {
            ObjPosition p = {{0, 0, 0}, {1, 1, 1}};
            char extra;
            int n = sscanf(s + 1, "%f %f %f %f %f %f %c", &p.xyz[0],
                &p.xyz[1], &p.xyz[2], &p.rgb[0], &p.rgb[1], &p.rgb[2], &extra);
            size_t i;
            if ((n != 3 && n != 6) || !ObjFinite(p.xyz, 3) ||
                !ObjFinite(p.rgb, 3)) goto done;
            for (i = 0; i < 3; i++) if (p.rgb[i] < 0 || p.rgb[i] > 1) goto done;
            if (!ObjAppend(&positions, &p)) goto done;
        } else if (s[0] == 'v' && s[1] == 'n' && isspace((unsigned char)s[2])) {
            ObjNormal n;
            char extra;
            if (sscanf(s + 2, "%f %f %f %c", &n.xyz[0], &n.xyz[1],
                       &n.xyz[2], &extra) != 3 || !ObjFinite(n.xyz, 3) ||
                !ObjAppend(&normals, &n)) goto done;
        } else if (s[0] == 'v' && s[1] == 't' && isspace((unsigned char)s[2])) {
            ObjUV uv;
            char extra;
            if (sscanf(s + 2, "%f %f %c", &uv.uv[0], &uv.uv[1], &extra) != 2 ||
                !ObjFinite(uv.uv, 2) || !ObjAppend(&uvs, &uv)) goto done;
        } else if (!strncmp(s, "usemtl ", 7)) {
            char *end;
            unsigned long value;
            s += 7;
            while (isspace((unsigned char)*s)) s++;
            if (strncmp(s, "rage_", 5) || !isdigit((unsigned char)s[5])) goto done;
            errno = 0;
            value = strtoul(s + 5, &end, 10);
            if (errno || value > UINT32_MAX) goto done;
            while (isspace((unsigned char)*end)) end++;
            if (*end) goto done;
            material = (uint32_t)value;
        } else if (s[0] == 'f' && isspace((unsigned char)s[1])) {
            size_t corner;
            s++;
            for (corner = 0; corner < 3; corner++) {
                size_t p, t, n, j;
                RageRuntimeVertex v;
                ObjPosition *position;
                while (isspace((unsigned char)*s)) s++;
                if (!ObjIndex(&s, positions.count, &p) || *s++ != '/' ||
                    !ObjIndex(&s, uvs.count, &t) || *s++ != '/' ||
                    !ObjIndex(&s, normals.count, &n) ||
                    (*s && !isspace((unsigned char)*s))) goto done;
                position = &((ObjPosition *)positions.data)[p];
                memcpy(v.position, position->xyz, sizeof(v.position));
                memcpy(v.normal, ((ObjNormal *)normals.data)[n].xyz, sizeof(v.normal));
                for (j = 0; j < 3; j++)
                    v.color[j] = (uint8_t)lroundf(ObjToLinear(position->rgb[j]) * 255.0f);
                v.color[3] = 255;
                v.uv[0] = ((ObjUV *)uvs.data)[t].uv[0];
                v.uv[1] = 1.0f - ((ObjUV *)uvs.data)[t].uv[1];
                v.material = material;
                if (!ObjAppend(&vertices, &v)) goto done;
            }
            while (isspace((unsigned char)*s)) s++;
            if (*s) goto done;
        } else if (strncmp(s, "o ", 2) && strncmp(s, "g ", 2) &&
                   strncmp(s, "s ", 2) && strncmp(s, "mtllib ", 7)) goto done;
    }
    if (!ferror(in) && vertices.count) ok = ObjWriteMesh(out, &vertices);
done:
    if (!ok) fprintf(stderr, "rage-mesh-obj: invalid OBJ or I/O failure at line %zu\n", lineNumber);
    free(positions.data); free(normals.data); free(uvs.data); free(vertices.data);
    return ok;
}

static int ObjExport(const RageRuntimeMesh *mesh, uint32_t part, FILE *out) {
    uint32_t first, count, i;
    if (!RuntimeMeshRange(mesh, part, &first, &count) || !count) return 0;
    fprintf(out, "# Rage native mesh; Y up; preserve rage_N materials\no model-%u\n", part);
    for (i = 0; i < count; i++) {
        uint32_t index;
        RageRuntimeVertex v;
        if (!RuntimeMeshIndex(mesh, first + i, &index) ||
            !RuntimeMeshVertex(mesh, index, &v) || v.color[3] != 255) return 0;
        fprintf(out, "v %.9g %.9g %.9g %.9g %.9g %.9g\n",
                v.position[0], v.position[1], v.position[2],
                ObjToSRGB(v.color[0]), ObjToSRGB(v.color[1]), ObjToSRGB(v.color[2]));
        fprintf(out, "vt %.9g %.9g\nvn %.9g %.9g %.9g\n", v.uv[0], 1.0f-v.uv[1],
                v.normal[0], v.normal[1], v.normal[2]);
    }
    for (i = 0; i < count; i += 3) {
        uint32_t j, material = 0;
        for (j = 0; j < 3; j++) {
            uint32_t index;
            RageRuntimeVertex v;
            if (!RuntimeMeshIndex(mesh, first + i + j, &index) ||
                !RuntimeMeshVertex(mesh, index, &v)) return 0;
            if (j && v.material != material) return 0;
            material = v.material;
        }
        fprintf(out, "usemtl rage_%u\nf %u/%u/%u %u/%u/%u %u/%u/%u\n",
            material, i+1, i+1, i+1, i+2, i+2, i+2, i+3, i+3, i+3);
    }
    return !ferror(out);
}

#ifndef RAGE_MESH_OBJ_TEST
static int ObjMaterials(const RageRuntimeMesh *mesh, uint32_t part, FILE *out) {
    ObjArray seen = {NULL, 0, sizeof(uint32_t), 0};
    uint32_t first, count, i;
    int ok = 0;
    if (!RuntimeMeshRange(mesh, part, &first, &count)) return 0;
    for (i = 0; i < count; i += 3) {
        uint32_t index;
        RageRuntimeVertex v;
        size_t j;
        if (!RuntimeMeshIndex(mesh, first + i, &index) ||
            !RuntimeMeshVertex(mesh, index, &v)) goto done;
        for (j = 0; j < seen.count; j++)
            if (((uint32_t *)seen.data)[j] == v.material) break;
        if (j < seen.count) continue;
        if (!ObjAppend(&seen, &v.material)) goto done;
        fprintf(out, "newmtl rage_%u\nKd 1 1 1\nd 1\n\n", v.material);
    }
    ok = !ferror(out);
done:
    free(seen.data);
    return ok;
}

int main(int argc, char **argv) {
    FILE *in, *out, *mtl = NULL;
    char *materialPath = NULL;
    int ok = 0;
    if (!((argc == 5 && !strcmp(argv[1], "export")) ||
          (argc == 4 && !strcmp(argv[1], "import")))) {
        fprintf(stderr, "usage: rage-mesh-obj export bank.rmesh part output.obj\n"
                        "       rage-mesh-obj import triangulated.obj output.rmesh\n");
        return EXIT_FAILURE;
    }
    if (!strcmp(argv[2], argv[argc-1])) return EXIT_FAILURE;
    in = fopen(argv[2], "rb");
    if (!in) { perror(argv[2]); return EXIT_FAILURE; }
    out = tmpfile();
    if (!out) { perror(argv[argc-1]); fclose(in); return EXIT_FAILURE; }
    if (argc == 4) ok = ObjImport(in, out);
    else {
        long size;
        void *bytes = NULL;
        RageRuntimeMesh mesh;
        char *end;
        unsigned long part;
        errno = 0;
        part = strtoul(argv[3], &end, 10);
        if (isdigit((unsigned char)argv[3][0]) && !*end && !errno && part <= UINT32_MAX &&
            !fseek(in, 0, SEEK_END) && (size = ftell(in)) > 0 &&
            !fseek(in, 0, SEEK_SET) && (bytes = malloc((size_t)size)) != NULL &&
            fread(bytes, 1, (size_t)size, in) == (size_t)size &&
            RuntimeMeshOpen(&mesh, bytes, (size_t)size)) {
            const char *name = argv[argc-1], *p;
            materialPath = malloc(strlen(name) + 5);
            mtl = tmpfile();
            if (materialPath && mtl) {
                sprintf(materialPath, "%s.mtl", name);
                for (p = name; *p; p++) if (*p == '/' || *p == '\\') name = p + 1;
                fprintf(out, "mtllib %s.mtl\n", name);
                ok = ObjExport(&mesh, (uint32_t)part, out) &&
                    ObjMaterials(&mesh, (uint32_t)part, mtl);
            }
        }
        free(bytes);
    }
    if (fclose(in)) ok = 0;
    if (ok) {
        FILE *staged[2] = {mtl, out};
        const char *paths[2] = {materialPath, argv[argc-1]};
        size_t file;
        for (file = mtl ? 0 : 1; ok && file < 2; file++) {
            FILE *destination;
            char chunk[4096];
            size_t n;
            if (fflush(staged[file])) { ok = 0; break; }
            rewind(staged[file]);
            destination = fopen(paths[file], "wb");
            if (!destination) { perror(paths[file]); ok = 0; break; }
            while ((n = fread(chunk, 1, sizeof(chunk), staged[file])) > 0)
                if (fwrite(chunk, 1, n, destination) != n) { ok = 0; break; }
            if (ferror(staged[file]) || ferror(destination)) ok = 0;
            if (fclose(destination)) ok = 0;
        }
    }
    if (mtl && fclose(mtl)) ok = 0;
    free(materialPath);
    if (fclose(out)) ok = 0;
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
