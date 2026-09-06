#include "../../launcher/native/mod_package_cli.h"
#include <assert.h>
#include <string.h>

static int Write(const char *bytes, size_t size, const char *target) {
    const char *input = "mod_package_writer_input.tmp";
    FILE *file = fopen(input, "wbx"); assert(file);
    assert(fwrite(bytes, 1, size, file) == size);
    assert(fclose(file) == 0);
    assert(freopen(input, "rb", stdin));
    int result = PackageMetadata(NULL, target);
#ifdef _WIN32
    assert(freopen("NUL", "rb", stdin));
#else
    assert(freopen("/dev/null", "rb", stdin));
#endif
    assert(remove(input) == 0);
    return result;
}

int main(void) {
    const char *target = "mod_package_writer_\xc5\xbc_output.tmp";
    const char valid[] = "{\"format\":1,\"name\":\"Za\\u017c\",\"region\":\"PAL\",\"extension\":[1,true]}\n";
    char oversized[RAGE_MOD_PACKAGE_BYTES + 1];
    memset(oversized, ' ', sizeof(oversized));
    assert(Write("{", 1, target) == 1);
    assert(Write(oversized, sizeof(oversized), target) == 1);
    FILE *file = PackageOpenMetadata(target); assert(!file);
    assert(Write(valid, sizeof(valid)-1, target) == 0);
    assert(Write(valid, sizeof(valid)-1, target) == 1);
    file = PackageOpenMetadata(target); assert(file);
    char actual[sizeof(valid)] = {0};
    assert(fread(actual, 1, sizeof(actual), file) == sizeof(valid)-1);
    assert(fclose(file) == 0);
    assert(!memcmp(actual, valid, sizeof(valid)-1));
    assert(PackageMetadata(target, NULL) == 0);
#ifdef _WIN32
    assert(_wremove(L"mod_package_writer_\x017c_output.tmp") == 0);
#else
    assert(remove(target) == 0);
#endif
    return 0;
}
