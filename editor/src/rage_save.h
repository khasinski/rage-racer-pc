#ifndef RAGE_SAVE_H
#define RAGE_SAVE_H

/*
 * A Rage Racer save, as the port writes it.
 *
 * The layout comes from the game's own header rather than a copy of it, so a
 * field added to the save cannot go missing here without the build noticing.
 */

#include "game/save_format.h"

#include <stddef.h>

enum {
    RAGE_SAVE_ICON_SIZE = 0x200,
    RAGE_SAVE_FILE_SIZE = 0x1300,
    RAGE_SAVE_SLOTS = 3,
    /* Four pixels to a halfword, sixteen halfwords to a row. */
    RAGE_LOGO_WIDTH = 64,
    RAGE_LOGO_HEIGHT = 64,
    RAGE_TEAM_NAME_LENGTH = 7,
};

/*
 * Which release a save belongs to. Only the file's name on the card and the
 * title shown in the console's memory card screen differ; the fields below
 * are the same three ways round.
 */
typedef enum RageRegion {
    RAGE_REGION_UNKNOWN = 0,
    RAGE_REGION_PAL,
    RAGE_REGION_NTSC_U,
    RAGE_REGION_NTSC_J
} RageRegion;

typedef struct RageRegionInfo {
    RageRegion region;
    const char *name;      /* "PAL" */
    const char *serial;    /* "SCES-006.50" */
    const char *cardPrefix;/* "BESCES-00650" */
    int verified;          /* 0 when the serial could not be checked here */
} RageRegionInfo;

const RageRegionInfo *RageRegionTable(size_t *count);
const RageRegionInfo *RageRegionFind(RageRegion region);
/* Reads the region out of a memory card file name, ignoring directories. */
RageRegion RageRegionFromPath(const char *path);
/* "BESCES-00650 RAGE001" for slot 1 of a PAL save. */
int RageRegionCardName(RageRegion region, int slot, char *out, size_t size);

/* The file is one icon block, a header, the payload and the header again. */
typedef struct RageSaveFile {
    unsigned char icon[RAGE_SAVE_ICON_SIZE];
    GameSaveHeaderRow header;
    GameSaveBlock block;
    GameSaveHeaderRow trailer;
} RageSaveFile;

typedef enum RageSaveStatus {
    RAGE_SAVE_OK = 0,
    RAGE_SAVE_UNREADABLE,
    RAGE_SAVE_WRONG_SIZE,
    RAGE_SAVE_NOT_A_SAVE
} RageSaveStatus;

typedef struct RageSaveReport {
    RageSaveStatus status;
    char detail[256];
    RageRegion region;
    /* Each is 1 when the stored sum matches what the contents add up to. */
    int iconRecognised;
    int headerChecksumValid;
    int blockChecksumValid;
    int trailerChecksumValid;
    int trailerMatchesHeader;
} RageSaveReport;

unsigned int RageSaveHeaderChecksum(const GameSaveHeaderRow *row);
unsigned int RageSaveBlockChecksum(const GameSaveBlock *block);

/* Reads the file and says what is wrong with it rather than refusing it: a
 * save with a broken checksum is exactly the one worth opening in an editor. */
int RageSaveLoad(const char *path, RageSaveFile *save, RageSaveReport *report);
/* Recomputes every checksum, then writes. */
int RageSaveStore(const char *path, RageSaveFile *save, RageSaveReport *report);
void RageSaveRefresh(RageSaveFile *save);
void RageSaveCheck(const RageSaveFile *save, RageSaveReport *report);
/* A blank card file with the icon header and defaults the game accepts. */
void RageSaveInit(RageSaveFile *save, RageRegion region, int slot);

/* The team name is stored as indices into the game's own character set. */
extern const char kRageNameCharset[];
size_t RageNameCharsetSize(void);
void RageSaveReadTeamName(const GameSaveHeaderRow *row, char *out, size_t size);
void RageSaveWriteTeamName(GameSaveHeaderRow *row, const char *text);

/*
 * Saves the port has already written on this machine, so the usual answer to
 * "which file" is a list to pick from rather than a file dialog.
 */
typedef struct RageSaveEntry {
    char path[1024];
    char team[RAGE_TEAM_NAME_LENGTH + 1];
    RageRegion region;
    int slot;
    int money;
    int valid;      /* 0 when it loaded but its checksums disagree */
} RageSaveEntry;

enum { RAGE_SAVE_DISCOVER_MAX = 32 };

/* Returns how many were found, up to RAGE_SAVE_DISCOVER_MAX. */
int RageSaveDiscover(RageSaveEntry *entries, int max);
/* Where this platform keeps them, for showing and for saving into. */
int RageSaveCardDirectory(int card, char *out, size_t size);
/* Reads the slot back out of a "... RAGE002" name; -1 when there is none. */
int RageSaveSlotFromPath(const char *path);

/* Team logo: sixteen colours, and one nibble per pixel. */
int RageLogoPixel(const GameSaveBlock *block, int x, int y);
void RageLogoSetPixel(GameSaveBlock *block, int x, int y, int colour);
/* Expands a stored BGR555 entry into three bytes plus its transparency bit. */
void RageLogoColour(unsigned short entry, unsigned char *rgb, int *transparent);
unsigned short RageLogoPackColour(const unsigned char *rgb, int transparent);

#endif
