#include "rage/jp_text.h"

#include <string.h>

enum {
    PSX_EXE_HEADER = 0x800,
    JP_MC_MESSAGE_COUNT = 20,
    JP_MC_BANNER_FIRST = 16,
    JP_MC_ROW_LIMIT = 16,   /* longest retail message has 11 glyphs */
    JP_MC_COLUMN_COUNT = 5,
};

typedef struct JpMemoryCardTable {
    int active;
    JpTextGlyph rows[JP_MC_MESSAGE_COUNT][JP_MC_ROW_LIMIT];
    int columnX[JP_MC_COLUMN_COUNT];
} JpMemoryCardTable;

static JpMemoryCardTable s_mc;
static int s_japanese;

static unsigned int Le32(const unsigned char *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static int Le16s(const unsigned char *p) {
    return (short)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

typedef struct ExeImage {
    const unsigned char *data;
    size_t size;
    unsigned int base; /* load address of the first byte after the header */
} ExeImage;

static const unsigned char *ExeAt(const ExeImage *exe, unsigned int address,
                                  size_t length) {
    size_t offset;
    if (address < exe->base) return NULL;
    offset = (size_t)(address - exe->base) + PSX_EXE_HEADER;
    if (offset > exe->size || exe->size - offset < length) return NULL;
    return exe->data + offset;
}

/* The three memory card banners ("LOAD/SAVE/FORMAT DATA OK") are one glyph
 * each, a 108-pixel strip at v = 0, 24 and 48 of the phrase sheet. Nothing
 * else in the executable looks like those twelve bytes, and the message
 * pointer table follows the glyph runs and points 16..18 at them. */
static int FindMemoryCardTable(const ExeImage *exe) {
    static const unsigned char banners[12] = {
        0, 0, 108, 0, 0, 24, 108, 0, 0, 48, 108, 0,
    };
    size_t offset;
    size_t span = exe->size - PSX_EXE_HEADER;

    if (exe->size <= PSX_EXE_HEADER + sizeof(banners)) return 0;
    for (offset = 0; offset + sizeof(banners) <= span; offset += 4) {
        unsigned int banner;
        unsigned int scan;
        if (memcmp(exe->data + PSX_EXE_HEADER + offset, banners,
                   sizeof(banners)) != 0)
            continue;
        banner = exe->base + (unsigned int)offset;
        for (scan = banner + 12; scan < banner + 0x100; scan += 4) {
            const unsigned char *table =
                ExeAt(exe, scan, JP_MC_MESSAGE_COUNT * 4 + 10);
            int message;
            if (table == NULL) break;
            if (Le32(table + JP_MC_BANNER_FIRST * 4) != banner ||
                Le32(table + (JP_MC_BANNER_FIRST + 1) * 4) != banner + 4 ||
                Le32(table + (JP_MC_BANNER_FIRST + 2) * 4) != banner + 8)
                continue;
            memset(&s_mc, 0, sizeof(s_mc));
            for (message = 0; message < JP_MC_MESSAGE_COUNT; message++) {
                unsigned int row = Le32(table + message * 4);
                int glyph;
                for (glyph = 0; glyph < JP_MC_ROW_LIMIT; glyph++) {
                    const unsigned char *g = ExeAt(exe, row + glyph * 4u, 4);
                    if (g == NULL) return 0;
                    s_mc.rows[message][glyph].u = g[0];
                    s_mc.rows[message][glyph].v = g[1];
                    s_mc.rows[message][glyph].width = g[2];
                    s_mc.rows[message][glyph].column = g[3];
                    if (g[3] == 0) break;
                    if (g[3] >= JP_MC_COLUMN_COUNT) return 0;
                }
                if (glyph == JP_MC_ROW_LIMIT) return 0;
            }
            /* The column table shares its first word with the last
             * pointer; only entries 2..4 are ever read. */
            for (message = 2; message < JP_MC_COLUMN_COUNT; message++) {
                s_mc.columnX[message] =
                    Le16s(table + JP_MC_MESSAGE_COUNT * 4 - 4 + message * 2);
            }
            s_mc.active = 1;
            return 1;
        }
    }
    return 0;
}

void JpTextReset(void) {
    memset(&s_mc, 0, sizeof(s_mc));
    s_japanese = 0;
}

int JpTextActive(void) {
    return s_japanese;
}

int JpTextAdoptExecutable(const unsigned char *data, size_t size) {
    ExeImage exe;
    JpTextReset();
    if (data == NULL || size <= PSX_EXE_HEADER ||
        memcmp(data, "PS-X EXE", 8) != 0)
        return 0;
    exe.data = data;
    exe.size = size;
    exe.base = Le32(data + 0x18);
    s_japanese = 1;
    return FindMemoryCardTable(&exe);
}

int JpTextMemoryCardActive(void) {
    return s_mc.active;
}

const JpTextGlyph *JpTextMemoryCardRow(int message) {
    if (!s_mc.active || message < 0 || message >= JP_MC_MESSAGE_COUNT)
        return NULL;
    return s_mc.rows[message];
}

int JpTextMemoryCardColumnX(int column) {
    if (column < 2 || column >= JP_MC_COLUMN_COUNT) return 0;
    return s_mc.columnX[column];
}
