#include "modern/modern_gpu_state.h"

#include <assert.h>

static uint32_t AreaWord(uint32_t command, int x, int y) {
    return command << 24 | (uint32_t)x | (uint32_t)y << 10;
}

int main(void) {
    Modern2DState state;
    Modern2DStateInit(&state);
    assert(state.twin == 0x0000FFFFu);
    Modern2DStateApplyGp0(&state, 0xE1000123u, 240);
    assert(state.tpage == 0x123u);

    Modern2DStateApplyGp0(&state, AreaWord(0xE3, 86, 240), 240);
    Modern2DStateApplyGp0(&state, AreaWord(0xE4, 233, 239), 240);
    assert(state.hasScissor && state.areaEmpty && state.scissor.h == 0);

    Modern2DStateInit(&state);
    Modern2DStateApplyGp0(&state, AreaWord(0xE3, 10, 250), 240);
    Modern2DStateApplyGp0(&state, AreaWord(0xE4, 20, 300), 240);
    assert(!state.areaEmpty);
    assert(state.scissor.x == 10 && state.scissor.y == 10);
    assert(state.scissor.w == 11 && state.scissor.h == 51);

    Modern2DStateApplyGp0(&state, 0xE5000000u | 0x7FFu | (0x7FEu << 11), 0);
    assert(state.offsetX == -1 && state.offsetY == -2);
    assert(ModernTextureWindowFromGp0(0) == 0x0000FFFFu);
    return 0;
}
