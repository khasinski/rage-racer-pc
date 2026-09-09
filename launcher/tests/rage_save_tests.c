/*
 * What the editor must get right about a save file.
 *
 * The checksums are checked against the game's own routine rather than
 * against a number written down here: an editor that agrees with itself and
 * not with the game produces files the game refuses to load.
 */

#include "rage_save.h"
#include "game/save_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int s_failures;

static void Check(int condition, const char *what) {
    if (!condition) {
        printf("FAIL %s\n", what);
        s_failures++;
    }
}

static void CheckEqual(unsigned long got, unsigned long want, const char *what) {
    if (got != want) {
        printf("FAIL %s: got %lu, expected %lu\n", what, got, want);
        s_failures++;
    }
}

/* Something with a bit set in most fields, so a layout mistake shows up. */
static void FillSave(RageSaveFile *save) {
    unsigned char *bytes = (unsigned char *)&save->block;
    size_t i;

    RageSaveInit(save, RAGE_REGION_PAL, 0);
    for (i = 0; i < sizeof(save->block); i++) bytes[i] = (unsigned char)(i * 7);
    save->block.checksum = 0;
    RageSaveRefresh(save);
}

static void TestChecksumsAgreeWithTheGame(void) {
    RageSaveFile save;

    FillSave(&save);
    CheckEqual(RageSaveBlockChecksum(&save.block),
               CalculateSaveBlockChecksum(&save.block),
               "the payload checksum matches the game's");
    CheckEqual(save.block.checksum, CalculateSaveBlockChecksum(&save.block),
               "refreshing stores the checksum the game would compute");
}

static void TestRoundTrip(void) {
    RageSaveFile written;
    RageSaveFile read;
    RageSaveReport report;
    const char *path = "rage-editor-roundtrip.sav";

    FillSave(&written);
    RageSaveWriteTeamName(&written.header, "AYA");
    written.block.grandPrixProgress.money = 123456789;
    written.block.bgmSelection = 3;
    RageSaveRefresh(&written);

    Check(RageSaveStore(path, &written, &report), "the save writes");
    Check(RageSaveLoad(path, &read, &report), "the save reads back");
    Check(report.status == RAGE_SAVE_OK, "a written save reads as valid");
    Check(report.headerChecksumValid, "the header checksum survives");
    Check(report.blockChecksumValid, "the payload checksum survives");
    Check(report.trailerChecksumValid, "the repeated header is checksummed");
    Check(report.trailerMatchesHeader, "the repeated header is a copy");
    Check(memcmp(&written, &read, sizeof(written)) == 0,
          "every byte comes back unchanged");
    CheckEqual((unsigned long)read.block.grandPrixProgress.money, 123456789,
               "a field survives the round trip");
    remove(path);
}

static void TestBrokenChecksumStillOpens(void) {
    RageSaveFile save;
    RageSaveFile read;
    RageSaveReport report;
    const char *path = "rage-editor-broken.sav";

    FillSave(&save);
    save.block.checksum ^= 0xFFFFu;
    /* Write the file by hand: RageSaveStore would repair it. */
    {
        FILE *stream = fopen(path, "wb");
        Check(stream != NULL, "the damaged file opens for writing");
        if (stream != NULL) {
            fwrite(&save, 1, sizeof(save), stream);
            fclose(stream);
        }
    }
    Check(RageSaveLoad(path, &read, &report),
          "a save with a bad checksum still loads, which is the one worth "
          "opening in an editor");
    Check(!report.blockChecksumValid, "and the bad checksum is reported");
    remove(path);
}

static void TestRefusesWhatIsNotASave(void) {
    RageSaveFile save;
    RageSaveReport report;
    const char *path = "rage-editor-notasave.sav";
    FILE *stream = fopen(path, "wb");
    unsigned char junk[RAGE_SAVE_FILE_SIZE];

    memset(junk, 0xAB, sizeof(junk));
    if (stream != NULL) {
        fwrite(junk, 1, sizeof(junk), stream);
        fclose(stream);
    }
    Check(!RageSaveLoad(path, &save, &report), "a file with no card signature "
                                               "is refused");
    Check(report.status == RAGE_SAVE_NOT_A_SAVE, "and says why");
    remove(path);

    stream = fopen(path, "wb");
    if (stream != NULL) {
        fwrite(junk, 1, 64, stream);
        fclose(stream);
    }
    Check(!RageSaveLoad(path, &save, &report), "a short file is refused");
    Check(report.status == RAGE_SAVE_WRONG_SIZE, "and says it is the size");
    remove(path);
}

