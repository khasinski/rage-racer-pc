#ifndef RAGE_MODERN_MATERIAL_TRANSACTION_H
#define RAGE_MODERN_MATERIAL_TRANSACTION_H
#include "modern_asset_image.h"
#include "render/render_material_storage.h"
/* One publication boundary for provider-produced material/image pairs. */
static inline int ModernMaterialTransaction(
    int (*build)(void *,RageRenderMaterial *,ModernAssetImage *,RageRenderMaterialStorage *),
    void *context,void (*release)(ModernAssetImage *),
    RageRenderMaterial *definition,ModernAssetImage *image,RageRenderMaterialStorage *storage) {
    RageRenderMaterial working;
    RageRenderMaterialStorage workingStorage;
    ModernAssetImage workingImage={0};
    if(!build||!release||!definition||!image||!storage)return 0;
    RenderMaterialDefault(&working);
    if(!build(context,&working,&workingImage,&workingStorage)||
       !ModernAssetImageValidRGBA(&workingImage)||
       !RenderMaterialStorePaths(&working,storage)) {
        release(&workingImage);return 0;
    }
    *definition=working;*image=workingImage;return 1;
}
#endif
