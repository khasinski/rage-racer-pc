#include "game/audio.h"
#include "game/prim.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_internal.h"

void DrawClassRecordDetail(void) {
    GameOrderingTableEntry *base = GamePrimaryOrderingTable(51);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);
    s32 idx = g_ScreenOffsetEditY * 6 + g_ScreenOffsetEditX;
    s32 x;
    s32 y = 0x38;
    s32 i;

    if (g_GameMode == 3) {
        next = AddTilePrim(GamePrimaryOrderingTable(53), next,
                          g_ClassRecordCellPoints[idx].vx - 2, g_ClassRecordCellPoints[idx].vy - 4,
                          0x24, 0x58, 0x89, 0xFF, 0x76);
    }
    next = GameQueueSpriteTrans(base, next, 0xBC, 0x40, 0x18, 0x10, 0, 0x6C, 0x7F40);
    next = GameQueueSpriteTrans(base, next, 0xD8, 0x40, 8, 0x10, g_ScreenOffsetEditX * 8 + 8, 0x18, 0x7F40);

    x = 0xB4;
    if (g_ClassRecords[idx].place == -1) {
        for (i = 0; i < 8; i++) {
            next = GameQueueSpriteTrans(base, next, x + 0x30 + i * 8, y + 8, 8, 0x10, 0x38, 0x28, 0x7F40);
        }
    } else {
        next = GameQueueSpriteTrans(base, next, 0xE4, 0x40,
                                    g_ClassRecordNameSprites[idx].b, 0x10,
                                    g_ClassRecordNameSprites[idx].r, g_ClassRecordNameSprites[idx].g, 0x7F40);
    }

    next = GameQueueSpriteTrans(base, next, x + 8, y + 0x28, 0x44, 0x10, 0x1C, 0x6C, 0x7F40);
    next = GameQueueSpriteTrans(base, next, x + 100, y + 0x28, 8, 0x10,
                                (s16)((s16)g_ClassRecords[idx].clears / 10) << 3, 0x18, 0x7F40);
    next = GameQueueSpriteTrans(base, next, x + 108, y + 0x28, 8, 0x10,
                                (s16)((s16)g_ClassRecords[idx].clears % 10) << 3, 0x18, 0x7F40);
    next = QueueDrawModePrim(base, next, 0x3B);
    next = AddTilePrim(base, next, x + 78, y + 47, 0x14, 2, 0xFF, 0xFF, 0xFF);
    next = AddTilePrim(base, next, x, y, 0x7C, 0x1E, 0, 0, 0);
    next = AddTilePrim(base, next, x, y + 32, 0x7C, 0x1E, 0, 0, 0);
    RENDER_PRIM_CURSOR_AS(u8) = AddTilePrim(base, next, x - 1, y - 2, 0x7E, 0x42, 0xFF, 0xFF, 0xFF);
}

void DrawClassRecordGrid(void) {
    GameOrderingTableEntry *base;
    GameOrderingTableEntry *labelBase;
    u8 *next;
    s32 i;
    s32 x, y;
    s32 flag;

    /* The row labels live on OPTION's text page (0x3f), while the trophy
     * cells below switch between pages 0x3e and 0x3c.  Keeping all of them
     * in OT 0 made the later trophy draw mode reinterpret TROPHIES/EXIT as
     * trophy pixels. */
    labelBase = GamePrimaryOrderingTable(51);
    next = RENDER_PRIM_CURSOR_AS(u8);
    next = GameQueueSpriteTrans(labelBase, next, 0x24, 0x38, 0x24, 0x18, 0x38, 0x90, 0x7F40);
    next = GameQueueSpriteTrans(labelBase, next, 0x24, 0x58, 0x1C, 0x18, 0xD0, 0x60, 0x7F40);
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(labelBase, next, 0x3F);
    DrawMenuCursorArrow(0x14, (g_ClassRecordMenuCursor * 32) + 56);
    next = RENDER_PRIM_CURSOR_AS(u8);

    base = GamePrimaryOrderingTable(0);

    for (i = 0; i < 11; i++) {
        x = g_ClassRecordCellPoints[i].vx;
        y = g_ClassRecordCellPoints[i].vy;
        flag = g_ClassRecords[i].place;
        switch (flag) {
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
    RENDER_PRIM_CURSOR_AS(u8) = next;
    DrawOptionHintBar(0);
}

/* g_GameModeHandlers[2]: two-row menu into the class-record grid. */
void UpdateClassRecordMenu(void) {
    s32 oldCursor;
    u16 buttons;

    DrawClassRecordGrid();

    oldCursor = g_ClassRecordMenuCursor;
    buttons = g_PadPressed;
    if (buttons & PAD_UP) {
        g_ClassRecordMenuCursor = oldCursor - 1;
    }
    if (buttons & PAD_DOWN) {
        g_ClassRecordMenuCursor++;
    }

    g_ClassRecordMenuCursor = (g_ClassRecordMenuCursor + 2) % 2;
    if (oldCursor != g_ClassRecordMenuCursor) {
        PlaySoundCue(1);
    }

    buttons = g_PadPressed;
    if (buttons & PAD_CONFIRM) {
        PlaySoundCue(2);
        if (g_ClassRecordMenuCursor != 0) {
            g_GameMode = 1;
        } else {
            g_GameMode = 3;
        }
    } else if (buttons & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = 1;
    }

    DrawClassRecordDetail();
}

/* g_GameModeHandlers[3]: moves the cursor over the eleven class cells. */
void UpdateClassRecordBrowse(void) {
    s32 oldCursor;
    s32 oldFlag;
    u16 b;
    DrawClassRecordGrid();
    oldCursor = g_ScreenOffsetEditX;
    oldFlag = g_ScreenOffsetEditY;
    if ((g_PadPressed & PAD_UP) && oldFlag == 1) {
        g_ScreenOffsetEditY = 0;
    }
    if ((g_PadPressed & PAD_DOWN) && g_ScreenOffsetEditY == 0) {
        g_ScreenOffsetEditY = 1;
    }
    b = g_PadPressed;
    if (b & PAD_LEFT) {
        g_ScreenOffsetEditX--;
    }
    if (b & PAD_RIGHT) {
        g_ScreenOffsetEditX++;
    }
    g_ScreenOffsetEditX = (g_ScreenOffsetEditX + 6) % 6;
    if (g_ScreenOffsetEditX == 5) {
        g_ScreenOffsetEditY = 0;
    }
    if (oldCursor != g_ScreenOffsetEditX ||
        oldFlag != g_ScreenOffsetEditY) {
        PlaySoundCue(1);
    }
    if (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) {
        PlaySoundCue(2);
        g_GameMode = 2;
    }
    DrawClassRecordDetail();
}
