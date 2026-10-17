#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif

#include "port/platform_paths.h"
#include "content/track_pack.h"
#include "port/car_catalog.h"
#include "yyjson.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* PSY-Q's Win64 u_long is 64-bit; Winsock's network ABI requires 32-bit. */
#define u_long WinsockUnsignedLong
#include <winsock2.h>
#include <ws2tcpip.h>
#undef u_long
#include <io.h>
#include <fcntl.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static uint32_t ReadU32LE(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void WriteU32LE(unsigned char *p, uint32_t n) {
    p[0] = (unsigned char)(n & 0xffu);
    p[1] = (unsigned char)((n >> 8) & 0xffu);
    p[2] = (unsigned char)((n >> 16) & 0xffu);
    p[3] = (unsigned char)((n >> 24) & 0xffu);
}

static int16_t ClampI16(double v) {
    long r = lround(v);
    if (r < -32768L) return -32768;
    if (r > 32767L) return 32767;
    return (int16_t)r;
}

static int32_t ClampI32(double v) {
    long long r = llround(v);
    if (r < (long long)INT32_MIN) return INT32_MIN;
    if (r > (long long)INT32_MAX) return INT32_MAX;
    return (int32_t)r;
}

static double NormalizeAngleDeltaRad(double rad) {
    while (rad > M_PI) rad -= 2.0 * M_PI;
    while (rad < -M_PI) rad += 2.0 * M_PI;
    return rad;
}

static void RecalculateRoadGeometry(const RoadPoint *basePts, RoadPoint *draftPts, uint32_t n) {
    uint32_t i;
    if (n == 0) return;
    for (i = 0; i < n; i++) {
        uint32_t nextIdx = (i + 1u) % n;
        const RoadPoint *b0 = &basePts[i];
        const RoadPoint *b1 = &basePts[nextIdx];
        RoadPoint *d0 = &draftPts[i];
        const RoadPoint *d1 = &draftPts[nextIdx];
        int moved = (d0->x != b0->x || d0->y != b0->y || d0->z != b0->z ||
                     d1->x != b1->x || d1->y != b1->y || d1->z != b1->z);
        if (!moved) continue;
        double origDx = (double)b1->x - (double)b0->x;
        double origDz = (double)b1->z - (double)b0->z;
        double newDx = (double)d1->x - (double)d0->x;
        double newDz = (double)d1->z - (double)d0->z;
        double origChord = hypot(origDx, origDz);
        double newChord = hypot(newDx, newDz);
        double ratio = origChord > 1.0 ? (newChord / origChord) : 1.0;
        long lenVal = lround((double)(b0->length ? b0->length : 64u) * ratio);
        if (lenVal < 1L) lenVal = 1L;
        if (lenVal > 32767L) lenVal = 32767L;
        d0->length = (uint16_t)lenVal;

        if (newChord > 0.5 && origChord > 0.5) {
            double dRad = NormalizeAngleDeltaRad(atan2(newDz, newDx) - atan2(origDz, origDx));
            int dUnits = (int)lround((dRad * 4096.0) / (2.0 * M_PI));
            int raw = (int)b0->angle + dUnits;
            if (b0->angle < 0) {
                d0->angle = (int16_t)((((raw + 2048) % 4096 + 4096) % 4096) - 2048);
            } else {
                d0->angle = (int16_t)(((raw % 4096) + 4096) % 4096);
            }
        }
        double origPitch = atan2((double)b1->y - (double)b0->y, origChord > 1.0 ? origChord : 1.0);
        double newPitch = atan2((double)d1->y - (double)d0->y, newChord > 1.0 ? newChord : 1.0);
        int dPitch = (int)lround((NormalizeAngleDeltaRad(newPitch - origPitch) * 4096.0) / (2.0 * M_PI));
        d0->pitch = ClampI16((double)b0->pitch + (double)dPitch);
    }
}

/* Try calling the live Studio HTTP server on 127.0.0.1:4173 if it is running.
 * Returns a newly allocated yyjson_doc* on HTTP 200, or NULL if offline. */
static yyjson_doc *CallLiveStudioHttp(const char *method, const char *argsJson) {
    int port = 4173;
    const char *envPort = getenv("RAGE_STUDIO_PORT");
    if (envPort && atoi(envPort) > 0) port = atoi(envPort);

#ifdef _WIN32
    static int wsaInit = 0;
    if (!wsaInit) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) wsaInit = 1;
    }
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return NULL;
    DWORD timeoutMs = 4000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeoutMs, sizeof(timeoutMs));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeoutMs, sizeof(timeoutMs));
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NULL;
    struct timeval tv;
    tv.tv_sec = 4;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(0x7f000001u);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return NULL;
    }

    char body[65536];
    snprintf(body, sizeof(body), "{\"args\":%s}", argsJson ? argsJson : "[]");
    size_t bodyLen = strlen(body);

    char reqHeader[1024];
    int reqLen = snprintf(
        reqHeader, sizeof(reqHeader),
        "POST /api/%s HTTP/1.0\r\n"
        "Host: 127.0.0.1:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        method, port, bodyLen);

    if (reqLen <= 0 || (size_t)reqLen >= sizeof(reqHeader)) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return NULL;
    }

    (void)send(sock, reqHeader, reqLen, 0);
    (void)send(sock, body, (int)bodyLen, 0);

    size_t cap = 65536;
    size_t len = 0;
    char *resp = (char *)malloc(cap);
    if (!resp) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return NULL;
    }

    for (;;) {
        if (len + 4096 >= cap) {
            size_t nextCap = cap * 2;
            if (nextCap > 32u * 1024u * 1024u) break;
            char *next = (char *)realloc(resp, nextCap);
            if (!next) break;
            resp = next;
            cap = nextCap;
        }
        int r = (int)recv(sock, resp + len, (int)(cap - len - 1), 0);
        if (r <= 0) break;
        len += (size_t)r;
    }
    resp[len] = '\0';

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    char *bodyStart = strstr(resp, "\r\n\r\n");
    if (!bodyStart) {
        free(resp);
        return NULL;
    }
    bodyStart += 4;
    yyjson_doc *doc = yyjson_read(bodyStart, len - (size_t)(bodyStart - resp), 0);
    free(resp);
    return doc;
}

static void ResolveLauncherDataDir(char *out, size_t outSize) {
    const char *env = getenv("RAGE_STUDIO_USER_DATA");
    if (env && env[0]) {
        snprintf(out, outSize, "%s", env);
        return;
    }
#ifdef __APPLE__
    const char *home = getenv("HOME");
    if (home && home[0]) {
        snprintf(out, outSize, "%s/Library/Application Support/Rage Racer Launcher", home);
        return;
    }
#elif defined(_WIN32)
    const char *appData = getenv("APPDATA");
    if (appData && appData[0]) {
        snprintf(out, outSize, "%s\\Rage Racer Launcher", appData);
        return;
    }
#else
    const char *home = getenv("HOME");
    if (home && home[0]) {
        snprintf(out, outSize, "%s/.config/Rage Racer Launcher", home);
        return;
    }
#endif
    snprintf(out, outSize, ".");
}

