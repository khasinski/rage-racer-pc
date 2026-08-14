.include "macro.inc"

.set noat
.set noreorder

.section .text, "ax"

/* libgte exception-vector patch, hand-written in the original. */

.globl func_80069FA8
func_80069FA8:
/* 80069FA8 3C01800A */  lui $at, %hi(D_8009AEDC)
/* 80069FAC AC3FAEDC */  sw $ra, %lo(D_8009AEDC)($at)
/* 80069FB0 0C018C84 */  jal EnterCriticalSection
/* 80069FB4 00000000 */  nop
/* 80069FB8 240A00B0 */  addiu $t2, $zero, 0xB0
/* 80069FBC 0140F809 */  jalr $t2
/* 80069FC0 24090056 */  addiu $t1, $zero, 0x56
/* 80069FC4 3C0A8007 */  lui $t2, %hi(D_8006A010)
/* 80069FC8 3C098007 */  lui $t1, %hi(func_8006A048)
/* 80069FCC 8C420018 */  lw $v0, 0x18($v0)
/* 80069FD0 254AA010 */  addiu $t2, $t2, %lo(D_8006A010)
/* 80069FD4 2529A048 */  addiu $t1, $t1, %lo(func_8006A048)
.L80069FD8:
/* 80069FD8 8D430000 */  lw $v1, 0x0($t2)
/* 80069FDC 254A0004 */  addiu $t2, $t2, 0x4
/* 80069FE0 24420004 */  addiu $v0, $v0, 0x4
/* 80069FE4 1549FFFC */  bne $t2, $t1, .L80069FD8
/* 80069FE8 AC43FFFC */  sw $v1, -0x4($v0)
/* 80069FEC 0C01A812 */  jal func_8006A048
/* 80069FF0 00000000 */  nop
/* 80069FF4 0C018C88 */  jal ExitCriticalSection
/* 80069FF8 00000000 */  nop
/* 80069FFC 3C1F800A */  lui $ra, %hi(D_8009AEDC)
/* 8006A000 8FFFAEDC */  lw $ra, %lo(D_8009AEDC)($ra)
/* 8006A004 00000000 */  nop
/* 8006A008 03E00008 */  jr $ra
/* 8006A00C 00000000 */  nop
/* 8006A010 00000000 */  nop
/* 8006A014 00000000 */  nop
