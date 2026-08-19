/*
 * rage-extract: unpack RAGE.BIN into a directory of editable files.
 *
 * The archive holds 135 entries. An entry is rarely a single asset: most are
 * packs whose GameSceneAssetHeader lists up to eleven sub-block offsets, and
 * images sit inside those. So every entry is written out raw, and anything
 * that parses as an image chain is additionally decoded to PNG next to a JSON
 * sidecar describing where it belongs in VRAM.
 *
 * An image chain is a run of [s32 size][size bytes] links ending at a size of
 * zero or less. Each link holds a word, a flags word, an optional CLUT block
 * when flags & 8, and the pixel block. A block is
 * { u32 size; u16 x, y, w, h; u8 pixels[] } where w counts 16-bit VRAM words,
 * so the texel width depends on the depth the CLUT implies: 16 entries mean
 * 4 bits per texel, 256 mean 8.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ARCHIVE_ENTRIES 135
#define SECTOR_BYTES 2048u
#define SCENE_OFFSETS 11
#define VRAM_W 1024u
#define VRAM_H 512u

typedef struct Entry {
    uint32_t offset, size;
} Entry;

static uint32_t ReadU32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}
static uint16_t ReadU16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* ---- PNG, written with stored deflate blocks so nothing has to be linked ---- */

