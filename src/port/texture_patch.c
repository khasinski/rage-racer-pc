#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <miniz.h>

/*
 * Putting an edited PNG back into an asset. The game does this in memory as it
 * loads, and rage-pack does it to the files; both go through here so there is
 * one implementation of what "packing a texture" means.
 *
 * Which textures belong to which asset comes from textures/index.txt, written
 * by rage-extract, so nothing here has to walk a directory.
 */

/* ---- PNG ---- */

static uint32_t Be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static int Paeth(int a, int b, int c) {
    int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

/* Reads 8-bit RGB, RGBA and palette PNGs into RGBA. */
static uint8_t *ReadPng(const char *path, uint32_t *width, uint32_t *height,
                        const char **error) {
    FILE *file = fopen(path, "rb");
    uint8_t *file_data, *idat = NULL, *raw = NULL, *rgba = NULL;
    uint8_t palette[256 * 3], alpha[256];
    long size;
    size_t at, idatSize = 0, rawSize, rowSize, pixelCount;
    mz_ulong inflatedSize;
    uint32_t w = 0, h = 0;
    int depth = 0, colour = 0, channels, paletteCount = 0, y;

    *error = NULL;
    if (file == NULL) { *error = "cannot be opened"; return NULL; }
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        *error = "cannot be read";
        return NULL;
    }
    if (size < 8) { fclose(file); *error = "is not a PNG"; return NULL; }
    file_data = malloc((size_t)size);
    if (file_data == NULL || fread(file_data, 1, (size_t)size, file) != (size_t)size) {
        fclose(file); free(file_data); *error = "cannot be read"; return NULL;
    }
    fclose(file);
    if (memcmp(file_data, "\x89PNG\r\n\x1a\n", 8) != 0) {
        free(file_data); *error = "is not a PNG"; return NULL;
    }
    memset(alpha, 255, sizeof(alpha));

    at = 8;
    while (at + 8 <= (size_t)size) {
        uint32_t length = Be32(file_data + at);
        const char *tag = (const char *)file_data + at + 4;
        const uint8_t *body = file_data + at + 8;
        if ((size_t)size - at < 12 ||
            (size_t)length > (size_t)size - at - 12) break;
        if (!memcmp(tag, "IHDR", 4) && length >= 13) {
            w = Be32(body); h = Be32(body + 4);
            depth = body[8]; colour = body[9];
            if (body[12] != 0) { free(file_data); *error = "is interlaced"; return NULL; }
        } else if (!memcmp(tag, "PLTE", 4)) {
            paletteCount = (int)(length / 3);
            if (paletteCount > 256) paletteCount = 256;
            memcpy(palette, body, (size_t)paletteCount * 3);
        } else if (!memcmp(tag, "tRNS", 4)) {
            uint32_t i;
            for (i = 0; i < length && i < 256; i++) alpha[i] = body[i];
        } else if (!memcmp(tag, "IDAT", 4)) {
            uint8_t *grown;

            if ((size_t)length > SIZE_MAX - idatSize) {
                free(idat); free(file_data); *error = "is too large"; return NULL;
            }
            grown = realloc(idat, idatSize + length);
            if (grown == NULL) { free(idat); free(file_data); *error = "is too large"; return NULL; }
            idat = grown;
            memcpy(idat + idatSize, body, length);
            idatSize += length;
        } else if (!memcmp(tag, "IEND", 4)) {
            break;
        }
        at += 12 + length;
    }
    free(file_data);

    if (depth != 8) { free(idat); *error = "is not 8 bits per channel"; return NULL; }
    if (colour != 2 && colour != 6 && colour != 3) {
        free(idat); *error = "is not RGB, RGBA or palette colour"; return NULL;
    }
    channels = colour == 2 ? 3 : colour == 6 ? 4 : 1;
    if (w == 0 || h == 0 || w > INT_MAX || h > INT_MAX ||
        w > (SIZE_MAX - 1) / (size_t)channels) {
        free(idat); *error = "has invalid dimensions"; return NULL;
    }
    rowSize = 1 + (size_t)w * (size_t)channels;
    if ((size_t)h > SIZE_MAX / rowSize ||
        (size_t)w > SIZE_MAX / (size_t)h) {
        free(idat); *error = "is too large"; return NULL;
    }
    rawSize = rowSize * (size_t)h;
    pixelCount = (size_t)w * (size_t)h;
    if (rawSize > ULONG_MAX || idatSize > ULONG_MAX ||
        pixelCount > SIZE_MAX / 4) {
        free(idat); *error = "is too large"; return NULL;
    }
    if (colour == 3 && paletteCount == 0) {
        free(idat); *error = "has no palette"; return NULL;
    }
    if (idat == NULL || idatSize == 0) {
        free(idat); *error = "has no image data"; return NULL;
    }
    raw = malloc(rawSize);
    if (raw == NULL) {
        free(idat); *error = "is too large"; return NULL;
    }
    inflatedSize = (mz_ulong)rawSize;
    if (mz_uncompress(raw, &inflatedSize, idat, (mz_ulong)idatSize) != MZ_OK ||
        inflatedSize != (mz_ulong)rawSize) {
        free(raw);
        free(idat); *error = "has image data that cannot be decompressed"; return NULL;
    }
    free(idat);

    rgba = malloc(pixelCount * 4);
    if (rgba == NULL) { free(raw); *error = "is too large"; return NULL; }
    for (y = 0; y < (int)h; y++) {
        size_t stride = (size_t)w * channels;
        uint8_t *row = raw + (size_t)y * (1 + stride) + 1;
        uint8_t filter = raw[(size_t)y * (1 + stride)];
        uint8_t *previous = y ? raw + (size_t)(y - 1) * (1 + stride) + 1 : NULL;
        size_t i;
        if (filter > 4) {
            free(raw); free(rgba); *error = "uses an invalid row filter"; return NULL;
        }
        for (i = 0; i < stride; i++) {
            int a = i >= (size_t)channels ? row[i - channels] : 0;
            int b = previous ? previous[i] : 0;
            int c = (previous && i >= (size_t)channels) ? previous[i - channels] : 0;
            switch (filter) {
            case 1: row[i] = (uint8_t)(row[i] + a); break;
            case 2: row[i] = (uint8_t)(row[i] + b); break;
            case 3: row[i] = (uint8_t)(row[i] + ((a + b) >> 1)); break;
            case 4: row[i] = (uint8_t)(row[i] + Paeth(a, b, c)); break;
            default: break;
            }
        }
        for (i = 0; i < w; i++) {
            uint8_t *out = rgba + ((size_t)y * w + i) * 4;
            if (colour == 3) {
                int index = row[i] < paletteCount ? row[i] : 0;
                out[0] = palette[index * 3];
                out[1] = palette[index * 3 + 1];
                out[2] = palette[index * 3 + 2];
                out[3] = alpha[index];
            } else {
                out[0] = row[i * channels];
                out[1] = row[i * channels + 1];
                out[2] = row[i * channels + 2];
                out[3] = channels == 4 ? row[i * channels + 3] : 255;
            }
        }
    }
    free(raw);
    *width = w; *height = h;
    return rgba;
}

