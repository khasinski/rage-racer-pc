.set noat
.set noreorder

/* Exception entry, reached by the hardware rather than called.
 *
 * $k0 and $k1 are the two registers the kernel reserves for exactly this, and
 * the frame it walks to sits at a fixed address. It saves the caller's $at,
 * $v0, $v1 and $ra into that frame and reads the CAUSE register, all of which
 * has to happen before anything a compiler emits would be safe. Assembly in
 * the original, and it stays that way.
 */
.global func_8006A018
func_8006A018:
    /* 8006A018 */  addiu      $k0, $zero, 0x100
    /* 8006A01C */  lw         $k0, 0x8($k0)
    /* 8006A020 */  nop
    /* 8006A024 */  lw         $k0, 0x0($k0)
    /* 8006A028 */  nop
    /* 8006A02C */  addi       $k0, $k0, 0x8
    /* 8006A030 */  sw         $at, 0x4($k0)
    /* 8006A034 */  sw         $v0, 0x8($k0)
    /* 8006A038 */  sw         $v1, 0xC($k0)
    /* 8006A03C */  sw         $ra, 0x7C($k0)
    /* 8006A040 */  mfc0       $v0, $13
    /* 8006A044 */  nop

glabel func_8006A048                /* A0(0x44) */
    /* 8006A048 */  addiu      $t2, $zero, 0xA0
    /* 8006A04C */  jr         $t2
    /* 8006A050 */   addiu     $t1, $zero, 0x44
endlabel func_8006A048
    /* 8006A054 */  nop
