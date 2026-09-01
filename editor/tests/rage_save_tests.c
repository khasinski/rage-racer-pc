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
    CheckEqual(RageRegionFromPath("BISLPS-00744 RAGE001"),
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
