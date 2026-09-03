#include "game/car.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

u8 g_PadType;
u16 g_PadHeld;
u16 g_PadButtonMapping[16];
ControllerMappingIndex g_NegconMappingIndex;
s16 g_NegconAnalogI;
s16 g_NegconAnalogII;
s16 g_NegconAnalogL;
s16 g_RacePhase;

static int s_failures;

static void Check(GameCarDrive *drive, s32 accelerator, s32 brake,
                  const char *description) {
    if (drive->acceleratorInput.value != accelerator ||
        drive->brakeInput != brake) {
        printf("FAIL %s: accelerator=%d brake=%d; expected %d,%d\n",
               description, drive->acceleratorInput.value, drive->brakeInput,
               accelerator, brake);
        s_failures++;
    }
}

static void CheckNegconMapping(s16 mapping, s32 accelerator, s32 brake) {
    GameCarDrive drive;
    char description[40];

    memset(&drive, 0, sizeof(drive));
    g_NegconMappingIndex = mapping;
    ReadPlayerCarInput(&drive);
    snprintf(description, sizeof(description), "NeGcon mapping %d", mapping);
    Check(&drive, accelerator, brake, description);
}

int main(void) {
    GameCarDrive drive;

    memset(g_PadButtonMapping, 0, sizeof(g_PadButtonMapping));
    g_PadButtonMapping[2] = 0x01;
    g_PadButtonMapping[3] = 0x02;
    g_PadButtonMapping[10] = 0x04;
    g_PadButtonMapping[11] = 0x08;
    g_RacePhase = 2;

    memset(&drive, 0, sizeof(drive));
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadHeld = 0x01;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0x100, 0, "digital accelerator");
    g_PadHeld = 0x02;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0, 0x100, "digital brake");

    g_PadType = PAD_TYPE_NEGCON;
    g_PadHeld = 0x0C;
    g_NegconAnalogI = 53;
    g_NegconAnalogII = 106;
    g_NegconAnalogL = 26;
    CheckNegconMapping(0, 128, 256);
    CheckNegconMapping(1, 256, 128);
    CheckNegconMapping(2, 256, (26 << 8) / 106);
    CheckNegconMapping(3, 256, (26 << 8) / 106);
    CheckNegconMapping(4, 256, 256);
    CheckNegconMapping(5, 128, 256);
    CheckNegconMapping(6, 256, 128);
    CheckNegconMapping(7, 256, 256);

    g_NegconAnalogI = 0;
    g_NegconAnalogII = 106;
    CheckNegconMapping(0, 0, 256);

    g_NegconAnalogI = INT16_MAX;
    g_NegconAnalogII = INT16_MIN;
    CheckNegconMapping(0, 13599, -13601);

    memset(&drive, 0x7F, sizeof(drive));
    g_PadType = 0;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0, 0, "unsupported controller");

    memset(&drive, 0x7F, sizeof(drive));
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadHeld = 0x03;
    g_RacePhase = 4;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0, 0, "finished race");

    if (s_failures != 0) {
        printf("%d player input checks failed\n", s_failures);
        return 1;
    }
    puts("player pedals follow every controller mapping");
    return 0;
}
