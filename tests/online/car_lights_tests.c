#include "render/car_lights.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    CarLights car={0};CarLightsSetEnabled(1);
    UpdateCarLights(&car,1,1,0,0.02f);
    CHECK(car.headlights==0&&fabsf(car.tail-0.2f)<0.001f);
    UpdateCarLights(&car,0.1f,1,0,0.4f);
    CHECK(!car.automatic&&car.headlights==0);
    UpdateCarLights(&car,1,1,0,0.02f);CHECK(car.darkSeconds==0);
    UpdateCarLights(&car,0.1f,1,0,0.61f);CHECK(car.automatic&&car.headlights==1);
    UpdateCarLights(&car,0.1f,1,1,0.02f);CHECK(car.stop==1);
    UpdateCarLights(&car,0.1f,1,0,0.02f);CHECK(fabsf(car.stop-0.8f)<0.001f);
    UpdateCarLights(&car,0.1f,1,0,0.1f);CHECK(car.stop==0);
    CarLightsSetEnabled(0);UpdateCarLights(&car,0.1f,1,1,0.02f);
    CHECK(!car.headlights&&!car.tail&&!car.stop&&car.automatic);
    CarLightsSetEnabled(1);UpdateCarLights(&car,0.1f,1,0,0.02f);
    CHECK(car.headlights>0&&car.headlights<1);
    return 0;
}
