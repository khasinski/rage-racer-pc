.include "macro.inc"

.set noat
.set noreorder

/* Leaves the kernel's critical section. The kernel is entered with a `syscall`
   instruction, which has no C spelling, so this one stays assembly. */
glabel ExitCriticalSection
    /* 24040002 */  addiu      $a0, $zero, 0x2
    /* 0000000C */  syscall    0
    /* 03E00008 */  jr         $ra
    /* 00000000 */   nop
endlabel ExitCriticalSection
