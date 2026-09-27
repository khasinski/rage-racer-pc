#include "game/race_data.h"
#include "disc_iso.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
enum { SECTOR_SIZE = 2352, USER_SIZE = 2048, SECTORS = 20 };
static int MemorySector(void *context, unsigned int sector, unsigned char *raw) {
    if (sector >= SECTORS) return 0;
    memcpy(raw, (const u8 *)context + sector * SECTOR_SIZE, SECTOR_SIZE);
    return 1;
}
static void Word(u8 *data, u32 value) {
    for (int i = 0; i < 4; i++) data[i] = (u8)(value >> (i * 8));
}
static void BuildDisc(u8 *data, s32 offset) {
    const char name[] = "RAGE.BIN;1";
    memset(data, 0, SECTORS * SECTOR_SIZE);
    u8 *volume = data + 16 * SECTOR_SIZE + offset;
    volume[0] = 1;
    memcpy(volume + 1, "CD001", 5);
    volume[156] = 34;
    Word(volume + 158, 17);
    Word(volume + 166, USER_SIZE);
    u8 *record = data + 17 * SECTOR_SIZE + offset;
    const u32 carSize = sizeof(RaceCarAssetHeader) + sizeof(GameCarSpec) + 4;
    record[0] = 44;
    Word(record + 2, 18);
    Word(record + 10, USER_SIZE + carSize);
    record[32] = sizeof(name) - 1;
    memcpy(record + 33, name, sizeof(name) - 1);
    record += 44;
    const char *boot = offset == 16 ? "SCES_006.50;1" : "SLPS_009.00;1";
    record[0] = 46;
    Word(record + 2, 13);
    Word(record + 10, 1);
    record[32] = (u8)strlen(boot);
    memcpy(record + 33, boot, strlen(boot));
    data[13 * SECTOR_SIZE + offset] = 0x42;
    record += 46;
    const char configName[] = "SYSTEM.CNF;1";
    const char *config = offset == 16 ? "BOOT = cdrom:\\SCES_006.50;1\n"
                                      : "BOOT = cdrom:\\SLPS_009.00;1\n";
    record[0] = 46;
    Word(record + 2, 14);
    Word(record + 10, (u32)strlen(config));
    record[32] = sizeof(configName) - 1;
    memcpy(record + 33, configName, sizeof(configName) - 1);
    memcpy(data + 14 * SECTOR_SIZE + offset, config, strlen(config));
    u8 *index = data + 18 * SECTOR_SIZE + offset;
    Word(index + 11 * 8, 1);
    Word(index + 11 * 8 + 4, carSize);
    u8 *pack = data + 19 * SECTOR_SIZE + offset;
    const RaceCarAssetHeader header = {sizeof(header), sizeof(header) + sizeof(GameCarSpec),
        sizeof(header) + sizeof(GameCarSpec) + 1, sizeof(header) + sizeof(GameCarSpec) + 2,
        sizeof(header) + sizeof(GameCarSpec) + 3};
    const GameCarSpec spec = {.topGear = 6};
    memcpy(pack, &header, sizeof header);
    memcpy(pack + header.specificationOffset, &spec, sizeof spec);
}
static int WriteDisc(const char *path, const u8 *data, s32 prefix, s32 sectors) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) return 0;
    u8 empty[SECTOR_SIZE] = {0};
    int ok = 1;
    for (s32 i = 0; i < prefix; i++) {
        if (fwrite(empty, 1, sizeof empty, file) != sizeof empty) ok = 0;
    }
    if (fwrite(data, SECTOR_SIZE, sectors, file) != (size_t)sectors) ok = 0;
    if (fclose(file) != 0) ok = 0;
    return ok;
}
int main(void) {
    u8 *disc = malloc(SECTORS * SECTOR_SIZE);
    CHECK(disc != NULL);
    const char *bin = "race-disc-test.bin", *cue = "race-disc-test.CUE";
    for (int mode = 1; mode <= 2; mode++) {
        const char *boot = mode == 1 ? "SCES_006.50" : "SLPS_009.00";
        BuildDisc(disc, mode == 1 ? 16 : 24);
        CHECK(WriteDisc(bin, disc, 0, SECTORS));
        RaceData *first = LoadRaceDisc(bin), *second = LoadRaceDisc(bin);
        CHECK(first != NULL && second != NULL && first->data != second->data);
        CHECK(strcmp(first->boot, boot) == 0 && strcmp(second->boot, first->boot) == 0);
        const u8 code = 0x42;
        CHECK(first->executable == ArchiveFingerprint(&code, 1));
        CHECK(second->executable == first->executable);
        DiscIsoReader reader;
        CHECK(DiscIsoOpen(&reader, MemorySector, disc));
        RaceData *mounted = LoadRaceIso(&reader);
        CHECK(mounted && mounted->data != first->data && mounted->size == first->size);
        CHECK(memcmp(mounted->data, first->data, first->size) == 0);
        CHECK(strcmp(mounted->boot, first->boot) == 0 && mounted->executable == first->executable);
        memset(&reader, 0, sizeof(reader)); /* Loader retains no reader/context. */
        memset(disc, 0, SECTORS * SECTOR_SIZE); /* Nor the source bytes. */
        GameCarSpec spec;
        CHECK(ReadRaceCar(first, 0, &spec) && spec.topGear == 6);
        CHECK(ReadRaceCar(mounted, 0, &spec) && spec.topGear == 6);
        FreeRaceData(mounted);
        BuildDisc(disc, mode == 1 ? 16 : 24);
        FreeRaceData(first);
        CHECK(ReadRaceCar(second, 0, &spec) && spec.topGear == 6);
        FreeRaceData(second);
        CHECK(WriteDisc(bin, disc, 2, SECTORS));
        FILE *file = fopen(cue, "w");
        CHECK(file != NULL);
        CHECK(fprintf(file, "FILE \"%s\" BINARY\nTRACK 01 MODE%d/2352\nINDEX 01 00:00:02\n", bin, mode) > 0);
        CHECK(fclose(file) == 0);
        RaceData *fromCue = LoadRaceDisc(cue);
        CHECK(fromCue != NULL && ReadRaceCar(fromCue, 0, &spec) && spec.topGear == 6);
        CHECK(strcmp(fromCue->boot, boot) == 0);
        CHECK(fromCue->executable == ArchiveFingerprint(&code, 1));
        disc[13 * SECTOR_SIZE + (mode == 1 ? 16 : 24)] ^= 1;
        CHECK(WriteDisc(bin, disc, 2, SECTORS));
        RaceData *revision = LoadRaceDisc(cue);
        CHECK(revision != NULL && strcmp(revision->boot, fromCue->boot) == 0);
        CHECK(revision->size == fromCue->size &&
              memcmp(revision->data, fromCue->data, revision->size) == 0);
        CHECK(revision->executable != fromCue->executable);
        FreeRaceData(revision);
        FreeRaceData(fromCue);
        CHECK(WriteDisc(bin, disc, 2, SECTORS - 1));
        CHECK(LoadRaceDisc(cue) == NULL);
        CHECK(remove(cue) == 0 && remove(bin) == 0);
    }
    BuildDisc(disc, 24);
    /* A readable archive is not evidence of a region. No metadata fallback. */
    memset(disc + 17 * SECTOR_SIZE + 24 + 44, 0, USER_SIZE - 44);
    CHECK(WriteDisc(bin, disc, 0, SECTORS));
    RaceData *unknown = LoadRaceDisc(bin);
    GameCarSpec unknownSpec;
    CHECK(unknown != NULL && unknown->boot[0] == '\0');
    CHECK(unknown->executable == 0);
    CHECK(ReadRaceCar(unknown, 0, &unknownSpec) && unknownSpec.topGear == 6);
    FreeRaceData(unknown);
    CHECK(remove(bin) == 0);
    BuildDisc(disc, 24);
    Word(disc + 18 * SECTOR_SIZE + 24 + 11 * 8, UINT32_MAX);
    CHECK(WriteDisc(bin, disc, 0, SECTORS));
    CHECK(LoadRaceDisc(bin) == NULL);
    CHECK(remove(bin) == 0);
    CHECK(LoadRaceDisc(bin) == NULL && LoadRaceDisc(cue) == NULL);
    CHECK(LoadRaceDisc(NULL) == NULL && LoadRaceDisc("invalid.txt") == NULL);
    DiscIsoReader empty = {0};
    CHECK(!LoadRaceIso(NULL) && !LoadRaceIso(&empty));
    FreeRaceData(NULL);
    free(disc);
    return 0;
}
