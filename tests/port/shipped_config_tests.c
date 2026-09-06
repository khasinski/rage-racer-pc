#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *Trim(char *value) {
    while (isspace((unsigned char)*value)) ++value;
    char *end = value + strlen(value);
    while (end > value && isspace((unsigned char)end[-1])) --end;
    *end = 0;
    return value;
}

/* Release-policy check only: do not load user profiles or environment
 * overrides. These four settings must be present explicitly in the artifact.
 * Resolve section-qualified keys like the runtime; unrelated settings are
 * not checked against this release policy. */
static int Validate(FILE *file, int quiet) {
    static const char *keys[] = {"video.draw_distance", "camera.chase_turn_lookahead",
        "input.steering_linearity", "diagnostics.marker_capture"};
    unsigned seen = 0;
    char line[1400], section[64] = "";
    while (fgets(line, sizeof(line), file)) {
        if (!strchr(line, '\n') && !feof(file)) {
            if (!quiet) fprintf(stderr, "Configuration line is too long\n");
            return 0;
        }
        char *key = Trim(line);
        if (*key == '#' || *key == ';') continue;
        if (*key == '[') {
            char *end = strchr(key + 1, ']');
            if (!end) return 0;
            *end = 0;
            char *name = Trim(key + 1);
            if (strlen(name) >= sizeof(section)) return 0;
            strcpy(section, name);
            continue;
        }
        char *equals = strchr(key, '=');
        if (!equals) continue;
        *equals = 0;
        key = Trim(key);
        char qualified[128];
        int length = *section ? snprintf(qualified, sizeof(qualified), "%s.%s", section, key)
                              : snprintf(qualified, sizeof(qualified), "%s", key);
        if (length < 0 || (size_t)length >= sizeof(qualified)) return 0;
        key = qualified;
        char *value = Trim(equals + 1);
        for (unsigned i = 0; i < 4; ++i) {
            if (strcmp(key, keys[i])) continue;
            int valid = !(seen & (1u << i));
            seen |= 1u << i;
            if (i == 3) {
                for (char *p = value; *p; ++p) *p = (char)tolower((unsigned char)*p);
                valid = valid && (!strcmp(value, "false") || !strcmp(value, "off") ||
                    !strcmp(value, "no") || !strcmp(value, "0"));
            } else {
                char *end;
                double number = strtod(value, &end);
                valid = valid && end != value && *end == 0 && isfinite(number);
                valid = valid && (i == 0 ? number >= 0 && number <= 1 :
                                  i == 1 ? number == 0 : number == 0.5);
            }
            if (!valid) {
                if (!quiet) fprintf(stderr, "Unsafe, malformed or duplicate shipped setting: %s=%s\n", key, value);
                return 0;
            }
        }
    }
    if (ferror(file) || seen != 15) {
        if (!quiet) fprintf(stderr, "Missing required shipped settings or read error (mask=%u)\n", seen);
        return 0;
    }
    return 1;
}

static int Fixture(const char *distance, const char *camera, const char *steering,
                   const char *marker, const char *extra, int expected) {
    FILE *file = tmpfile();
    if (!file) return 0;
    fprintf(file, "# draw_distance=100\n[video]\n draw_distance = %s\r\n"
        "[camera]\nchase_turn_lookahead=%s\n[input]\nsteering_linearity=%s\n"
        "[diagnostics]\nmarker_capture=%s\n%s",
        distance, camera, steering, marker, extra);
    rewind(file);
    int result = Validate(file, 1);
    fclose(file);
    return result == expected;
}

static int SelfTest(void) {
    const char *disabled[] = {"false", "OFF", "No", "0"};
    for (unsigned i = 0; i < 4; ++i)
        if (!Fixture("1", "0", "0.5", disabled[i], "", 1)) return 1;
    const char *invalid[] = {"2", "-1", "nan", "inf", "1junk", ""};
    for (unsigned i = 0; i < 6; ++i)
        if (!Fixture(invalid[i], "0", "0.5", "false", "", 0)) return 1;
    if (!Fixture("1", "1", "0.5", "false", "", 0) ||
        !Fixture("1", "0", "0.6", "false", "", 0) ||
        !Fixture("1", "0", "0.5", "true", "", 0) ||
        !Fixture("1", "0", "0.5", "false", "marker_capture=true\n", 0) ||
        !Fixture("1", "0", "0.5", "false", "[unrelated]\nmarker_capture=true\n", 1)) return 1;
    FILE *file = tmpfile();
    if (!file) return 1;
    fputs("[video]\ndraw_distance=1\n[camera]\nchase_turn_lookahead=0\n"
          "[input]\nsteering_linearity=0.5\n[wrong_section]\nmarker_capture=false\n", file);
    rewind(file);
    int missingRejected = !Validate(file, 1);
    fclose(file);
    if (!missingRejected) return 1;
    file = tmpfile();
    if (!file) return 1;
    fputs("video.draw_distance=1\ncamera.chase_turn_lookahead=0\n"
          "input.steering_linearity=0.5\ndiagnostics.marker_capture=off\n", file);
    rewind(file);
    int dottedAccepted = Validate(file, 1);
    fclose(file);
    return dottedAccepted ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    if (!strcmp(argv[1], "--self-test")) return SelfTest();
    FILE *file = fopen(argv[1], "r");
    if (!file) { perror(argv[1]); return 1; }
    int valid = Validate(file, 0);
    fclose(file);
    return valid ? 0 : 1;
}