static void TestRegions(void) {
    char name[64];

    CheckEqual(RageRegionFromPath("bu00/BESCES-00650 RAGE000"),
               RAGE_REGION_PAL, "the PAL card name is recognised");
    CheckEqual(RageRegionFromPath("/tmp/BASLUS-00403 RAGE002"),
               RAGE_REGION_NTSC_U, "the American card name is recognised");
    CheckEqual(RageRegionFromPath("BISLPS-00600 RAGE001"),
               RAGE_REGION_NTSC_J, "the Japanese card name is recognised");
    /* An unfamiliar serial still says which territory it came from. */
    CheckEqual(RageRegionFromPath("BISLPS-99999 RAGE000"), RAGE_REGION_NTSC_J,
               "an unknown Japanese serial is still Japanese");
    CheckEqual(RageRegionFromPath("savegame.bin"), RAGE_REGION_UNKNOWN,
               "an ordinary file name says nothing");

    Check(RageRegionCardName(RAGE_REGION_PAL, 1, name, sizeof(name)),
          "a PAL card name is built");
    Check(strcmp(name, "BESCES-00650 RAGE001") == 0,
          "and it is the name the game writes");
    Check(!RageRegionCardName(RAGE_REGION_PAL, 3, name, sizeof(name)),
          "there is no fourth slot");

    /* The slot is read back out of the same name, so opening a file does not
     * have to ask which one it is. */
    CheckEqual((unsigned long)(RageSaveSlotFromPath("bu00/BESCES-00650 RAGE000") + 1),
               1, "slot zero is read from the file name");
    CheckEqual((unsigned long)(RageSaveSlotFromPath("BISLPS-00600 RAGE001") + 1),
               2, "slot one is read from the file name");
    CheckEqual((unsigned long)(RageSaveSlotFromPath("/tmp/BASLUS-00403 RAGE002") + 1),
               3, "slot two is read from the file name");
    CheckEqual((unsigned long)(RageSaveSlotFromPath("savegame.bin") + 1), 0,
               "a name with no slot in it says so");
}

/* A new save must be one the game would have written itself. */
static void TestNewSaveDefaults(void) {
    RageSaveFile save;
    int i;
    int owned = 0;

    RageSaveInit(&save, RAGE_REGION_PAL, 0);
    CheckEqual(save.block.negconSteerPlay, 1,
               "the neGcon steering keeps the play the game gives it");
    CheckEqual((unsigned long)save.block.bgmVolume, 0xF,
               "music starts at full volume");
    CheckEqual((unsigned long)save.block.sfxVolume, 0xF,
               "effects start at full volume");
    CheckEqual((unsigned long)save.block.grandPrixProgress.carIndex, 3,
               "the car you start in is the Gnade");
    CheckEqual((unsigned long)(save.block.grandPrixProgress.maxClassReached + 1),
               0, "no class has been entered yet");
    for (i = 0; i < 13; i++) owned += save.block.carSetup[0][i].enabled != 0;
    CheckEqual((unsigned long)owned, 1, "exactly one car is owned to begin");
    Check(save.block.carSetup[0][3].enabled != 0, "and it is the Gnade");
    CheckEqual((unsigned long)save.block.classRecords[1].grade, 0xFFFF,
               "an empty class record is empty, not a first place");
    CheckEqual((unsigned long)save.block.grandPrixCourseProgress[6], 5,
               "five retries are left");
}

