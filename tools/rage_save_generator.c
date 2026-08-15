#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "game/save_format.h"

#define SAVE_FILE_SIZE 0x1300
#define SAVE_PAYLOAD_OFS 0x280
#define SAVE_TRAILING_HEADER_OFS 0x1280
#define COMPLETE_MONEY 999999999

static const u8 kSaveDefaults[13][8] = {
    {0, 2, 0, 0, 0, 0, 0, 0}, {0, 3, 0, 1, 1, 0, 0, 0},
    {0, 4, 1, 2, 2, 0, 0, 0}, {0, 1, 0, 3, 3, 1, 0, 0},
    {0, 0, 0, 4, 4, 0, 0, 0}, {0, 0, 0, 5, 5, 0, 0, 0},
    {0, 0, 1, 6, 6, 0, 0, 0}, {0, 2, 0, 7, 7, 0, 0, 0},
    {0, 2, 1, 8, 8, 0, 0, 0}, {0, 3, 1, 9, 9, 0, 0, 0},
    {0, 4, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 0, 0, 0, 0, 0},
    {0, 3, 1, 0, 0, 0, 0, 0},
};

/* Highest valid grade before the next model's asset-bank base. */
static const u8 kMaxModelVariant[13] = {3, 2, 1, 4, 3, 2, 1, 2, 1, 0, 0, 0, 0};
static const s32 kDefaultLapTimes[2][4] = {
    {100765, 146765, 145765, 35765},
    {97765, 135765, 128765, 35765},
};
static const s32 kDefaultTotalTimes[2][4] = {
    {310765, 448765, 445765, 220765},
    {301765, 415765, 394765, 220765},
};
static const char kNameCharset[] = "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ.-!?@";

_Static_assert(sizeof(GameSaveBlock) == MC_BLOCK_SIZE,
               "generator must use the game's save payload layout");
_Static_assert(sizeof(GameSaveHeaderRow) == 0x80,
               "generator must use the game's save header layout");

static u32 complement_halfword_sum(const void *data, size_t halfwords) {
    const u8 *bytes = data;
    u32 sum = 0;
    size_t i;

    for (i = 0; i < halfwords; i++) {
        sum += (u32)bytes[i * 2] | ((u32)bytes[i * 2 + 1] << 8);
    }
    return ~sum;
}

static int encode_team_name(GameSaveHeaderRow *header, const char *name) {
    size_t length = strlen(name);
    size_t i;

    if (length == 0 || length > 7) {
        fprintf(stderr, "team name must contain 1 to 7 characters\n");
        return 0;
    }
    header->fields.nameLength = (u8)length;
    for (i = 0; i < length; i++) {
        const char *encoded;
        int ch = toupper((unsigned char)name[i]);
        encoded = strchr(kNameCharset, ch);
        if (!encoded) {
            fprintf(stderr, "unsupported team-name character: '%c'\n", name[i]);
            return 0;
        }
        header->fields.name[i] = (u8)(encoded - kNameCharset);
    }
    return 1;
}

static void fill_complete_payload(GameSaveBlock *save) {
    int table;
    int car;
    int series;
    int course;
    int record;

    memset(save, 0, sizeof(*save));
    save->negconSteerPlay = 1;
    save->grandPrixProgress = (SavedRaceProgress){0, 3, 0, 4, COMPLETE_MONEY};
    save->extraGrandPrixProgress =
        (SavedRaceProgress){0, 3, 0, 5, COMPLETE_MONEY};
    save->timeAttackProgress = (SavedRaceProgress){0, 3, 0, 5, 0};
    save->advancedUnlocked = 1;
    save->maxClassReached[0] = 4;
    save->maxClassReached[1] = 5;

    for (table = 0; table < 3; table++) {
        for (car = 0; car < 13; car++) {
            memcpy(&save->carSetup[table][car], kSaveDefaults[car], 8);
            save->carSetup[table][car].modelVariant = kMaxModelVariant[car];
            save->carSetup[table][car].enabled = 1;
        }
    }
    for (record = 0; record < 11; record++) {
        save->classRecords[record].grade = 1;
        save->classRecords[record].clears = 99;
    }
    for (series = 0; series < 2; series++) {
        for (course = 0; course < 4; course++) {
            int slot;
            for (slot = 0; slot < 2; slot++) {
                save->bestLapTimes[series][course][slot] =
                    kDefaultLapTimes[series][course];
                save->bestTotalTimes[series][course][slot] =
                    kDefaultTotalTimes[series][course];
            }
            for (slot = 0; slot < 3; slot++) {
                save->bestSectorTimes[series][course][slot] =
                    kDefaultLapTimes[series][course];
            }
        }
    }
    save->bgmVolume = 15;
    save->sfxVolume = 15;
    save->grandPrixCourseProgress[0] = 1;
    save->grandPrixCourseProgress[1] = 1;
    save->grandPrixCourseProgress[2] = 1;
    save->grandPrixCourseProgress[3] = 1;
    save->grandPrixCourseProgress[6] = 5;
    memcpy(save->extraGrandPrixCourseProgress,
           save->grandPrixCourseProgress,
           sizeof(save->grandPrixCourseProgress));
    save->checksum = complement_halfword_sum(save, 0x7FE);
}

