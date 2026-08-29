#include "game/menu.h"

typedef union FadingMenuTableAddress {
    TimedDrawCommand *commands;
    s32 *timers;
} FadingMenuTableAddress;


void DrawScriptedSprite(s32 elapsed, ScriptedSpriteShape *shape, ScriptedSpriteMotion *motion, s32 type) {
    ScriptedSpriteMotion *motionReg = motion;
    ScriptedSpriteShape *shapeReg;
    s32 flags8;
    OT_TYPE *otBase;
    s32 mode;
    s32 flags4;
    s32 limit;
    s32 packed;
    s32 x;
    s32 y;
    s32 temp;
    s32 interp;
    u32 interpProduct;
    s32 flagByte;
    s32 alpha;

    limit = motionReg->limit;
    otBase = SCRATCH_OT_BASE_AS(OT_TYPE);
    packed = motionReg->packedVelocity;
    shapeReg = shape;
    if (limit < elapsed) {
        elapsed = limit;
    }

    x = motionReg->x;
    if (packed & 0x8000) {
        temp = packed | 0xFFFF0000;
    } else {
        temp = packed & 0x7FFF;
    }
    interpProduct = elapsed * temp;
    interp = interpProduct / 32;

    
    y = motionReg->y;
    x += interp;
    if (packed < 0) {
        s32 hi;
        s32 mask;

        hi = packed >> 16;
        mask = 0xFFFF0000;
        temp = hi | mask;
    } else {
        temp = (packed >> 16) & 0x7FFF;
    }
    interpProduct = elapsed * temp;
    interp = interpProduct / 32;
    y += interp;
    

    switch (shapeReg->flags & 3) {
    case 0:
        mode = 0;
        break;
    case 1:
        mode = 3;
        break;
    case 2:
        mode = 5;
        break;
    case 3:
        mode = 0x2BE;
        break;
    }

    flagByte = shapeReg->flags;
    flags8 = flagByte & 8;
    flags4 = flagByte & 4;
    if (type != 0) {
        temp = shapeReg->alpha & 0x7F;
        
        alpha = (u8)temp;
    } else {
        alpha = 0x80;
    }

    DrawSprite(
        otBase + mode,
        (s16)x,
        (s16)y,
        shapeReg->width,
        shapeReg->height,
        shapeReg->u,
        shapeReg->v,
        motionReg->r,
        motionReg->g,
        motionReg->b,
        motionReg->clut,
        flags8,
        flags4,
        alpha);
}

void DrawScriptedLine(s32 elapsed, ScriptedLineShape *shape, ScriptedLineMotion *motion) {
    ScriptedLineMotion *motionReg = motion;
    ScriptedLineShape *shapeReg;
    OT_TYPE *otBase;
    s32 mode;
    s32 y1Reg;
    s32 x0;
    s32 y0Call;
    s32 x1Base;
    s32 x1;
    s32 y1;
    s32 limit;
    s32 xPacked;
    s32 yPacked;
    s32 temp;
    s32 interp;
    u32 interpProduct;
    s32 alpha;

    limit = motionReg->limit;
    otBase = SCRATCH_OT_BASE_AS(OT_TYPE);
    xPacked = motionReg->packedVelocity0;
    yPacked = motionReg->packedVelocity1;
    shapeReg = shape;
    if (limit < elapsed) {
        elapsed = limit;
    }

    y1 = motionReg->x0;
    if (xPacked & 0x8000) {
        temp = xPacked | 0xFFFF0000;
    } else {
        temp = xPacked & 0x7FFF;
    }
    interpProduct = elapsed * temp;
    interp = interpProduct / 32;
    x0 = y1 + interp;

    y1 = motionReg->y0;
    if (xPacked < 0) {
        s32 mask;

        y1Reg = xPacked >> 16;
        mask = 0xFFFF0000;
        temp = y1Reg | mask;
    } else {
        temp = (xPacked >> 16) & 0x7FFF;
    }
    interpProduct = elapsed * temp;
    interp = interpProduct / 32;
    y1 += interp;

    
    x1Base = motionReg->x1;
    if (yPacked & 0x8000) {
        y0Call = y1;
        temp = yPacked | 0xFFFF0000;
    } else {
        y0Call = y1;
        temp = yPacked & 0x7FFF;
    }
    interpProduct = elapsed * temp;
    interp = interpProduct / 32;

    
    y1 = motionReg->y1;
    x1 = x1Base + interp;
    if (yPacked < 0) {
        y1Reg = yPacked >> 16;
        x1Base = 0xFFFF0000;
        temp = y1Reg | x1Base;
    } else {
        temp = (yPacked >> 16) & 0x7FFF;
    }
    interpProduct = elapsed * temp;
    interp = interpProduct / 32;
    y1 += interp;
    

    switch (shapeReg->flags & 3) {
    case 0:
        mode = 0;
        break;
    case 1:
        mode = 3;
        break;
    case 2:
        mode = 5;
        break;
    case 3:
        mode = 0x2BE;
        break;
    }

    if (shapeReg->flags & 4) {
        alpha = shapeReg->flags & 0x60;
    } else {
        alpha = 0xFF;
    }

    y1Reg = (s16)y1;
    /* Retail's sll/sra pair sign-extends the low halfword. */
    x0 = (s16)x0;
    y1 = (s16)y0Call;
    x1 = (s16)x1;
    
    DrawLine(
        &otBase[mode],
        x0,
        y1,
        x1,
        y1Reg,
        shapeReg->r,
        shapeReg->g,
        shapeReg->b,
        alpha);
}

