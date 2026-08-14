.set noat
.set noreorder

/* libgte matrix stack.
 *
 * cfc2/ctc2 move the GTE rotation matrix and translation vector between the
 * coprocessor and a 20-deep stack in bss, which is the whole job and has no C
 * spelling. The stack depth and the saved $ra live at fixed globals that the
 * routines reach through %hi/%lo, so the block relocates with the image
 * instead of pinning the retail addresses.
 */

.globl ScaleMatrixL
ScaleMatrixL:
/* 80069110 8C880000 */  lw $t0, 0x0($a0)
/* 80069114 8CAB0000 */  lw $t3, 0x0($a1)
/* 80069118 3109FFFF */  andi $t1, $t0, 0xFFFF
/* 8006911C 00094C00 */  sll $t1, $t1, 16
/* 80069120 00094C03 */  sra $t1, $t1, 16
/* 80069124 012B0019 */  multu $t1, $t3
/* 80069128 00085403 */  sra $t2, $t0, 16
/* 8006912C 8CAC0004 */  lw $t4, 0x4($a1)
/* 80069130 8CAD0008 */  lw $t5, 0x8($a1)
/* 80069134 8C880004 */  lw $t0, 0x4($a0)
/* 80069138 00801021 */  addu $v0, $a0, $zero
/* 8006913C 00004812 */  mflo $t1
/* 80069140 00094B03 */  sra $t1, $t1, 12
/* 80069144 3129FFFF */  andi $t1, $t1, 0xFFFF
/* 80069148 014B0019 */  multu $t2, $t3
/* 8006914C 00005012 */  mflo $t2
/* 80069150 000A5303 */  sra $t2, $t2, 12
/* 80069154 000A5400 */  sll $t2, $t2, 16
/* 80069158 012A4825 */  or $t1, $t1, $t2
/* 8006915C AC890000 */  sw $t1, 0x0($a0)
/* 80069160 3109FFFF */  andi $t1, $t0, 0xFFFF
/* 80069164 00094C00 */  sll $t1, $t1, 16
/* 80069168 00094C03 */  sra $t1, $t1, 16
/* 8006916C 012B0019 */  multu $t1, $t3
/* 80069170 00085403 */  sra $t2, $t0, 16
/* 80069174 8C880008 */  lw $t0, 0x8($a0)
/* 80069178 00004812 */  mflo $t1
/* 8006917C 00094B03 */  sra $t1, $t1, 12
/* 80069180 3129FFFF */  andi $t1, $t1, 0xFFFF
/* 80069184 014C0019 */  multu $t2, $t4
/* 80069188 00005012 */  mflo $t2
/* 8006918C 000A5303 */  sra $t2, $t2, 12
/* 80069190 000A5400 */  sll $t2, $t2, 16
/* 80069194 012A4825 */  or $t1, $t1, $t2
/* 80069198 AC890004 */  sw $t1, 0x4($a0)
/* 8006919C 3109FFFF */  andi $t1, $t0, 0xFFFF
/* 800691A0 00094C00 */  sll $t1, $t1, 16
/* 800691A4 00094C03 */  sra $t1, $t1, 16
/* 800691A8 012C0019 */  multu $t1, $t4
/* 800691AC 00085403 */  sra $t2, $t0, 16
/* 800691B0 8C88000C */  lw $t0, 0xC($a0)
/* 800691B4 00004812 */  mflo $t1
/* 800691B8 00094B03 */  sra $t1, $t1, 12
/* 800691BC 3129FFFF */  andi $t1, $t1, 0xFFFF
/* 800691C0 014C0019 */  multu $t2, $t4
/* 800691C4 00005012 */  mflo $t2
/* 800691C8 000A5303 */  sra $t2, $t2, 12
/* 800691CC 000A5400 */  sll $t2, $t2, 16
/* 800691D0 012A4825 */  or $t1, $t1, $t2
/* 800691D4 AC890008 */  sw $t1, 0x8($a0)
/* 800691D8 3109FFFF */  andi $t1, $t0, 0xFFFF
/* 800691DC 00094C00 */  sll $t1, $t1, 16
/* 800691E0 00094C03 */  sra $t1, $t1, 16
/* 800691E4 012D0019 */  multu $t1, $t5
/* 800691E8 00085403 */  sra $t2, $t0, 16
/* 800691EC 8C880010 */  lw $t0, 0x10($a0)
/* 800691F0 00004812 */  mflo $t1
/* 800691F4 00094B03 */  sra $t1, $t1, 12
/* 800691F8 3129FFFF */  andi $t1, $t1, 0xFFFF
/* 800691FC 014D0019 */  multu $t2, $t5
/* 80069200 00005012 */  mflo $t2
/* 80069204 000A5303 */  sra $t2, $t2, 12
/* 80069208 000A5400 */  sll $t2, $t2, 16
/* 8006920C 012A4825 */  or $t1, $t1, $t2
/* 80069210 AC89000C */  sw $t1, 0xC($a0)
/* 80069214 3109FFFF */  andi $t1, $t0, 0xFFFF
/* 80069218 00094C00 */  sll $t1, $t1, 16
/* 8006921C 00094C03 */  sra $t1, $t1, 16
/* 80069220 012D0019 */  multu $t1, $t5
/* 80069224 00004812 */  mflo $t1
/* 80069228 00094B03 */  sra $t1, $t1, 12
/* 8006922C 03E00008 */  jr $ra
/* 80069230 AC890010 */  sw $t1, 0x10($a0)

