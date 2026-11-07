#include "online/core_api.h"
#include "render/render_world.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
    RenderCamera before,after;CHECK(on_copy_camera(&before,sizeof(before)));
    CHECK(on_local_seat(0));
    for(int i=0;i<2;i++)CHECK(rw_tick()>=0);
    CHECK(on_prepare(16.0f/9.0f,1)>0);CHECK(on_copy_camera(&after,sizeof(after)));
    float dx=after.transform.position.x-before.transform.position.x;
    float dy=after.transform.position.y-before.transform.position.y;
    float dz=after.transform.position.z-before.transform.position.z;
    CHECK(sqrtf(dx*dx+dy*dy+dz*dz)<100.0f);
    on_release();return 0;
}