void DrawScriptedTriangle(s32 time, ScriptedTriangleShape *styleArg, ScriptedTriangleMotion *recordArg) {
    ScriptedTriangleShape *style;
    ScriptedTriangleMotion *record;
    OT_TYPE *ot;
    s32 limit;
    s32 packedSpeed;
    s32 product;
    s32 x;
    s32 y0;
    s32 y;
    s32 y1;
    s32 mode;
    s32 semiTrans;
    s32 flags;
    u32 productResult;

    style = styleArg;
    record = recordArg;
    /* The barrier is load-bearing: without it the scheduler sinks the
     * scratchpad load past the second record load. */
    limit = record->limit;
    ot = SCRATCH_OT_BASE_AS(OT_TYPE);
    
    packedSpeed = record->packedVelocity;
    if (limit < time) {
        time = limit;
    }

    limit = record->x;
    if (packedSpeed & 0x8000) {
        product = packedSpeed | 0xFFFF0000;
    } else {
        product = packedSpeed & 0x7FFF;
    }
    productResult = time * product;
    product = productResult / 32;
    productResult = limit + product;
    product = productResult;
    

    y = record->y;
    x = product;
    if (packedSpeed < 0) {
        product = packedSpeed >> 16;
        packedSpeed = 0xFFFF0000;
        product |= packedSpeed;
    } else {
        product = packedSpeed >> 16;
        product &= 0x7FFF;
    }
    productResult = time * product;
    product = productResult;
    
    productResult = product;
    product = productResult / 32;
    y += product;

    product = style->y1;
    packedSpeed = style->y2;
    y0 = product + y;
    y1 = packedSpeed + y;
    product = style->x1;
    packedSpeed = style->x2;
    productResult = x + product;
    product = productResult;
    
    limit = product;
    packedSpeed = x + packedSpeed;

    switch (style->flags & 3) {
    case 0:
        mode = 0;
        break;
    case 1:
        mode = 3;
        break;
    case 2:
        mode = 5;
        break;
    case 3:
        mode = 0x2BE;
        break;
    }

    {
        s32 alpha;

        alpha = style->flags;
        semiTrans = alpha & 4;
        if (semiTrans != 0) {
            alpha &= 0x60;
            
            flags = (u8)alpha;
        } else {
            flags = 0x80;
        }
    }

    
    DrawFlatTriangle(
        ot + mode,
        (s16)x,
        (s16)y,
        (s16)limit,
        (s16)y0,
        (s16)packedSpeed,
        (s16)y1,
        style->r,
        style->g,
        style->b,
        semiTrans,
        flags);
}

