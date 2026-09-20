#include "car_lamps.h"
#include "render_instance_transform.h"
#include <stddef.h>

float CarLightDaylight(Vec3 sky, Vec3 horizon) {
    /* A bright orange sunset is not daylight. Its warm horizon glow must
     * not keep the lamps off; use the neutral part of that light instead. */
    float skyLight = sky.x * 0.2126f + sky.y * 0.7152f + sky.z * 0.0722f;
    return fmaxf(skyLight, fminf(horizon.x, fminf(horizon.y, horizon.z)));
}

unsigned CarLamps(const RenderMeshInstance *body, const Lamp **lamps) {
    /* Centers projected through model 0/material 3's UV triangles. Keeping
     * the patch and its emitter together prevents independent placement drift. */
    static const Lamp special[] = {
        {3, {12, 131, 28, 139}, {91.88095f, 19.41667f, 434.88889f}, LAMP_HEAD, 0},
        {3, {85, 131, 98, 139}, {-92.98106f, 19.41667f, 434.88889f}, LAMP_HEAD, 0},
        /* Both rear corners reuse this circular lens in the atlas. */
        {1, {74, 222, 80, 229}, {104.73947f, 43.53947f, -77.00053f}, LAMP_TAIL_STOP, 1},
        {1, {74, 222, 80, 229}, {-104.77895f, 43.53947f, -76.81105f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp rival[] = {
        {7, {29, 160, 42, 165}, {-64.43902f, 35.7f, 386.50244f}, LAMP_HEAD, 0},
        {7, {29, 160, 42, 165}, {64.43902f, 35.7f, 386.50244f}, LAMP_HEAD, 0},
        /* Inner rear-fender lenses share UVs, but have separate emitters. */
        {5, {71, 85, 75, 89}, {-114, 4.03590f, -92.69744f}, LAMP_TAIL_STOP, 1},
        {5, {71, 85, 75, 89}, {115.5f, 3.63942f, -92.58494f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp rivalCoupe[] = {
        {18, {83, 175, 100, 181}, {-62.87805f, 37.1f, 387.60488f}, LAMP_HEAD, 0},
        {18, {83, 175, 100, 181}, {62.87805f, 37.1f, 387.60488f}, LAMP_HEAD, 0},
        {15, {67, 163, 72, 173}, {-122, -6.6f, -95.2f}, LAMP_TAIL_STOP, 1},
        {15, {67, 163, 72, 173}, {123, -7, -95.125f}, LAMP_TAIL_STOP, 1},
    };
    /* The distant body has its own atlas panels. Keep the emitters at the
     * same physical positions when changing detail; only the lens UVs change. */
    static const Lamp rivalFar[] = {
        {7, {86, 134, 89, 137}, {-64.43902f, 35.7f, 386.50244f}, LAMP_HEAD, 0},
        {7, {102, 134, 105, 137}, {64.43902f, 35.7f, 386.50244f}, LAMP_HEAD, 0},
        {0, {178, 85, 181, 88}, {-114, 4.03590f, -92.69744f}, LAMP_TAIL_STOP, 1},
        {0, {211, 85, 214, 88}, {115.5f, 3.63942f, -92.58494f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp rivalSport[] = {
        /* The popup covers are closed. Use the round bumper lamps. */
        {24, {124, 46, 131, 52}, {69.39130f, -5.25f, 476.90909f}, LAMP_HEAD, 1},
        {24, {182, 46, 188, 52}, {-71.82609f, -5.25f, 476.90909f}, LAMP_HEAD, 1},
        {23, {16, 58, 26, 65}, {99.78125f, 44.0625f, -77.43125f}, LAMP_TAIL_STOP, 1},
        {23, {16, 58, 26, 65}, {-99.45313f, 44.0625f, -77.29063f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp rivalPrototype[] = {
        {31, {10, 128, 16, 134}, {101.15019f, 17.19011f, 449.28517f}, LAMP_HEAD, 1},
        {31, {80, 128, 86, 134}, {-102.65209f, 17.09125f, 447.63688f}, LAMP_HEAD, 1},
        {28, {103, 142, 109, 148}, {91.63235f, 47, -158.25f}, LAMP_TAIL_STOP, 1},
        {28, {114, 142, 120, 148}, {61.54412f, 47, -158.25f}, LAMP_TAIL_STOP, 1},
        {28, {159, 142, 165, 148}, {-61.54412f, 47, -158.25f}, LAMP_TAIL_STOP, 1},
        {28, {171, 142, 177, 148}, {-94.44444f, 46.94444f, -157.83333f}, LAMP_TAIL_STOP, 1},
    };
    /* These bodies partly share geometry with clubCars, but use different
     * lamp textures. Keep their lens masks tied to their own atlas. */
    static const Lamp sportCars[7][4] = {{
        {0, {10, 8, 30, 14}, {-73.10076f, 24.40558f, 482.14449f}, LAMP_HEAD, 0},
        {0, {65, 8, 85, 14}, {72.65603f, 24.22222f, 482.29078f}, LAMP_HEAD, 0},
        {1, {103, 12, 125, 16}, {87.30842f, 45, -127.2619f}, LAMP_TAIL_STOP, 0},
        {1, {163, 12, 185, 16}, {-90.84265f, 45, -127.2619f}, LAMP_TAIL_STOP, 0},
    }, {
        {5, {6, 52, 18, 64}, {-94.85106f, 34.5f, 498.68085f}, LAMP_HEAD, 1},
        {5, {78, 52, 90, 64}, {97.68554f, 34.91058f, 498.80328f}, LAMP_HEAD, 1},
        {6, {110, 57, 115, 65}, {83.69164f, 43.67647f, -135.73529f}, LAMP_TAIL_STOP, 0},
        {6, {172, 57, 178, 65}, {-83.29161f, 43.67647f, -135.73529f}, LAMP_TAIL_STOP, 0},
    }, {
        {10, {5, 101, 16, 112}, {65.96667f, 46.66667f, 337.33333f}, LAMP_HEAD, 1},
        {10, {79, 101, 91, 112}, {-66.025f, 46.66667f, 337.31f}, LAMP_HEAD, 1},
        /* Only the upper red strips, excluding amber and reversing lenses. */
        {11, {111, 114, 120, 117}, {72.9f, 38.92857f, -58.38571f}, LAMP_TAIL_STOP, 0},
        {11, {168, 114, 177, 117}, {-75.42857f, 38.92857f, -57.21429f}, LAMP_TAIL_STOP, 0},
    }, {
        {14, {3, 150, 26, 155}, {-86.12357f, 28.23954f, 478.97909f}, LAMP_HEAD, 0},
        {14, {69, 150, 94, 155}, {88.47163f, 28.11111f, 478.71986f}, LAMP_HEAD, 0},
        {15, {104, 156, 123, 161}, {87.30842f, 45, -127.2619f}, LAMP_TAIL_STOP, 0},
        {15, {164, 156, 184, 161}, {-90.84265f, 45, -127.2619f}, LAMP_TAIL_STOP, 0},
    }, {
        {17, {10, 7, 30, 12}, {-61.36957f, 21.82353f, 463.48338f}, LAMP_HEAD, 0},
        {17, {65, 7, 85, 12}, {61.36957f, 21.82353f, 463.48338f}, LAMP_HEAD, 0},
        {18, {103, 8, 120, 13}, {68.6087f, 57.875f, -85.13587f}, LAMP_TAIL_STOP, 0},
        {18, {170, 8, 187, 13}, {-74.9375f, 57.875f, -82.77083f}, LAMP_TAIL_STOP, 0},
    }, {
        {22, {7, 53, 16, 61}, {-81.69565f, 22.5f, 451.32609f}, LAMP_HEAD, 1},
        {22, {80, 53, 88, 61}, {82.8913f, 22.5f, 450.65217f}, LAMP_HEAD, 1},
        {23, {103, 59, 122, 64}, {66.45652f, 50.9375f, -91.13315f}, LAMP_TAIL_STOP, 0},
        {23, {166, 59, 186, 64}, {-69.34375f, 50.9375f, -90.01042f}, LAMP_TAIL_STOP, 0},
    }, {
        {26, {7, 100, 25, 111}, {-70.93478f, 21.14706f, 458.2711f}, LAMP_HEAD, 1},
        {26, {70, 100, 88, 111}, {70.93478f, 21.14706f, 458.2711f}, LAMP_HEAD, 1},
        {27, {104, 104, 118, 110}, {69.72826f, 56.71875f, -85.81658f}, LAMP_TAIL_STOP, 0},
        {27, {170, 104, 184, 110}, {-71.70313f, 56.71875f, -84.47396f}, LAMP_TAIL_STOP, 0},
    }};
    static const Lamp expertCars[7][4] = {{
        /* Closed popups: the low round lamps are the exposed front lenses. */
        {0, {5, 26, 15, 35}, {-99.99557f, -5.05226f, 481.18158f}, LAMP_HEAD, 1},
        {0, {81, 26, 90, 35}, {101.34145f, -5.07086f, 481.00044f}, LAMP_HEAD, 1},
        {1, {102, 14, 124, 17}, {93.07807f, 41, -127.35714f}, LAMP_TAIL_STOP, 0},
        {1, {164, 14, 187, 17}, {-99.47516f, 41, -127.35714f}, LAMP_TAIL_STOP, 0},
    }, {
        {5, {8, 67, 24, 72}, {-77.09302f, 35.38161f, 413.63742f}, LAMP_HEAD, 0},
        {5, {72, 67, 88, 72}, {77.09302f, 35.38161f, 413.63742f}, LAMP_HEAD, 0},
        {6, {108, 59, 127, 62}, {63.46178f, 56.52174f, -134.95652f}, LAMP_TAIL_STOP, 0},
        {6, {163, 59, 181, 62}, {-66.6557f, 56.52174f, -134.95652f}, LAMP_TAIL_STOP, 0},
    }, {
        {9, {12, 99, 22, 108}, {77.475f, 43.08f, 401.2f}, LAMP_HEAD, 1},
        {9, {74, 99, 84, 108}, {-80.125f, 43.08f, 400.12f}, LAMP_HEAD, 1},
        {10, {108, 112, 116, 119}, {92.94118f, 41.5f, -57}, LAMP_TAIL_STOP, 0},
        {10, {172, 112, 180, 119}, {-92.90541f, 41.5f, -57}, LAMP_TAIL_STOP, 0},
    }, {
        {12, {5, 152, 22, 158}, {-87.875f, 22.16667f, 405.70833f}, LAMP_HEAD, 0},
        {12, {75, 152, 91, 158}, {89.45238f, 22.12698f, 405.53968f}, LAMP_HEAD, 0},
        {13, {114, 150, 121, 157}, {68.35833f, 39.8f, -118}, LAMP_TAIL_STOP, 1},
        {13, {167, 150, 175, 157}, {-70.96889f, 39.8f, -118}, LAMP_TAIL_STOP, 1},
    }, {
        {16, {7, 6, 26, 12}, {-77.71739f, 25.77778f, 479.41787f}, LAMP_HEAD, 0},
        {16, {71, 6, 90, 12}, {80.1087f, 25.77778f, 479.07005f}, LAMP_HEAD, 0},
        {17, {104, 8, 112, 16}, {96.68085f, 50.875f, -132}, LAMP_TAIL_STOP, 1},
        {17, {176, 8, 184, 16}, {-99.36842f, 50.875f, -132}, LAMP_TAIL_STOP, 1},
    }, {
        {20, {7, 52, 26, 61}, {-77.71739f, 26.55556f, 479.1401f}, LAMP_HEAD, 0},
        {20, {70, 52, 88, 61}, {75.32609f, 26.55556f, 479.48792f}, LAMP_HEAD, 0},
        {21, {107, 59, 115, 67}, {88.51064f, 44.725f, -132}, LAMP_TAIL_STOP, 0},
        {21, {172, 59, 180, 67}, {-88.18526f, 44.725f, -132}, LAMP_TAIL_STOP, 0},
    }, {
        {24, {6, 101, 20, 111}, {-84.8913f, 25, 478.65217f}, LAMP_HEAD, 0},
        {24, {76, 101, 89, 111}, {84.8913f, 25, 478.65217f}, LAMP_HEAD, 0},
        {25, {102, 108, 126, 112}, {80.34043f, 46.775f, -132}, LAMP_TAIL_STOP, 0},
        {25, {162, 108, 186, 112}, {-82.93053f, 46.775f, -132}, LAMP_TAIL_STOP, 0},
    }};
    static const Lamp proCars[7][4] = {{
        {0, {8, 8, 24, 13}, {-84.92776f, 25.13688f, 480.27376f}, LAMP_HEAD, 0},
        {0, {72, 8, 88, 13}, {85.92553f, 25, 480.20213f}, LAMP_HEAD, 0},
        {1, {101, 8, 113, 15}, {108.37542f, 50, -127.14286f}, LAMP_TAIL_STOP, 0},
        {1, {173, 8, 186, 15}, {-108.66615f, 50, -127.14286f}, LAMP_TAIL_STOP, 0},
    }, {
        {5, {10, 56, 27, 64}, {-87.05988f, 51.625f, 500.925f}, LAMP_HEAD, 0},
        {5, {68, 56, 85, 64}, {87.39207f, 51.625f, 500.925f}, LAMP_HEAD, 0},
        {6, {100, 59, 105, 68}, {124.625f, 42.875f, -152.58333f}, LAMP_TAIL_STOP, 0},
        {6, {182, 59, 187, 68}, {-124.38663f, 42.875f, -152.58333f}, LAMP_TAIL_STOP, 0},
    }, {
        {10, {6, 102, 23, 109}, {-46.44262f, 29.29098f, 387.29303f}, LAMP_HEAD, 0},
        {10, {72, 102, 90, 109}, {46.44262f, 29.29098f, 387.29303f}, LAMP_HEAD, 0},
        {11, {102, 103, 113, 110}, {93.8f, 34.95f, -73.725f}, LAMP_TAIL_STOP, 0},
        {11, {175, 103, 185, 110}, {-95.12368f, 34.95f, -73.725f}, LAMP_TAIL_STOP, 0},
    }, {
        {15, {4, 171, 11, 177}, {-113.7432f, -5.00151f, 473.7855f}, LAMP_HEAD, 1},
        {15, {85, 171, 91, 177}, {114.86337f, -4.97384f, 471.34302f}, LAMP_HEAD, 1},
        {16, {103, 149, 111, 155}, {107.76136f, 61.64773f, -109.28409f}, LAMP_TAIL_STOP, 0},
        {16, {176, 149, 184, 155}, {-107.44828f, 61.82759f, -108.93103f}, LAMP_TAIL_STOP, 0},
    }, {
        {19, {7, 5, 25, 11}, {-77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {19, {70, 5, 90, 11}, {77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {20, {102, 10, 124, 15}, {83.06383f, 48.825f, -132}, LAMP_TAIL_STOP, 0},
        {20, {164, 10, 186, 15}, {-85.76f, 48.825f, -132}, LAMP_TAIL_STOP, 0},
    }, {
        {23, {6, 71, 12, 75}, {-91.98455f, 1.49024f, 481.49675f}, LAMP_HEAD, 0},
        {23, {83, 71, 89, 75}, {91.98455f, 1.49024f, 481.49675f}, LAMP_HEAD, 0},
        {24, {108, 58, 122, 65}, {77.61702f, 48.825f, -132}, LAMP_TAIL_STOP, 0},
        {24, {166, 58, 180, 65}, {-80.34043f, 48.825f, -132}, LAMP_TAIL_STOP, 0},
    }, {
        {27, {6, 100, 23, 108}, {-77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {27, {72, 100, 90, 108}, {77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {28, {112, 106, 120, 112}, {74.89362f, 48.825f, -132}, LAMP_TAIL_STOP, 0},
        {28, {168, 106, 176, 112}, {-77.61702f, 48.825f, -132}, LAMP_TAIL_STOP, 0},
    }};
    static const Lamp eliteCars[7][4] = {[0] = {
        {0, {7, 8, 23, 13}, {-86.24525f, 25.13181f, 480.09696f}, LAMP_HEAD, 0},
        {0, {73, 8, 89, 13}, {88.58511f, 25, 479.84043f}, LAMP_HEAD, 0},
        {1, {103, 13, 121, 16}, {93.21595f, 44, -127.28571f}, LAMP_TAIL_STOP, 0},
        {1, {169, 13, 187, 16}, {-99.60404f, 44, -127.28571f}, LAMP_TAIL_STOP, 0},
    }, [1] = {
        {5, {16, 79, 26, 86}, {-76.41558f, 3.28571f, 508.85714f}, LAMP_HEAD, 0},
        {5, {69, 79, 79, 86}, {76.46013f, 3.28571f, 508.85714f}, LAMP_HEAD, 0},
        {6, {101, 55, 106, 62}, {120.37791f, 61.05556f, -149.69444f}, LAMP_TAIL_STOP, 0},
        {6, {182, 55, 187, 62}, {-121.97222f, 61.05556f, -149.69444f}, LAMP_TAIL_STOP, 0},
    }, [4] = {
        {20, {7, 5, 25, 11}, {-77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {20, {70, 5, 90, 11}, {77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {21, {102, 14, 119, 17}, {88.51064f, 44.725f, -132}, LAMP_TAIL_STOP, 0},
        {21, {168, 14, 186, 17}, {-90.88f, 44.725f, -132}, LAMP_TAIL_STOP, 0},
    }, [5] = {
        {24, {6, 52, 26, 60}, {-77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {24, {70, 52, 90, 60}, {77.71739f, 27.33333f, 478.86232f}, LAMP_HEAD, 0},
        {25, {102, 62, 114, 65}, {96.68085f, 42.675f, -132}, LAMP_TAIL_STOP, 0},
        {25, {172, 62, 185, 65}, {-96.13474f, 42.675f, -132}, LAMP_TAIL_STOP, 0},
    }, [6] = {
        {28, {9, 103, 25, 109}, {-72.93478f, 24.22222f, 480.66908f}, LAMP_HEAD, 0},
        {28, {70, 103, 86, 109}, {72.93478f, 24.22222f, 480.66908f}, LAMP_HEAD, 0},
        {29, {117, 104, 125, 112}, {61.2766f, 50.875f, -132}, LAMP_TAIL_STOP, 1},
        {29, {163, 104, 171, 112}, {-64, 50.875f, -132}, LAMP_TAIL_STOP, 1},
    }};
    static const Lamp clubCars[7][4] = {
        {
            {0, {5, 4, 25, 12}, {-88.72814f, 29.00634f, 478.34601f}, LAMP_HEAD, 0},
            {0, {70, 4, 90, 12}, {85.78369f, 28.88889f, 478.80142f}, LAMP_HEAD, 0},
            {1, {102, 15, 122, 18}, {93.03212f, 40, -127.38095f}, LAMP_TAIL_STOP, 0},
            {1, {167, 15, 187, 18}, {-99.43219f, 40, -127.38095f}, LAMP_TAIL_STOP, 0},
        }, {
            {5, {4, 53, 12, 61}, {-104.59886f, 27.39163f, 476.78327f}, LAMP_HEAD, 1},
            {5, {83, 53, 91, 61}, {104.45745f, 27.33333f, 476.82979f}, LAMP_HEAD, 1},
            {6, {102, 57, 110, 65}, {114.19103f, 47, -127.21429f}, LAMP_TAIL_STOP, 1},
            {6, {177, 57, 185, 65}, {-111.47205f, 47, -127.21429f}, LAMP_TAIL_STOP, 1},
        }, {
            {8, {5, 103, 28, 110}, {-83.64068f, 24.36502f, 480.73004f}, LAMP_HEAD, 0},
            {8, {68, 103, 92, 110}, {83.29433f, 24.22222f, 480.84397f}, LAMP_HEAD, 0},
            {9, {102, 111, 122, 114}, {93.03212f, 40, -127.38095f}, LAMP_TAIL_STOP, 0},
            {9, {167, 111, 187, 114}, {-99.43219f, 40, -127.38095f}, LAMP_TAIL_STOP, 0},
        }, {
            {11, {5, 149, 26, 154}, {-86.03232f, 30.57034f, 478.14068f}, LAMP_HEAD, 0},
            {11, {70, 149, 91, 154}, {85.72695f, 30.44444f, 478.24113f}, LAMP_HEAD, 0},
            {12, {102, 161, 116, 164}, {101.82447f, 37, -127.45238f}, LAMP_TAIL_STOP, 0},
            {12, {173, 161, 187, 164}, {-108.10766f, 37, -127.45238f}, LAMP_TAIL_STOP, 0},
        }, {
            {14, {7, 5, 22, 12}, {-74.52174f, 23.17647f, 455.12532f}, LAMP_HEAD, 0},
            {14, {73, 5, 88, 12}, {74.52174f, 23.17647f, 455.12532f}, LAMP_HEAD, 0},
            {15, {107, 5, 114, 12}, {70.78261f, 62.5f, -80.97826f}, LAMP_TAIL_STOP, 1},
            {15, {174, 5, 181, 12}, {-73, 62.5f, -79.16667f}, LAMP_TAIL_STOP, 1},
        }, {
            {19, {10, 53, 26, 59}, {-68.54348f, 23.85294f, 458.25064f}, LAMP_HEAD, 0},
            {19, {69, 53, 85, 59}, {68.54348f, 23.85294f, 458.25064f}, LAMP_HEAD, 0},
            {20, {104, 57, 123, 61}, {63.07609f, 56.71875f, -87.25136f}, LAMP_TAIL_STOP, 0},
            {20, {166, 57, 184, 61}, {-65.32813f, 56.71875f, -85.84896f}, LAMP_TAIL_STOP, 0},
        }, {
            {23, {8, 103, 16, 111}, {-80.5f, 19.79412f, 453.41176f}, LAMP_HEAD, 1},
            {23, {79, 103, 87, 111}, {80.5f, 19.79412f, 453.41176f}, LAMP_HEAD, 1},
            {24, {101, 106, 127, 112}, {58.68478f, 52.09375f, -91.88723f}, LAMP_TAIL_STOP, 0},
            {24, {160, 106, 186, 112}, {-60.89063f, 52.09375f, -90.82813f}, LAMP_TAIL_STOP, 0},
        },
    };
    static const Lamp compact[] = {
        {3, {204, 46, 212, 57}, {-64.88971f, 41.61765f, 342.73235f}, LAMP_HEAD, 1},
        {3, {204, 118, 212, 129}, {64.42647f, 41.61765f, 342.91765f}, LAMP_HEAD, 1},
        /* Upper red lenses only; leave the white and amber sections unlit. */
        {0, {110, 13, 113, 17}, {74.35294f, 47, -48.73529f}, LAMP_TAIL_STOP, 0},
        {0, {174, 13, 177, 17}, {-74.13333f, 47, -47.5f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp coupe[] = {
        /* White inner lenses; the amber outer corners are indicators. */
        {3, {15, 8, 33, 13}, {58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        {3, {62, 8, 80, 13}, {-58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        /* Lower outer rear lenses, excluding the white reversing lights. */
        {0, {102, 15, 110, 20}, {107.25127f, 48.48477f, -51.85787f}, LAMP_TAIL_STOP, 1},
        {0, {177, 15, 185, 20}, {-107.09167f, 47.95f, -51.76667f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp compactUpgrade[] = {
        /* Upgrade uses material 4 in front and the lower rear atlas panel. */
        {4, {204, 46, 212, 57}, {-64.01471f, 41.61765f, 343.08235f}, LAMP_HEAD, 1},
        {4, {204, 118, 212, 129}, {63.55147f, 41.61765f, 343.26765f}, LAMP_HEAD, 1},
        {0, {14, 205, 17, 209}, {74.35294f, 47, -48.73529f}, LAMP_TAIL_STOP, 0},
        {0, {78, 205, 81, 209}, {-74.13333f, 47, -47.5f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp coupe1[] = {
        {3, {15, 176, 33, 181}, {58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        {3, {62, 176, 80, 181}, {-58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        {0, {203, 102, 208, 110}, {107.255f, 47.95f, -52.175f}, LAMP_TAIL_STOP, 1},
        {0, {203, 177, 208, 185}, {-107.09167f, 47.95f, -51.76667f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp coupe2[] = {
        {3, {15, 176, 33, 181}, {-58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        {3, {62, 176, 80, 181}, {58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        {0, {101, 207, 108, 211}, {108.61538f, 44.87660f, -52.03846f}, LAMP_TAIL_STOP, 1},
        {0, {180, 207, 187, 211}, {-111.07692f, 44.1875f, -51.80769f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp sport[] = {
        {3, {7, 3, 16, 11}, {-49.55208f, 33.20833f, 381.82292f}, LAMP_HEAD, 1},
        {3, {80, 3, 89, 11}, {50.60417f, 33.16667f, 381.27083f}, LAMP_HEAD, 1},
        {0, {100, 6, 109, 11}, {102.78723f, 39.55319f, -74.76596f}, LAMP_TAIL_STOP, 1},
        {0, {178, 6, 187, 11}, {-101.87805f, 39.58537f, -74.78049f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp compact2[] = {
        {4, {5, 10, 17, 21}, {-64.86029f, 40.88235f, 341.46765f}, LAMP_HEAD, 1},
        {4, {78, 10, 90, 21}, {64.44853f, 40.88235f, 341.63235f}, LAMP_HEAD, 1},
        {0, {14, 205, 17, 209}, {74.35294f, 47, -48.73529f}, LAMP_TAIL_STOP, 0},
        {0, {78, 205, 81, 209}, {-74.13333f, 47, -47.5f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp sport1[] = {
        {3, {103, 189, 112, 197}, {-49.55208f, 33.20833f, 381.82292f}, LAMP_HEAD, 1},
        {3, {176, 189, 185, 197}, {50.60417f, 33.16667f, 381.27083f}, LAMP_HEAD, 1},
        {0, {109, 151, 114, 158}, {84.56662f, 36.47826f, -74.08696f}, LAMP_TAIL_STOP, 1},
        {0, {174, 151, 179, 158}, {-85.94279f, 36.47826f, -74.08696f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp compact3[] = {
        {3, {5, 10, 17, 21}, {-69.07795f, 36.88023f, 357.18631f}, LAMP_HEAD, 1},
        {3, {78, 10, 90, 21}, {69.07795f, 36.88023f, 357.18631f}, LAMP_HEAD, 1},
        {0, {244, 113, 250, 119}, {64.93048f, 39.63333f, -90}, LAMP_TAIL_STOP, 1},
        {0, {244, 170, 250, 176}, {-68.36596f, 39.63333f, -90}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp sedan[] = {
        {3, {10, 2, 30, 9}, {-68.25f, 29.66667f, 482.52381f}, LAMP_HEAD, 0},
        {3, {65, 2, 86, 9}, {69.66667f, 29.66667f, 482.09524f}, LAMP_HEAD, 0},
        {0, {104, 6, 120, 9}, {91.47727f, 65.09091f, -127.29545f}, LAMP_TAIL_STOP, 0},
        {0, {167, 6, 184, 9}, {-93.52727f, 65.2f, -126.34545f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp muscle[] = {
        {3, {6, 4, 32, 9}, {-73.37234f, 37.83191f, 497.28511f}, LAMP_HEAD, 0},
        {3, {64, 4, 90, 9}, {74.95738f, 38.30213f, 497.02892f}, LAMP_HEAD, 0},
        {0, {103, 8, 109, 16}, {100.95294f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
        {0, {111, 8, 117, 16}, {79.56347f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
        {0, {171, 8, 177, 16}, {-80.67084f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
        {0, {179, 8, 185, 16}, {-101.94743f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp muscle1[] = {
        {4, {6, 4, 32, 9}, {-73.37234f, 37.83191f, 497.28511f}, LAMP_HEAD, 0},
        {4, {64, 4, 90, 9}, {74.95738f, 38.30213f, 497.02892f}, LAMP_HEAD, 0},
        {0, {103, 8, 109, 16}, {100.95294f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
        {0, {111, 8, 117, 16}, {79.56347f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
        {0, {171, 8, 177, 16}, {-80.67084f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
        {0, {179, 8, 185, 16}, {-101.94743f, 45.38235f, -135.67647f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp muscle3[] = {
        {5, {6, 4, 32, 9}, {-73.37234f, 22.77888f, 498.56763f}, LAMP_HEAD, 0},
        {5, {64, 4, 90, 9}, {75.94681f, 23.19885f, 498.22831f}, LAMP_HEAD, 0},
        {0, {103, 8, 109, 16}, {100.95294f, 49.38235f, -133.67647f}, LAMP_TAIL_STOP, 1},
        {0, {111, 8, 117, 16}, {79.56347f, 49.38235f, -133.67647f}, LAMP_TAIL_STOP, 1},
        {0, {171, 8, 177, 16}, {-80.67084f, 49.38235f, -133.67647f}, LAMP_TAIL_STOP, 1},
        {0, {179, 8, 185, 16}, {-101.94743f, 49.38235f, -133.67647f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp sedan12[] = {
        {3, {10, 162, 30, 169}, {-68.25f, 29.66667f, 482.52381f}, LAMP_HEAD, 0},
        {3, {65, 162, 86, 169}, {69.66667f, 29.66667f, 482.09524f}, LAMP_HEAD, 0},
        {0, {104, 6, 120, 9}, {91.47727f, 65.09091f, -127.29545f}, LAMP_TAIL_STOP, 0},
        {0, {167, 6, 184, 9}, {-93.52727f, 65.2f, -126.34545f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp sedan3[] = {
        {5, {106, 162, 126, 169}, {-75.375f, 29.66667f, 485.07738f}, LAMP_HEAD, 0},
        {5, {161, 162, 182, 169}, {77.16667f, 29.66667f, 484.97619f}, LAMP_HEAD, 0},
        {0, {104, 6, 120, 9}, {91.32143f, 67, -126.64286f}, LAMP_TAIL_STOP, 0},
        {0, {167, 6, 184, 9}, {-93.64706f, 67, -126.97059f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp sedan4[] = {
        {3, {248, 20, 254, 36}, {-70.25f, 24.11538f, 484.32308f}, LAMP_HEAD, 0},
        {3, {248, 76, 254, 92}, {72.92073f, 23.71951f, 484.12195f}, LAMP_HEAD, 0},
        {0, {242, 124, 247, 138}, {102.01934f, 61.40055f, -130.9558f}, LAMP_TAIL_STOP, 0},
        {0, {242, 196, 247, 210}, {-99.17665f, 61.4521f, -131.29341f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp wedge[] = {
        /* Existing bumper driving lamps; the pop-up covers stay opaque. */
        {1, {10, 41, 25, 47}, {-64.35112f, 10.52357f, 465.31514f}, LAMP_HEAD, 1},
        {1, {70, 41, 86, 47}, {64.97467f, 10.812f, 464.78267f}, LAMP_HEAD, 1},
        {0, {104, 12, 117, 16}, {78.81556f, 53.58696f, -136.32609f}, LAMP_TAIL_STOP, 0},
        {0, {170, 12, 184, 16}, {-78.40658f, 53.58696f, -136.32609f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp wedge1[] = {
        {1, {10, 41, 25, 47}, {64.36037f, 10.81649f, 465.05053f}, LAMP_HEAD, 1},
        {1, {70, 41, 86, 47}, {-65.44293f, 10.51737f, 464.83747f}, LAMP_HEAD, 1},
        {0, {104, 12, 117, 16}, {78.81556f, 53.58696f, -136.32609f}, LAMP_TAIL_STOP, 0},
        {0, {170, 12, 184, 16}, {-78.40658f, 53.58696f, -136.32609f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp wedge2[] = {
        {1, {10, 234, 25, 240}, {64.35112f, 0.74814f, 465.31514f}, LAMP_HEAD, 1},
        {1, {70, 234, 86, 240}, {-65.44293f, 0.76179f, 464.83747f}, LAMP_HEAD, 1},
        {0, {104, 12, 117, 16}, {78.96339f, 56.58696f, -134.91304f}, LAMP_TAIL_STOP, 0},
        {0, {170, 12, 184, 16}, {-78.46256f, 56.58696f, -134.91304f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp truck[] = {
        {3, {7, 9, 26, 15}, {-82.43182f, 51.72727f, 502.25f}, LAMP_HEAD, 0},
        {3, {70, 9, 89, 15}, {85.09091f, 51.72727f, 502}, LAMP_HEAD, 0},
        /* Red inner rear lenses; the amber indicators are outside. */
        {0, {120, 28, 131, 32}, {54.99630f, 11.70833f, -150.08333f}, LAMP_TAIL_STOP, 0},
        {0, {157, 28, 169, 32}, {-58.34496f, 11.70833f, -150.08333f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp truck1[] = {
        {3, {7, 9, 26, 15}, {-82.43182f, 41.13636f, 502.95455f}, LAMP_HEAD, 0},
        {3, {70, 9, 89, 15}, {85.09091f, 41.13636f, 503}, LAMP_HEAD, 0},
        {0, {120, 28, 131, 32}, {54.99630f, 14.70833f, -149.375f}, LAMP_TAIL_STOP, 0},
        {0, {157, 28, 169, 32}, {-58.34496f, 14.70833f, -149.375f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp exotic2[] = {
        {2, {227, 5, 237, 17}, {99.55495f, 21.38462f, 439.6978f}, LAMP_HEAD, 1},
        {2, {227, 79, 237, 91}, {-101.65385f, 21.47692f, 439.07385f}, LAMP_HEAD, 1},
        {0, {21, 248, 26, 252}, {59.545f, 33.5f, -125.5f}, LAMP_TAIL_STOP, 1},
        {0, {70, 248, 75, 252}, {-62.02315f, 33.5f, -125.5f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp prototype1[] = {
        {2, {6, 6, 17, 10}, {75.56522f, -6.53261f, 508.46429f}, LAMP_HEAD, 0},
        {2, {79, 6, 90, 10}, {-76.09627f, -6.33385f, 508.59472f}, LAMP_HEAD, 0},
        {0, {100, 9, 111, 13}, {106.57895f, 57.52632f, -106.73684f}, LAMP_TAIL_STOP, 0},
        {0, {176, 9, 187, 13}, {-105.23134f, 58.44403f, -106.3694f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp exotic[] = {
        {3, {5, 3, 17, 13}, {93.55f, 19.01667f, 423.38333f}, LAMP_HEAD, 1},
        {3, {79, 3, 91, 13}, {-96.5f, 18.66667f, 423.58333f}, LAMP_HEAD, 1},
        {0, {117, 8, 122, 12}, {59.54615f, 32.69231f, -125.76923f}, LAMP_TAIL_STOP, 1},
        {0, {166, 8, 171, 12}, {-62.02422f, 32.69231f, -125.76923f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp prototype[] = {
        {2, {6, 6, 17, 10}, {75.56522f, -5.53261f, 508.46429f}, LAMP_HEAD, 0},
        {2, {79, 6, 90, 10}, {-76.09627f, -5.33385f, 508.59472f}, LAMP_HEAD, 0},
        {0, {100, 9, 111, 13}, {106.57895f, 57.52632f, -106.73684f}, LAMP_TAIL_STOP, 0},
        {0, {176, 9, 187, 13}, {-105.3f, 57.55f, -106.95f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp racer[] = {
        {3, {12, 90, 17, 95}, {-107.77885f, 4.75481f, 491.31731f}, LAMP_HEAD, 0},
        {3, {78, 90, 83, 95}, {106.93496f, 2.16667f, 491.19106f}, LAMP_HEAD, 0},
        /* Both rear corners reuse the inner circular red lens. */
        {0, {204, 5, 208, 10}, {96.40606f, 34.8f, -108.96364f}, LAMP_TAIL_STOP, 1},
        {0, {204, 5, 208, 10}, {-95.83943f, 33.89634f, -108.60976f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp vintage[] = {
        {7, {90, 22, 106, 28}, {-61.17073f, 37.1f, 387.87317f}, LAMP_HEAD, 0},
        {7, {90, 22, 106, 28}, {61.17073f, 37.1f, 387.87317f}, LAMP_HEAD, 0},
        {6, {94, 164, 99, 169}, {-115.5f, -2.87097f, -94.62298f}, LAMP_TAIL_STOP, 1},
        {6, {94, 164, 99, 169}, {115.5f, -3.30847f, -94.62298f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp concept[] = {
        {4, {21, 91, 35, 100}, {77.41462f, 14.83357f, 469.47323f}, LAMP_HEAD, 1},
        {4, {93, 91, 107, 100}, {-79.52826f, 14.49246f, 467.80445f}, LAMP_HEAD, 1},
        /* One continuous red strip, with spill from both ends. */
        {0, {5, 239, 81, 244}, {77.95588f, 46, -158.5f}, LAMP_TAIL_STOP, 0},
        {0, {5, 239, 81, 244}, {-77.95588f, 46, -158.5f}, LAMP_TAIL_STOP, 0},
    };
    static const struct {
        unsigned key;
        const Lamp *lamps;
        unsigned count;
    } players[] = {
        {10, compact, sizeof(compact) / sizeof(*compact)},
        {12, compactUpgrade, sizeof(compactUpgrade) / sizeof(*compactUpgrade)},
        {14, compact2, sizeof(compact2) / sizeof(*compact2)},
        {16, compact3, sizeof(compact3) / sizeof(*compact3)},
        {18, coupe, sizeof(coupe) / sizeof(*coupe)},
        {20, coupe1, sizeof(coupe1) / sizeof(*coupe1)},
        {22, coupe2, sizeof(coupe2) / sizeof(*coupe2)},
        {24, sport, sizeof(sport) / sizeof(*sport)},
        {26, sport1, sizeof(sport1) / sizeof(*sport1)},
        {28, sedan, sizeof(sedan) / sizeof(*sedan)},
        {30, sedan12, sizeof(sedan12) / sizeof(*sedan12)},
        {32, sedan12, sizeof(sedan12) / sizeof(*sedan12)},
        {34, sedan3, sizeof(sedan3) / sizeof(*sedan3)},
        {36, sedan4, sizeof(sedan4) / sizeof(*sedan4)},
        {38, muscle, sizeof(muscle) / sizeof(*muscle)},
        {40, muscle1, sizeof(muscle1) / sizeof(*muscle1)},
        {44, muscle3, sizeof(muscle3) / sizeof(*muscle3)},
        {46, wedge, sizeof(wedge) / sizeof(*wedge)},
        {48, wedge1, sizeof(wedge1) / sizeof(*wedge1)},
        {50, wedge2, sizeof(wedge2) / sizeof(*wedge2)},
        {52, truck, sizeof(truck) / sizeof(*truck)},
        {54, truck1, sizeof(truck1) / sizeof(*truck1)},
        {56, exotic, sizeof(exotic) / sizeof(*exotic)},
        {58, exotic, sizeof(exotic) / sizeof(*exotic)},
        {60, exotic2, sizeof(exotic2) / sizeof(*exotic2)},
        {62, prototype, sizeof(prototype) / sizeof(*prototype)},
        {64, prototype1, sizeof(prototype1) / sizeof(*prototype1)},
        {66, racer, sizeof(racer) / sizeof(*racer)},
        {68, special, sizeof(special) / sizeof(*special)},
        {70, vintage, sizeof(vintage) / sizeof(*vintage)},
        {72, concept, sizeof(concept) / sizeof(*concept)},
    };
    *lamps = NULL;
    if (body->component != 0) return 0;
    /* The three road courses share their class bank (mesh AND textures).
     * Oval banks are separate; only explicitly verified aliases belong here. */
    uint32_t bank = body->assetKey;
    if ((bank & 1u) == 0 && (bank & 6u) != 6u) bank &= ~7u;
    if (bank == 94) bank = 120;
    if (bank == 134) bank = 128;
    if (body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK) {
        if (body->mesh != 0) return 0;
        for (unsigned i = 0; i < sizeof(players) / sizeof(*players); ++i) {
            if (players[i].key != body->assetKey) continue;
            *lamps = players[i].lamps;
            return players[i].count;
        }
    } else if (body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
               bank == 104 && body->mesh <= 30 && body->mesh % 5 == 0) {
        *lamps = expertCars[body->mesh / 5];
        return 4;
    } else if (body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
               bank == 112 && body->mesh <= 30 && body->mesh % 5 == 0) {
        *lamps = proCars[body->mesh / 5];
        return 4;
    } else if (body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
               bank == 120 && body->mesh <= 30 && body->mesh % 5 == 0 &&
               body->mesh != 10 && body->mesh != 15) {
        *lamps = eliteCars[body->mesh / 5];
        return 4;
    } else if (body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
               bank == 96 &&
               body->mesh <= 30 && body->mesh % 5 == 0) {
        *lamps = sportCars[body->mesh / 5];
        return 4;
    } else if (body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
               bank == 88 && body->mesh <= 30 &&
               body->mesh % 5 == 0) {
        *lamps = clubCars[body->mesh / 5];
        return 4;
    } else if (body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
               bank == 128) {
        if (body->mesh == 0) {
            *lamps = rival;
            return sizeof(rival) / sizeof(*rival);
        }
        if (body->mesh == 5) {
            *lamps = rivalCoupe;
            return sizeof(rivalCoupe) / sizeof(*rivalCoupe);
        }
        if (body->mesh == 4) {
            *lamps = rivalFar;
            return sizeof(rivalFar) / sizeof(*rivalFar);
        }
        if (body->mesh == 10) {
            *lamps = rivalSport;
            return sizeof(rivalSport) / sizeof(*rivalSport);
        }
        if (body->mesh == 15) {
            *lamps = rivalPrototype;
            return sizeof(rivalPrototype) / sizeof(*rivalPrototype);
        }
    }
    return 0;
}

float CarLampIntensity(const CarLights *state, LampKind kind) {
    switch (kind) {
    case LAMP_HEAD: return state->headlights;
    case LAMP_TAIL: return state->tail;
    case LAMP_STOP: return state->stop;
    case LAMP_TAIL_STOP: return fmaxf(state->tail, state->stop);
    }
    return 0;
}

void RenderCarSpotLights(RenderWorld *world) {
    for (uint32_t i = 0; i < world->instanceCount; ++i) {
        const RenderMeshInstance *body = &world->instances[i];
        const Lamp *lamps;
        if (body->pass != RAGE_RENDER_PASS_MAIN) continue;
        unsigned count = CarLamps(body, &lamps);
        if (!count) continue;
        RenderInstanceTransform transform = RenderPrepareInstanceTransform(&body->transform);
        for (unsigned j = 0; j < count; ++j) {
            const Lamp *lamp = &lamps[j];
            float strength = CarLampIntensity(&body->lamps, lamp->kind);
            if (strength <= 0) continue;
            int front = lamp->kind == LAMP_HEAD;
            SpotLight light = {0};
            light.position = RenderTransformInstancePoint(&transform, lamp->position);
            light.direction = RenderRotateInstanceVector(&transform,
                (Vec3){0, -0.06f, front ? 1.0f : -1.0f});
            light.range = front ? 1200.0f : 160.0f;
            light.innerCos = front ? 0.96f : 0.75f;
            light.outerCos = front ? 0.80f : 0.25f;
            light.color = front
                ? (Vec3){strength * 5, strength * 4.7f, strength * 4}
                : (Vec3){strength * 1.5f, strength * 0.025f, strength * 0.01f};
            RenderWorldSubmitSpotLight(world, &light);
        }
    }
}