static int ResolveDiscDataDir(char *discDataOut, size_t outSize, char *discPathOut, size_t pathOutSize) {
    char launcherDir[4096];
    char launcherJson[4096];
    ResolveLauncherDataDir(launcherDir, sizeof(launcherDir));
    snprintf(launcherJson, sizeof(launcherJson), "%s/launcher.json", launcherDir);

    size_t fileSize = 0;
    void *bytes = SDL_LoadFile(launcherJson, &fileSize);
    if (!bytes) return 0;

    yyjson_doc *doc = yyjson_read((const char *)bytes, fileSize, 0);
    SDL_free(bytes);
    if (!doc) return 0;

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *disc = yyjson_obj_get(root, "disc");
    const char *dataStr = disc ? yyjson_get_str(yyjson_obj_get(disc, "data")) : NULL;
    const char *pathStr = disc ? yyjson_get_str(yyjson_obj_get(disc, "path")) : NULL;
    int ok = 0;
    if (dataStr && dataStr[0]) {
        snprintf(discDataOut, outSize, "%s", dataStr);
        if (discPathOut && pathOutSize) {
            snprintf(discPathOut, pathOutSize, "%s", pathStr ? pathStr : "");
        }
        ok = 1;
    }
    yyjson_doc_free(doc);
    return ok;
}

static int ResolveTrackBinPath(int bank, char *outPath, size_t outSize) {
    if (bank < 88 || bank > 134 || (bank % 2) != 0) return 0;
    char modsDir[4096];
    if (PlatformUserConfigPath("mods", modsDir, sizeof(modsDir))) {
        char modTrack[4096];
        snprintf(modTrack, sizeof(modTrack), "%s/active/raw/asset_%03d.bin", modsDir, bank);
        SDL_PathInfo info;
        if (SDL_GetPathInfo(modTrack, &info) && info.type == SDL_PATHTYPE_FILE) {
            snprintf(outPath, outSize, "%s", modTrack);
            return 1;
        }
    }
    char discData[4096];
    if (ResolveDiscDataDir(discData, sizeof(discData), NULL, 0)) {
        snprintf(outPath, outSize, "%s/raw/asset_%03d.bin", discData, bank);
        return 1;
    }
    return 0;
}

static int EnsureTrackModPath(int bank, char *outModPath, size_t outSize) {
    char sourcePath[4096];
    if (!ResolveTrackBinPath(bank, sourcePath, sizeof(sourcePath))) return 0;

    char modsDir[4096];
    if (!PlatformUserConfigPath("mods", modsDir, sizeof(modsDir))) return 0;
    char activeDir[4096], rawDir[4096];
    snprintf(activeDir, sizeof(activeDir), "%s/active", modsDir);
    snprintf(rawDir, sizeof(rawDir), "%s/active/raw", modsDir);
    PlatformEnsureDirectory(modsDir);
    PlatformEnsureDirectory(activeDir);
    PlatformEnsureDirectory(rawDir);

    snprintf(outModPath, outSize, "%s/asset_%03d.bin", rawDir, bank);
    if (strcmp(sourcePath, outModPath) != 0) {
        size_t sz = 0;
        void *data = SDL_LoadFile(sourcePath, &sz);
        if (!data) return 0;
        FILE *f = fopen(outModPath, "wb");
        if (!f) {
            SDL_free(data);
            return 0;
        }
        fwrite(data, 1, sz, f);
        fclose(f);
        SDL_free(data);
    }
    return 1;
}

static const char *kToolsSchemaJson =
    "["
    "{\"name\":\"studio_status\",\"description\":\"Get Rage Car Studio & Rage Track Studio status, disc readiness, and project state.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}},"
    "{\"name\":\"car_list\",\"description\":\"List all editable cars in Rage Car Studio, including base slots (0-31, banks 10-72) and expandable slots (32-63, banks 200-262).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"includeRivals\":{\"type\":\"boolean\"}},\"additionalProperties\":false}},"
    "{\"name\":\"car_get_spec\",\"description\":\"Read full identity, engine, transmission, and handling specifications for a player car bank (10..72 or 200..262).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"bank\":{\"type\":\"integer\"}},\"required\":[\"bank\"],\"additionalProperties\":false}},"
    "{\"name\":\"car_update_spec\",\"description\":\"Update car identity, engine, gear ratios, or handling fields in Rage Car Studio.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"bank\":{\"type\":\"integer\"},\"values\":{\"type\":\"object\"},\"apply\":{\"type\":\"boolean\"}},\"required\":[\"bank\",\"values\"],\"additionalProperties\":false}},"
    "{\"name\":\"car_create\",\"description\":\"Create a new car in an expandable slot (32-63, banks 200-262) or overwrite an existing slot (10-72).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"targetBank\":{\"oneOf\":[{\"type\":\"string\",\"enum\":[\"new\"]},{\"type\":\"integer\"}]},\"sourceBank\":{\"type\":\"integer\"},\"name\":{\"type\":\"string\"},\"manufacturer\":{\"type\":\"string\"},\"className\":{\"type\":\"string\"},\"unlockClass\":{\"type\":\"integer\"},\"price\":{\"type\":\"integer\"},\"upgradePrice\":{\"type\":\"integer\"},\"apply\":{\"type\":\"boolean\"}},\"required\":[\"name\"],\"additionalProperties\":false}},"
    "{\"name\":\"track_list\",\"description\":\"List all tracks and class layouts available in Rage Track Studio (banks 88..134).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}},"
    "{\"name\":\"track_get_summary\",\"description\":\"Get a concise summary of a track in Rage Track Studio: road segment count, world bounds, elevation range, populated 32x32 terrain units, and placed decorations.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"bank\":{\"type\":\"integer\"}},\"required\":[\"bank\"],\"additionalProperties\":false}},"
    "{\"name\":\"track_get_road_points\",\"description\":\"Read road route segments for a track with world coordinates (x, z), heightUp (-y), width, and banking.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"bank\":{\"type\":\"integer\"},\"start\":{\"type\":\"integer\"},\"count\":{\"type\":\"integer\"}},\"required\":[\"bank\"],\"additionalProperties\":false}},"
    "{\"name\":\"track_deform_road\",\"description\":\"Shape the track road route in Rage Track Studio (move road segments by dx/dz/heightDelta with smooth ±radius falloff, or apply smooth/straighten/level/widen/bank).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"bank\":{\"type\":\"integer\"},\"pointIndex\":{\"type\":\"integer\"},\"radius\":{\"type\":\"integer\"},\"dx\":{\"type\":\"number\"},\"dz\":{\"type\":\"number\"},\"heightDelta\":{\"type\":\"number\"},\"shapeOp\":{\"type\":\"string\",\"enum\":[\"smooth\",\"straighten\",\"level\",\"widen\",\"bank\"]},\"shapeAmount\":{\"type\":\"number\"},\"apply\":{\"type\":\"boolean\"}},\"required\":[\"bank\",\"pointIndex\"],\"additionalProperties\":false}},"
    "{\"name\":\"track_set_terrain_unit\",\"description\":\"Replace a terrain unit in the 32x32 terrain grid of a track (assign a 3D terrain model ID 0..terrainModels-1, or 1023 for empty).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"bank\":{\"type\":\"integer\"},\"col\":{\"type\":\"integer\"},\"row\":{\"type\":\"integer\"},\"cell\":{\"type\":\"integer\"},\"model\":{\"type\":\"integer\"},\"apply\":{\"type\":\"boolean\"}},\"required\":[\"bank\",\"model\"],\"additionalProperties\":false}},"
    "{\"name\":\"track_manage_decorations\",\"description\":\"List, place (add), move/rotate/change model (update), duplicate, or remove track decorations (3D scenery objects).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"bank\":{\"type\":\"integer\"},\"action\":{\"type\":\"string\",\"enum\":[\"list\",\"add\",\"update\",\"duplicate\",\"remove\"]},\"objectIndex\":{\"type\":\"integer\"},\"model\":{\"type\":\"integer\"},\"roadPointIndex\":{\"type\":\"integer\"},\"x\":{\"type\":\"number\"},\"y\":{\"type\":\"number\"},\"z\":{\"type\":\"number\"},\"yawDeg\":{\"type\":\"number\"},\"apply\":{\"type\":\"boolean\"}},\"required\":[\"bank\",\"action\"],\"additionalProperties\":false}},"
    "{\"name\":\"project_save_and_apply\",\"description\":\"Apply/compile all pending Car Studio and Track Studio edits into the game, or undo/redo project history.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"action\":{\"type\":\"string\",\"enum\":[\"apply\",\"undo\",\"redo\"]}},\"additionalProperties\":false}},"
    "{\"name\":\"launch_race\",\"description\":\"Save/apply pending edits and boot Rage Racer directly into a race (boot.direct=true) with the chosen car and track.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"carBank\":{\"type\":\"integer\"},\"trackBank\":{\"type\":\"integer\"},\"trackPoint\":{\"type\":\"integer\"}},\"additionalProperties\":false}}"
    "]";

