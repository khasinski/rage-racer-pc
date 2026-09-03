#include "disc_cue.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void Check(int condition, const char *label) {
    if (condition) return;
    fprintf(stderr, "FAIL: %s\n", label);
    failures++;
}

static int WriteCue(const char *text) {
    FILE *file = fopen("disc_cue_test.cue", "w");

    if (file == NULL) return 0;
    fputs(text, file);
    return fclose(file) == 0;
}

static void TestRelativeDataTrack(void) {
    char image[128];
    long offset = -1;

    Check(WriteCue("FILE \"audio.bin\" BINARY\n"
                   "  TRACK 01 AUDIO\n"
                   "    INDEX 01 00:00:00\n"
                   "FILE \"Track 01.bin\" BINARY\n"
                   "\ttrack 02 mode2/2352\n"
                   "\t  index 01 01:02:03\n"),
          "writes a multi-file CUE");
    Check(DiscCueResolveDataTrack("disc_cue_test.cue", image, sizeof(image),
                                  &offset),
          "resolves a lower-case, tab-indented data track");
    Check(strcmp(image, "./Track 01.bin") == 0,
          "keeps the file belonging to the data track");
    Check(offset == (long)(60 * 75 + 2 * 75 + 3) * 2352,
          "converts INDEX 01 to a byte offset");
}

static void TestUnquotedFile(void) {
    char image[128];
    long offset = -1;

    Check(WriteCue("FILE track.bin BINARY\n"
                   "TRACK 01 MODE1/2352\n"
                   "INDEX 01 00:00:00\n"),
          "writes an unquoted CUE");
    Check(DiscCueResolveDataTrack("disc_cue_test.cue", image, sizeof(image),
                                  &offset) &&
              strcmp(image, "./track.bin") == 0 && offset == 0,
          "accepts an unquoted data file");
}

static void TestInvalidInputs(void) {
    char image[8];
    long offset = 123;

    Check(WriteCue("FILE \"track.bin\" BINARY\n"
                   "TRACK 01 MODE2/2352\n"
                   "INDEX 01 00:60:00\n"),
          "writes a malformed CUE");
    Check(!DiscCueResolveDataTrack("disc_cue_test.cue", image, sizeof(image),
                                   &offset),
          "rejects an invalid INDEX timestamp");
    Check(image[0] == '\0' && offset == 0,
          "clears outputs after an invalid INDEX timestamp");
    Check(WriteCue("FILE \"track.bin\" BINARY\n"
                   "TRACK 01 MODE2/2352\n"
                   "INDEX 01 00:00:00garbage\n"),
          "writes an INDEX with trailing garbage");
    Check(!DiscCueResolveDataTrack("disc_cue_test.cue", image, sizeof(image),
                                   &offset),
          "rejects trailing INDEX characters");
    Check(WriteCue("FILE \"track.bin\" BINARY\nTRACK 01 MODE2/2352\n"),
          "writes a CUE without an index");
    Check(!DiscCueResolveDataTrack("disc_cue_test.cue", image, sizeof(image),
                                   &offset),
          "rejects a data track without INDEX 01");
    Check(WriteCue("FILE \"track.bin\" BINARY\n"
                   "TRACK 01 MODE2/2352\n"
                   "INDEX 01 00:00:00\n"),
          "writes a valid CUE for argument checks");
    Check(!DiscCueResolveDataTrack("disc_cue_test.cue", image, sizeof(image),
                                   &offset),
          "rejects an output path that is too small");
    Check(image[0] == '\0' && offset == 0,
          "does not publish an offset when the path is too small");
    strcpy(image, "stale");
    offset = 123;
    Check(!DiscCueResolveDataTrack(NULL, image, sizeof(image), &offset) &&
              image[0] == '\0' && offset == 0,
          "clears valid output buffers for a missing CUE path");
    Check(!DiscCueResolveDataTrack(NULL, image, sizeof(image), &offset) &&
              !DiscCueResolveDataTrack("disc_cue_test.cue", NULL, 0, &offset),
          "rejects invalid output arguments");
}

int main(void) {
    TestRelativeDataTrack();
    TestUnquotedFile();
    TestInvalidInputs();
    remove("disc_cue_test.cue");
    if (failures != 0) return 1;
    puts("CUE data tracks resolve independently of the disc runtime");
    return 0;
}
