#include "game/course_select_internal.h"

#include <stdio.h>

static int CheckLabel(s32 index, CourseLabelSprites expected) {
    CourseLabelSprites actual;

    if (!GetCourseLabelSprites(index, &actual) ||
        actual.prefixTextureU != expected.prefixTextureU ||
        actual.nameWidth != expected.nameWidth ||
        actual.nameTextureU != expected.nameTextureU ||
        actual.nameTextureV != expected.nameTextureV ||
        actual.distanceWidth != expected.distanceWidth ||
        actual.distanceTextureU != expected.distanceTextureU) {
        fprintf(stderr, "wrong label for course %d\n", index);
        return 0;
    }
    return 1;
}

int main(void) {
    CourseLabelSprites sprites = {1, 2, 3, 4, 5, 6};

    if (!CheckLabel(0, (CourseLabelSprites){8, 0x54, 0, 0x9C, 0x20, 0x44}) ||
        !CheckLabel(1,
                    (CourseLabelSprites){0x10, 0x4C, 0x54, 0x9C, 0x20,
                                         0x64}) ||
        !CheckLabel(2,
                    (CourseLabelSprites){0x18, 0x48, 0, 0xAC, 0x20, 0x84}) ||
        !CheckLabel(3,
                    (CourseLabelSprites){0x20, 0x5C, 0xA4, 0x9C, 0x1E,
                                         0xA4})) {
        return 1;
    }
    if (GetCourseLabelSprites(-1, &sprites) || sprites.nameWidth != 0 ||
        GetCourseLabelSprites(4, &sprites) || sprites.nameWidth != 0 ||
        GetCourseLabelSprites(0, NULL)) {
        puts("invalid course unexpectedly has a label");
        return 1;
    }

    puts("course label tests passed");
    return 0;
}
