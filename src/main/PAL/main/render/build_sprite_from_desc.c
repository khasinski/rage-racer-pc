#include "game/prim.h"
#include "game/render.h"

void BuildSpriteFromDesc(SPRT *sprite, GameSpriteDesc *desc) {
    SetSprt(sprite);
    sprite->x0 = desc->x;
    sprite->y0 = desc->y;
    sprite->w = desc->w;
    sprite->h = desc->h;
    sprite->u0 = desc->u0;
    sprite->v0 = desc->v0;
    sprite->clut = desc->clut;
    SetSemiTrans(sprite, desc->semiTrans);
    SetShadeTex(sprite, 1);
}
