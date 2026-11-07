#include "online/core_api.h"
#include "render/render_world.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render/retail_tachometer.h"
#include "render/speed_units_poc.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);on_release();return 1;}}while(0)

/* Reproduce the spectator -> driver handover using real camera/track data.
 * It must keep the chase view instead of jumping into the cockpit. */
int main(void){
    const char *disc=getenv("RAGE_TEST_DISC");if(!disc||!*disc)return 77;
    on_native_client();CHECK(rw_load_disc(disc));
    int car=-1;for(int i=0;i<13&&car<0;i++)car=rw_class_car(0,i);
    CHECK(car>=0&&rw_start_race(0,0,car,1,0,2,1));
    for(int i=0;i<200;i++)CHECK(rw_tick()>=0);
    CHECK(on_local_seat(-1));CHECK(rw_set_view_seat(0));
    for(int i=0;i<20;i++)CHECK(rw_tick()>=0);
    CHECK(on_prepare(16.0f/9.0f,1)>0);
    CHECK(!rw_tachometer()[RAGE_TACHO_VISIBLE]);
    uint32_t preparedRevision=rw_hud_atlas_revision();
    CHECK(on_prepare_driver_hud(0));CHECK(rw_tachometer()[RAGE_TACHO_VISIBLE]);
    CHECK(rw_hud_atlas_revision()==preparedRevision);
    RenderCamera before,after;CHECK(on_copy_camera(&before,sizeof(before)));
    CHECK(on_local_seat(0));
    CHECK(rw_hud_atlas_revision()==preparedRevision);
    for(int i=0;i<2;i++)CHECK(rw_tick()>=0);
    CHECK(on_prepare(16.0f/9.0f,1)>0);CHECK(on_copy_camera(&after,sizeof(after)));
    float dx=after.transform.position.x-before.transform.position.x;
    float dy=after.transform.position.y-before.transform.position.y;
    float dz=after.transform.position.z-before.transform.position.z;
    CHECK(sqrtf(dx*dx+dy*dy+dz*dz)<100.0f);
    CHECK(rw_tachometer()[RAGE_TACHO_VISIBLE]);
    int32_t idleNeedle[8];memcpy(idleNeedle,rw_tachometer()+RAGE_TACHO_NEEDLE,sizeof(idleNeedle));
    int32_t throttle[8]={1,0,0,0,256,0,0,0};CHECK(on_set_input(throttle));
    for(int i=0;i<150;i++)CHECK(rw_tick()>=0);
    CHECK(rw_hud()[7]>0);
    CHECK(memcmp(idleNeedle,rw_tachometer()+RAGE_TACHO_NEEDLE,sizeof(idleNeedle))!=0);
    CHECK(rw_tachometer()[RAGE_TACHO_GEAR+2]==rw_hud()[8]);
    int metricSpeed=rw_hud()[7];
    uint8_t unitBytes[RAGE_HUD_UNIT_BYTES];RageHudUnitDiff unitDiff;
    CHECK(on_hud_unit_snapshot(unitBytes,sizeof(unitBytes))==car+1);
    CHECK(RageSpeedUnitsRetailDiff((unsigned)rw_car_model(car),&unitDiff));
    CHECK(RageHudUnitDiffApply(&unitDiff,(unsigned)rw_car_model(car),1,unitBytes));
    CHECK(on_hud_apply_units(unitBytes,sizeof(unitBytes),1));
    CHECK(rw_tachometer()[RAGE_TACHO_SPEED+2]==metricSpeed*100/160);
    uint32_t previousRevision=rw_hud_atlas_revision();
    for(int model=0;model<13;model++){
        int variant=rw_class_car(5,model);CHECK(variant>=0);
        for(int manual=0;manual<=1;manual++){
            if(!manual&&!rw_car_automatic(variant))continue;
            CHECK(rw_start_race(5,0,variant,manual,0,2,1));
            CHECK(rw_hud_atlas_revision()>previousRevision);previousRevision=rw_hud_atlas_revision();
            CHECK(rw_tick()>=0);CHECK(on_prepare(16.0f/9.0f,1)>0);
            const int32_t *t=rw_tachometer();CHECK(t&&t[RAGE_TACHO_VISIBLE]);
            CHECK(t[RAGE_TACHO_DIGIT_CLUT]==(manual?0:1));
            RageRetailTachometer frame={.atlas=rw_hud_atlas()};CHECK(frame.atlas);
            memcpy(frame.words,t,sizeof(frame.words));RageTachometerQuad quads[7];
            CHECK(RenderRetailTachometerBuild(&frame,0,quads)==7);
            CHECK(on_hud_unit_snapshot(unitBytes,sizeof(unitBytes))==variant+1);
            CHECK(RageSpeedUnitsRetailDiff((unsigned)model,&unitDiff));
            CHECK(RageHudUnitDiffApply(&unitDiff,(unsigned)model,1,unitBytes));
            CHECK(on_hud_apply_units(unitBytes,sizeof(unitBytes),1));
            CHECK(RageHudUnitDiffApply(&unitDiff,(unsigned)model,0,unitBytes));
            CHECK(on_hud_apply_units(unitBytes,sizeof(unitBytes),0));
            for(int face=0;face<2;face++){
                unsigned opaque=0;for(int y=0;y<94;y++)for(int x=0;x<96;x++)
                    opaque+=frame.atlas[(y*RAGE_TACHO_ATLAS_WIDTH+x+face*96)*4+3]!=0;
                CHECK(opaque>500);
            }
        }
    }
    on_release();return 0;
}