static void TestCarNames(void) {
    Check(strcmp(RageCarName(0, RAGE_REGION_PAL), "Erriso") == 0,
          "car zero is the Erriso outside Japan");
    Check(strcmp(RageCarName(0, RAGE_REGION_NTSC_J), "Alouette") == 0,
          "and the Alouette in Japan");
    Check(strcmp(RageCarName(4, RAGE_REGION_NTSC_U), "Acceron") == 0,
          "the American release keeps the international name");
    Check(strcmp(RageCarName(12, RAGE_REGION_NTSC_J), "Dragone") == 0,
          "the last car is renamed too");
    Check(strcmp(RageCarName(1, RAGE_REGION_PAL),
                 RageCarName(1, RAGE_REGION_NTSC_J)) == 0,
          "a car with one name has it in both releases");
    Check(RageCarName(13, RAGE_REGION_PAL) == NULL, "there is no car thirteen");
    Check(strcmp(RageCarMaker(3), "Gnade") == 0,
          "the car you start in is a Gnade");
}

/*
 * A memory card, built here rather than shipped: a DexDrive header, the card
 * signature, one directory entry and a save in the block it names.
 */
static void BuildCard(const char *path, int withDexHeader) {
    static const size_t kDex = 3904;
    static const size_t kImage = 8192 * 16;
    unsigned char *file;
    unsigned char *image;
    unsigned char *entry;
    RageSaveFile save;
    FILE *stream;
    size_t size = kImage + (withDexHeader ? kDex : 0);

    file = calloc(1, size);
    if (file == NULL) return;
    if (withDexHeader) memcpy(file, "123-456-STD", 11);
    image = file + (withDexHeader ? kDex : 0);
    memcpy(image, "MC", 2);

    /* Block three holds the save, and the entry for it is frame three. */
    entry = image + 3 * 128;
    entry[0] = 0x51;
    entry[4] = 0x00;
    entry[5] = 0x20;   /* 8192, little endian */
    entry[8] = 0xFF;
    entry[9] = 0xFF;
    memcpy(entry + 10, "BASLUS-00403 RAGE001", 20);

    RageSaveInit(&save, RAGE_REGION_NTSC_U, 1);
    RageSaveWriteTeamName(&save.header, "DEX");
    save.block.grandPrixProgress.money = 4242;
    RageSaveRefresh(&save);
    memcpy(image + 3 * 8192, &save, sizeof(save));

    stream = fopen(path, "wb");
    if (stream != NULL) {
        fwrite(file, 1, size, stream);
        fclose(stream);
    }
    free(file);
}

static void TestMemoryCard(void) {
    const char *path = "rage-editor-card.gme";
    RageCard card;
    RageSaveReport report;
    RageSaveFile save;

    BuildCard(path, 1);
    Check(RageCardLooksLikeCard(path), "a DexDrive file is recognised by size");
    Check(RageCardLoad(path, &card, &report), "the card loads");
    CheckEqual((unsigned long)card.count, 1, "one Rage Racer save is found");
    Check(card.fromDexDrive, "and the DexDrive header is noticed");
    CheckEqual((unsigned long)card.entries[0].block, 3, "in the block the "
                                                       "directory names");
    CheckEqual((unsigned long)card.entries[0].region, RAGE_REGION_NTSC_U,
               "the release comes from the name on the card");
    CheckEqual((unsigned long)card.entries[0].slot, 1,
               "and so does the slot");
    Check(strcmp(card.entries[0].team, "DEX") == 0, "the team name is read");
    CheckEqual((unsigned long)card.entries[0].money, 4242,
               "and so is what it holds");
    Check(card.entries[0].valid, "the save inside passes its checksums");

    Check(RageCardRead(&card, 0, &save), "the save comes out of the card");
    save.block.grandPrixProgress.money = 777;
    Check(RageCardWrite(&card, 0, &save), "and goes back into it");
    Check(RageCardStore(path, &card, &report), "the card writes");
    RageCardFree(&card);

    /* Everything else on the card has to survive being written through. */
    Check(RageCardLoad(path, &card, &report), "the card loads again");
    Check(RageCardRead(&card, 0, &save), "the save is still there");
    CheckEqual((unsigned long)save.block.grandPrixProgress.money, 777,
               "with the change kept");
    {
        FILE *stream = fopen(path, "rb");
        char magic[12] = {0};
        if (stream != NULL) {
            fread(magic, 1, 11, stream);
            fclose(stream);
        }
        Check(strcmp(magic, "123-456-STD") == 0,
              "and the DexDrive header in front of it untouched");
    }
    RageCardFree(&card);
    remove(path);

    /* A card dumped without that header is the same card. */
    BuildCard(path, 0);
    Check(RageCardLooksLikeCard(path), "a raw card image is recognised too");
    Check(RageCardLoad(path, &card, &report), "and loads");
    Check(!card.fromDexDrive, "without claiming a header it does not have");
    CheckEqual((unsigned long)card.count, 1, "with the save on it");
    RageCardFree(&card);
    remove(path);

    /* One save is not a card. */
    Check(!RageCardLooksLikeCard("rage-editor-tests.c"),
          "an ordinary file is not mistaken for a card");
}

