.set noat
.set noreorder

/* MDEC bitstream decoder, hand-assembled in the original.
 *
 * g_MdecVlcBufferSize is a mutable word sitting inside .text with the accessor
 * that reads and writes it immediately after, reaching it through its own
 * address. MdecUnpackStatus is the VLC inner loop: a resumable state machine
 * that keeps its working set in a global block rather than on the stack, packs
 * its own delay slots, and ends by setting a bit in the COP0 status register.
 * Neither shape is something a C compiler produces.
 */

/* The buffer size itself: a mutable word the original placed inside .text,
   immediately ahead of the routine that reads and writes it. */
dlabel g_MdecVlcBufferSize
/* 80064554 00FFFFFF */  .word 0x00FFFFFF
enddlabel g_MdecVlcBufferSize

glabel MdecSetVlcBufferSize
/* 80064558 3C088006 */  lui $t0, %hi(g_MdecVlcBufferSize)
/* 8006455C 25084554 */  addiu $t0, $t0, %lo(g_MdecVlcBufferSize)
/* 80064560 2081FFFF */  addi $at, $a0, -0x1
/* 80064564 18200004 */  blez $at, .L80064578
/* 80064568 8D020000 */  lw $v0, 0x0($t0)
/* 8006456C 00040840 */  sll $at, $a0, 1
/* 80064570 03E00008 */  jr $ra
/* 80064574 AD010000 */  sw $at, 0x0($t0)
.L80064578:
/* 80064578 3C0100FF */  lui $at, 0xFF
/* 8006457C 3421FFFF */  ori $at, $at, 0xFFFF
/* 80064580 03E00008 */  jr $ra
/* 80064584 AD010000 */  sw $at, 0x0($t0)
endlabel MdecSetVlcBufferSize

