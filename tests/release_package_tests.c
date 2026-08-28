#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

static int ReadFile(const char *root, const char *relative, char **bytesOut) {
    char path[4096];
    FILE *file;
    long size;
    char *bytes;
    if (snprintf(path, sizeof(path), "%s%c%s", root, PATH_SEPARATOR,
                 relative) >= (int)sizeof(path))
        return 0;
    file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0 ||
        (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0) {
        if (file != NULL) fclose(file);
        return 0;
    }
    bytes = malloc((size_t)size + 1);
    if (bytes == NULL || fread(bytes, 1, (size_t)size, file) != (size_t)size) {
        free(bytes);
        fclose(file);
        return 0;
    }
    bytes[size] = '\0';
    fclose(file);
    *bytesOut = bytes;
    return 1;
}

static int RequireFile(const char *root, const char *relative) {
    char *bytes;
    if (!ReadFile(root, relative, &bytes)) {
        fprintf(stderr, "missing release source file: %s\n", relative);
        return 0;
    }
    free(bytes);
    return 1;
}

static int RequireText(const char *name, const char *bytes,
                       const char *needle) {
    if (strstr(bytes, needle) != NULL) return 1;
    fprintf(stderr, "%s does not contain: %s\n", name, needle);
    return 0;
}

int main(int argc, char **argv) {
    static const char *const required[] = {
        "README.md", "LICENSE.md", "rage-port.ini", "race-scenario.ini",
        "packaging/icon/rage-racer.png", "packaging/macos/RageRacer.icns",
        "packaging/windows/RageRacer.ico",
        "packaging/windows/rage-racer.rc",
        "packaging/linux/rage-racer.desktop",
    };
    static const char *const workflows[] = {
        ".github/workflows/linux-release.yml",
        ".github/workflows/windows-release.yml",
        ".github/workflows/macos-release.yml",
    };
    char *cmake;
    size_t index;
    int ok = 1;
    if (argc != 2) {
        fprintf(stderr, "usage: release_package_tests SOURCE_ROOT\n");
        return 2;
    }
    for (index = 0; index < sizeof(required) / sizeof(required[0]); index++)
        ok &= RequireFile(argv[1], required[index]);
    if (!ReadFile(argv[1], "CMakeLists.txt", &cmake)) return 1;
    ok &= RequireText("CMakeLists.txt", cmake,
                      "RAGE_RACER_RELEASE_VERSION \"0.6-alpha\"");
    ok &= RequireText("CMakeLists.txt", cmake, "RageRacer.icns");
    ok &= RequireText("CMakeLists.txt", cmake, "rage-racer.rc");
    free(cmake);
    for (index = 0; index < sizeof(workflows) / sizeof(workflows[0]); index++) {
        char *workflow;
        if (!ReadFile(argv[1], workflows[index], &workflow)) {
            fprintf(stderr, "missing release workflow: %s\n", workflows[index]);
            ok = 0;
            continue;
        }
        ok &= RequireText(workflows[index], workflow, "default: 0.6-alpha");
        ok &= RequireText(workflows[index], workflow, "README.md");
        ok &= RequireText(workflows[index], workflow, "LICENSE.md");
        ok &= RequireText(workflows[index], workflow, "rage-port.ini");
        ok &= RequireText(workflows[index], workflow, "race-scenario.ini");
        ok &= RequireText(workflows[index], workflow, "*.zip");
        if (strstr(workflow, "tools/rage-launcher.py") != NULL ||
            strstr(workflow, "tools/assetbrowser") != NULL ||
            strstr(workflow, "run: python") != NULL) {
            fprintf(stderr, "%s packages or executes Python\n",
                    workflows[index]);
            ok = 0;
        }
        free(workflow);
    }
    return ok ? 0 : 1;
}
