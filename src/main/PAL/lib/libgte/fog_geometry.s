.set noat
.set noreorder

/* libgte InitGeom: brings the GTE up.
 *
 * It calls the exception-vector patch, sets the COP2-enable bit in the COP0
 * status register, then loads the GTE control registers with the screen offset,
 * projection distance and fog range the library defaults to. mfc0/mtc0 and
 * ctc2 are the entire routine; none of it is expressible in C, so it was
 * assembly in the original. $ra is parked at a fixed global across the call,
 * reached through %hi/%lo so the block still relocates.
 */
glabel InitGeom
    /* 80068928 */  lui        $at, %hi(D_80094B0C)
    /* 8006892C */  sw         $ra, %lo(D_80094B0C)($at)
    /* 80068930 */  jal        func_80069FA8
    /* 80068934 */   nop
    /* 80068938 */  lui        $ra, %hi(D_80094B0C)
    /* 8006893C */  lw         $ra, %lo(D_80094B0C)($ra)
    /* 80068940 */  nop
    /* 80068944 */  mfc0       $v0, $12
    /* 80068948 */  lui        $v1, (0x40000000 >> 16)
    /* 8006894C */  or         $v0, $v0, $v1
    /* 80068950 */  mtc0       $v0, $12
    /* 80068954 */  nop
    /* 80068958 */  addiu      $t0, $zero, 0x155
    /* 8006895C */  ctc2       $t0, $29
    /* 80068960 */  nop
    /* 80068964 */  addiu      $t0, $zero, 0x100
    /* 80068968 */  ctc2       $t0, $30
    /* 8006896C */  nop
    /* 80068970 */  addiu      $t0, $zero, 0x3E8
    /* 80068974 */  ctc2       $t0, $26
    /* 80068978 */  nop
    /* 8006897C */  addiu      $t0, $zero, -0x1062
    /* 80068980 */  ctc2       $t0, $27
    /* 80068984 */  nop
    /* 80068988 */  lui        $t0, (0x1400000 >> 16)
    /* 8006898C */  ctc2       $t0, $28
    /* 80068990 */  nop
    /* 80068994 */  ctc2       $zero, $24
    /* 80068998 */  ctc2       $zero, $25
    /* 8006899C */  nop
    /* 800689A0 */  jr         $ra
    /* 800689A4 */   nop
endlabel InitGeom

.globl SquareRoot0
SquareRoot0:
/* 800689A8 4884F000 */  mtc2 $a0, $30
/* 800689AC 00000000 */  nop
/* 800689B0 00000000 */  nop
/* 800689B4 4802F800 */  mfc2 $v0, $31
/* 800689B8 24010020 */  addiu $at, $zero, 0x20
/* 800689BC 10410019 */  beq $v0, $at, .L80068A24
/* 800689C0 00000000 */  nop
/* 800689C4 30480001 */  andi $t0, $v0, 0x1
/* 800689C8 240AFFFE */  addiu $t2, $zero, -0x2
/* 800689CC 004A5024 */  and $t2, $v0, $t2
/* 800689D0 2409001F */  addiu $t1, $zero, 0x1F
/* 800689D4 012A4822 */  sub $t1, $t1, $t2
/* 800689D8 00094843 */  sra $t1, $t1, 1
/* 800689DC 214BFFE8 */  addi $t3, $t2, -0x18
/* 800689E0 05600003 */  bltz $t3, .L800689F0
/* 800689E4 00000000 */  nop
/* 800689E8 01646004 */  sllv $t4, $a0, $t3
/* 800689EC 10000003 */  b .L800689FC
.L800689F0:
/* 800689F0 240B0018 */  addiu $t3, $zero, 0x18
/* 800689F4 016A5822 */  sub $t3, $t3, $t2
/* 800689F8 01646007 */  srav $t4, $a0, $t3
.L800689FC:
/* 800689FC 218CFFC0 */  addi $t4, $t4, -0x40
/* 80068A00 000C6040 */  sll $t4, $t4, 1
/* 80068A04 3C0D8009 */  lui $t5, %hi(D_80094B1C)
/* 80068A08 01AC6821 */  addu $t5, $t5, $t4
/* 80068A0C 85AD4B1C */  lh $t5, %lo(D_80094B1C)($t5)
/* 80068A10 00000000 */  nop
/* 80068A14 012D6804 */  sllv $t5, $t5, $t1
/* 80068A18 000D1302 */  srl $v0, $t5, 12
/* 80068A1C 03E00008 */  jr $ra
/* 80068A20 00000000 */  nop
.L80068A24:
/* 80068A24 03E00008 */  jr $ra
/* 80068A28 24020000 */  addiu $v0, $zero, 0x0
