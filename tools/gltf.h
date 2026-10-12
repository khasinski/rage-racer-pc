#ifndef RR_GLTF_H
#define RR_GLTF_H
int GlbExportTrack(const char *input,unsigned part,const char *output,const char *textures);
int GlbImportTrack(const char *input,const char *output,const char *textures);
int GlbExport(const char *input, unsigned part, const char *output);
int GlbImport(const char *input, const char *output);
int GlbExportTextures(const char *input, unsigned part, const char *output, const char *textures);
int GlbImportTextures(const char *input, const char *output, const char *textures);
int GlbAssemble(const char *output);
int GlbSeparate(const char *input, const char *directory);
int GlbMapping(const char *input, const char *output);
int GltfPack(const char *input, const char *output);
#endif