static uint32_t Crc32(const uint8_t *data, size_t length, uint32_t crc) {
    static uint32_t table[256];
    static int ready;
    size_t i;
    if (!ready) {
        uint32_t n, c, k;
        for (n = 0; n < 256; n++) {
            c = n;
            for (k = 0; k < 8; k++)
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[n] = c;
        }
        ready = 1;
    }
    crc = ~crc;
    for (i = 0; i < length; i++) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

static void PutU32(FILE *out, uint32_t value) {
    fputc((int)((value >> 24) & 0xFF), out);
    fputc((int)((value >> 16) & 0xFF), out);
    fputc((int)((value >> 8) & 0xFF), out);
    fputc((int)(value & 0xFF), out);
}

/* The chunk CRC covers the tag and the data together, so join them first. */
static void PngChunkJoined(FILE *out, const char *tag, const uint8_t *data,
                           size_t length) {
    uint8_t *joined = malloc(4 + length);
    if (joined == NULL) return;
    memcpy(joined, tag, 4);
    if (length) memcpy(joined + 4, data, length);
    PutU32(out, (uint32_t)length);
    fwrite(joined, 1, 4 + length, out);
    PutU32(out, Crc32(joined, 4 + length, 0));
    free(joined);
}

static int WritePng(const char *path, const uint8_t *rgba, uint32_t w,
                    uint32_t h) {
    static const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    FILE *out = fopen(path, "wb");
    uint8_t header[13];
    uint8_t *raw, *z;
    size_t rawSize, zSize, at, offset;
    uint32_t adlerA = 1, adlerB = 0, row;
    if (out == NULL) return 0;
    fwrite(signature, 1, sizeof(signature), out);

    header[0] = (uint8_t)(w >> 24); header[1] = (uint8_t)(w >> 16);
    header[2] = (uint8_t)(w >> 8);  header[3] = (uint8_t)w;
    header[4] = (uint8_t)(h >> 24); header[5] = (uint8_t)(h >> 16);
    header[6] = (uint8_t)(h >> 8);  header[7] = (uint8_t)h;
    header[8] = 8;    /* bit depth */
    header[9] = 6;    /* RGBA */
    header[10] = header[11] = header[12] = 0;
    PngChunkJoined(out, "IHDR", header, sizeof(header));

    rawSize = (size_t)h * (1 + (size_t)w * 4);
    raw = malloc(rawSize);
    if (raw == NULL) { fclose(out); return 0; }
    for (row = 0; row < h; row++) {
        raw[(size_t)row * (1 + (size_t)w * 4)] = 0; /* no filter */
        memcpy(raw + (size_t)row * (1 + (size_t)w * 4) + 1,
               rgba + (size_t)row * w * 4, (size_t)w * 4);
    }
    for (at = 0; at < rawSize; at++) {
        adlerA = (adlerA + raw[at]) % 65521u;
        adlerB = (adlerB + adlerA) % 65521u;
    }

    /* zlib header, then stored deflate blocks of at most 65535 bytes. */
    zSize = 2 + rawSize + 5 * (rawSize / 65535 + 1) + 4;
    z = malloc(zSize);
    if (z == NULL) { free(raw); fclose(out); return 0; }
    offset = 0;
    z[offset++] = 0x78; z[offset++] = 0x01;
    at = 0;
    do {
        size_t chunk = rawSize - at;
        int last;
        if (chunk > 65535) chunk = 65535;
        last = (at + chunk >= rawSize);
        z[offset++] = (uint8_t)(last ? 1 : 0);
        z[offset++] = (uint8_t)(chunk & 0xFF);
        z[offset++] = (uint8_t)(chunk >> 8);
        z[offset++] = (uint8_t)(~chunk & 0xFF);
        z[offset++] = (uint8_t)((~chunk >> 8) & 0xFF);
        if (chunk) memcpy(z + offset, raw + at, chunk);
        offset += chunk;
        at += chunk;
    } while (at < rawSize);
    z[offset++] = (uint8_t)(adlerB >> 8); z[offset++] = (uint8_t)adlerB;
    z[offset++] = (uint8_t)(adlerA >> 8); z[offset++] = (uint8_t)adlerA;

    PngChunkJoined(out, "IDAT", z, offset);
    PngChunkJoined(out, "IEND", NULL, 0);
    free(z);
    free(raw);
    fclose(out);
    return 1;
}

/* ---- PS1 colour ---- */

/* 16-bit VRAM is one semi-transparency bit and five bits each of blue, green
 * and red. A value of zero is the transparent texel every CLUT reserves. */
static void Bgr555ToRgba(uint16_t value, uint8_t *out) {
    unsigned r = value & 0x1F, g = (value >> 5) & 0x1F, b = (value >> 10) & 0x1F;
    out[0] = (uint8_t)((r * 255 + 15) / 31);
    out[1] = (uint8_t)((g * 255 + 15) / 31);
    out[2] = (uint8_t)((b * 255 + 15) / 31);
    out[3] = value == 0 ? 0 : 255;
}

/* ---- image chain ---- */

typedef struct Block {
    uint32_t size, at;
    uint16_t x, y, w, h;
} Block;

static int ReadBlock(const uint8_t *data, size_t size, size_t at, Block *block) {
    if (at + 12 > size) return 0;
    block->size = ReadU32(data + at);
    block->x = ReadU16(data + at + 4);
    block->y = ReadU16(data + at + 6);
    block->w = ReadU16(data + at + 8);
    block->h = ReadU16(data + at + 10);
    block->at = (uint32_t)(at + 12);
    if (block->size < 12 || at + block->size > size) return 0;
    if (block->w == 0 || block->h == 0) return 0;
    if (block->x + block->w > VRAM_W || block->y + block->h > VRAM_H) return 0;
    if ((size_t)block->w * block->h * 2 + 12 > block->size) return 0;
    return 1;
}

static int DecodeBlocks(const uint8_t *data, size_t size, const Block *clut,
                        const Block *pixels, const char *pngPath) {
    uint8_t *rgba;
    uint32_t texelW, x, y;
    int depth = 0;
    if (clut != NULL) depth = clut->w * clut->h == 16 ? 4 : (clut->w * clut->h == 256 ? 8 : 0);
    texelW = depth == 4 ? pixels->w * 4u : depth == 8 ? pixels->w * 2u : pixels->w;
    rgba = malloc((size_t)texelW * pixels->h * 4);
    if (rgba == NULL) return 0;
    for (y = 0; y < pixels->h; y++) {
        for (x = 0; x < texelW; x++) {
            const uint8_t *row = data + pixels->at + (size_t)y * pixels->w * 2;
            uint8_t *out = rgba + ((size_t)y * texelW + x) * 4;
            uint16_t value;
            if (depth == 4) {
                uint8_t packed = row[x / 2];
                unsigned index = (x & 1) ? (packed >> 4) : (packed & 0x0F);
                value = ReadU16(data + clut->at + index * 2);
            } else if (depth == 8) {
                value = ReadU16(data + clut->at + row[x] * 2);
            } else {
                value = ReadU16(row + x * 2);
            }
            Bgr555ToRgba(value, out);
        }
    }
    (void)size;
    if (!WritePng(pngPath, rgba, texelW, pixels->h)) { free(rgba); return 0; }
    free(rgba);
    return (int)texelW;
}

/* Walk one image chain, writing a PNG and a sidecar per link. Returns how many
 * links were decoded, or -1 when the data is not an image chain at all. */
static int ExtractImageChain(const uint8_t *data, size_t size, size_t at,
                             const char *directory, const char *stem) {
    int written = 0;
    if (at + 4 > size) return -1;
    at += 4; /* the asset opens with a word the uploader steps over */
    for (;;) {
        int32_t linkSize;
        size_t payload;
        int32_t flags;
        Block clut, pixels;
        const Block *clutPtr = NULL;
        char pngPath[1024], jsonPath[1024];
        int texelW;
        FILE *sidecar;

        if (at + 4 > size) break;
        linkSize = (int32_t)ReadU32(data + at);
        at += 4;
        if (linkSize <= 0) break;
        if ((size_t)linkSize > size - at) return written > 0 ? written : -1;
        payload = at;
        if (payload + 8 > size) return written > 0 ? written : -1;
        flags = (int32_t)ReadU32(data + payload + 4);
        payload += 8;
        if (flags & 8) {
            if (!ReadBlock(data, size, payload, &clut)) return written > 0 ? written : -1;
            clutPtr = &clut;
            payload += clut.size;
        }
        if (!ReadBlock(data, size, payload, &pixels)) return written > 0 ? written : -1;

        snprintf(pngPath, sizeof(pngPath), "%s/%s_%02d.png", directory, stem, written);
        snprintf(jsonPath, sizeof(jsonPath), "%s/%s_%02d.json", directory, stem, written);
        texelW = DecodeBlocks(data, size, clutPtr, &pixels, pngPath);
        if (texelW == 0) return written > 0 ? written : -1;
        sidecar = fopen(jsonPath, "w");
        if (sidecar != NULL) {
            fprintf(sidecar,
                    "{\n  \"vram\": { \"x\": %u, \"y\": %u, \"words\": %u, \"rows\": %u },\n"
                    "  \"depth\": %d,\n  \"width\": %d,\n  \"height\": %u",
                    pixels.x, pixels.y, pixels.w, pixels.h,
                    clutPtr == NULL ? 16 : (clut.w * clut.h == 16 ? 4 : 8),
                    texelW, pixels.h);
            if (clutPtr != NULL)
                fprintf(sidecar,
                        ",\n  \"clut\": { \"x\": %u, \"y\": %u, \"colours\": %u }",
                        clut.x, clut.y, (unsigned)(clut.w * clut.h));
            fprintf(sidecar, "\n}\n");
            fclose(sidecar);
        }
        written++;
        at += (size_t)linkSize;
    }
    return written > 0 ? written : -1;
}

static int WriteFile(const char *path, const uint8_t *data, size_t size) {
    FILE *out = fopen(path, "wb");
    if (out == NULL) return 0;
    if (size) fwrite(data, 1, size, out);
    fclose(out);
    return 1;
}

static void MakeDirectory(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0777);
#endif
}

