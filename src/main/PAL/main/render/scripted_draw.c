#include "game/menu.h"

static s32 LimitScriptElapsed(s32 elapsed, s32 limit) {
    return elapsed < limit ? elapsed : limit;
}
static s32 ScriptVelocity(s32 packed, s32 upperHalf) {
    return upperHalf ? (s16)(packed >> 16) : (s16)packed;
}

/* Retail performs this multiply and division as unsigned operations. Keeping
 * that rule also avoids signed-overflow undefined behaviour for malformed
 * script data. The draw calls ultimately consume the low signed halfword. */
static s32 ScriptOffset(s32 elapsed, s32 velocity) {
    return (s32)(((u32)elapsed * (u32)velocity) / 32);
}

static s32 ScriptOtOffset(u8 flags) {
    switch (flags & 3) {
    case 0:
        return 0;
    case 1:
        return 3;
    case 2:
        return 5;
    case 3:
        return 0x2BE;
    }
    return 0;
}

void DrawScriptedSprite(s32 elapsed, const ScriptedSpriteShape *shape,
                        const ScriptedSpriteMotion *motion, s32 type) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 x;
    s32 y;
    s32 alpha;

    elapsed = LimitScriptElapsed(elapsed, motion->limit);
    x = motion->x +
        ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity, 0));
    y = motion->y +
        ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity, 1));
    alpha = type != 0 ? shape->alpha & 0x7F : 0x80;

    DrawSprite(ot + ScriptOtOffset(shape->flags), (s16)x, (s16)y,
               shape->width, shape->height, shape->u, shape->v,
               motion->r, motion->g, motion->b, motion->clut,
               shape->flags & 8, shape->flags & 4, alpha);
}

void DrawScriptedLine(s32 elapsed, const ScriptedLineShape *shape,
                      const ScriptedLineMotion *motion) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
    s32 alpha;

    elapsed = LimitScriptElapsed(elapsed, motion->limit);
    x0 = motion->x0 +
         ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity0, 0));
    y0 = motion->y0 +
         ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity0, 1));
    x1 = motion->x1 +
         ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity1, 0));
    y1 = motion->y1 +
         ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity1, 1));
    alpha = shape->flags & 4 ? shape->flags & 0x60 : 0xFF;

    DrawLine(ot + ScriptOtOffset(shape->flags), (s16)x0, (s16)y0,
             (s16)x1, (s16)y1, shape->r, shape->g, shape->b, alpha);
}

void DrawScriptedTriangle(s32 elapsed, const ScriptedTriangleShape *shape,
                          const ScriptedTriangleMotion *motion) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 x;
    s32 y;
    s32 semiTrans;
    s32 alpha;

    elapsed = LimitScriptElapsed(elapsed, motion->limit);
    x = motion->x +
        ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity, 0));
    y = motion->y +
        ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity, 1));
    semiTrans = shape->flags & 4;
    alpha = semiTrans ? shape->flags & 0x60 : 0x80;

    DrawFlatTriangle(ot + ScriptOtOffset(shape->flags),
                     (s16)x, (s16)y,
                     (s16)(x + shape->x1), (s16)(y + shape->y1),
                     (s16)(x + shape->x2), (s16)(y + shape->y2),
                     shape->r, shape->g, shape->b, semiTrans, alpha);
}

void DrawScriptedQuad(s32 elapsed, const ScriptedQuadShape *shape,
                      const ScriptedQuadMotion *motion) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 x;
    s32 y;
    s32 width;
    s32 height;

    elapsed = LimitScriptElapsed(elapsed, motion->limit);
    x = motion->x +
        ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity, 0));
    y = motion->y +
        ScriptOffset(elapsed, ScriptVelocity(motion->packedVelocity, 1));
    width = motion->width + ScriptOffset(
        elapsed, ScriptVelocity(motion->packedSizeVelocity, 0));
    height = motion->height + ScriptOffset(
        elapsed, ScriptVelocity(motion->packedSizeVelocity, 1));

    GameDrawTexturedQuad(ot + ScriptOtOffset(shape->flags),
                         (s16)x, (s16)y, (s16)(x + width), (s16)y,
                         (s16)x, (s16)(y + height),
                         (s16)(x + width), (s16)(y + height),
                         shape->u0, shape->v0, shape->u1, shape->v1,
                         shape->u2, shape->v2, shape->u3, shape->v3,
                         shape->r, shape->g, shape->b, shape->clut,
                         shape->flags & 8, shape->flags & 4, shape->alpha);
}

/*
 * A script is a clock and a picture of where that clock stands, and until now
 * one function was both. Screens gate their input on the return value, so
 * stepping a screen meant drawing it, and checking what it draws meant
 * driving a renderer. The two are separated here; RunTimedDrawScript still
 * does both, in the order it always did, for the hundred and thirty-six
 * places that ask for both.
 */