glabel MdecUnpackStatus
/* 80064588 3C088006 */  lui $t0, %hi(g_MdecVlcBufferSize)
/* 8006458C 25084554 */  addiu $t0, $t0, %lo(g_MdecVlcBufferSize)
/* 80064590 3C068008 */  lui $a2, %hi(D_800839A0)
/* 80064594 24C639A0 */  addiu $a2, $a2, %lo(D_800839A0)
/* 80064598 3C078009 */  lui $a3, %hi(D_800939A0)
/* 8006459C 24E739A0 */  addiu $a3, $a3, %lo(D_800939A0)
/* 800645A0 1480000F */  bnez $a0, .L800645E0
/* 800645A4 8D090000 */  lw $t1, 0x0($t0)
/* 800645A8 3C088006 */  lui $t0, %hi(D_800648C8)
/* 800645AC 250848C8 */  addiu $t0, $t0, %lo(D_800648C8)
/* 800645B0 8D040000 */  lw $a0, 0x0($t0)
/* 800645B4 8D050004 */  lw $a1, 0x4($t0)
/* 800645B8 8D020008 */  lw $v0, 0x8($t0)
/* 800645BC 8D03000C */  lw $v1, 0xC($t0)
/* 800645C0 8D0C0010 */  lw $t4, 0x10($t0)
/* 800645C4 8D0D0014 */  lw $t5, 0x14($t0)
/* 800645C8 8D0F001C */  lw $t7, 0x1C($t0)
/* 800645CC 8D180020 */  lw $t8, 0x20($t0)
/* 800645D0 8D190024 */  lw $t9, 0x24($t0)
/* 800645D4 01294820 */  add $t1, $t1, $t1
/* 800645D8 0401005F */  bgez $zero, .L80064758
/* 800645DC 00A97020 */  add $t6, $a1, $t1
.L800645E0:
/* 800645E0 00006820 */  add $t5, $zero, $zero
/* 800645E4 00007820 */  add $t7, $zero, $zero
/* 800645E8 0000C020 */  add $t8, $zero, $zero
/* 800645EC 0000C820 */  add $t9, $zero, $zero
/* 800645F0 01294820 */  add $t1, $t1, $t1
/* 800645F4 00A97020 */  add $t6, $a1, $t1
/* 800645F8 94880000 */  lhu $t0, 0x0($a0)
/* 800645FC 94890002 */  lhu $t1, 0x2($a0)
/* 80064600 948C0004 */  lhu $t4, 0x4($a0)
/* 80064604 948A0006 */  lhu $t2, 0x6($a0)
/* 80064608 94820008 */  lhu $v0, 0x8($a0)
/* 8006460C 9483000A */  lhu $v1, 0xA($a0)
/* 80064610 214AFFFD */  addi $t2, $t2, -0x3
/* 80064614 05400002 */  bltz $t2, .L80064620
/* 80064618 000C6280 */  sll $t4, $t4, 10
/* 8006461C 200D0001 */  addi $t5, $zero, 0x1
.L80064620:
/* 80064620 2084000C */  addi $a0, $a0, 0xC
/* 80064624 00021400 */  sll $v0, $v0, 16
/* 80064628 00431025 */  or $v0, $v0, $v1
/* 8006462C 00001825 */  move $v1, $zero
/* 80064630 A4A80000 */  sh $t0, 0x0($a1)
/* 80064634 A4A90002 */  sh $t1, 0x2($a1)
/* 80064638 20A50002 */  addi $a1, $a1, 0x2
.L8006463C:
/* 8006463C 11A00035 */  beqz $t5, .L80064714
/* 80064640 00024582 */  srl $t0, $v0, 22
/* 80064644 390103FF */  xori $at, $t0, 0x3FF
/* 80064648 10200085 */  beqz $at, .L80064860
/* 8006464C 20A50002 */  addi $a1, $a1, 0x2
/* 80064650 21A1FFFD */  addi $at, $t5, -0x3
/* 80064654 04200002 */  bltz $at, .L80064660
/* 80064658 20C1FC00 */  addi $at, $a2, -0x400
/* 8006465C 2021FC00 */  addi $at, $at, -0x400
.L80064660:
/* 80064660 00024602 */  srl $t0, $v0, 24
/* 80064664 00084080 */  sll $t0, $t0, 2
/* 80064668 01014020 */  add $t0, $t0, $at
/* 8006466C 95090000 */  lhu $t1, 0x0($t0)
/* 80064670 950A0002 */  lhu $t2, 0x2($t0)
/* 80064674 00004024 */  and $t0, $zero, $zero
/* 80064678 1140000A */  beqz $t2, .L800646A4
/* 8006467C 01221004 */  sllv $v0, $v0, $t1
/* 80064680 20010020 */  addi $at, $zero, 0x20
/* 80064684 002A0822 */  sub $at, $at, $t2
/* 80064688 00224006 */  srlv $t0, $v0, $at
/* 8006468C 04400004 */  bltz $v0, .L800646A0
/* 80064690 01421004 */  sllv $v0, $v0, $t2
/* 80064694 200BFFFF */  addi $t3, $zero, -0x1
/* 80064698 002B5806 */  srlv $t3, $t3, $at
/* 8006469C 010B4022 */  sub $t0, $t0, $t3
.L800646A0:
/* 800646A0 006A1820 */  add $v1, $v1, $t2
.L800646A4:
/* 800646A4 00691820 */  add $v1, $v1, $t1
/* 800646A8 30610010 */  andi $at, $v1, 0x10
/* 800646AC 10200005 */  beqz $at, .L800646C4
/* 800646B0 3063000F */  andi $v1, $v1, 0xF
/* 800646B4 94890000 */  lhu $t1, 0x0($a0)
/* 800646B8 20840002 */  addi $a0, $a0, 0x2
/* 800646BC 00694804 */  sllv $t1, $t1, $v1
/* 800646C0 00491025 */  or $v0, $v0, $t1
.L800646C4:
/* 800646C4 21A1FFFE */  addi $at, $t5, -0x2
/* 800646C8 1C200008 */  bgtz $at, .L800646EC
/* 800646CC 03284820 */  add $t1, $t9, $t0
/* 800646D0 10200004 */  beqz $at, .L800646E4
/* 800646D4 03084820 */  add $t1, $t8, $t0
/* 800646D8 01E84820 */  add $t1, $t7, $t0
/* 800646DC 04010004 */  bgez $zero, .L800646F0
/* 800646E0 01E87820 */  add $t7, $t7, $t0
.L800646E4:
/* 800646E4 04010002 */  bgez $zero, .L800646F0
/* 800646E8 0308C020 */  add $t8, $t8, $t0
.L800646EC:
/* 800646EC 0328C820 */  add $t9, $t9, $t0
.L800646F0:
/* 800646F0 00094880 */  sll $t1, $t1, 2
/* 800646F4 312903FF */  andi $t1, $t1, 0x3FF
/* 800646F8 01894825 */  or $t1, $t4, $t1
/* 800646FC 21AD0001 */  addi $t5, $t5, 0x1
/* 80064700 21A1FFF9 */  addi $at, $t5, -0x7
/* 80064704 14200011 */  bnez $at, .L8006474C
/* 80064708 A4A90000 */  sh $t1, 0x0($a1)
/* 8006470C 0401000F */  bgez $zero, .L8006474C
/* 80064710 21ADFFFA */  addi $t5, $t5, -0x6
.L80064714:
/* 80064714 390101FF */  xori $at, $t0, 0x1FF
/* 80064718 10200051 */  beqz $at, .L80064860
/* 8006471C 20A50002 */  addi $a1, $a1, 0x2
/* 80064720 00021280 */  sll $v0, $v0, 10
/* 80064724 2063000A */  addi $v1, $v1, 0xA
/* 80064728 30610010 */  andi $at, $v1, 0x10
/* 8006472C 10200005 */  beqz $at, .L80064744
/* 80064730 3063000F */  andi $v1, $v1, 0xF
/* 80064734 94890000 */  lhu $t1, 0x0($a0)
/* 80064738 20840002 */  addi $a0, $a0, 0x2
/* 8006473C 00694804 */  sllv $t1, $t1, $v1
/* 80064740 00491025 */  or $v0, $v0, $t1
.L80064744:
/* 80064744 01884025 */  or $t0, $t4, $t0
/* 80064748 A4A80000 */  sh $t0, 0x0($a1)
.L8006474C:
/* 8006474C 00AE0823 */  subu $at, $a1, $t6
/* 80064750 04210050 */  bgez $at, .L80064894
/* 80064754 20A50002 */  addi $a1, $a1, 0x2
.L80064758:
/* 80064758 000244C2 */  srl $t0, $v0, 19
/* 8006475C 000840C0 */  sll $t0, $t0, 3
/* 80064760 01064020 */  add $t0, $t0, $a2
/* 80064764 8D090000 */  lw $t1, 0x0($t0)
/* 80064768 00000000 */  nop
/* 8006476C 15200011 */  bnez $t1, .L800647B4
/* 80064770 312100FF */  andi $at, $t1, 0xFF
/* 80064774 00021200 */  sll $v0, $v0, 8
/* 80064778 20630008 */  addi $v1, $v1, 0x8
/* 8006477C 30610010 */  andi $at, $v1, 0x10
/* 80064780 10200005 */  beqz $at, .L80064798
/* 80064784 3063000F */  andi $v1, $v1, 0xF
/* 80064788 94880000 */  lhu $t0, 0x0($a0)
/* 8006478C 20840002 */  addi $a0, $a0, 0x2
/* 80064790 00684004 */  sllv $t0, $t0, $v1
/* 80064794 00481025 */  or $v0, $v0, $t0
.L80064798:
/* 80064798 000245C2 */  srl $t0, $v0, 23
/* 8006479C 00084080 */  sll $t0, $t0, 2
/* 800647A0 01074020 */  add $t0, $t0, $a3
/* 800647A4 8D090000 */  lw $t1, 0x0($t0)
/* 800647A8 00005820 */  add $t3, $zero, $zero
/* 800647AC 04010002 */  bgez $zero, .L800647B8
/* 800647B0 312100FF */  andi $at, $t1, 0xFF
.L800647B4:
/* 800647B4 8D0B0004 */  lw $t3, 0x4($t0)
.L800647B8:
/* 800647B8 00221004 */  sllv $v0, $v0, $at
/* 800647BC 00611820 */  add $v1, $v1, $at
/* 800647C0 30610010 */  andi $at, $v1, 0x10
/* 800647C4 10200005 */  beqz $at, .L800647DC
/* 800647C8 3063000F */  andi $v1, $v1, 0xF
/* 800647CC 94880000 */  lhu $t0, 0x0($a0)
/* 800647D0 20840002 */  addi $a0, $a0, 0x2
/* 800647D4 00684004 */  sllv $t0, $t0, $v1
/* 800647D8 00481025 */  or $v0, $v0, $t0
.L800647DC:
/* 800647DC 00094C02 */  srl $t1, $t1, 16
/* 800647E0 39217C1F */  xori $at, $t1, 0x7C1F
/* 800647E4 10200015 */  beqz $at, .L8006483C
/* 800647E8 3921FE00 */  xori $at, $t1, 0xFE00
/* 800647EC 1020FF93 */  beqz $at, .L8006463C
/* 800647F0 A4A90000 */  sh $t1, 0x0($a1)
/* 800647F4 1160FFD8 */  beqz $t3, .L80064758
/* 800647F8 20A50002 */  addi $a1, $a1, 0x2
/* 800647FC 316AFFFF */  andi $t2, $t3, 0xFFFF
/* 80064800 39417C1F */  xori $at, $t2, 0x7C1F
/* 80064804 1020000D */  beqz $at, .L8006483C
/* 80064808 3941FE00 */  xori $at, $t2, 0xFE00
/* 8006480C 1020FF8B */  beqz $at, .L8006463C
/* 80064810 A4AA0000 */  sh $t2, 0x0($a1)
/* 80064814 000B5402 */  srl $t2, $t3, 16
/* 80064818 1140FFCF */  beqz $t2, .L80064758
/* 8006481C 20A50002 */  addi $a1, $a1, 0x2
/* 80064820 39417C1F */  xori $at, $t2, 0x7C1F
/* 80064824 10200005 */  beqz $at, .L8006483C
/* 80064828 3941FE00 */  xori $at, $t2, 0xFE00
/* 8006482C 1020FF83 */  beqz $at, .L8006463C
/* 80064830 A4AA0000 */  sh $t2, 0x0($a1)
/* 80064834 0401FFC8 */  bgez $zero, .L80064758
/* 80064838 20A50002 */  addi $a1, $a1, 0x2
.L8006483C:
/* 8006483C 00024402 */  srl $t0, $v0, 16
/* 80064840 A4A80000 */  sh $t0, 0x0($a1)
/* 80064844 20A50002 */  addi $a1, $a1, 0x2
/* 80064848 94880000 */  lhu $t0, 0x0($a0)
/* 8006484C 20840002 */  addi $a0, $a0, 0x2
/* 80064850 00021400 */  sll $v0, $v0, 16
/* 80064854 00684004 */  sllv $t0, $t0, $v1
/* 80064858 0401FFBF */  bgez $zero, .L80064758
/* 8006485C 00481025 */  or $v0, $v0, $t0
.L80064860:
/* 80064860 3408FE00 */  ori $t0, $zero, 0xFE00
/* 80064864 20020040 */  addi $v0, $zero, 0x40
.L80064868:
/* 80064868 A4A80000 */  sh $t0, 0x0($a1)
/* 8006486C 20A50002 */  addi $a1, $a1, 0x2
/* 80064870 1440FFFD */  bnez $v0, .L80064868
/* 80064874 2042FFFF */  addi $v0, $v0, -0x1
/* 80064878 40096000 */  mfc0 $t1, $12
/* 8006487C 00000000 */  nop
/* 80064880 3C010002 */  lui $at, 0x2
/* 80064884 01214825 */  or $t1, $t1, $at
/* 80064888 40896000 */  mtc0 $t1, $12
/* 8006488C 03E00008 */  jr $ra
/* 80064890 00001020 */  add $v0, $zero, $zero
.L80064894:
/* 80064894 3C088006 */  lui $t0, %hi(D_800648C8)
/* 80064898 250848C8 */  addiu $t0, $t0, %lo(D_800648C8)
/* 8006489C AD040000 */  sw $a0, 0x0($t0)
/* 800648A0 AD050004 */  sw $a1, 0x4($t0)
/* 800648A4 AD020008 */  sw $v0, 0x8($t0)
/* 800648A8 AD03000C */  sw $v1, 0xC($t0)
/* 800648AC AD0C0010 */  sw $t4, 0x10($t0)
/* 800648B0 AD0D0014 */  sw $t5, 0x14($t0)
/* 800648B4 AD0F001C */  sw $t7, 0x1C($t0)
/* 800648B8 AD180020 */  sw $t8, 0x20($t0)
/* 800648BC AD190024 */  sw $t9, 0x24($t0)
/* 800648C0 03E00008 */  jr $ra
/* 800648C4 20020001 */  addi $v0, $zero, 0x1
endlabel MdecUnpackStatus
.globl D_800648C8
D_800648C8:
/* 800648C8 00000000 */  nop
/* 800648CC 00000000 */  nop
/* 800648D0 00000000 */  nop
/* 800648D4 00000000 */  nop
/* 800648D8 00000000 */  nop
/* 800648DC 00000000 */  nop
/* 800648E0 00000000 */  nop
/* 800648E4 00000000 */  nop
/* 800648E8 00000000 */  nop