int main(int argc, char **argv) {
    const char *archivePath, *outPath;
    FILE *archive;
    uint8_t *data;
    long size;
    char path[1024];
    Entry entries[ARCHIVE_ENTRIES];
    FILE *manifest;
    int index, images = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: rage-extract <RAGE.BIN> <output directory>\n");
        return 2;
    }
    archivePath = argv[1];
    outPath = argv[2];

    archive = fopen(archivePath, "rb");
    if (archive == NULL) {
        fprintf(stderr, "rage-extract: cannot open %s\n", archivePath);
        return 1;
    }
    fseek(archive, 0, SEEK_END);
    size = ftell(archive);
    fseek(archive, 0, SEEK_SET);
    if (size < (long)(ARCHIVE_ENTRIES * 8)) {
        fprintf(stderr, "rage-extract: %s is too small to be the archive\n", archivePath);
        fclose(archive);
        return 1;
    }
    data = malloc((size_t)size);
    if (data == NULL || fread(data, 1, (size_t)size, archive) != (size_t)size) {
        fprintf(stderr, "rage-extract: cannot read %s\n", archivePath);
        fclose(archive);
        free(data);
        return 1;
    }
    fclose(archive);

    for (index = 0; index < ARCHIVE_ENTRIES; index++) {
        entries[index].offset = ReadU32(data + index * 8) * SECTOR_BYTES;
        entries[index].size = ReadU32(data + index * 8 + 4);
    }

    MakeDirectory(outPath);
    snprintf(path, sizeof(path), "%s/raw", outPath);
    MakeDirectory(path);
    snprintf(path, sizeof(path), "%s/textures", outPath);
    MakeDirectory(path);

    snprintf(path, sizeof(path), "%s/manifest.json", outPath);
    manifest = fopen(path, "w");
    if (manifest == NULL) {
        fprintf(stderr, "rage-extract: cannot write into %s\n", outPath);
        free(data);
        return 1;
    }
    fprintf(manifest, "{\n  \"entries\": [\n");

    for (index = 0; index < ARCHIVE_ENTRIES; index++) {
        char stem[64];
        int decoded, sub;
        if (entries[index].size == 0 ||
            (size_t)entries[index].offset + entries[index].size > (size_t)size) {
            fprintf(manifest, "    { \"index\": %d, \"present\": false }%s\n",
                    index, index + 1 == ARCHIVE_ENTRIES ? "" : ",");
            continue;
        }
        snprintf(path, sizeof(path), "%s/raw/asset_%03d.bin", outPath, index);
        WriteFile(path, data + entries[index].offset, entries[index].size);

        snprintf(path, sizeof(path), "%s/textures", outPath);
        snprintf(stem, sizeof(stem), "asset_%03d", index);
        decoded = ExtractImageChain(data + entries[index].offset,
                                    entries[index].size, 0, path, stem);
        /* Packs keep their images behind the eleven scene offsets. */
        if (decoded < 0 && entries[index].size > SCENE_OFFSETS * 4) {
            for (sub = 0; sub < SCENE_OFFSETS; sub++) {
                uint32_t at = ReadU32(data + entries[index].offset + sub * 4);
                char subStem[64];
                if (at == 0 || at >= entries[index].size) continue;
                snprintf(subStem, sizeof(subStem), "asset_%03d_block_%02d", index, sub);
                if (ExtractImageChain(data + entries[index].offset,
                                      entries[index].size, at, path, subStem) > 0)
                    images++;
            }
        } else if (decoded > 0) {
            images++;
        }

        fprintf(manifest,
                "    { \"index\": %d, \"offset\": %u, \"size\": %u, \"raw\": \"raw/asset_%03d.bin\" }%s\n",
                index, entries[index].offset, entries[index].size, index,
                index + 1 == ARCHIVE_ENTRIES ? "" : ",");
    }

    fprintf(manifest, "  ]\n}\n");
    fclose(manifest);
    free(data);
    printf("rage-extract: %d entries written, %d carrying images\n",
           ARCHIVE_ENTRIES, images);
    return 0;
}