/* ---- sidecar ---- */

typedef struct Sidecar {
    int asset, depth, width, height;
    long pixelsOffset, clutOffset, pixelBytes, clutColours;
    int hasClut;
} Sidecar;

static int SidecarNumber(const char *text, const char *key, long *value) {
    const char *at = strstr(text, key);
    char *end;
    long parsed;

    if (at == NULL) return 0;
    at = strchr(at, ':');
    if (at == NULL) return 0;
    errno = 0;
    parsed = strtol(at + 1, &end, 10);
    if (end == at + 1 || errno == ERANGE) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end != ',' && *end != '}' && *end != '\0') return 0;
    *value = parsed;
    return 1;
}

static int ReadSidecar(const char *path, Sidecar *out) {
    FILE *file = fopen(path, "rb");
    char text[4096];
    size_t length;
    long value;
    if (file == NULL) return 0;
    length = fread(text, 1, sizeof(text) - 1, file);
    fclose(file);
    text[length] = '\0';
    memset(out, 0, sizeof(*out));
    if (!SidecarNumber(text, "\"asset\"", &value)) return 0;
    if (value < INT_MIN || value > INT_MAX) return 0;
    out->asset = (int)value;
    if (!SidecarNumber(text, "\"pixels_offset\"", &out->pixelsOffset)) return 0;
    if (!SidecarNumber(text, "\"pixel_bytes\"", &out->pixelBytes)) return 0;
    if (!SidecarNumber(text, "\"depth\"", &value)) return 0;
    if (value < INT_MIN || value > INT_MAX) return 0;
    out->depth = (int)value;
    if (!SidecarNumber(text, "\"width\"", &value)) return 0;
    if (value < INT_MIN || value > INT_MAX) return 0;
    out->width = (int)value;
    if (!SidecarNumber(text, "\"height\"", &value)) return 0;
    if (value < INT_MIN || value > INT_MAX) return 0;
    out->height = (int)value;
    if (SidecarNumber(text, "\"colours\"", &out->clutColours) &&
        SidecarNumber(text, "\"offset\"", &out->clutOffset))
        out->hasClut = 1;
    return 1;
}