void DrawScriptedQuad(s32 time, ScriptedQuadShape *desc, ScriptedQuadMotion *ctx) {
    s32 duration;
    u8 *table;
    ScriptedQuadShape *entry;
    s32 velocity0;
    s32 velocity1;
    s32 x;
    s32 y;
    s32 dx;
    s32 dy;
    s32 index;
    s32 posX;
    s32 posY;
    s32 posX2;
    s32 posY2;
    u32 velocityX;
    u32 velocityY;
    u32 velocityX2;
    u32 velocityY2;
    s32 value;
    s32 flags;

    duration = ctx->limit;
    table = (u8 *)SCRATCH_OT_BASE_AS(OT_TYPE);
    velocity0 = ctx->packedVelocity;
    velocity1 = ctx->packedSizeVelocity;
    entry = desc;
    if (duration < time) {
        time = duration;
    }

    posX = ctx->x;
    if (velocity0 & 0x8000) {
        velocityX = velocity0 | 0xFFFF0000;
    } else {
        velocityX = velocity0 & 0x7FFF;
    }
    posX += (time * velocityX) >> 5;
    
    x = posX;

    posY = ctx->y;
    if (velocity0 < 0) {
        value = velocity0 >> 16;
        velocityY = value | 0xFFFF0000;
    } else {
        value = velocity0 / 65536;
        velocityY = value & 0x7FFF;
    }
    value = posY + ((time * velocityY) >> 5);
    y = value;
    

    posX2 = ctx->width;
    if (velocity1 & 0x8000) {
        velocityX2 = velocity1 | 0xFFFF0000;
    } else {
        velocityX2 = velocity1 & 0x7FFF;
    }
    value = posX2 + ((time * velocityX2) >> 5);
    
    dx = value;

    posY2 = ctx->height;
    if (velocity1 < 0) {
        value = velocity1 >> 16;
        velocityY2 = value | 0xFFFF0000;
    } else {
        value = velocity1 / 65536;
        velocityY2 = value & 0x7FFF;
    }
    posY2 += (time * velocityY2) >> 5;
    
    dy = posY2;

    switch (entry->flags & 3) {
    case 0:
        index = 0;
        break;
    case 1:
        index = 3;
        break;
    case 2:
        index = 5;
        break;
    case 3:
        index = 0x2BE;
        break;
    }

    flags = entry->flags;
    GameDrawTexturedQuad(&((OT_TYPE *)table)[index], (s16)x, (s16)y,
                  (s16)(x + dx), (s16)y, (s16)x, (s16)(y + dy),
                  (s16)(x + dx), (s16)(y + dy), entry->u0, entry->v0,
                  entry->u1, entry->v1, entry->u2, entry->v2, entry->u3,
                  entry->v3, entry->r, entry->g, entry->b, entry->clut,
                  flags & 8, flags & 4, entry->alpha);
}

s32 RunTimedDrawScript(void *commands, s32 *progress, s32 step) {
    TimedDrawCommand *base = commands;
    s32 *progressPtr = progress;
    s32 stepReg = step;
    TimedDrawCommand *cmd;
    TimedDrawCommandAddress commandAddress;
    s32 index = 0;
    s32 remaining;
    u32 type;
    s32 nextProgress;
    s32 updatedProgress;
    s32 limit;

    
    if (stepReg < 0) {
        nextProgress = *progressPtr + stepReg;
        if (nextProgress > 0) {
            *progressPtr = nextProgress;
        } else {
            *progressPtr = 0;
        }
    }

    nextProgress = (index * 3) << 2;
    commandAddress.pointer = base;
    commandAddress.value = nextProgress + commandAddress.value;
    cmd = commandAddress.pointer;
    while (cmd->time >= 0) {
        remaining = *progressPtr - cmd->time;
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
        index++;
        }

    if (stepReg >= 0) {
        commandAddress.value = *progressPtr;
        updatedProgress = stepReg + commandAddress.value;
        limit = base[index].motion.value;
        if (updatedProgress < limit) {
            *progressPtr = updatedProgress;
        } else {
            *progressPtr = limit;
            return 1;
        }
    }

    return 0;
}