static char *ExecuteToolJson(const char *argv0, const char *toolName, yyjson_val *args, int *isErrorOut) {
    *isErrorOut = 0;
    yyjson_mut_doc *outDoc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(outDoc);
    yyjson_mut_doc_set_root(outDoc, root);

    if (strcmp(toolName, "studio_status") == 0) {
        yyjson_doc *live = CallLiveStudioHttp("snapshot", "[]");
        if (live) {
            yyjson_val *val = yyjson_obj_get(yyjson_doc_get_root(live), "value");
            yyjson_mut_obj_add_bool(outDoc, root, "liveStudioConnected", true);
            yyjson_mut_obj_add_bool(outDoc, root, "ready", yyjson_get_bool(yyjson_obj_get(val, "ready")));
            yyjson_doc_free(live);
        } else {
            char discData[4096], discPath[4096];
            int ready = ResolveDiscDataDir(discData, sizeof(discData), discPath, sizeof(discPath));
            yyjson_mut_obj_add_bool(outDoc, root, "liveStudioConnected", false);
            yyjson_mut_obj_add_bool(outDoc, root, "ready", ready != 0);
            if (ready) {
                yyjson_mut_obj_add_strcpy(outDoc, root, "discPath", discPath);
                yyjson_mut_obj_add_strcpy(outDoc, root, "discData", discData);
            }
        }
        char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
        yyjson_mut_doc_free(outDoc);
        return s;
    }

    if (strcmp(toolName, "track_list") == 0) {
        static const char *kNames[4] = {"Mythical Coast", "Over Pass City", "Lakeside Gate", "The Extreme Oval"};
        yyjson_mut_val *arr = yyjson_mut_obj_add_arr(outDoc, root, "tracks");
        int grade, course;
        for (grade = 1; grade <= 6; grade++) {
            for (course = 0; course < 4; course++) {
                int bank = 88 + (grade - 1) * 8 + course * 2;
                yyjson_mut_val *t = yyjson_mut_arr_add_obj(outDoc, arr);
                yyjson_mut_obj_add_int(outDoc, t, "bank", bank);
                yyjson_mut_obj_add_str(outDoc, t, "name", kNames[course]);
                yyjson_mut_obj_add_int(outDoc, t, "grade", grade);
                yyjson_mut_obj_add_bool(outDoc, t, "available", course != 3 || grade >= 3);
            }
        }
        char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
        yyjson_mut_doc_free(outDoc);
        return s;
    }

    if (strcmp(toolName, "track_get_summary") == 0 || strcmp(toolName, "track_get_road_points") == 0) {
        int bank = (int)yyjson_get_sint(yyjson_obj_get(args, "bank"));
        char reqArgs[64];
        snprintf(reqArgs, sizeof(reqArgs), "[%d]", bank);
        yyjson_doc *live = CallLiveStudioHttp("trackScene", reqArgs);
        if (live) {
            yyjson_val *scene = yyjson_obj_get(yyjson_doc_get_root(live), "value");
            if (scene && yyjson_is_obj(scene)) {
                yyjson_val *road = yyjson_obj_get(scene, "road");
                yyjson_val *pts = road ? yyjson_obj_get(road, "points") : NULL;
                if (strcmp(toolName, "track_get_road_points") == 0 && yyjson_is_arr(pts)) {
                    size_t total = yyjson_arr_size(pts);
                    size_t start = (size_t)(yyjson_get_sint(yyjson_obj_get(args, "start")) > 0 ? yyjson_get_sint(yyjson_obj_get(args, "start")) : 0);
                    size_t count = (size_t)(yyjson_get_sint(yyjson_obj_get(args, "count")) > 0 ? yyjson_get_sint(yyjson_obj_get(args, "count")) : 50);
                    if (count > 500) count = 500;
                    yyjson_mut_obj_add_int(outDoc, root, "bank", bank);
                    yyjson_mut_obj_add_uint(outDoc, root, "totalSegments", total);
                    yyjson_mut_val *outPts = yyjson_mut_obj_add_arr(outDoc, root, "points");
                    size_t idx, max;
                    yyjson_val *pt;
                    yyjson_arr_foreach(pts, idx, max, pt) {
                        if (idx < start || idx >= start + count) continue;
                        yyjson_mut_val *item = yyjson_mut_arr_add_obj(outDoc, outPts);
                        int y = (int)yyjson_get_sint(yyjson_obj_get(pt, "y"));
                        yyjson_mut_obj_add_uint(outDoc, item, "index", idx);
                        yyjson_mut_obj_add_sint(outDoc, item, "x", yyjson_get_sint(yyjson_obj_get(pt, "x")));
                        yyjson_mut_obj_add_int(outDoc, item, "y", y);
                        yyjson_mut_obj_add_int(outDoc, item, "heightUp", -y);
                        yyjson_mut_obj_add_sint(outDoc, item, "z", yyjson_get_sint(yyjson_obj_get(pt, "z")));
                        yyjson_mut_obj_add_sint(outDoc, item, "leftWidth", yyjson_get_sint(yyjson_obj_get(pt, "leftWidth")));
                        yyjson_mut_obj_add_sint(outDoc, item, "rightWidth", yyjson_get_sint(yyjson_obj_get(pt, "rightWidth")));
                        yyjson_mut_obj_add_sint(outDoc, item, "crossSlope", yyjson_get_sint(yyjson_obj_get(pt, "crossSlope")));
                    }
                    yyjson_doc_free(live);
                    char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
                    yyjson_mut_doc_free(outDoc);
                    return s;
                }
            }
            yyjson_doc_free(live);
        }

        char binPath[4096];
        size_t sz = 0;
        void *bytes = NULL;
        TrackBlock blocks[TRACK_PACK_BLOCK_COUNT];
        Road road;
        if (!ResolveTrackBinPath(bank, binPath, sizeof(binPath)) ||
            !(bytes = SDL_LoadFile(binPath, &sz)) ||
            !TrackPackOpen(bytes, sz, blocks) ||
            !RoadOpen(blocks[4].data, blocks[4].size, &road)) {
            SDL_free(bytes);
            yyjson_mut_doc_free(outDoc);
            *isErrorOut = 1;
            return strdup("Cannot open track binary for the requested bank");
        }

        if (strcmp(toolName, "track_get_road_points") == 0) {
            uint32_t start = (uint32_t)(yyjson_get_sint(yyjson_obj_get(args, "start")) > 0 ? yyjson_get_sint(yyjson_obj_get(args, "start")) : 0);
            uint32_t count = (uint32_t)(yyjson_get_sint(yyjson_obj_get(args, "count")) > 0 ? yyjson_get_sint(yyjson_obj_get(args, "count")) : 50);
            if (count > 500u) count = 500u;
            yyjson_mut_obj_add_int(outDoc, root, "bank", bank);
            yyjson_mut_obj_add_uint(outDoc, root, "totalSegments", road.count);
            yyjson_mut_val *outPts = yyjson_mut_obj_add_arr(outDoc, root, "points");
            uint32_t i;
            for (i = start; i < road.count && i < start + count; i++) {
                RoadPoint p;
                RoadRead(&road, i, &p);
                yyjson_mut_val *item = yyjson_mut_arr_add_obj(outDoc, outPts);
                yyjson_mut_obj_add_uint(outDoc, item, "index", i);
                yyjson_mut_obj_add_int(outDoc, item, "x", p.x);
                yyjson_mut_obj_add_int(outDoc, item, "y", p.y);
                yyjson_mut_obj_add_int(outDoc, item, "heightUp", -(int)p.y);
                yyjson_mut_obj_add_int(outDoc, item, "z", p.z);
                yyjson_mut_obj_add_int(outDoc, item, "leftWidth", p.leftWidth);
                yyjson_mut_obj_add_int(outDoc, item, "rightWidth", p.rightWidth);
                yyjson_mut_obj_add_int(outDoc, item, "crossSlope", p.crossSlope);
            }
            SDL_free(bytes);
            char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
            yyjson_mut_doc_free(outDoc);
            return s;
        }

        yyjson_mut_obj_add_int(outDoc, root, "bank", bank);
        yyjson_mut_obj_add_uint(outDoc, root, "roadSegmentsCount", road.count);
        yyjson_mut_obj_add_uint(outDoc, root, "curveArcsCount", road.arcCount);

        if (blocks[7].size >= 6144) {
            Terrain terrain;
            if (TerrainOpen(blocks[7].data, blocks[7].size, &terrain)) {
                yyjson_mut_obj_add_uint(outDoc, root, "terrainModelsAvailable", terrain.models);
                yyjson_mut_val *tArr = yyjson_mut_obj_add_arr(outDoc, root, "populatedTerrainUnits");
                uint32_t cell;
                for (cell = 0; cell < TERRAIN_GRID_CELLS; cell++) {
                    uint16_t word = 1023;
                    TerrainRead(&terrain, cell, &word);
                    uint16_t m = word & 1023u;
                    if (m != 1023u) {
                        yyjson_mut_val *u = yyjson_mut_arr_add_obj(outDoc, tArr);
                        yyjson_mut_obj_add_uint(outDoc, u, "cell", cell);
                        yyjson_mut_obj_add_uint(outDoc, u, "col", cell & 31u);
                        yyjson_mut_obj_add_uint(outDoc, u, "row", cell >> 5u);
                        yyjson_mut_obj_add_uint(outDoc, u, "model", m);
                    }
                }
            }
        }

        uint32_t courseModels = blocks[5].size >= 4 ? ReadU32LE(blocks[5].data) : 0;
        Placements placements;
        if (PlacementsOpen(blocks[8].data, blocks[8].size, courseModels, &placements)) {
            yyjson_mut_obj_add_uint(outDoc, root, "decorationModelsAvailable", courseModels);
            yyjson_mut_val *dArr = yyjson_mut_obj_add_arr(outDoc, root, "placedDecorations");
            uint32_t i;
            for (i = 0; i < placements.count; i++) {
                Placement p;
                PlacementRead(&placements, i, &p);
                if (p.model < 0) continue;
                yyjson_mut_val *d = yyjson_mut_arr_add_obj(outDoc, dArr);
                yyjson_mut_obj_add_uint(outDoc, d, "objectIndex", i);
                yyjson_mut_obj_add_int(outDoc, d, "model", p.model);
                yyjson_mut_obj_add_int(outDoc, d, "x", p.x);
                yyjson_mut_obj_add_int(outDoc, d, "y", p.y);
                yyjson_mut_obj_add_int(outDoc, d, "z", p.z);
                yyjson_mut_obj_add_real(outDoc, d, "yawDeg", ((double)p.yaw * 360.0) / 4096.0);
            }
        }

        SDL_free(bytes);
        char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
        yyjson_mut_doc_free(outDoc);
        return s;
    }

    if (strcmp(toolName, "track_deform_road") == 0) {
        int bank = (int)yyjson_get_sint(yyjson_obj_get(args, "bank"));
        int ptIdx = (int)yyjson_get_sint(yyjson_obj_get(args, "pointIndex"));
        int radius = yyjson_obj_get(args, "radius") ? (int)yyjson_get_sint(yyjson_obj_get(args, "radius")) : 6;
        double dx = yyjson_get_num(yyjson_obj_get(args, "dx"));
        double dz = yyjson_get_num(yyjson_obj_get(args, "dz"));
        double heightDelta = yyjson_get_num(yyjson_obj_get(args, "heightDelta"));
        double dy = -heightDelta;
        const char *shapeOp = yyjson_get_str(yyjson_obj_get(args, "shapeOp"));
        double shapeAmount = yyjson_get_num(yyjson_obj_get(args, "shapeAmount"));

        /* Try live studio server first */
        char sceneReq[64];
        snprintf(sceneReq, sizeof(sceneReq), "[%d]", bank);
        yyjson_doc *liveSceneDoc = CallLiveStudioHttp("trackScene", sceneReq);
        if (liveSceneDoc) {
            yyjson_val *sceneVal = yyjson_obj_get(yyjson_doc_get_root(liveSceneDoc), "value");
            int rev = (int)yyjson_get_sint(yyjson_obj_get(sceneVal, "revision"));
            yyjson_val *ptsArr = yyjson_obj_get(yyjson_obj_get(sceneVal, "road"), "points");
            uint32_t n = (uint32_t)yyjson_arr_size(ptsArr);
            if (n > 0) {
                RoadPoint *basePts = (RoadPoint *)calloc(n, sizeof(RoadPoint));
                RoadPoint *draftPts = (RoadPoint *)calloc(n, sizeof(RoadPoint));
                size_t idx, max;
                yyjson_val *ptVal;
                yyjson_arr_foreach(ptsArr, idx, max, ptVal) {
                    basePts[idx].x = (int32_t)yyjson_get_sint(yyjson_obj_get(ptVal, "x"));
                    basePts[idx].y = (int16_t)yyjson_get_sint(yyjson_obj_get(ptVal, "y"));
                    basePts[idx].z = (int32_t)yyjson_get_sint(yyjson_obj_get(ptVal, "z"));
                    basePts[idx].angle = (int16_t)yyjson_get_sint(yyjson_obj_get(ptVal, "angle"));
                    basePts[idx].pitch = (int16_t)yyjson_get_sint(yyjson_obj_get(ptVal, "pitch"));
                    basePts[idx].crossSlope = (int16_t)yyjson_get_sint(yyjson_obj_get(ptVal, "crossSlope"));
                    basePts[idx].leftWidth = (int16_t)yyjson_get_sint(yyjson_obj_get(ptVal, "leftWidth"));
                    basePts[idx].rightWidth = (int16_t)yyjson_get_sint(yyjson_obj_get(ptVal, "rightWidth"));
                    basePts[idx].arc = (uint16_t)yyjson_get_uint(yyjson_obj_get(ptVal, "arc"));
                    basePts[idx].length = (uint16_t)yyjson_get_uint(yyjson_obj_get(ptVal, "length"));
                    draftPts[idx] = basePts[idx];
                }
                int center = ((ptIdx % (int)n) + (int)n) % (int)n;
                int r = radius < 0 ? 0 : (radius > (int)(n / 2) ? (int)(n / 2) : radius);
                int off;
                for (off = -r; off <= r; off++) {
                    int k = (center + off + (int)n) % (int)n;
                    double w = r == 0 ? 1.0 : 0.5 * (1.0 + cos((M_PI * fabs((double)off)) / (double)(r + 1)));
                    if (dx != 0.0) draftPts[k].x = ClampI32((double)draftPts[k].x + dx * w);
                    if (dy != 0.0) draftPts[k].y = ClampI16((double)draftPts[k].y + dy * w);
                    if (dz != 0.0) draftPts[k].z = ClampI32((double)draftPts[k].z + dz * w);
                    if (shapeOp && strcmp(shapeOp, "widen") == 0) {
                        int16_t dw = ClampI16((shapeAmount != 0.0 ? shapeAmount : 40.0) * w);
                        draftPts[k].leftWidth = ClampI16((double)draftPts[k].leftWidth + dw);
                        draftPts[k].rightWidth = ClampI16((double)draftPts[k].rightWidth + dw);
                    } else if (shapeOp && strcmp(shapeOp, "bank") == 0) {
                        draftPts[k].crossSlope = ClampI16((double)draftPts[k].crossSlope + shapeAmount * w);
                    }
                }
                RecalculateRoadGeometry(basePts, draftPts, n);

                yyjson_mut_doc *batchDoc = yyjson_mut_doc_new(NULL);
                yyjson_mut_val *batchArr = yyjson_mut_arr(batchDoc);
                yyjson_mut_doc_set_root(batchDoc, batchArr);
                yyjson_mut_arr_add_int(batchDoc, batchArr, bank);
                yyjson_mut_val *entriesArr = yyjson_mut_arr_add_arr(batchDoc, batchArr);
                uint32_t modified = 0;
                for (idx = 0; idx < n; idx++) {
                    if (memcmp(&basePts[idx], &draftPts[idx], sizeof(RoadPoint)) != 0) {
                        yyjson_mut_val *e = yyjson_mut_arr_add_obj(batchDoc, entriesArr);
                        yyjson_mut_obj_add_str(batchDoc, e, "type", "road");
                        yyjson_mut_obj_add_uint(batchDoc, e, "index", idx);
                        yyjson_mut_val *v = yyjson_mut_obj_add_obj(batchDoc, e, "values");
                        yyjson_mut_obj_add_int(batchDoc, v, "x", draftPts[idx].x);
                        yyjson_mut_obj_add_int(batchDoc, v, "y", draftPts[idx].y);
                        yyjson_mut_obj_add_int(batchDoc, v, "z", draftPts[idx].z);
                        yyjson_mut_obj_add_int(batchDoc, v, "angle", draftPts[idx].angle);
                        yyjson_mut_obj_add_int(batchDoc, v, "pitch", draftPts[idx].pitch);
                        yyjson_mut_obj_add_int(batchDoc, v, "cross_slope", draftPts[idx].crossSlope);
                        yyjson_mut_obj_add_int(batchDoc, v, "left_width", draftPts[idx].leftWidth);
                        yyjson_mut_obj_add_int(batchDoc, v, "right_width", draftPts[idx].rightWidth);
                        yyjson_mut_obj_add_uint(batchDoc, v, "length", draftPts[idx].length);
                        modified++;
                    }
                }
                yyjson_mut_arr_add_int(batchDoc, batchArr, rev);
                char *batchJson = yyjson_mut_write(batchDoc, 0, NULL);
                yyjson_doc *saveDoc = CallLiveStudioHttp("saveRoadBatch", batchJson);
                if (saveDoc) yyjson_doc_free(saveDoc);
                free(batchJson);
                yyjson_mut_doc_free(batchDoc);
                free(basePts);
                free(draftPts);
                yyjson_doc_free(liveSceneDoc);

                yyjson_mut_obj_add_int(outDoc, root, "bank", bank);
                yyjson_mut_obj_add_int(outDoc, root, "pointIndex", center);
                yyjson_mut_obj_add_uint(outDoc, root, "modifiedSegmentsCount", modified);
                char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
                yyjson_mut_doc_free(outDoc);
                return s;
            }
            yyjson_doc_free(liveSceneDoc);
        }

        /* Standalone C fallback directly on track binary */
        char modPath[4096];
        size_t sz = 0;
        void *bytes = NULL;
        TrackBlock blocks[TRACK_PACK_BLOCK_COUNT];
        Road road;
        if (!EnsureTrackModPath(bank, modPath, sizeof(modPath)) ||
            !(bytes = SDL_LoadFile(modPath, &sz)) ||
            !TrackPackOpen(bytes, sz, blocks) ||
            !RoadOpen(blocks[4].data, blocks[4].size, &road) || road.count == 0) {
            SDL_free(bytes);
            yyjson_mut_doc_free(outDoc);
            *isErrorOut = 1;
            return strdup("Cannot open track binary for road deformation");
        }
        uint32_t n = road.count;
        RoadPoint *basePts = (RoadPoint *)calloc(n, sizeof(RoadPoint));
        RoadPoint *draftPts = (RoadPoint *)calloc(n, sizeof(RoadPoint));
        uint32_t i;
        for (i = 0; i < n; i++) {
            RoadRead(&road, i, &basePts[i]);
            draftPts[i] = basePts[i];
        }
        int center = ((ptIdx % (int)n) + (int)n) % (int)n;
        int r = radius < 0 ? 0 : (radius > (int)(n / 2) ? (int)(n / 2) : radius);
        int off;
        for (off = -r; off <= r; off++) {
            int k = (center + off + (int)n) % (int)n;
            double w = r == 0 ? 1.0 : 0.5 * (1.0 + cos((M_PI * fabs((double)off)) / (double)(r + 1)));
            if (dx != 0.0) draftPts[k].x = ClampI32((double)draftPts[k].x + dx * w);
            if (dy != 0.0) draftPts[k].y = ClampI16((double)draftPts[k].y + dy * w);
            if (dz != 0.0) draftPts[k].z = ClampI32((double)draftPts[k].z + dz * w);
        }
        RecalculateRoadGeometry(basePts, draftPts, n);
        uint32_t modified = 0;
        for (i = 0; i < n; i++) {
            if (memcmp(&basePts[i], &draftPts[i], sizeof(RoadPoint)) != 0) {
                RoadWrite((unsigned char *)bytes + (blocks[4].data - (unsigned char *)bytes), blocks[4].size, i, &draftPts[i]);
                modified++;
            }
        }
        FILE *f = fopen(modPath, "wb");
        if (f) {
            fwrite(bytes, 1, sz, f);
            fclose(f);
        }
        free(basePts);
        free(draftPts);
        SDL_free(bytes);

        yyjson_mut_obj_add_int(outDoc, root, "bank", bank);
        yyjson_mut_obj_add_int(outDoc, root, "pointIndex", center);
        yyjson_mut_obj_add_uint(outDoc, root, "modifiedSegmentsCount", modified);
        char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
        yyjson_mut_doc_free(outDoc);
        return s;
    }

    if (strcmp(toolName, "track_set_terrain_unit") == 0) {
        int bank = (int)yyjson_get_sint(yyjson_obj_get(args, "bank"));
        int model = (int)yyjson_get_sint(yyjson_obj_get(args, "model"));
        int cell = yyjson_obj_get(args, "cell")
            ? (int)yyjson_get_sint(yyjson_obj_get(args, "cell"))
            : (((int)yyjson_get_sint(yyjson_obj_get(args, "row")) & 31) * 32 +
               ((int)yyjson_get_sint(yyjson_obj_get(args, "col")) & 31));

        char sceneReq[64];
        snprintf(sceneReq, sizeof(sceneReq), "[%d]", bank);
        yyjson_doc *liveScene = CallLiveStudioHttp("trackScene", sceneReq);
        if (liveScene) {
            int rev = (int)yyjson_get_sint(yyjson_obj_get(yyjson_obj_get(yyjson_doc_get_root(liveScene), "value"), "revision"));
            yyjson_doc_free(liveScene);
            char saveReq[256];
            snprintf(saveReq, sizeof(saveReq), "[%d,%d,{\"model\":%d},%d]", bank, cell, model, rev);
            yyjson_doc *saveDoc = CallLiveStudioHttp("saveTerrainCell", saveReq);
            if (saveDoc) yyjson_doc_free(saveDoc);
        } else {
            char modPath[4096];
            size_t sz = 0;
            void *bytes = NULL;
            TrackBlock blocks[TRACK_PACK_BLOCK_COUNT];
            if (EnsureTrackModPath(bank, modPath, sizeof(modPath)) &&
                (bytes = SDL_LoadFile(modPath, &sz)) &&
                TrackPackOpen(bytes, sz, blocks)) {
                TerrainWriteModel((unsigned char *)bytes + (blocks[7].data - (unsigned char *)bytes),
                                  blocks[7].size, (uint32_t)cell, (uint16_t)(model & 1023));
                FILE *f = fopen(modPath, "wb");
                if (f) {
                    fwrite(bytes, 1, sz, f);
                    fclose(f);
                }
            }
            SDL_free(bytes);
        }

        yyjson_mut_obj_add_int(outDoc, root, "bank", bank);
        yyjson_mut_obj_add_int(outDoc, root, "cell", cell);
        yyjson_mut_obj_add_int(outDoc, root, "col", cell & 31);
        yyjson_mut_obj_add_int(outDoc, root, "row", cell >> 5);
        yyjson_mut_obj_add_int(outDoc, root, "model", model & 1023);
        char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
        yyjson_mut_doc_free(outDoc);
        return s;
    }

    if (strcmp(toolName, "track_manage_decorations") == 0) {
        int bank = (int)yyjson_get_sint(yyjson_obj_get(args, "bank"));
        const char *action = yyjson_get_str(yyjson_obj_get(args, "action"));
        if (!action) action = "list";

        char sceneReq[64];
        snprintf(sceneReq, sizeof(sceneReq), "[%d]", bank);
        yyjson_doc *liveScene = CallLiveStudioHttp("trackScene", sceneReq);
        if (liveScene) {
            int rev = (int)yyjson_get_sint(yyjson_obj_get(yyjson_obj_get(yyjson_doc_get_root(liveScene), "value"), "revision"));
            yyjson_doc_free(liveScene);
            char reqBuf[512];
            if (strcmp(action, "add") == 0) {
                int model = (int)yyjson_get_sint(yyjson_obj_get(args, "model"));
                int pt = (int)yyjson_get_sint(yyjson_obj_get(args, "roadPointIndex"));
                snprintf(reqBuf, sizeof(reqBuf), "[%d,%d,%d,%d]", bank, model, pt, rev);
                yyjson_doc *d = CallLiveStudioHttp("addTrackObject", reqBuf);
                if (d) yyjson_doc_free(d);
            } else if (strcmp(action, "remove") == 0) {
                int objIdx = (int)yyjson_get_sint(yyjson_obj_get(args, "objectIndex"));
                snprintf(reqBuf, sizeof(reqBuf), "[%d,%d,%d]", bank, objIdx, rev);
                yyjson_doc *d = CallLiveStudioHttp("removeTrackObject", reqBuf);
                if (d) yyjson_doc_free(d);
            } else if (strcmp(action, "duplicate") == 0) {
                int objIdx = (int)yyjson_get_sint(yyjson_obj_get(args, "objectIndex"));
                snprintf(reqBuf, sizeof(reqBuf), "[%d,%d,%d]", bank, objIdx, rev);
                yyjson_doc *d = CallLiveStudioHttp("duplicateTrackObject", reqBuf);
                if (d) yyjson_doc_free(d);
            } else if (strcmp(action, "update") == 0) {
                int objIdx = (int)yyjson_get_sint(yyjson_obj_get(args, "objectIndex"));
                yyjson_mut_doc *pDoc = yyjson_mut_doc_new(NULL);
                yyjson_mut_val *pArr = yyjson_mut_arr(pDoc);
                yyjson_mut_doc_set_root(pDoc, pArr);
                yyjson_mut_arr_add_int(pDoc, pArr, bank);
                yyjson_mut_arr_add_int(pDoc, pArr, objIdx);
                yyjson_mut_val *pObj = yyjson_mut_arr_add_obj(pDoc, pArr);
                if (yyjson_obj_get(args, "model")) yyjson_mut_obj_add_int(pDoc, pObj, "model", (int)yyjson_get_sint(yyjson_obj_get(args, "model")));
                if (yyjson_obj_get(args, "x")) yyjson_mut_obj_add_int(pDoc, pObj, "x", (int)lround(yyjson_get_num(yyjson_obj_get(args, "x"))));
                if (yyjson_obj_get(args, "y")) yyjson_mut_obj_add_int(pDoc, pObj, "y", (int)lround(yyjson_get_num(yyjson_obj_get(args, "y"))));
                if (yyjson_obj_get(args, "z")) yyjson_mut_obj_add_int(pDoc, pObj, "z", (int)lround(yyjson_get_num(yyjson_obj_get(args, "z"))));
                if (yyjson_obj_get(args, "yawDeg")) yyjson_mut_obj_add_int(pDoc, pObj, "yaw", (int)lround(yyjson_get_num(yyjson_obj_get(args, "yawDeg")) * 4096.0 / 360.0));
                yyjson_mut_arr_add_int(pDoc, pArr, rev);
                char *pStr = yyjson_mut_write(pDoc, 0, NULL);
                yyjson_doc *d = CallLiveStudioHttp("saveTrackObject", pStr);
                if (d) yyjson_doc_free(d);
                free(pStr);
                yyjson_mut_doc_free(pDoc);
            }
        }
        yyjson_mut_obj_add_int(outDoc, root, "bank", bank);
        yyjson_mut_obj_add_strcpy(outDoc, root, "action", action);
        char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
        yyjson_mut_doc_free(outDoc);
        return s;
    }

    if (strcmp(toolName, "car_list") == 0) {
        yyjson_doc *live = CallLiveStudioHttp("snapshot", "[]");
        if (live) {
            yyjson_val *val = yyjson_obj_get(yyjson_doc_get_root(live), "value");
            yyjson_val *cars = val ? yyjson_obj_get(val, "cars") : NULL;
            if (cars) {
                yyjson_mut_obj_add_int(outDoc, root, "totalSlots", 64);
                yyjson_mut_obj_add_val(outDoc, root, "cars", yyjson_val_mut_copy(outDoc, cars));
                yyjson_doc_free(live);
                char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
                yyjson_mut_doc_free(outDoc);
                return s;
            }
            yyjson_doc_free(live);
        }
        RageCarCatalog cat;
        char err[256] = {0};
        char exeDir[4096], tomlPath[4096];
        memset(&cat, 0, sizeof(cat));
        if (PlatformExecutableDirectory(argv0, exeDir, sizeof(exeDir))) {
            snprintf(tomlPath, sizeof(tomlPath), "%s/cars.toml", exeDir);
            if (!CarCatalogReadFile(tomlPath, &cat, err, sizeof(err))) {
                CarCatalogReadFile("cars.toml", &cat, err, sizeof(err));
            }
        } else {
            CarCatalogReadFile("cars.toml", &cat, err, sizeof(err));
        }
        yyjson_mut_obj_add_int(outDoc, root, "totalSlots", 64);
        yyjson_mut_val *arr = yyjson_mut_obj_add_arr(outDoc, root, "cars");
        for (int slot = 0; slot < 64; slot++) {
            int bank = slot < 32 ? (10 + slot * 2) : (200 + (slot - 32) * 2);
            yyjson_mut_val *item = yyjson_mut_arr_add_obj(outDoc, arr);
            yyjson_mut_obj_add_int(outDoc, item, "slot", slot);
            yyjson_mut_obj_add_int(outDoc, item, "bank", bank);
            yyjson_mut_obj_add_bool(outDoc, item, "expandable", slot >= 32);
            if (slot < (int)RAGE_CAR_CATALOG_ENTRY_COUNT) {
                const RageCarCatalogEntry *e = &cat.entries[slot];
                yyjson_mut_obj_add_strcpy(outDoc, item, "name", e->name[0] ? e->name : "Base Car");
                yyjson_mut_obj_add_strcpy(outDoc, item, "manufacturer", e->manufacturer);
                yyjson_mut_obj_add_strcpy(outDoc, item, "className", e->className);
                yyjson_mut_obj_add_int(outDoc, item, "unlockClass", e->unlockClass);
                yyjson_mut_obj_add_int(outDoc, item, "price", e->price);
            } else {
                yyjson_mut_obj_add_str(outDoc, item, "name", "Expandable Custom Slot");
                yyjson_mut_obj_add_str(outDoc, item, "manufacturer", "");
                yyjson_mut_obj_add_str(outDoc, item, "className", "");
                yyjson_mut_obj_add_int(outDoc, item, "unlockClass", 1);
                yyjson_mut_obj_add_int(outDoc, item, "price", 0);
            }
        }
        char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
        yyjson_mut_doc_free(outDoc);
        return s;
    }

    if (strcmp(toolName, "car_get_spec") == 0 ||
        strcmp(toolName, "car_update_spec") == 0 || strcmp(toolName, "car_create") == 0 ||
        strcmp(toolName, "project_save_and_apply") == 0 || strcmp(toolName, "launch_race") == 0) {
        const char *apiMethod = "snapshot";
        char reqArgs[2048] = "[]";
        if (strcmp(toolName, "car_get_spec") == 0) {
            apiMethod = "carSpec";
            snprintf(reqArgs, sizeof(reqArgs), "[%d]", (int)yyjson_get_sint(yyjson_obj_get(args, "bank")));
        } else if (strcmp(toolName, "car_update_spec") == 0) {
            int bank = (int)yyjson_get_sint(yyjson_obj_get(args, "bank"));
            char specReq[64];
            snprintf(specReq, sizeof(specReq), "[%d]", bank);
            yyjson_doc *specDoc = CallLiveStudioHttp("carSpec", specReq);
            if (specDoc && !yyjson_get_bool(yyjson_obj_get(yyjson_doc_get_root(specDoc), "ok"))) {
                char *error = yyjson_val_write(yyjson_doc_get_root(specDoc), 0, NULL);
                yyjson_doc_free(specDoc); yyjson_mut_doc_free(outDoc); *isErrorOut = 1;
                return error;
            }
            int rev = specDoc ? (int)yyjson_get_sint(yyjson_obj_get(yyjson_obj_get(yyjson_doc_get_root(specDoc), "value"), "revision")) : 0;
            if (specDoc) yyjson_doc_free(specDoc);
            char *valsStr = yyjson_val_write(yyjson_obj_get(args, "values"), 0, NULL);
            snprintf(reqArgs, sizeof(reqArgs), "[%d,%s,%d]", bank, valsStr ? valsStr : "{}", rev);
            free(valsStr);
            apiMethod = "saveCarSpec";
        } else if (strcmp(toolName, "car_create") == 0) {
            yyjson_doc *projDoc = CallLiveStudioHttp("project", "[]");
            if (projDoc && !yyjson_get_bool(yyjson_obj_get(yyjson_doc_get_root(projDoc), "ok"))) {
                char *error = yyjson_val_write(yyjson_doc_get_root(projDoc), 0, NULL);
                yyjson_doc_free(projDoc); yyjson_mut_doc_free(outDoc); *isErrorOut = 1;
                return error;
            }
            int rev = projDoc ? (int)yyjson_get_sint(yyjson_obj_get(yyjson_obj_get(yyjson_doc_get_root(projDoc), "value"), "revision")) : 0;
            if (projDoc) yyjson_doc_free(projDoc);
            char *argsStr = yyjson_val_write(args, 0, NULL);
            snprintf(reqArgs, sizeof(reqArgs), "[%s,%d]", argsStr ? argsStr : "{}", rev);
            free(argsStr);
            apiMethod = "createNewCar";
        } else if (strcmp(toolName, "project_save_and_apply") == 0) {
            yyjson_doc *projDoc = CallLiveStudioHttp("project", "[]");
            if (projDoc && !yyjson_get_bool(yyjson_obj_get(yyjson_doc_get_root(projDoc), "ok"))) {
                char *error = yyjson_val_write(yyjson_doc_get_root(projDoc), 0, NULL);
                yyjson_doc_free(projDoc); yyjson_mut_doc_free(outDoc); *isErrorOut = 1;
                return error;
            }
            int rev = projDoc ? (int)yyjson_get_sint(yyjson_obj_get(yyjson_obj_get(yyjson_doc_get_root(projDoc), "value"), "revision")) : 0;
            if (projDoc) yyjson_doc_free(projDoc);
            const char *action = yyjson_get_str(yyjson_obj_get(args, "action"));
            if (action && (strcmp(action,"undo")==0 || strcmp(action,"redo")==0)) {
                snprintf(reqArgs,sizeof(reqArgs),"[\"%s\",%d]",action,rev);
                apiMethod = "projectHistory";
            } else {
                snprintf(reqArgs, sizeof(reqArgs), "[%d]", rev);
                apiMethod = "applyProject";
            }
        } else if (strcmp(toolName, "launch_race") == 0) {
            int carBank = yyjson_obj_get(args, "carBank") ? (int)yyjson_get_sint(yyjson_obj_get(args, "carBank")) : 10;
            int trackBank = yyjson_obj_get(args, "trackBank") ? (int)yyjson_get_sint(yyjson_obj_get(args, "trackBank")) : 88;
            int trackPoint = yyjson_obj_get(args, "trackPoint") ? (int)yyjson_get_sint(yyjson_obj_get(args, "trackPoint")) : 0;
            snprintf(reqArgs, sizeof(reqArgs), "[{\"carBank\":%d,\"trackBank\":%d,\"mode\":\"time-attack\",\"trackPoint\":%d}]",
                     carBank, trackBank, trackPoint);
            apiMethod = "play";
        }

        yyjson_doc *live = CallLiveStudioHttp(apiMethod, reqArgs);
        if (live) {
            yyjson_val *val = yyjson_obj_get(yyjson_doc_get_root(live), "value");
            char *resStr = yyjson_val_write(val ? val : yyjson_doc_get_root(live), YYJSON_WRITE_PRETTY, NULL);
            yyjson_doc_free(live);
            yyjson_mut_doc_free(outDoc);
            return resStr ? resStr : strdup("{}");
        }

        if (strcmp(toolName, "launch_race") == 0) {
            int carBank = yyjson_obj_get(args, "carBank") ? (int)yyjson_get_sint(yyjson_obj_get(args, "carBank")) : 10;
            int trackBank = yyjson_obj_get(args, "trackBank") ? (int)yyjson_get_sint(yyjson_obj_get(args, "trackBank")) : 88;
            char exeDir[4096], cmd[8192];
            if (PlatformExecutableDirectory(argv0, exeDir, sizeof(exeDir))) {
                snprintf(cmd, sizeof(cmd),
                         "\"%s/rage-racer\" --set boot.direct=true --set boot.mode=time-attack --set boot.car_bank=%d --set boot.track_bank=%d &",
                         exeDir, carBank, trackBank);
                (void)system(cmd);
            }
            yyjson_mut_obj_add_bool(outDoc, root, "launched", true);
            yyjson_mut_obj_add_bool(outDoc, root, "directBoot", true);
            yyjson_mut_obj_add_int(outDoc, root, "carBank", carBank);
            yyjson_mut_obj_add_int(outDoc, root, "trackBank", trackBank);
            char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
            yyjson_mut_doc_free(outDoc);
            return s;
        }
    }

    (void)WriteU32LE;
    yyjson_mut_obj_add_strcpy(outDoc, root, "tool", toolName);
    char *s = yyjson_mut_write(outDoc, YYJSON_WRITE_PRETTY, NULL);
    yyjson_mut_doc_free(outDoc);
    return s;
}