/* ---- packing ---- */

static uint16_t RgbaToBgr555(const uint8_t *rgba) {
    unsigned r = (rgba[0] * 31 + 127) / 255;
    unsigned g = (rgba[1] * 31 + 127) / 255;
    unsigned b = (rgba[2] * 31 + 127) / 255;
    uint16_t value = (uint16_t)(r | (g << 5) | (b << 10));
    if (rgba[3] < 128) return 0;
    return value == 0 ? 1 : value; /* zero is the transparent texel */
}

static int NearestIndex(const uint16_t *clut, int count, const uint8_t *rgba,
                        int *exact) {
    int best = 0, bestCost = 1 << 30, i;
    if (rgba[3] < 128) { *exact = 1; return 0; }
    for (i = 0; i < count; i++) {
        int r = (clut[i] & 0x1F) * 255 / 31;
        int g = ((clut[i] >> 5) & 0x1F) * 255 / 31;
        int b = ((clut[i] >> 10) & 0x1F) * 255 / 31;
        int cost;
        if (clut[i] == 0) continue; /* reserved transparent slot */
        cost = (r - rgba[0]) * (r - rgba[0]) + (g - rgba[1]) * (g - rgba[1]) +
               (b - rgba[2]) * (b - rgba[2]);
        if (cost < bestCost) { bestCost = cost; best = i; }
    }
    *exact = bestCost <= 12;
    return best;
}

/* Decode one texel the way rage-extract does, so packing can tell an untouched
 * pixel from an edited one. */
static void DecodeTexel(const uint8_t *row, int x, int depth,
                        const uint16_t *clut, uint8_t *rgba) {
    uint16_t value;
    if (depth == 4) {
        uint8_t packed = row[x / 2];
        value = clut[(x & 1) ? (packed >> 4) : (packed & 0x0F)];
    } else if (depth == 8) {
        value = clut[row[x]];
    } else {
        value = (uint16_t)(row[x * 2] | (row[x * 2 + 1] << 8));
    }
    {
        unsigned r = value & 0x1F, g = (value >> 5) & 0x1F, b = (value >> 10) & 0x1F;
        rgba[0] = (uint8_t)((r * 255 + 15) / 31);
        rgba[1] = (uint8_t)((g * 255 + 15) / 31);
        rgba[2] = (uint8_t)((b * 255 + 15) / 31);
        rgba[3] = value == 0 ? 0 : 255;
    }
}