static s32 TimedDrawScriptLength(const TimedDrawCommand *base) {
    s32 index = 0;
    while (base[index].time >= 0) {
        index++;
    }
    return index;
}

/*
 * Move the clock. A negative step rewinds, and never past the start; a
 * positive one advances, and stops at the end the script records after its
 * last command, which is when it reports itself finished. Either way the
 * progress the commands are drawn at is the one before the advance, which is
 * why it is answered rather than left to the caller to work out.
 */
TimedDrawScriptTick AdvanceTimedDrawScript(
    const TimedDrawCommand *commands, s32 *progress, s32 step) {
    const TimedDrawCommand *base = commands;
    TimedDrawScriptTick tick;

    tick.finished = 0;
    if (step < 0) {
        s32 rewound = *progress + step;
        *progress = (rewound > 0) ? rewound : 0;
    }
    tick.drawAt = *progress;
    if (step >= 0) {
        s32 limit = base[TimedDrawScriptLength(base)].motion.value;
        s32 advanced = step + *progress;
        if (advanced < limit) {
            *progress = advanced;
        } else {
            *progress = limit;
            tick.finished = 1;
        }
    }
    return tick;
}

/* Draw every command the clock has reached, each with the time that has
 * passed since it was due. */
void DrawTimedDrawScript(const TimedDrawCommand *commands, s32 progress) {
    const TimedDrawCommand *base = commands;
    const TimedDrawCommand *cmd;
    s32 remaining;
    u32 type;

    cmd = base;
    while (cmd->time >= 0) {
        remaining = progress - cmd->time;
        if (remaining >= 0) {
            type = cmd->type;
            if (type < 40) {
                switch (type) {
                case 9:
                    if (g_MenuAltLayout != 0) {
                        break;
                    }
                    DrawScriptedSprite(
                        remaining, cmd->shape.spriteShape,
                        cmd->motion.spriteMotion, type);
                    break;
                case 0:
                case 1:
                    DrawScriptedSprite(
                        remaining, cmd->shape.spriteShape,
                        cmd->motion.spriteMotion, type);
                    break;
                case 19:
                    if (g_MenuAltLayout != 0) {
                        break;
                    }
                    DrawScriptedLine(
                        remaining, cmd->shape.lineShape, cmd->motion.lineMotion);
                    break;
                case 10:
                    DrawScriptedLine(
                        remaining, cmd->shape.lineShape, cmd->motion.lineMotion);
                    break;
                case 29:
                    if (g_MenuAltLayout != 0) {
                        break;
                    }
                    DrawScriptedTriangle(
                        remaining, cmd->shape.triangleShape,
                        cmd->motion.triangleMotion);
                    break;
                case 20:
                    DrawScriptedTriangle(
                        remaining, cmd->shape.triangleShape,
                        cmd->motion.triangleMotion);
                    break;
                case 39:
                    if (g_MenuAltLayout != 0) {
                        break;
                    }
                    DrawScriptedQuad(
                        remaining, cmd->shape.quadShape, cmd->motion.quadMotion);
                    break;
                case 30:
                    DrawScriptedQuad(
                        remaining, cmd->shape.quadShape, cmd->motion.quadMotion);
                    break;
                default:
                    break;
                }
            }
        }
        cmd++;
    }

}

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    TimedDrawScriptTick tick = AdvanceTimedDrawScript(commands, progress,
                                                      step);
    DrawTimedDrawScript(commands, tick.drawAt);
    return tick.finished;
}


void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    ScriptedSpriteMotion *firstMotion;
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 i;
    s32 xOffset;
    s32 yOffset;
    s32 elapsed;

    elapsed = progress - g_MenuRowScript[0].time;
    if (elapsed < 0 || count < 0) {
        return;
    }

    firstMotion = g_MenuRowScript[0].motion.spriteMotion;
    elapsed = LimitScriptElapsed(elapsed, firstMotion->limit);
    g_MenuRowFlashLevels[slot] = 0x1FC;
    xOffset = ScriptOffset(
        elapsed, ScriptVelocity(firstMotion->packedVelocity, 0));
    yOffset = ScriptOffset(
        elapsed, ScriptVelocity(firstMotion->packedVelocity, 1));

    for (i = 0; i <= count; i++) {
        TimedDrawCommand *command = &g_MenuRowScript[i];
        ScriptedSpriteShape *shape = command->shape.spriteShape;
        ScriptedSpriteMotion *motion = command->motion.spriteMotion;
        s32 *timer = &g_MenuRowFlashLevels[i];
        u32 fade = *timer & 0x1FF;

        fade >>= 2;
        DrawSprite(ot + 2,
                   (s16)(motion->x + xOffset),
                   (s16)(motion->y + yOffset),
                   shape->width, shape->height, shape->u, shape->v,
                   fade, fade, fade, motion->clut, 0, 1, shape->alpha);
        *timer = (*timer & 0x1FF) >= 60 ? (*timer & 0x1FF) - 60 : 0;
    }
}
