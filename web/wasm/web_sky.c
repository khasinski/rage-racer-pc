#include "web_sky.h"

#include <string.h>

#include "game/angle.h"
#include "game/integer.h"
#include "native_sky.h"
#include "native_texture.h"
#include "sky_panorama_layout.h"
#include "track_texture_snapshot.h"

static s32 DivideBy32TowardZero(s32 value) {
    if (value < 0) value += 31;
    return value >> 5;
}

static s32 SignedAngle12(s32 angle) {
    angle &= ANGLE_MASK;
    return angle >= 0x800 ? angle - 0x1000 : angle;
}

/* draw_sky_background.c MeasureSkyGridLayout for a normal (not mirrored)
 * course, so mirror mode is zero. The ordering flag is one only in the
 * rear-view mirror pass, which measures the same camera angles with pitch and
 * yaw negated and its panel origin at the top of the 36-line mirror. */
typedef struct WebSkyGrid {
    s32 panelXFixed, panelYFixed, lowerPanelXFixed, lowerPanelYFixed;
    s32 columnStepX, columnStepY, rowStepX, rowStepY, textureColumn;
} WebSkyGrid;

static WebSkyGrid MeasureSkyGrid(s32 cameraY, s32 pitch, s32 yaw, s32 roll, int mirrorPass) {
    WebSkyGrid grid;
    if (mirrorPass) {
        pitch = -pitch;
        yaw = -yaw;
    }
    const s32 cameraPitch = SignedAngle12(pitch) + 2 + DivideBy32TowardZero(WrapSigned32((int64_t)cameraY - 6000));
    const s32 yawAngle = (yaw + 0x200) & ANGLE_MASK;
    const s32 nearVerticalFixed = (-0x80 - cameraPitch / 2) * 256;
    const s32 farVerticalFixed = (-0x80 - (cameraPitch / 2 + 0x50)) * 256;
    const s32 horizontalFixed = (-0x100 - ((yawAngle >> 1) & 0x3F)) * 256;
    const s32 rollAngle = -roll;
    const s32 sinRoll = SinAngle(rollAngle);
    const s32 cosRoll = CosAngle(rollAngle);
    const s32 rotatedHorizontalY = WrapSigned32((int64_t)-sinRoll * horizontalFixed);
    const s32 nearX = WrapSigned32((int64_t)cosRoll * horizontalFixed + (int64_t)sinRoll * nearVerticalFixed);
    const s32 nearY = WrapSigned32((int64_t)rotatedHorizontalY + (int64_t)cosRoll * nearVerticalFixed);
    const s32 farX = WrapSigned32((int64_t)cosRoll * horizontalFixed + (int64_t)sinRoll * farVerticalFixed);
    const s32 farY = WrapSigned32((int64_t)rotatedHorizontalY + (int64_t)cosRoll * farVerticalFixed);
    const s32 verticalOrigin = mirrorPass ? 0x2400 : 0x7800;

    grid.panelXFixed = nearX / 4096 + 0xA000;
    grid.panelYFixed = nearY / 4096 + verticalOrigin;
    grid.lowerPanelXFixed = farX / 4096 + 0xA000;
    grid.lowerPanelYFixed = farY / 4096 + verticalOrigin;
    grid.columnStepX = cosRoll * 4;
    grid.columnStepY = -sinRoll * 4;
    grid.rowStepX = sinRoll * 8;
    grid.rowStepY = cosRoll * 8;
    grid.textureColumn = yawAngle >> 7;
    return grid;
}

void WebSkySetCamera(RenderCamera *camera, const ClientRace *race, s32 cameraY,
                     s32 pitch, s32 yaw, s32 roll, int mirrorPass) {
    const WebSkyGrid grid = MeasureSkyGrid(cameraY, pitch, yaw, roll, mirrorPass);
    if (!camera || !race) return;
    /* client_frame.c CaptureClientFrame: the client race's panorama identity. */
    camera->hasSkyLayout = (uint8_t)RetailSkyLayout(&camera->skyLayout, (int)race->env.skyRowBase);
    camera->skyAssetKey = race->primaryMesh.cached.assetKey;
    camera->skyCloudRow = race->env.skyRowBase;
    /* render_world_game.c GameRenderWorldBuildCamera. */
    camera->skyGridOrigin.x = grid.panelXFixed * (1.0f / 256.0f);
    camera->skyGridOrigin.y = grid.panelYFixed * (1.0f / 256.0f);
    camera->skyGridOrigin.z = grid.lowerPanelXFixed * (1.0f / 256.0f);
    camera->skyGridColumn.x = grid.columnStepX * (1.0f / 256.0f);
    camera->skyGridColumn.y = grid.columnStepY * (1.0f / 256.0f);
    camera->skyGridColumn.z = (float)grid.textureColumn;
    camera->skyGridRow.x = grid.rowStepX * (1.0f / 256.0f);
    camera->skyGridRow.y = grid.rowStepY * (1.0f / 256.0f);
    camera->skyGridRow.z = grid.lowerPanelYFixed * (1.0f / 256.0f);
}

static void StoreColor(float *out, Vec3 color) {
    out[0] = color.x; out[1] = color.y; out[2] = color.z; out[3] = 1.0f;
}

void WebSkyUniform(const RenderCamera *camera, float aspect, float out[WEB_SKY_FLOATS]) {
    memset(out, 0, WEB_SKY_FLOATS * sizeof(*out));
    StoreColor(&out[0], camera->skyTopColor);
    StoreColor(&out[4], camera->skyColor);
    StoreColor(&out[8], camera->skyHorizonColor);
    /* Alpha carries the cloud sheet, which the game tiles over the gradient. */
    StoreColor(&out[12], camera->skyBottomColor);
    out[16] = camera->skyGridOrigin.x;
    out[17] = camera->skyGridOrigin.y;
    out[18] = camera->skyGridColumn.z;
    out[19] = 240.0f * aspect;
    out[20] = camera->skyGridColumn.x;
    out[21] = camera->skyGridColumn.y;
    out[22] = camera->skyGridRow.x;
    out[23] = camera->skyGridRow.y;
    out[24] = camera->skyGridOrigin.z;
    out[25] = camera->skyGridRow.z;
    out[26] = camera->skyCloudRow == 0 ? 1.0f : 4.0f;
}

int WebSkyDecode(const ClientRace *race, const RenderCamera *camera, int page,
                 uint8_t *rgba, size_t size) {
    if (!race || !camera || !race->pixels || !camera->hasSkyLayout || page < 0 || page > 1)
        return 0;
    const TextureImage image = {race->pixels->pages[page],
        RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT,
        0, 0, RAGE_TRACK_VRAM_WIDTH, RAGE_TRACK_VRAM_HEIGHT, NULL};
    const TextureImage palette = {race->env.clut, 16, 224, 486, 16, 1, &image};
    return DecodeSky(&palette, &camera->skyLayout, rgba, size);
}
