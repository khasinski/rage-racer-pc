#include <stdint.h>
/* Stored-deflate PNG writer adapted from tools/rage_extract.c. */
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
static int PngChunkJoined(FILE *out, const char *tag, const uint8_t *data,
                           size_t length) {
    uint8_t *joined = malloc(4 + length);
    if (joined == NULL) return 0;
    memcpy(joined, tag, 4);
    if (length) memcpy(joined + 4, data, length);
    PutU32(out, (uint32_t)length);
    fwrite(joined, 1, 4 + length, out);
    PutU32(out, Crc32(joined, 4 + length, 0));
    free(joined);
    return !ferror(out);
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
    if(!PngChunkJoined(out, "IHDR", header, sizeof(header))){fclose(out);return 0;}

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

    int ok=PngChunkJoined(out, "IDAT", z, offset)&&PngChunkJoined(out, "IEND", NULL, 0);
    free(z);
    free(raw);
    if(fclose(out))ok=0;
    return ok;
}
