#include "asset_path.h"

int AssetPathIsRelativeFile(const char *path, size_t size) {
    size_t start = 0;
    if (path == NULL || size == 0) return 0;
    for (size_t i = 0; i <= size; ++i) {
        if (i < size) {
            unsigned char c = (unsigned char)path[i];
            if (c < 32 || c == 127 || c == '\\' || c == ':') return 0;
            if (c != '/') continue;
        }
        size_t length = i - start;
        if (length == 0 || (length == 1 && path[start] == '.') ||
            (length == 2 && path[start] == '.' && path[start + 1] == '.'))
            return 0;
        start = i + 1;
    }
    return 1;
}
