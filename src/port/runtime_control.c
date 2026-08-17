#include "scenario_control.h"

int RagePortShouldExit(int frame_number) {
    (void)frame_number;
    return RagePortScenarioShouldExit();
}