static int build_complete_save(u8 file[SAVE_FILE_SIZE], const char *name) {
    GameSaveHeaderRow header;
    GameSaveBlock payload;

    memset(file, 0, SAVE_FILE_SIZE);
    memset(&header, 0, sizeof(header));
    file[0] = 'S';
    file[1] = 'C';
    file[2] = 0x11;
    file[3] = 1;
    memcpy(file + 4, "RAGE RACER COMPLETE", 19);
    if (!encode_team_name(&header, name)) {
        return 0;
    }
    header.fields.checksum = complement_halfword_sum(&header, 0x3E);
    fill_complete_payload(&payload);
    memcpy(file + 0x200, &header, sizeof(header));
    memcpy(file + SAVE_PAYLOAD_OFS, &payload, sizeof(payload));
    memcpy(file + SAVE_TRAILING_HEADER_OFS, &header, sizeof(header));
    return 1;
}

static int validate_complete_save(const u8 file[SAVE_FILE_SIZE]) {
    const GameSaveHeaderRow *header = (const void *)(file + 0x200);
    const GameSaveHeaderRow *trailing =
        (const void *)(file + SAVE_TRAILING_HEADER_OFS);
    const GameSaveBlock *save = (const void *)(file + SAVE_PAYLOAD_OFS);
    int table;
    int car;

    if (file[0] != 'S' || file[1] != 'C' ||
        memcmp(header, trailing, sizeof(*header)) != 0 ||
        header->fields.checksum != complement_halfword_sum(header, 0x3E) ||
        save->checksum != complement_halfword_sum(save, 0x7FE) ||
        save->advancedUnlocked != 1 || save->maxClassReached[0] != 4 ||
        save->maxClassReached[1] != 5) {
        return 0;
    }
    for (table = 0; table < 3; table++) {
        for (car = 0; car < 13; car++) {
            if (save->carSetup[table][car].enabled != 1) {
                return 0;
            }
        }
    }
    return 1;
}

static void usage(const char *program) {
    fprintf(stderr,
            "usage: %s [--slot 0|1|2] [--name NAME] [--output PATH]\n",
            program);
}

int main(int argc, char **argv) {
    u8 file[SAVE_FILE_SIZE];
    const char *name = "UNLOCK";
    const char *output = NULL;
    char default_output[64];
    int slot = 0;
    int i;
    FILE *stream;

    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) {
        return !(build_complete_save(file, name) && validate_complete_save(file));
    }
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--slot") == 0 && i + 1 < argc) {
            slot = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            name = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (slot < 0 || slot > 2) {
        fprintf(stderr, "slot must be 0, 1 or 2\n");
        return EXIT_FAILURE;
    }
    if (!build_complete_save(file, name)) {
        return EXIT_FAILURE;
    }
    if (!output) {
        if (mkdir("bu00", 0755) != 0 && errno != EEXIST) {
            perror("bu00");
            return EXIT_FAILURE;
        }
        snprintf(default_output, sizeof(default_output),
                 "bu00/BESCES-00650 RAGE%03d", slot);
        output = default_output;
    }
    stream = fopen(output, "wb");
    if (!stream) {
        perror(output);
        return EXIT_FAILURE;
    }
    if (fwrite(file, 1, sizeof(file), stream) != sizeof(file) ||
        fclose(stream) != 0) {
        fprintf(stderr, "failed to write complete save: %s\n", output);
        return EXIT_FAILURE;
    }
    printf("complete Rage Racer save written to %s (slot %d, team %s)\n",
           output, slot, name);
    return EXIT_SUCCESS;
}
