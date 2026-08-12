#include <psyz/gte.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_words(const char *text, uint32_t words[32]) {
    int i;
    for (i = 0; i < 32; i++) {
        char *end;
        words[i] = (uint32_t)strtoul(text, &end, 16);
        if (end == text || (i != 31 && *end != ',')) return 0;
        text = end + (i != 31);
    }
    return 1;
}

static const char *field(const char *line, const char *name) {
    const char *found = strstr(line, name);
    return found == NULL ? NULL : found + strlen(name);
}

int main(int argc, char **argv) {
    FILE *input;
    char line[16384];
    unsigned sample = 0, mismatched = 0;
    if (argc != 2) {
        fprintf(stderr, "usage: rage_gte_replay TRACE.log\n");
        return 2;
    }
    input = fopen(argv[1], "r");
    if (input == NULL) {
        perror(argv[1]);
        return 2;
    }
    while (fgets(line, sizeof(line), input) != NULL) {
        uint32_t before[32], control[32], expected[32], expected_control[32];
        const char *instruction_text = field(line, "insn=");
        const char *before_text = field(line, "before=");
        const char *control_text = field(line, "control=");
        const char *after_text = field(line, "after=");
        const char *after_control_text = field(line, "after_control=");
        unsigned long instruction;
        unsigned reg;
        int different = 0;
        if (instruction_text == NULL || before_text == NULL ||
            control_text == NULL || after_text == NULL || after_control_text == NULL ||
            !parse_words(before_text, before) ||
            !parse_words(control_text, control) ||
            !parse_words(after_text, expected) ||
            !parse_words(after_control_text, expected_control)) {
            fprintf(stderr, "invalid trace line %u\n", sample + 1);
            return 2;
        }
        instruction = strtoul(instruction_text, NULL, 16);
        for (reg = 0; reg < 32; reg++) Psyz_GteCtrlWrite(reg, control[reg]);
        /* 15 is the SXY FIFO push alias. 28/29 are IRGB/ORGB aliases and
         * 30/31 are the unrelated leading-zero pair; the replayed terrain
         * commands neither consume nor produce the latter aliases. */
        for (reg = 0; reg < 28; reg++) {
            if (reg == 15) continue;
            Psyz_GteDataWrite(reg, before[reg]);
        }
        Psyz_GteCommand((uint32_t)instruction);
        sample++;
        for (reg = 0; reg < 32; reg++) {
            uint32_t actual;
            if (reg == 15 || reg >= 28) continue;
            actual = Psyz_GteDataRead(reg);
            if (actual == expected[reg]) continue;
            if (!different) {
                printf("sample=%u pc=%.8s op=%02lx", sample,
                       field(line, "pc="), instruction & 0x3f);
                different = 1;
            }
            printf(" r%u=%08x/%08x", reg, expected[reg], actual);
        }
        {
            uint32_t actual_flag = Psyz_GteCtrlRead(31);
            if (actual_flag != expected_control[31]) {
                if (!different) {
                    printf("sample=%u pc=%.8s op=%02lx", sample,
                           field(line, "pc="), instruction & 0x3f);
                    different = 1;
                }
                printf(" flag=%08x/%08x", expected_control[31], actual_flag);
            }
        }
        if (different) {
            putchar('\n');
            mismatched++;
        }
    }
    fclose(input);
    printf("gte replay: samples=%u mismatched=%u\n", sample, mismatched);
    return mismatched == 0 ? 0 : 1;
}
