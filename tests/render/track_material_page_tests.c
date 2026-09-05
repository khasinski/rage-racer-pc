#include "track_material_page.h"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void) {
    for (unsigned v = 0; v < 8; ++v) {
        int terrain = TrackMaterialAlternateVariant(RAGE_RENDER_ASSET_TERRAIN,v);
        int course = TrackMaterialAlternateVariant(RAGE_RENDER_ASSET_COURSE,v);
        CHECK((terrain & 1) == (v & 1));
        CHECK((course & 3) == (v & 3));
        for (int current = 0; current <= 1; ++current) {
            CHECK(TrackMaterialPage(RAGE_RENDER_ASSET_TERRAIN,v,current) == ((v>>1)&1));
            CHECK(TrackMaterialPage(RAGE_RENDER_ASSET_COURSE,v,current) == ((v>>2)&1));
            CHECK(TrackMaterialPage(RAGE_RENDER_ASSET_TERRAIN,terrain,current) !=
                  TrackMaterialPage(RAGE_RENDER_ASSET_TERRAIN,v,current));
            CHECK(TrackMaterialPage(RAGE_RENDER_ASSET_COURSE,course,current) !=
                  TrackMaterialPage(RAGE_RENDER_ASSET_COURSE,v,current));
        }
        CHECK(TrackMaterialAlternateVariant(RAGE_RENDER_ASSET_MODEL_BANK,v) == -1);
        CHECK(TrackMaterialAlternateVariant(RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,v) == -1);
    }
    return 0;
}
