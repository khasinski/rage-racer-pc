#include "game/memcard.h"
#include "game/menu.h"

#include <string.h>

static s32 OpenSaveFileForWrite(char *path, s32 attempt) {
    s32 fd = BiosFileOpen(path, 2);

    if (fd >= 0) {
        return fd;
    }

    fd = BiosFileOpen(path, 0x10200);
    if (fd < 0) {
        GameMenuLoadPhase = attempt | 0x1520;
        return -1;
    }

    BiosFileClose(fd);
    fd = BiosFileOpen(path, 2);
    if (fd < 0) {
        GameMenuLoadPhase = attempt | 0x1510;
    }
    return fd;
}

static s32 SaveHeaderChecksumValid(const GameSaveHeaderRow *header) {
    s32 sum = 0;
    u32 i;

    for (i = 0; i < 0x3E; i++) {
        sum += header->halfwords[i];
    }
    return header->fields.checksum == (u32)~sum;
}

s32 WriteMemoryCardSaveFile(
    char *path,
    char *title,
    void *iconBlock,
    GameSaveHeaderRow *header,
    void *saveBlock) {
    s32 fd;
    s32 written;
    s32 attempt;

    GameMenuLoadPhase = 0x1100;
    BuildSaveIconBlock(iconBlock, title, 0x222, 0x3C0, 0x1F0);
    GameMenuLoadPhase = 0x1200;
    WriteSaveHeaderRow(header);
    GameMenuLoadPhase = 0x1300;
    StoreSaveStateBlock(saveBlock);
    GameMenuLoadPhase = 0x1500;

    for (attempt = 0; attempt < 2; attempt++) {
        fd = OpenSaveFileForWrite(path, attempt);
        if (fd >= 0) break;
    }
    if (fd < 0) {
        return 0;
    }

    GameMenuLoadPhase = attempt | 0x1530;
    if (BiosFileWrite(fd, iconBlock, MC_ICON_BLOCK_SIZE) !=
        MC_ICON_BLOCK_SIZE) {
        BiosFileClose(fd);
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1540;
    written = BiosFileWrite(fd, header, MC_HEADER_SIZE);
    if (written != MC_HEADER_SIZE) {
        BiosFileClose(fd);
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1550;
    if (BiosFileWrite(fd, saveBlock, MC_BLOCK_SIZE) != MC_BLOCK_SIZE) {
        BiosFileClose(fd);
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1560;
    if (BiosFileWrite(fd, header, MC_HEADER_SIZE) != MC_HEADER_SIZE) {
        BiosFileClose(fd);
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1570;
    BiosFileClose(fd);
    return 1;
}

s32 WriteMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *header) {
    u8 block0[MC_ICON_BLOCK_SIZE];
    _Alignas(GameSaveBlock) u8 block1[MC_BLOCK_SIZE];

    memset(block0, 0, sizeof(block0));

    GameMenuLoadPhase = 0x1000;
    return WriteMemoryCardSaveFile(
        g_SaveFilePath + slot * 0x1A,
        g_SaveTitleSjis + slot * 0x46,
        block0,
        header,
        block1);
}

/* The header is stored twice, at 0x1280 and at 0x200; the first copy whose
 * 16-bit sum matches the complement stored in its last word wins. */
s32 ReadVerifiedSaveHeader(s32 slot, GameSaveHeaderRow *header) {
    GameMenuLoadPhase = 0x120;
    if (BiosFileSeek(slot, MC_BACKUP_HEADER_OFS, 0) < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x130;
    if (BiosFileRead(slot, header, MC_HEADER_SIZE) != MC_HEADER_SIZE) {
        return 0;
    }

    GameMenuLoadPhase = 0x140;
    if (SaveHeaderChecksumValid(header)) {
        return 1;
    }

    GameMenuLoadPhase = 0x150;
    if (BiosFileSeek(slot, MC_ICON_BLOCK_SIZE, 0) < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x160;
    if (BiosFileRead(slot, header, MC_HEADER_SIZE) != MC_HEADER_SIZE) {
        return 0;
    }

    GameMenuLoadPhase = 0x170;
    if (SaveHeaderChecksumValid(header)) {
        return 1;
    }

    GameMenuLoadPhase = 0x180;
    return 0;
}

s32 ScanMemoryCardSaveHeaders(GameSaveHeaderRow *headers) {
    s32 fd;
    s32 i;
    s32 mask;

    mask = 0;
    GameMenuLoadPhase = 0x110;
    for (i = 0; i < MEMORY_CARD_SAVE_SLOT_COUNT; i++) {
        fd = BiosFileOpen(g_SaveFilePath + i * 0x1A, 1);
        if (fd >= 0) {
            if (ReadVerifiedSaveHeader(fd, &headers[i]) == 0) {
                mask |= 0x10000 << i;
            } else {
                mask |= 1 << i;
            }
            BiosFileClose(fd);
        }
    }

    GameMenuLoadPhase = 0x190;
    return mask;
}

s32 LoadMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *outHeader) {
    GameSaveBlock block;
    s32 tries;
    s32 fd;
    s32 i;

    GameMenuLoadPhase = 0x3000;
    for (tries = 0; tries < 2; tries++) {
        fd = BiosFileOpen(g_SaveFilePath + slot * 0x1A, 1);
        if (fd >= 0) break;
    }

    GameMenuLoadPhase = tries | 0x3100;

    if (fd < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x3300;
    if (ReadVerifiedSaveHeader(fd, outHeader) == 0) {
        BiosFileClose(fd);
        return 0;
    }

    GameMenuLoadPhase = 0x3500;
    if (BiosFileSeek(fd, MC_SAVE_BLOCK_OFS, 0) < 0) {
        BiosFileClose(fd);
        return 0;
    }

    GameMenuLoadPhase = 0x3600;
    if (BiosFileRead(fd, &block, MC_BLOCK_SIZE) != MC_BLOCK_SIZE) {
        BiosFileClose(fd);
        return 0;
    }

    BiosFileClose(fd);
    GameMenuLoadPhase = 0x3700;
    if (LoadSaveStateBlock(&block) == 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x3800;
    g_TeamNameLength = outHeader->fields.nameLength;
    for (i = 0; i < 7; i++) {
        g_TeamNameChars[i] = outHeader->fields.name[i];
    }
    GameMenuLoadPhase = tries | 0x3900;
    g_SaveElapsedTicks = outHeader->fields.saveCounter;
    return 1;
}


s32 CountMemoryCardFiles(s32 port, s32 slot) {
    char path[0x20];
    DirEntry *entry;
    void *ret;
    s32 count;

    count = 0;
    sprintf(path, g_FmtCardWildcard, port, slot);
    entry = g_McDirEntries;

    if (BiosFirstFile(path, entry) == entry) {
        do {
            count++;
            if (count >= MEMORY_CARD_MAX_FILES) break;
            entry++;
            ret = BiosNextFile(entry);
        } while (ret == entry);
    }

    return count;
}
