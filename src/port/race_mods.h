#ifndef RAGE_RACE_MODS_H
#define RAGE_RACE_MODS_H

#include <stdint.h>

int PortFastMenusEnabled(void);
void PortSetFastMenusEnabled(int enabled);
int PortRandomTimeOfDayEnabled(void);
void PortSetRandomTimeOfDayEnabled(int enabled);
/* Called once when a new race starts. Replay seeks retain their recorded clock. */
int PortApplyRaceEnvironment(int originalOffset, int grandPrix);
int PortRaceEnvironmentProfile(int original, int grandPrix, int profileCount);
/* Sky artwork follows the installed environment, independently of geometry. */
uint32_t PortRaceEnvironmentSkyAssetKey(uint32_t original);

#endif
