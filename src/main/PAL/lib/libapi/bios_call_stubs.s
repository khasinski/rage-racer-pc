.include "macro.inc"

.set noat
.set noreorder

/* PsyQ kernel call stubs.
 *
 * The BIOS is entered by jumping to a fixed address - 0xA0, 0xB0 or 0xC0 -
 * with the call number in $t1. Each stub loads the entry point into $t2,
 * jumps, and puts the call number in the delay slot, so the number is set at
 * the moment the jump is taken. C has no way to express either the register
 * protocol or the delay slot, so these are assembly in the original and stay
 * assembly here.
 *
 * The trailing nop after each stub is the original's padding to 16 bytes.
 */

glabel BiosExit                     /* B0(0x38) - exit */
    /* 80063D9C */  addiu      $t2, $zero, 0xB0
    /* 80063DA0 */  jr         $t2
    /* 80063DA4 */   addiu     $t1, $zero, 0x38
endlabel BiosExit
    /* 80063DA8 */  nop

glabel _card_info                   /* A0(0xAB) */
    /* 80063DAC */  addiu      $t2, $zero, 0xA0
    /* 80063DB0 */  jr         $t2
    /* 80063DB4 */   addiu     $t1, $zero, 0xAB
endlabel _card_info
    /* 80063DB8 */  nop

glabel _card_load                   /* A0(0xAC) */
    /* 80063DBC */  addiu      $t2, $zero, 0xA0
    /* 80063DC0 */  jr         $t2
    /* 80063DC4 */   addiu     $t1, $zero, 0xAC
endlabel _card_load
    /* 80063DC8 */  nop

glabel InitCARD                     /* B0(0x4A) */
    /* 80063DCC */  addiu      $t2, $zero, 0xB0
    /* 80063DD0 */  jr         $t2
    /* 80063DD4 */   addiu     $t1, $zero, 0x4A
endlabel InitCARD
    /* 80063DD8 */  nop

glabel StartCARD                    /* B0(0x4B) */
    /* 80063DDC */  addiu      $t2, $zero, 0xB0
    /* 80063DE0 */  jr         $t2
    /* 80063DE4 */   addiu     $t1, $zero, 0x4B
endlabel StartCARD
    /* 80063DE8 */  nop
