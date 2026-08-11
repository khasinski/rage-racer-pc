#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/asset.h"
#include "game/render.h"

/* Called here with no argument, so this declaration must stay un-prototyped. */

/* GetCarAssetIndex(model, owned grade) written out longhand; indexes the price and engine tables. */
s32 GetOwnedCarAssetIndex(s32 model) {
    s32 state;
    u32 value;

    state = GetCarUnlockLevel(model) + 1;

    switch (model) {
    case 0:
        switch (state) {
        case 2:
            return 0;
        case 3:
            return 1;
        case 4:
            return 2;
        case 5:
            return 3;
        }


    case 1:
        switch (state) {
        case 3:
            return 4;
        case 4:
            return 5;
        case 5:
            return 6;
        }


    case 2:
        switch (state) {
        case 4:
            return 7;
        case 5:
            return 8;
        }


    case 3:
        value = state - 1;
        if (value < 5) {
            switch (value) {
            case 0:
                return 9;
            case 1:
                return 10;
            case 2:
                return 11;
            case 3:
                return 12;
            case 4:
                return 13;
            }
        }

    case 4:
        switch (state) {
        case 2:
            return 14;
        case 3:
            return 15;
        case 4:
            return 16;
        case 5:
            return 17;
        }


    case 5:
        switch (state) {
        case 3:
            return 18;
        case 4:
            return 19;
        case 5:
            return 20;
        }


    case 6:
        switch (state) {
        case 4:
            return 21;
        case 5:
            return 22;
        }


    case 7:
        switch (state) {
        case 3:
            return 23;
        case 4:
            return 24;
        case 5:
            return 25;
        }


    case 8:
        switch (state) {
        case 4:
            return 26;
        case 5:
            return 27;
        }
        return 28;

    case 9:
        return 28;
    case 10:
        return 29;
    case 11:
        return 30;
    case 12:
        return 31;
    }

    return -1;
}

/* Re-registers the showroom car after g_CarModelSlot changes. */
void InstallCarModelSlot(void) {
    SelectCarModelSlot(g_CarModelSlot);
    SelectModelBank(g_CarModelSlot);
    UploadCarImage(g_CarModelSlot);
}
