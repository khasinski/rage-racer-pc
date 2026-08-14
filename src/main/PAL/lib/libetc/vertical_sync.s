.set noat
.set noreorder

/* Two BIOS calls: load the table entry point into $t2, the call number into
   $t1, and jump. Neither can be spelled in C, so they stay assembly - and
   spelled this way each one carries its own exported name, which a u_long[]
   in .text could not. */

glabel ChangeClearRCnt
    /* 240A00B0 */  addiu      $t2, $zero, 0xB0
    /* 01400008 */  jr         $t2
    /* 2409005B */   addiu     $t1, $zero, 0x5B
    /* 00000000 */  nop
endlabel ChangeClearRCnt

glabel ChangeClearInterruptMask
    /* 240A00C0 */  addiu      $t2, $zero, 0xC0
    /* 01400008 */  jr         $t2
    /* 2409000A */   addiu     $t1, $zero, 0xA
    /* 00000000 */  nop
endlabel ChangeClearInterruptMask