/* Patch one texture into an asset already in memory. */
static int PatchTexture(const char *jsonPath, const char *pngPath,
                        int assetIndex, unsigned char *data, size_t size) {
    Sidecar sidecar;
    uint8_t *rgba;
    const char *error;
    uint32_t w, h;
    uint16_t *clut = NULL;
    uint8_t *bytes;
    int x, y, inexact = 0, edited = 0;
    size_t minimumRowBytes;

    if (!ReadSidecar(jsonPath, &sidecar)) {
        fprintf(stderr, "rage-port: %s is missing fields; skipping\n", jsonPath);
        return 0;
    }
    if (sidecar.asset != assetIndex || sidecar.width <= 0 ||
        sidecar.height <= 0 ||
        (sidecar.depth != 4 && sidecar.depth != 8 && sidecar.depth != 16) ||
        sidecar.pixelsOffset < 0 || sidecar.pixelBytes <= 0 ||
        sidecar.pixelBytes % sidecar.height != 0) {
        fprintf(stderr, "rage-port: %s has invalid texture metadata; skipping\n",
                jsonPath);
        return 0;
    }
    if ((size_t)sidecar.width >
        (SIZE_MAX - 7) / (size_t)sidecar.depth) {
        fprintf(stderr, "rage-port: %s has invalid texture storage; skipping\n",
                jsonPath);
        return 0;
    }
    minimumRowBytes = ((size_t)sidecar.width * (size_t)sidecar.depth + 7) / 8;
    if ((size_t)(sidecar.pixelBytes / sidecar.height) < minimumRowBytes ||
        (sidecar.depth != 16 &&
         (!sidecar.hasClut || sidecar.clutOffset < 0 ||
          sidecar.clutColours <= 0 ||
          sidecar.clutColours > (sidecar.depth == 4 ? 16 : 256)))) {
        fprintf(stderr, "rage-port: %s has invalid texture storage; skipping\n",
                jsonPath);
        return 0;
    }
    rgba = ReadPng(pngPath, &w, &h, &error);
    if (rgba == NULL) {
        /* Absent is not a problem: a mod only carries what it changes. */
        if (strcmp(error, "cannot be opened") != 0)
            fprintf(stderr, "rage-port: %s %s\n", pngPath, error);
        return 0;
    }
    if ((int)w != sidecar.width || (int)h != sidecar.height) {
        fprintf(stderr,
                "rage-port: %s is %ux%u but the texture is %dx%d; the size is fixed by where it lives in video memory\n",
                pngPath, w, h, sidecar.width, sidecar.height);
        free(rgba);
        return 0;
    }
    if ((size_t)sidecar.pixelsOffset > size ||
        (size_t)sidecar.pixelBytes > size - (size_t)sidecar.pixelsOffset) {
        fprintf(stderr,
                "rage-port: %s expects pixels at %ld but asset %d is %zu bytes; re-extract it\n",
                jsonPath, sidecar.pixelsOffset, sidecar.asset, size);
        free(rgba);
        return 0;
    }
    if (sidecar.hasClut &&
        ((size_t)sidecar.clutOffset > size ||
         (size_t)sidecar.clutColours >
             (size - (size_t)sidecar.clutOffset) / sizeof(uint16_t))) {
        fprintf(stderr, "rage-port: %s points at a palette outside asset %d\n",
                jsonPath, sidecar.asset);
        free(rgba);
        return 0;
    }

    bytes = data + sidecar.pixelsOffset;
    if (sidecar.hasClut) clut = (uint16_t *)(void *)(data + sidecar.clutOffset);

    for (y = 0; y < sidecar.height; y++) {
        size_t rowBytes =
            (size_t)sidecar.pixelBytes / (size_t)sidecar.height;
        uint8_t *row = bytes + (size_t)y * rowBytes;

        for (x = 0; x < sidecar.width; x++) {
            const uint8_t *pixel = rgba + ((size_t)y * w + x) * 4;
            uint8_t original[4];
            int exact = 1;
            /* Leave a texel exactly as it was unless the image changed it.
             * Re-encoding an untouched pixel would drift, because palettes hold
             * duplicate colours and the nearest match is not always the index
             * that was there. */
            DecodeTexel(row, x, sidecar.depth, clut, original);
            if (original[0] == pixel[0] && original[1] == pixel[1] &&
                original[2] == pixel[2] && original[3] == pixel[3])
                continue;
            edited++;
            if (sidecar.depth == 4) {
                int index = NearestIndex(clut, (int)sidecar.clutColours, pixel, &exact);
                if (x & 1) row[x / 2] = (uint8_t)((row[x / 2] & 0x0F) | (index << 4));
                else row[x / 2] = (uint8_t)((row[x / 2] & 0xF0) | (index & 0x0F));
            } else if (sidecar.depth == 8) {
                row[x] = (uint8_t)NearestIndex(clut, (int)sidecar.clutColours, pixel, &exact);
            } else {
                uint16_t value = RgbaToBgr555(pixel);
                row[x * 2] = (uint8_t)(value & 0xFF);
                row[x * 2 + 1] = (uint8_t)(value >> 8);
            }
            if (!exact) inexact++;
        }
    }
    free(rgba);
    if (inexact)
        fprintf(stderr,
                "rage-port: %s has %d pixels outside its palette; the closest colour was used\n",
                pngPath, inexact);
    return edited > 0;
}

int TexturePatchAsset(const char *directory, int assetIndex,
                          unsigned char *data, size_t size) {
    char indexPath[1024], line[512];
    FILE *index;
    int patched = 0;

    int written;

    if (directory == NULL || directory[0] == '\0' || data == NULL || size == 0) {
        return -1;
    }
    written = snprintf(indexPath, sizeof(indexPath), "%s/textures/index.txt",
                       directory);
    if (written < 0 || (size_t)written >= sizeof(indexPath)) return -1;
    index = fopen(indexPath, "rb");
    if (index == NULL) return -1;
    while (fgets(line, sizeof(line), index)) {
        char jsonPath[1024], pngPath[1024], stem[256];
        int owner;
        size_t length;
        if (sscanf(line, "%d %255s", &owner, stem) != 2) continue;
        if (owner != assetIndex) continue;
        length = strlen(stem);
        if (length < 6) continue;
        written = snprintf(jsonPath, sizeof(jsonPath), "%s/textures/%s",
                           directory, stem);
        if (written < 0 || (size_t)written >= sizeof(jsonPath)) continue;
        written = snprintf(pngPath, sizeof(pngPath), "%s/textures/%.*s.png",
                           directory, (int)(length - 5), stem);
        if (written < 0 || (size_t)written >= sizeof(pngPath)) continue;
        patched += PatchTexture(jsonPath, pngPath, assetIndex, data, size);
    }
    fclose(index);
    return patched;
}
