#include "car_lamps.h"
#include "render_instance_transform.h"
#include <stddef.h>

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
    if (body->component != 0 || body->mesh != 0) return 0;
    if (body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK) {
        for (unsigned i = 0; i < sizeof(players) / sizeof(*players); ++i) {
            if (players[i].key != body->assetKey) continue;
            *lamps = players[i].lamps;
            return players[i].count;
        }
    } else if (body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
               body->assetKey == 128) {
        *lamps = rival;
        return sizeof(rival) / sizeof(*rival);
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
