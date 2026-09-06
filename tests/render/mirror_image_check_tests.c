#define main MirrorCheckMain
#include "mirror_image_check.c"
#undef main
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

static void Fixture(const char *path, unsigned scene, unsigned bright, int shortFile, int trailing) {
    FILE *file = fopen(path, "wb"); assert(file);
    assert(fputs("P6\n320 240\n255\n", file) >= 0);
    unsigned index = 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) {
            unsigned char pixel[3] = {0};
            if (y >= 18 && y < 54 && x >= 86 && x < 234) {
                unsigned char value = index < bright ? 81 : index < scene ? 26 : 0;
                memset(pixel, value, sizeof(pixel));
                ++index;
            }
            if (!shortFile || y != 239 || x != 319)
                assert(fwrite(pixel, 1, 3, file) == 3);
        }
    if (trailing) assert(fputc(0, file) != EOF);
    assert(fclose(file) == 0);
}
int main(void) {
    char classic[] = "mirror-check-classic.tmp", modern[] = "mirror-check-modern.tmp";
    char *args[] = {"mirror-check", classic, modern};
    /* Reserve owned test paths before Fixture rewrites them. */
    FILE *file = fopen(classic, "wbx"); assert(file); assert(fclose(file) == 0);
    file = fopen(modern, "wbx"); assert(file); assert(fclose(file) == 0);
    assert(MirrorCheckMain(2, args) == 2);
    assert(MirrorCheckMain(3, args) == 1);
    Fixture(classic, 0, 0, 0, 0);
    Fixture(modern, 1200, 8, 0, 0);
    assert(MirrorCheckMain(3, args) == 0);
    Fixture(modern, 1199, 8, 0, 0);
    assert(MirrorCheckMain(3, args) == 1);
    Fixture(modern, 1200, 7, 0, 0);
    assert(MirrorCheckMain(3, args) == 1);
    Fixture(modern, 1200, 8, 1, 0);
    assert(MirrorCheckMain(3, args) == 1);
    Fixture(modern, 1200, 8, 0, 1);
    assert(MirrorCheckMain(3, args) == 1);
    file = fopen(modern, "wb"); assert(file);
    assert(fputs("P6\n1 1\n255\nabc", file) >= 0); assert(fclose(file) == 0);
    assert(MirrorCheckMain(3, args) == 1);
    assert(remove(classic) == 0); assert(remove(modern) == 0);
    return 0;
}
