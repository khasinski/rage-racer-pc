.set noat
.set noreorder

/* Kernel entry points used by the interrupt plumbing above.
 *
 * The first four are BIOS call stubs: load the entry point into $t2, jump, and
 * set the call number in $t1 from the delay slot.
 *
 * The last two are the context save and restore the kernel hands its callback
 * a pointer to. They read and write $ra, $gp, $sp, $fp and $s0-$s7 directly,
 * which is the whole point of them and is not something a compiler can be
 * asked to do, so both are assembly in the original.
 */

glabel SysEnqIntRP                  /* A0(0x72) */
    /* 8006E644 */  addiu      $t2, $zero, 0xA0
    /* 8006E648 */  jr         $t2
    /* 8006E64C */   addiu     $t1, $zero, 0x72
endlabel SysEnqIntRP
    /* 8006E650 */  nop

glabel ReturnFromException          /* B0(0x17) */
    /* 8006E654 */  addiu      $t2, $zero, 0xB0
    /* 8006E658 */  jr         $t2
    /* 8006E65C */   addiu     $t1, $zero, 0x17
endlabel ReturnFromException
    /* 8006E660 */  nop

glabel ResetEntryInt                /* B0(0x18) */
    /* 8006E664 */  addiu      $t2, $zero, 0xB0
    /* 8006E668 */  jr         $t2
    /* 8006E66C */   addiu     $t1, $zero, 0x18
endlabel ResetEntryInt
    /* 8006E670 */  nop

glabel HookEntryInt                 /* B0(0x19) */
    /* 8006E674 */  addiu      $t2, $zero, 0xB0
    /* 8006E678 */  jr         $t2
    /* 8006E67C */   addiu     $t1, $zero, 0x19
endlabel HookEntryInt
    /* 8006E680 */  nop

/* Stores the callee-saved set into the 0x30-byte block at $a0 and returns 0. */
glabel SaveKernelRegisters
    /* 8006E684 */  sw         $ra, 0x0($a0)
    /* 8006E688 */  sw         $gp, 0x2C($a0)
    /* 8006E68C */  sw         $sp, 0x4($a0)
    /* 8006E690 */  sw         $fp, 0x8($a0)
    /* 8006E694 */  sw         $s0, 0xC($a0)
    /* 8006E698 */  sw         $s1, 0x10($a0)
    /* 8006E69C */  sw         $s2, 0x14($a0)
    /* 8006E6A0 */  sw         $s3, 0x18($a0)
    /* 8006E6A4 */  sw         $s4, 0x1C($a0)
    /* 8006E6A8 */  sw         $s5, 0x20($a0)
    /* 8006E6AC */  sw         $s6, 0x24($a0)
    /* 8006E6B0 */  sw         $s7, 0x28($a0)
    /* 8006E6B4 */  addu       $v0, $zero, $zero
    /* 8006E6B8 */  jr         $ra
    /* 8006E6BC */   nop
endlabel SaveKernelRegisters

/* The inverse, returning $a1 so the kernel callback can hand back a value.
   Reached only through the block SaveKernelRegisters filled in. */
.global RestoreKernelRegisters
RestoreKernelRegisters:
    /* 8006E6C0 */  lw         $ra, 0x0($a0)
    /* 8006E6C4 */  lw         $gp, 0x2C($a0)
    /* 8006E6C8 */  lw         $sp, 0x4($a0)
    /* 8006E6CC */  lw         $fp, 0x8($a0)
    /* 8006E6D0 */  lw         $s0, 0xC($a0)
    /* 8006E6D4 */  lw         $s1, 0x10($a0)
    /* 8006E6D8 */  lw         $s2, 0x14($a0)
    /* 8006E6DC */  lw         $s3, 0x18($a0)
    /* 8006E6E0 */  lw         $s4, 0x1C($a0)
    /* 8006E6E4 */  lw         $s5, 0x20($a0)
    /* 8006E6E8 */  lw         $s6, 0x24($a0)
    /* 8006E6EC */  lw         $s7, 0x28($a0)
    /* 8006E6F0 */  addu       $v0, $a1, $zero
    /* 8006E6F4 */  jr         $ra
    /* 8006E6F8 */   nop
    /* 8006E6FC */  nop
    /* 8006E700 */  nop
