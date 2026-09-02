#include "game/memcard.h"
#include "game/menu.h"
#include "game/save_internal.h"

#include <string.h>

static char *SaveFilePath(s32 slot) {
    return g_SaveFilePath + slot * MC_SAVE_PATH_SIZE;
}

static char *SaveTitle(s32 slot) {
    return g_SaveTitleSjis + slot * MC_SAVE_TITLE_SIZE;
}

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
    return header->fields.checksum == CalculateSaveHeaderChecksum(header);
}

s32 WriteMemoryCardSaveFile(
    char *path,
    char *title,
    GameSaveIconBlock *iconBlock,
    GameSaveHeaderRow *header,
    GameSaveBlock *saveBlock) {
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
    GameSaveIconBlock block0;
    GameSaveBlock block1;

    if (!MemoryCardSaveSlotValid(slot)) {
        return 0;
    }
    memset(&block0, 0, sizeof(block0));

    GameMenuLoadPhase = 0x1000;
    return WriteMemoryCardSaveFile(
        SaveFilePath(slot),
        SaveTitle(slot),
        &block0,
        header,
        &block1);
}

/* The header is stored twice, at 0x1280 and at 0x200; the first copy whose
 * 16-bit sum matches the complement stored in its last word wins. */
s32 ReadVerifiedSaveHeader(s32 fd, GameSaveHeaderRow *header) {
    GameMenuLoadPhase = 0x120;
    if (BiosFileSeek(fd, MC_BACKUP_HEADER_OFS, 0) < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x130;
    if (BiosFileRead(fd, header, MC_HEADER_SIZE) != MC_HEADER_SIZE) {
        return 0;
    }

    GameMenuLoadPhase = 0x140;
    if (SaveHeaderChecksumValid(header)) {
        return 1;
    }

    GameMenuLoadPhase = 0x150;
    if (BiosFileSeek(fd, MC_ICON_BLOCK_SIZE, 0) < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x160;
    if (BiosFileRead(fd, header, MC_HEADER_SIZE) != MC_HEADER_SIZE) {
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
        fd = BiosFileOpen(SaveFilePath(i), 1);
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

    if (!MemoryCardSaveSlotValid(slot)) {
        return 0;
    }
    GameMenuLoadPhase = 0x3000;
    for (tries = 0; tries < 2; tries++) {
        fd = BiosFileOpen(SaveFilePath(slot), 1);
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
    g_TeamNameLength = outHeader->fields.nameLength < SAVE_TEAM_NAME_CAPACITY
                           ? outHeader->fields.nameLength
                           : SAVE_TEAM_NAME_CAPACITY;
    for (i = 0; i < SAVE_TEAM_NAME_CAPACITY; i++) {
        g_TeamNameChars[i] = outHeader->fields.name[i];
    }
    GameMenuLoadPhase = tries | 0x3900;
    g_SaveElapsedTicks = outHeader->fields.saveCounter;
    return 1;
}


s32 CountMemoryCardFiles(s32 port, s32 slot) {
    char path[0x20];
    DirEntry *entry;
    s32 count;
    s32 pathLength;

    pathLength = snprintf(path, sizeof(path), g_FmtCardWildcard, port, slot);
    if (pathLength < 0 || (size_t)pathLength >= sizeof(path)) {
        return 0;
    }
    entry = g_McDirEntries;
    if (BiosFirstFile(path, entry) != entry) {
        return 0;
    }

    count = 1;
    while (count < MEMORY_CARD_MAX_FILES) {
        entry++;
        if (BiosNextFile(entry) != entry) {
            break;
        }
        count++;
    }
    return count;
}