void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    ScriptedSpriteShape *shapePtr;
    ScriptedSpriteMotion *motionPtr;
    OT_TYPE *ot;
    s32 countReg;
    s32 i;
    TimedDrawCommand *cmd;
    s32 *timer;
    s32 xOffset;
    s32 yOffset;
    s32 nextTimer;
    s32 value;
    s32 temporary;
    FadingMenuTableAddress tableAddress;
    s32 timerValue;
    u32 fade;
    s32 drawX;
    s32 drawY;
    s32 drawW;
    s32 elapsed;
    s32 limit;
    s32 packed;
    u32 offsetProduct;

    shapePtr = g_MenuRowScript[0].shape.spriteShape;
    elapsed = progress - g_MenuRowScript[0].time;
    motionPtr = g_MenuRowScript[0].motion.spriteMotion;
    ot = SCRATCH_OT_BASE_AS(OT_TYPE);
    countReg = count;
    packed = motionPtr->packedVelocity;
    i = 0;

    if (elapsed < 0) {
        return;
    }

    limit = motionPtr->limit;
    if (limit < elapsed) {
        elapsed = limit;
    }

    g_MenuRowFlashLevels[slot] = 0x1FC;

    if (packed & 0x8000) {
        value = packed | 0xFFFF0000;
    } else {
        value = packed & 0x7FFF;
    }
    value = elapsed * value;
    offsetProduct = value;
    xOffset = offsetProduct / 32;

    if (packed < 0) {
        value = packed >> 0x10;
        temporary = 0xFFFF0000;
        value |= temporary;
    } else {
        value = (packed >> 0x10) & 0x7FFF;
    }
    value = elapsed * value;
    offsetProduct = value;
    yOffset = offsetProduct / 32;

    if (countReg < i) {
        return;
    }

    tableAddress.commands = g_MenuRowScript;
    cmd = &tableAddress.commands[i];

    for (; i <= countReg; i++, cmd++) {
        shapePtr = cmd->shape.spriteShape;
        motionPtr = cmd->motion.spriteMotion;
        tableAddress.timers = g_MenuRowFlashLevels;
        timer = &tableAddress.timers[i];

        fade = *timer & 0x1FF;
        *timer = fade;
        fade >>= 2;

        value = shapePtr->height;
        drawX = (u16)motionPtr->x;
        drawY = (u16)motionPtr->y;
        drawW = shapePtr->width;
        drawX = (s16)(drawX + xOffset);
        drawY = (s16)(drawY + yOffset);

        DrawSprite(ot + 2,
                      drawX,
                      drawY,
                      drawW,
                      value,
                      shapePtr->u,
                      shapePtr->v,
                      fade,
                      fade,
                      fade,
                      motionPtr->clut,
                      0,
                      1,
                      shapePtr->alpha);

        timerValue = *timer;
        nextTimer = 0;
        if (timerValue >= 60) {
            nextTimer = timerValue - 60;
        }
        *timer = nextTimer;
        }
}


void GameDrawMenuButton(s32 x0, s32 y0, s32 x1, s32 y1,
                   u8 r, u8 g, u8 b,
                   s32 flags, s32 textX, s32 textY, u8 *caption) {
    s32 f = flags;
    s32 p0 = x0;
    void *ot = SCRATCH_OT_BASE_AS(void);
    s32 p1 = y0;
    s32 p2 = x1;
    s32 p3 = y1;

    if (flags & 0x10) {
        if (flags % 2) {
            DrawLargeText((s16)(x0 + textX), (s16)(y0 + textY), (char *)caption,
                          0x7f, 0x7f, 0x7f, 0x244, (flags & 8) ? 0x20 : 0x40);
        } else {
            DrawSmallText((s16)(x0 + textX), (s16)(y0 + textY), (char *)caption,
                          0x7f, 0x7f, 0x7f, 0x244, (flags & 8) ? 0x20 : 0x40);
        }
    }
    DrawRectOutline(ot, (s16)p0, (s16)p1, (s16)p2, (s16)p3,
                    0xb4, 0xb4, 0xb4, (f & 4) ? (f & 0x60) : 0xff);
    DrawSolidRect(ot, (s16)p0, (s16)p1, (s16)p2, (s16)p3,
                  r, g, b, (f & 2) ? (f & 0x60) : 0xff);
    /* The second p3 use keeps it ahead of ot in global-alloc priority. */
    
}


void DrawMenuCursorBox(s32 x0, s32 y0, s32 x1, s32 y1, s32 useFlash) {
    void *ot;
    s32 savedX0;
    s32 savedY0;
    s32 savedX1;
    s32 savedY1;
    s32 color;
    s32 white;
    s32 counter;

    ot = SCRATCH_OT_BASE_AS(void);
    savedX0 = x0;
    savedY0 = y0;
    savedX1 = x1;
    savedY1 = y1;
    if (useFlash != 0) {
        if (g_AnimTimer & 2) {
            color = 0xFF;
        } else {
            color = 0x60;
        }
    } else {
        counter = g_MenuCursorPulsePhase;
        counter = rsin(counter % 4096);
        color = (counter / 64) - 0x41;
    }

    white = 0xFF;
    DrawRectOutline(
        ot,
        (s16)(savedX0 - 1),
        (s16)(savedY0 - 2),
        (s16)(savedX1 + 2),
        (s16)(savedY1 + 4),
        0,
        (u8)color,
        0,
        white);
    DrawRectOutline(
        ot, (s16)savedX0, (s16)savedY0, (s16)savedX1, (s16)(savedY1 + 0), 0, (u8)color, 0, white);
    g_MenuCursorPulsePhase += 0x60;
}