static void HandleRpcMessage(const char *argv0, const char *rawJson, size_t rawLen, int useContentLength) {
    yyjson_doc *doc = yyjson_read(rawJson, rawLen, 0);
    if (!doc) return;
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *idVal = yyjson_obj_get(root, "id");
    const char *method = yyjson_get_str(yyjson_obj_get(root, "method"));
    yyjson_val *params = yyjson_obj_get(root, "params");

    /* Notifications have no id */
    if (!idVal || yyjson_is_null(idVal) || !method) {
        yyjson_doc_free(doc);
        return;
    }

    yyjson_mut_doc *reply = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *rRoot = yyjson_mut_obj(reply);
    yyjson_mut_doc_set_root(reply, rRoot);
    yyjson_mut_obj_add_str(reply, rRoot, "jsonrpc", "2.0");
    yyjson_mut_obj_add_val(reply, rRoot, "id", yyjson_val_mut_copy(reply, idVal));

    if (strcmp(method, "initialize") == 0) {
        yyjson_mut_val *res = yyjson_mut_obj_add_obj(reply, rRoot, "result");
        const char *proto = params ? yyjson_get_str(yyjson_obj_get(params, "protocolVersion")) : NULL;
        yyjson_mut_obj_add_strcpy(reply, res, "protocolVersion", proto ? proto : "2024-11-05");
        yyjson_mut_val *caps = yyjson_mut_obj_add_obj(reply, res, "capabilities");
        yyjson_mut_obj_add_obj(reply, caps, "tools");
        yyjson_mut_val *info = yyjson_mut_obj_add_obj(reply, res, "serverInfo");
        yyjson_mut_obj_add_str(reply, info, "name", "rage-studio-mcp");
        yyjson_mut_obj_add_str(reply, info, "version", "1.0.0");
    } else if (strcmp(method, "ping") == 0) {
        yyjson_mut_obj_add_obj(reply, rRoot, "result");
    } else if (strcmp(method, "tools/list") == 0) {
        yyjson_mut_val *res = yyjson_mut_obj_add_obj(reply, rRoot, "result");
        yyjson_doc *toolsDoc = yyjson_read(kToolsSchemaJson, strlen(kToolsSchemaJson), 0);
        yyjson_mut_obj_add_val(reply, res, "tools", yyjson_val_mut_copy(reply, yyjson_doc_get_root(toolsDoc)));
        yyjson_doc_free(toolsDoc);
    } else if (strcmp(method, "tools/call") == 0) {
        const char *toolName = params ? yyjson_get_str(yyjson_obj_get(params, "name")) : NULL;
        yyjson_val *toolArgs = params ? yyjson_obj_get(params, "arguments") : NULL;
        int isErr = 0;
        char *text = ExecuteToolJson(argv0, toolName ? toolName : "", toolArgs, &isErr);
        yyjson_doc *payload = text ? yyjson_read(text, strlen(text), 0) : NULL;
        if (payload) {
            yyjson_val *ok = yyjson_obj_get(yyjson_doc_get_root(payload), "ok");
            if (yyjson_is_bool(ok) && !yyjson_get_bool(ok)) isErr = 1;
            yyjson_doc_free(payload);
        }
        yyjson_mut_val *res = yyjson_mut_obj_add_obj(reply, rRoot, "result");
        yyjson_mut_val *content = yyjson_mut_obj_add_arr(reply, res, "content");
        yyjson_mut_val *item = yyjson_mut_arr_add_obj(reply, content);
        yyjson_mut_obj_add_str(reply, item, "type", "text");
        yyjson_mut_obj_add_strcpy(reply, item, "text", text ? text : "{}");
        yyjson_mut_obj_add_bool(reply, res, "isError", isErr != 0);
        free(text);
    } else {
        yyjson_mut_val *err = yyjson_mut_obj_add_obj(reply, rRoot, "error");
        yyjson_mut_obj_add_int(reply, err, "code", -32601);
        yyjson_mut_obj_add_str(reply, err, "message", "Method not found");
    }

    size_t outLen = 0;
    char *outJson = yyjson_mut_write(reply, 0, &outLen);
    if (outJson) {
        if (useContentLength) {
            printf("Content-Length: %zu\r\n\r\n%s", outLen, outJson);
        } else {
            printf("%s\n", outJson);
        }
        fflush(stdout);
        free(outJson);
    }

    yyjson_mut_doc_free(reply);
    yyjson_doc_free(doc);
}

