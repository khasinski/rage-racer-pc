.include "macro.inc"

.set noreorder
.set noat
.section .text, "ax"

.globl D_800630B4
D_800630B4:
/* 800630B4 3C02800A */  lui $v0, %hi(D_8009AED8)
/* 800630B8 2442AED8 */  addiu $v0, $v0, %lo(D_8009AED8)
/* 800630BC 3C03801F */  lui $v1, %hi(main_BSS_END)
/* 800630C0 24632A10 */  addiu $v1, $v1, %lo(main_BSS_END)
.L800630C4:
/* 800630C4 AC400000 */  sw $zero, 0x0($v0)
/* 800630C8 24420004 */  addiu $v0, $v0, 0x4
/* 800630CC 0043082B */  sltu $at, $v0, $v1
/* 800630D0 1420FFFC */  bnez $at, .L800630C4
/* 800630D4 00000000 */  nop
/* 800630D8 24020004 */  addiu $v0, $zero, 0x4
/* 800630DC 00000000 */  nop
/* 800630E0 00000000 */  nop
/* 800630E4 00000000 */  nop
/* 800630E8 00000000 */  nop
/* 800630EC 3C048006 */  lui $a0, %hi(D_80063160)
/* 800630F0 24843160 */  addiu $a0, $a0, %lo(D_80063160)
/* 800630F4 00822021 */  addu $a0, $a0, $v0
/* 800630F8 8C820000 */  lw $v0, 0x0($a0)
/* 800630FC 3C088000 */  lui $t0, 0x8000
/* 80063100 0048E825 */  or $sp, $v0, $t0
/* 80063104 3C04801F */  lui $a0, %hi(main_BSS_END)
/* 80063108 24842A10 */  addiu $a0, $a0, %lo(main_BSS_END)
/* 8006310C 000420C0 */  sll $a0, $a0, 3
/* 80063110 000420C2 */  srl $a0, $a0, 3
/* 80063114 3C03800A */  lui $v1, %hi(D_8009A520)
/* 80063118 8C63A520 */  lw $v1, %lo(D_8009A520)($v1)
/* 8006311C 00000000 */  nop
/* 80063120 00432823 */  subu $a1, $v0, $v1
/* 80063124 00A42823 */  subu $a1, $a1, $a0
/* 80063128 00882025 */  or $a0, $a0, $t0
/* 8006312C 3C01800A */  lui $at, %hi(D_8009AED8)
/* 80063130 AC3FAED8 */  sw $ra, %lo(D_8009AED8)($at)
/* 80063134 3C1C800A */  lui $gp, %hi(D_8009AED8)
/* 80063138 279CAED8 */  addiu $gp, $gp, %lo(D_8009AED8)
/* 8006313C 03A0F021 */  addu $fp, $sp, $zero
/* 80063140 0C018C5C */  jal func_80063170
/* 80063144 20840004 */  addi $a0, $a0, 0x4
/* 80063148 3C1F800A */  lui $ra, %hi(D_8009AED8)
/* 8006314C 8FFFAED8 */  lw $ra, %lo(D_8009AED8)($ra)
/* 80063150 00000000 */  nop
/* 80063154 0C005944 */  jal MainLoop
/* 80063158 00000000 */  nop
/* 8006315C 0000004D */  break 0, 1
.globl D_80063160
D_80063160:
/* 80063160 00200000 */  .word 0x00200000
/* 80063164 00200000 */  .word 0x00200000
/* 80063168 00200000 */  .word 0x00200000
/* 8006316C 00200000 */  .word 0x00200000
.globl func_80063170
func_80063170:
/* 80063170 240A00A0 */  addiu $t2, $zero, 0xA0
/* 80063174 01400008 */  jr $t2
/* 80063178 24090039 */  addiu $t1, $zero, 0x39
/* 8006317C 00000000 */  nop
.globl BiosBuInit
BiosBuInit:
/* 80063180 240A00A0 */  addiu $t2, $zero, 0xA0
/* 80063184 01400008 */  jr $t2
/* 80063188 24090070 */  addiu $t1, $zero, 0x70
/* 8006318C 00000000 */  nop
.globl BiosSetMemSize
BiosSetMemSize:
/* 80063190 240A00A0 */  addiu $t2, $zero, 0xA0
/* 80063194 01400008 */  jr $t2
/* 80063198 2409009F */  addiu $t1, $zero, 0x9F
/* 8006319C 00000000 */  nop
.globl OpenEvent
OpenEvent:
/* 800631A0 240A00B0 */  addiu $t2, $zero, 0xB0
/* 800631A4 01400008 */  jr $t2
/* 800631A8 24090008 */  addiu $t1, $zero, 0x8
/* 800631AC 00000000 */  nop
.globl CloseEvent
CloseEvent:
/* 800631B0 240A00B0 */  addiu $t2, $zero, 0xB0
/* 800631B4 01400008 */  jr $t2
/* 800631B8 24090009 */  addiu $t1, $zero, 0x9
/* 800631BC 00000000 */  nop
.globl TestEvent
TestEvent:
/* 800631C0 240A00B0 */  addiu $t2, $zero, 0xB0
/* 800631C4 01400008 */  jr $t2
/* 800631C8 2409000B */  addiu $t1, $zero, 0xB
/* 800631CC 00000000 */  nop
.globl EnableEvent
EnableEvent:
/* 800631D0 240A00B0 */  addiu $t2, $zero, 0xB0
/* 800631D4 01400008 */  jr $t2
/* 800631D8 2409000C */  addiu $t1, $zero, 0xC
/* 800631DC 00000000 */  nop
.globl DisableEvent
DisableEvent:
/* 800631E0 240A00B0 */  addiu $t2, $zero, 0xB0
/* 800631E4 01400008 */  jr $t2
/* 800631E8 2409000D */  addiu $t1, $zero, 0xD
/* 800631EC 00000000 */  nop
.globl InitPad
InitPad:
/* 800631F0 240A00B0 */  addiu $t2, $zero, 0xB0
/* 800631F4 01400008 */  jr $t2
/* 800631F8 24090012 */  addiu $t1, $zero, 0x12
/* 800631FC 00000000 */  nop
.globl StartPad
StartPad:
/* 80063200 240A00B0 */  addiu $t2, $zero, 0xB0
/* 80063204 01400008 */  jr $t2
/* 80063208 24090013 */  addiu $t1, $zero, 0x13
/* 8006320C 00000000 */  nop
.globl EnterCriticalSection
EnterCriticalSection:
/* 80063210 24040001 */  addiu $a0, $zero, 0x1
/* 80063214 0000000C */  syscall 0
/* 80063218 03E00008 */  jr $ra
/* 8006321C 00000000 */  nop
