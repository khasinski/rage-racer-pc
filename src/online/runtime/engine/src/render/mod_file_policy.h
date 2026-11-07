#ifndef RAGE_MOD_FILE_POLICY_H
#define RAGE_MOD_FILE_POLICY_H
typedef enum RageModFileKind {
    RAGE_MOD_FILE_INVALID, RAGE_MOD_FILE_RAW, RAGE_MOD_FILE_TEXTURE,
    RAGE_MOD_FILE_MESH, RAGE_MOD_FILE_METADATA
} RageModFileKind;
/* Classify a package-relative file, not an absolute filesystem path. No I/O.
 * Only retail raw slots and supported data formats are accepted. */
RageModFileKind ModFileClassify(const char *path);
/* Package-relative directory, without trailing slash; at most eight levels. */
int ModDirectoryAllowed(const char *path);
enum { RAGE_MOD_FILE_GLOBAL = 1, RAGE_MOD_FILE_SEMANTIC = 2,
       RAGE_MOD_FILE_LEGACY = 4 };
/* Reference flags are SEMANTIC and/or LEGACY. Returns copy-role bits, zero
 * for metadata/unreferenced meshes, or -1 for invalid paths/reference kinds.
 * Backing files never become a global override just by sharing a filename. */
int ModFileDisposition(const char *path, unsigned references);
#endif
