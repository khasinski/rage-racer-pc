#include "game/memcard.h"
#include "game/menu.h"

s32 WriteMemoryCardSaveFile(
    char *path,
    char *title,
    void *iconBlock,
    GameSaveHeaderRow *header,
    void *saveBlock) {
    s32 fd;
    s32 prevFd;
    s32 written;
    s32 attempt;
    s32 ok;

    GameMenuLoadPhase = 0x1100;
    BuildSaveIconBlock(iconBlock, title, 0x222, 0x3C0, 0x1F0);
    GameMenuLoadPhase = 0x1200;
    WriteSaveHeaderRow(header);
    attempt = 0;
    GameMenuLoadPhase = 0x1300;
    StoreSaveStateBlock(saveBlock);
    GameMenuLoadPhase = 0x1500;

    do {
        fd = BiosFileOpen(path, 2);
        prevFd = fd;
        if (fd == -1) {
            fd = BiosFileOpen(path, 0x10200);
            if (fd == prevFd) {
                GameMenuLoadPhase = attempt | 0x1520;
            } else {
                BiosFileClose(fd);
                fd = BiosFileOpen(path, 2);
                if (fd == prevFd) {
                    GameMenuLoadPhase = attempt | 0x1510;
                }
            }
            ok = 0;
        } else {
            ok = 1;
        }

        if (ok) {
            break;
        }
        attempt++;
    } while (attempt < 2);

    if (!ok) {
        return 0;
    }

    GameMenuLoadPhase = attempt | 0x1530;
    if (BiosFileWrite(fd, iconBlock, 0x200) != 0x200) {
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1540;
    written = BiosFileWrite(fd, header, 0x80);
    if (written != 0x80) {
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1550;
    if (BiosFileWrite(fd, saveBlock, MC_BLOCK_SIZE) != MC_BLOCK_SIZE) {
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1560;
    if (BiosFileWrite(fd, header, 0x80) != written) {
        return 0;
    }
    GameMenuLoadPhase = attempt | 0x1570;
    BiosFileClose(fd);
    return 1;
}

s32 WriteMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *header) {
    u8 block0[0x200];
    _Alignas(GameSaveBlock) u8 block1[MC_BLOCK_SIZE];
    s32 i;

    for (i = 0x1FF; i >= 0; i--) {
        block0[i] = 0;
    }

    GameMenuLoadPhase = 0x1000;
    return WriteMemoryCardSaveFile(
        g_SaveFilePath + slot * 0x1A,
        g_SaveTitleSjis + slot * 0x46,
        block0,
        header,
        block1);
}

/* The header is stored twice, at 0x1280 and at 0x200; the first copy whose
 * 16-bit sum matches the complement stored in its last word wins.
 *
 * The scan is written as ptr[i] rather than *ptr++ so that gcc derives the
 * walking pointer itself: a strength-reduced giv is initialised after the
 * counter, which is the order retail emits the two moves in. */
s32 ReadVerifiedSaveHeader(s32 slot, GameSaveHeaderRow *header) {
    GameSaveHeaderRow *buffer;
    s32 sum;
    u32 i;
    u16 *ptr;

    buffer = header;
    sum = 0;

    GameMenuLoadPhase = 0x120;
    if (BiosFileSeek(slot, 0x1280, 0) < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x130;
    if (BiosFileRead(slot, buffer, 0x80) != 0x80) {
        return 0;
    }

    GameMenuLoadPhase = 0x140;
    ptr = buffer->halfwords;
    for (i = 0; i < 0x3E; i++) {
        sum += ptr[i];
    }
    /* Folding the complement back into sum is what puts it in sum's own
     * register; a `== ~sum` in the test needs a second one. */
    sum = ~sum;

    if (buffer->fields.checksum == (u32)sum) {
        return 1;
    }

    GameMenuLoadPhase = 0x150;
    if (BiosFileSeek(slot, 0x200, 0) < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x160;
    if (BiosFileRead(slot, buffer, 0x80) != 0x80) {
        return 0;
    }

    GameMenuLoadPhase = 0x170;
    sum = 0;
    ptr = buffer->halfwords;
    for (i = 0; i < 0x3E; i++) {
        sum += ptr[i];
    }
    /* Folding the complement back into sum is what puts it in sum's own
     * register; a `== ~sum` in the test needs a second one. */
    sum = ~sum;

    if (buffer->fields.checksum == (u32)sum) {
        return 1;
    }

    GameMenuLoadPhase = 0x180;
    return 0;
}

s32 ScanMemoryCardSaveHeaders(GameSaveHeaderRow *headers) {
    s32 fd;
    s32 i;
    s32 mask;
    s32 nameOffset;
    GameSaveHeaderRow *buffer;

    mask = 0;
    GameMenuLoadPhase = 0x110;
    i = 0;
    buffer = headers;
    nameOffset = 0;

    do {
        fd = BiosFileOpen(g_SaveFilePath + nameOffset, 1);
        if (fd >= 0) {
            if (ReadVerifiedSaveHeader(fd, buffer) == 0) {
                BiosFileClose(fd);
                mask |= 0x10000 << i;
            } else {
                BiosFileClose(fd);
                mask |= 1 << i;
            }
        }

        buffer++;
        i++;
        nameOffset += 0x1A;
    } while (i < 3);

    GameMenuLoadPhase = 0x190;
    return mask;
}

s32 LoadMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *outHeader) {
    GameSaveBlock block;
    GameSaveHeaderRow *header;
    s32 tries;
    s32 fd;
    s32 temp;
    s32 i;

    header = outHeader;
    GameMenuLoadPhase = 0x3000;
    tries = 0;
    temp = slot * 2;
    temp += slot;
    temp <<= 2;
    temp += slot;

    /* Written as a goto rather than a do/while on purpose: retail recomputes
     * g_SaveFilePath + nameOffset on both attempts, and any spelling the front
     * end marks as a loop lets loop.c hoist that pair of insns into the
     * preheader.  A backward goto never gets a NOTE_INSN_LOOP_BEG, so there is
     * nowhere to hoist to and the address is rebuilt each time round. */
    {
        s32 nameOffset = temp * 2;
        char *name;

    retry:
        name = g_SaveFilePath;
        name += nameOffset;
        fd = BiosFileOpen(name, 1);
        if (fd < 0) {
            tries++;
            if (tries < 2) {
                goto retry;
            }
        }
    }

    GameMenuLoadPhase = tries | 0x3100;

    if (fd < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x3300;
    if (ReadVerifiedSaveHeader(fd, header) == 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x3500;
    if (BiosFileSeek(fd, 0x280, 0) < 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x3600;
    if (BiosFileRead(fd, &block, MC_BLOCK_SIZE) != MC_BLOCK_SIZE) {
        return 0;
    }

    BiosFileClose(fd);
    GameMenuLoadPhase = 0x3700;
    if (LoadSaveStateBlock(&block) == 0) {
        return 0;
    }

    GameMenuLoadPhase = 0x3800;
    g_TeamNameLength = header->fields.nameLength;
    i = 0;
    do {
        g_TeamNameChars[i] = header->fields.name[i];
        i++;
    } while (i < 7);

    {
        s32 one = 1;
        s32 word;
        s32 status;

        word = header->fields.saveCounter;
        status = tries | 0x3900;
        GameMenuLoadPhase = status;
        g_SaveElapsedTicks = word;
        return one;
    }
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
            entry++;
            ret = BiosNextFile(entry);
        } while (ret == entry);
    }

    return count;
}