int main(int argc, char **argv) {
    (void)argc;
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    size_t cap = 65536;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) return 1;

    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (len + 2 >= cap) {
            size_t nextCap = cap * 2;
            char *next = (char *)realloc(buf, nextCap);
            if (!next) break;
            buf = next;
            cap = nextCap;
        }
        buf[len++] = (char)c;
        buf[len] = '\0';

        if (len >= 16 && strncmp(buf, "Content-Length:", 15) == 0) {
            char *hdrEnd = strstr(buf, "\r\n\r\n");
            if (!hdrEnd) continue;
            size_t bodyLen = (size_t)strtoul(buf + 15, NULL, 10);
            size_t totalNeeded = (size_t)(hdrEnd + 4 - buf) + bodyLen;
            if (len < totalNeeded) continue;
            HandleRpcMessage(argv[0], hdrEnd + 4, bodyLen, 1);
            memmove(buf, buf + totalNeeded, len - totalNeeded);
            len -= totalNeeded;
            buf[len] = '\0';
        } else if (c == '\n') {
            size_t lineLen = len;
            while (lineLen > 0 && (buf[lineLen - 1] == '\n' || buf[lineLen - 1] == '\r')) {
                lineLen--;
            }
            if (lineLen > 0) {
                HandleRpcMessage(argv[0], buf, lineLen, 0);
            }
            len = 0;
            buf[0] = '\0';
        }
    }

    free(buf);
    return 0;
}
