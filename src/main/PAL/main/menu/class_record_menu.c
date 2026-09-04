#include "game/audio.h"
#include "game/prim.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_internal.h"

enum {
    CLASS_RECORD_MENU_TROPHIES,
    CLASS_RECORD_MENU_EXIT,
    CLASS_RECORD_MENU_ITEM_COUNT,
    CLASS_RECORD_GRID_COLUMN_COUNT = 6,
    CLASS_RECORD_GRID_BOTTOM_ROW = 1,
    CLASS_RECORD_GRID_LAST_TOP_COLUMN = 5,
};

static s32 SelectedClassRecordIndex(void) {
    return g_ScreenOffsetEditY * CLASS_RECORD_GRID_COLUMN_COUNT +
           g_ScreenOffsetEditX;
}

static void DrawClassRecordDetail(void) {
    GameOrderingTableEntry *detailOt = GamePrimaryOrderingTable(51);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);
    s32 recordIndex = SelectedClassRecordIndex();
    s32 panelX = 0xB4;
    s32 panelY = 0x38;
    s32 clears;
    s32 i;

    if (g_GameMode == OPTION_MODE_CLASS_BROWSE) {
        next = AddTilePrim(GamePrimaryOrderingTable(53), next,
                          g_ClassRecordCellPoints[recordIndex].vx - 2,
                          g_ClassRecordCellPoints[recordIndex].vy - 4,
                          0x24, 0x58, 0x89, 0xFF, 0x76);
    }
    next = GameQueueSpriteTrans(detailOt, next, 0xBC, 0x40, 0x18, 0x10,
                                0, 0x6C, 0x7F40);
    next = GameQueueSpriteTrans(detailOt, next, 0xD8, 0x40, 8, 0x10,
                                g_ScreenOffsetEditX * 8 + 8, 0x18, 0x7F40);

    if (g_ClassRecords[recordIndex].place == -1) {
        for (i = 0; i < 8; i++) {
            next = GameQueueSpriteTrans(detailOt, next,
                                        panelX + 0x30 + i * 8, panelY + 8,
                                        8, 0x10, 0x38, 0x28, 0x7F40);
        }
    } else {
        next = GameQueueSpriteTrans(
            detailOt, next, 0xE4, 0x40,
            g_ClassRecordNameSprites[recordIndex].b, 0x10,
            g_ClassRecordNameSprites[recordIndex].r,
            g_ClassRecordNameSprites[recordIndex].g, 0x7F40);
    }

    next = GameQueueSpriteTrans(detailOt, next, panelX + 8, panelY + 0x28,
                                0x44, 0x10, 0x1C, 0x6C, 0x7F40);
    clears = AddClampedMenuValue(
        g_ClassRecords[recordIndex].clears, 0, 0, 99);
    next = GameQueueSpriteTrans(detailOt, next, panelX + 100,
                                panelY + 0x28, 8, 0x10,
                                (clears / 10) << 3,
                                0x18, 0x7F40);
    next = GameQueueSpriteTrans(detailOt, next, panelX + 108,
                                panelY + 0x28, 8, 0x10,
                                (clears % 10) << 3,
                                0x18, 0x7F40);
    next = QueueDrawModePrim(detailOt, next, 0x3B);
    next = AddTilePrim(detailOt, next, panelX + 78, panelY + 47,
                       0x14, 2, 0xFF, 0xFF, 0xFF);
    next = AddTilePrim(detailOt, next, panelX, panelY, 0x7C, 0x1E, 0, 0, 0);
    next = AddTilePrim(detailOt, next, panelX, panelY + 32,
                       0x7C, 0x1E, 0, 0, 0);
    g_RenderState.packetCursor = AddTilePrim(
        detailOt, next, panelX - 1, panelY - 2,
        0x7E, 0x42, 0xFF, 0xFF, 0xFF);
}

static void DrawClassRecordGrid(void) {
    GameOrderingTableEntry *base;
    GameOrderingTableEntry *labelBase;
    u8 *next;
    s32 i;
    s32 x, y;
    s32 place;

    /* The row labels live on OPTION's text page (0x3f), while the trophy
     * cells below switch between pages 0x3e and 0x3c.  Keeping all of them
     * in OT 0 made the later trophy draw mode reinterpret TROPHIES/EXIT as
     * trophy pixels. */
    labelBase = GamePrimaryOrderingTable(51);
    next = RENDER_PRIM_CURSOR_AS(u8);
    next = GameQueueSpriteTrans(labelBase, next, 0x24, 0x38, 0x24, 0x18, 0x38, 0x90, 0x7F40);
    next = GameQueueSpriteTrans(labelBase, next, 0x24, 0x58, 0x1C, 0x18, 0xD0, 0x60, 0x7F40);
    g_RenderState.packetCursor = QueueDrawModePrim(labelBase, next, 0x3F);
    DrawMenuCursorArrow(0x14, (g_ClassRecordMenuCursor * 32) + 56);
    next = RENDER_PRIM_CURSOR_AS(u8);

    base = GamePrimaryOrderingTable(0);

    for (i = 0; i < CLASS_RECORD_COUNT; i++) {
        x = g_ClassRecordCellPoints[i].vx;
        y = g_ClassRecordCellPoints[i].vy;
        place = g_ClassRecords[i].place;
        switch (place) {
        case 1:
            next = GameQueueSprite(base, next, x, y, 0x20, 0x50,
                                   g_ClassRecordCellSprites[i].u1, g_ClassRecordCellSprites[i].v1, g_ClassRecordCellSprites[i].clut1);
            break;
        case 2:
            next = GameQueueSprite(base, next, x, y, 0x20, 0x50,
                                   g_ClassRecordCellSprites[i].u2, g_ClassRecordCellSprites[i].v2, g_ClassRecordCellSprites[i].clut2);
            break;
        case 3:
            next = GameQueueSprite(base, next, x, y, 0x20, 0x50,
                                   g_ClassRecordCellSprites[i].u2, g_ClassRecordCellSprites[i].v2, g_ClassRecordCellSprites[i].clut3);
            break;
        }
        if (g_ClassRecords[i].place <= 0) {
            next = GameQueueSprite(base + 1, next, x, y, 0x20, 0x50, 0x60, 0x70, 0x7E80);
        } else {
            next = GameQueueSprite(base + 1, next, x, y, 0x20, 0x50, 0x80, 0x70, 0x7E81);
        }
    }

    next = QueueDrawModePrim(base, next, 0x3E);
    next = QueueDrawModePrim(base + 1, next, 0x3C);
    g_RenderState.packetCursor = next;
    DrawOptionHintBar(MENU_OPTION_HINT_CLASS_RECORDS);
}

