#ifndef RR_GLTF_H
#define RR_GLTF_H
int GlbExport(const char *input, unsigned part, const char *output);
int GlbImport(const char *input, const char *output);
int GlbExportTextures(const char *input, unsigned part, const char *output, const char *textures);
int GlbImportTextures(const char *input, const char *output, const char *textures);
#endif
