#include "port/modern/modern_material_transaction.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static int releases;
static void Release(ModernAssetImage *image){if(image->pixels){++releases;free(image->pixels);}memset(image,0,sizeof(*image));}
static int Build(void *context,RageRenderMaterial *material,ModernAssetImage *image,RageRenderMaterialStorage *storage) {
    int mode=*(int *)context;
    image->pixels=malloc(4);assert(image->pixels);image->size=4;image->width=image->height=1;
    memset(image->pixels,7,4);
    memcpy(storage->baseColorTexture,"owned.rgba",sizeof("owned.rgba"));
    material->baseColorTexture=(RageRenderMaterialPath){storage->baseColorTexture,mode==2?1024:10};
    return mode!=0;
}
int main(void) {
    RageRenderMaterial material,previous;
    RageRenderMaterialStorage storage,before;
    unsigned char existingPixel=42;
    ModernAssetImage image={&existingPixel,1,1,1};
    RenderMaterialDefault(&material);previous=material;
    memset(&storage,0x5a,sizeof(storage));before=storage;
    for(int mode=0;mode<3;mode+=2) {
        assert(!ModernMaterialTransaction(Build,&mode,Release,&material,&image,&storage));
        assert(!memcmp(&material,&previous,sizeof(material)));
        assert(!memcmp(&storage,&before,sizeof(storage)));
        assert(image.pixels==&existingPixel&&image.size==1&&existingPixel==42);
    }
    assert(releases==2);
    memset(&image,0,sizeof(image));
    int mode=1;
    assert(ModernMaterialTransaction(Build,&mode,Release,&material,&image,&storage));
    assert(material.baseColorTexture.text==storage.baseColorTexture);
    assert(!strcmp(material.baseColorTexture.text,"owned.rgba"));
    assert(image.size==4&&((unsigned char *)image.pixels)[0]==7&&releases==2);
    Release(&image);assert(releases==3);return 0;
}