/* OPTION_MODE_CLASS_MENU: two-row menu into the class-record grid. */
void UpdateClassRecordMenu(void) {
    s32 oldCursor;
    u16 buttons;

    g_ClassRecordMenuCursor = AddClampedMenuValue(
        g_ClassRecordMenuCursor, 0, 0, CLASS_RECORD_MENU_ITEM_COUNT - 1);
    DrawClassRecordGrid();

    oldCursor = g_ClassRecordMenuCursor;
    buttons = g_PadPressed;
    if (buttons & PAD_UP) {
        g_ClassRecordMenuCursor = WrapMenuIndex(
            g_ClassRecordMenuCursor, -1, CLASS_RECORD_MENU_ITEM_COUNT);
    }
    if (buttons & PAD_DOWN) {
        g_ClassRecordMenuCursor = WrapMenuIndex(
            g_ClassRecordMenuCursor, 1, CLASS_RECORD_MENU_ITEM_COUNT);
    }
    if (oldCursor != g_ClassRecordMenuCursor) {
        PlaySoundCue(1);
    }

    buttons = g_PadPressed;
    if (buttons & PAD_CONFIRM) {
        PlaySoundCue(2);
        if (g_ClassRecordMenuCursor == CLASS_RECORD_MENU_EXIT) {
            g_GameMode = OPTION_MODE_ROOT;
        } else {
            g_GameMode = OPTION_MODE_CLASS_BROWSE;
        }
    } else if (buttons & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = OPTION_MODE_ROOT;
    }

    DrawClassRecordDetail();
}

/* OPTION_MODE_CLASS_BROWSE: moves the cursor over the eleven class cells. */
void UpdateClassRecordBrowse(void) {
    s32 oldColumn;
    s32 oldRow;
    u16 buttons;

    g_ScreenOffsetEditX = AddClampedMenuValue(
        g_ScreenOffsetEditX, 0, 0, CLASS_RECORD_GRID_LAST_TOP_COLUMN);
    g_ScreenOffsetEditY = AddClampedMenuValue(
        g_ScreenOffsetEditY, 0, 0, CLASS_RECORD_GRID_BOTTOM_ROW);
    if (g_ScreenOffsetEditX == CLASS_RECORD_GRID_LAST_TOP_COLUMN) {
        g_ScreenOffsetEditY = 0;
    }
    DrawClassRecordGrid();
    oldColumn = g_ScreenOffsetEditX;
    oldRow = g_ScreenOffsetEditY;
    buttons = g_PadPressed;

    if ((buttons & PAD_UP) && oldRow == CLASS_RECORD_GRID_BOTTOM_ROW) {
        g_ScreenOffsetEditY = 0;
    }
    if ((buttons & PAD_DOWN) && g_ScreenOffsetEditY == 0) {
        g_ScreenOffsetEditY = CLASS_RECORD_GRID_BOTTOM_ROW;
    }
    if (buttons & PAD_LEFT) {
        g_ScreenOffsetEditX = WrapMenuIndex(
            g_ScreenOffsetEditX, -1, CLASS_RECORD_GRID_COLUMN_COUNT);
    }
    if (buttons & PAD_RIGHT) {
        g_ScreenOffsetEditX = WrapMenuIndex(
            g_ScreenOffsetEditX, 1, CLASS_RECORD_GRID_COLUMN_COUNT);
    }
    if (g_ScreenOffsetEditX == CLASS_RECORD_GRID_LAST_TOP_COLUMN) {
        g_ScreenOffsetEditY = 0;
    }
    if (oldColumn != g_ScreenOffsetEditX || oldRow != g_ScreenOffsetEditY) {
        PlaySoundCue(1);
    }
    if (buttons & (PAD_CONFIRM | PAD_CANCEL)) {
        PlaySoundCue(2);
        g_GameMode = OPTION_MODE_CLASS_MENU;
    }
    DrawClassRecordDetail();
}