glabel PushMatrix
    /* 80069234 */  lui        $t6, %hi(D_80094CA8)
    /* 80069238 */  lw         $t6, %lo(D_80094CA8)($t6)
    /* 8006923C */  nop
    /* 80069240 */  slti       $at, $t6, 0x280
    /* 80069244 */  bnez       $at, .L80069270
    /* 80069248 */   lui       $at, %hi(D_80094C9C)
    /* 8006924C */  sw         $ra, %lo(D_80094C9C)($at)
    /* 80069250 */  lui        $a0, %hi(D_80094F2C)
    /* 80069254 */  jal        printf
    /* 80069258 */   addiu     $a0, $a0, %lo(D_80094F2C)
    /* 8006925C */  lui        $ra, %hi(D_80094C9C)
    /* 80069260 */  lw         $ra, %lo(D_80094C9C)($ra)
    /* 80069264 */  nop
    /* 80069268 */  jr         $ra
    /* 8006926C */   nop
  .L80069270:
    /* 80069270 */  lui        $t7, %hi(D_80094CAC)
    /* 80069274 */  addu       $t7, $t7, $t6
    /* 80069278 */  addiu      $t7, $t7, %lo(D_80094CAC)
    /* 8006927C */  cfc2       $t0, $0
    /* 80069280 */  cfc2       $t1, $1
    /* 80069284 */  sw         $t0, 0x0($t7)
    /* 80069288 */  sw         $t1, 0x4($t7)
    /* 8006928C */  cfc2       $t0, $2
    /* 80069290 */  cfc2       $t1, $3
    /* 80069294 */  sw         $t0, 0x8($t7)
    /* 80069298 */  sw         $t1, 0xC($t7)
    /* 8006929C */  cfc2       $t0, $4
    /* 800692A0 */  nop
    /* 800692A4 */  sw         $t0, 0x10($t7)
    /* 800692A8 */  cfc2       $t0, $5
    /* 800692AC */  cfc2       $t1, $6
    /* 800692B0 */  cfc2       $t2, $7
    /* 800692B4 */  sw         $t0, 0x14($t7)
    /* 800692B8 */  sw         $t1, 0x18($t7)
    /* 800692BC */  sw         $t2, 0x1C($t7)
    /* 800692C0 */  addi       $t6, $t6, 0x20
    /* 800692C4 */  lui        $at, %hi(D_80094CA8)
    /* 800692C8 */  sw         $t6, %lo(D_80094CA8)($at)
    /* 800692CC */  jr         $ra
    /* 800692D0 */   nop
endlabel PushMatrix

glabel PopMatrix
    /* 800692D4 */  lui        $t6, %hi(D_80094CA8)
    /* 800692D8 */  lw         $t6, %lo(D_80094CA8)($t6)
    /* 800692DC */  nop
    /* 800692E0 */  bgtz       $t6, .L8006930C
    /* 800692E4 */   lui       $at, %hi(D_80094C9C)
    /* 800692E8 */  sw         $ra, %lo(D_80094C9C)($at)
    /* 800692EC */  lui        $a0, %hi(D_80094F5D)
    /* 800692F0 */  jal        printf
    /* 800692F4 */   addiu     $a0, $a0, %lo(D_80094F5D)
    /* 800692F8 */  lui        $ra, %hi(D_80094C9C)
    /* 800692FC */  lw         $ra, %lo(D_80094C9C)($ra)
    /* 80069300 */  nop
    /* 80069304 */  jr         $ra
    /* 80069308 */   nop
  .L8006930C:
    /* 8006930C */  addi       $t6, $t6, -0x20
    /* 80069310 */  lui        $at, %hi(D_80094CA8)
    /* 80069314 */  sw         $t6, %lo(D_80094CA8)($at)
    /* 80069318 */  lui        $t7, %hi(D_80094CAC)
    /* 8006931C */  addu       $t7, $t7, $t6
    /* 80069320 */  addiu      $t7, $t7, %lo(D_80094CAC)
    /* 80069324 */  lw         $t0, 0x0($t7)
    /* 80069328 */  lw         $t1, 0x4($t7)
    /* 8006932C */  ctc2       $t0, $0
    /* 80069330 */  ctc2       $t1, $1
    /* 80069334 */  lw         $t0, 0x8($t7)
    /* 80069338 */  lw         $t1, 0xC($t7)
    /* 8006933C */  ctc2       $t0, $2
    /* 80069340 */  ctc2       $t1, $3
    /* 80069344 */  lw         $t0, 0x10($t7)
    /* 80069348 */  nop
    /* 8006934C */  ctc2       $t0, $4
    /* 80069350 */  nop
    /* 80069354 */  lw         $t0, 0x14($t7)
    /* 80069358 */  lw         $t1, 0x18($t7)
    /* 8006935C */  lw         $t2, 0x1C($t7)
    /* 80069360 */  ctc2       $t0, $5
    /* 80069364 */  ctc2       $t1, $6
    /* 80069368 */  ctc2       $t2, $7
    /* 8006936C */  jr         $ra
    /* 80069370 */   nop
endlabel PopMatrix
