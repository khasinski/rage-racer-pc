#ifndef RAGE_MOD_FILE_POLICY_H
#define RAGE_MOD_FILE_POLICY_H
typedef enum RageModFileKind {
    RAGE_MOD_FILE_INVALID, RAGE_MOD_FILE_RAW, RAGE_MOD_FILE_TEXTURE,
    RAGE_MOD_FILE_MESH, RAGE_MOD_FILE_METADATA
} RageModFileKind;
/* Classify a package-relative file, not an absolute filesystem path. No I/O.
 * Only retail raw slots and supported data formats are accepted. */
RageModFileKind ModFileClassify(const char *path);
#endif