static void TestTeamName(void) {
    GameSaveHeaderRow row;
    char text[16];

    memset(&row, 0, sizeof(row));
    RageSaveWriteTeamName(&row, "ayato");
    RageSaveReadTeamName(&row, text, sizeof(text));
    Check(strcmp(text, "AYATO") == 0, "a name is stored in upper case");
    CheckEqual(row.fields.nameLength, 5, "and its length is stored");

    RageSaveWriteTeamName(&row, "TOOLONGNAME");
    RageSaveReadTeamName(&row, text, sizeof(text));
    CheckEqual(strlen(text), RAGE_TEAM_NAME_LENGTH,
               "a long name is cut to what the game holds");

    /* The game has no lower case and no comma; both become a space. */
    RageSaveWriteTeamName(&row, "A,B");
    RageSaveReadTeamName(&row, text, sizeof(text));
    Check(strcmp(text, "A B") == 0, "a letter the game cannot spell becomes a "
                                    "space rather than a hole");
}

static void TestTeamLogo(void) {
    RageSaveFile save;
    int x, y;
    int mismatches = 0;

    RageSaveInit(&save, RAGE_REGION_PAL, 0);
    for (y = 0; y < RAGE_LOGO_HEIGHT; y++)
        for (x = 0; x < RAGE_LOGO_WIDTH; x++)
            RageLogoSetPixel(&save.block, x, y, (x + y) & 0xF);
    for (y = 0; y < RAGE_LOGO_HEIGHT; y++)
        for (x = 0; x < RAGE_LOGO_WIDTH; x++)
            if (RageLogoPixel(&save.block, x, y) != ((x + y) & 0xF))
                mismatches++;
    CheckEqual((unsigned long)mismatches, 0,
               "every logo pixel reads back as it was written");

    /* Four pixels to a halfword, so the canvas is half a byte a pixel and
     * has to come out exactly the size of the field that holds it. */
    CheckEqual(sizeof(save.block.teamLogoCanvas),
               RAGE_LOGO_WIDTH * RAGE_LOGO_HEIGHT / 2,
               "the canvas fills its field");
}

static void TestLogoColours(void) {
    unsigned char rgb[3] = {0xF8, 0x00, 0x08};
    unsigned short packed = RageLogoPackColour(rgb, 0);
    unsigned char back[3];
    int transparent = 1;

    RageLogoColour(packed, back, &transparent);
    Check(back[0] > 0xF0 && back[1] == 0 && back[2] < 0x20,
          "a colour survives being packed into five bits a channel");
    Check(!transparent, "and its transparency bit stays clear");

    RageLogoColour(RageLogoPackColour(rgb, 1), back, &transparent);
    Check(transparent, "a transparent colour stays transparent");
}

int main(void) {
    TestChecksumsAgreeWithTheGame();
    TestRoundTrip();
    TestBrokenChecksumStillOpens();
    TestRefusesWhatIsNotASave();
    TestRegions();
    TestNewSaveDefaults();
    TestCarNames();
    TestMemoryCard();
    TestTeamName();
    TestTeamLogo();
    TestLogoColours();

    if (s_failures != 0) {
        printf("%d checks failed\n", s_failures);
        return 1;
    }
    printf("the save format reads, writes and checksums as the game does\n");
    return 0;
}
