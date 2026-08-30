# Untangling host_state.c

`src/port/host_state.c` is the retail data segment transcribed into C. Every
subsystem's globals sit in one file, in the order the original build happened
to lay them out in memory, and that address order is the only organising
principle the file has.

## What is actually wrong with it

Measured, not guessed.

**The symbol boundaries follow addresses, not meaning.** An array's declared
size runs to the next address the retail build gave a name to, so it swallows
whatever sat in between. That is why the paths to all 135 assets live inside a
symbol called `g_MsgNegconMaxTwist`, under the string "Maximum twist.", and
why the names of the courses live inside `g_CaptionBestLapTime` under a
four-letter caption code. Sixteen arrays are runs of strings like this.

**The definitions do not have the types their headers promise.** 614 of the
740 raw byte arrays are declared in a header as something else: `s16
g_SinTable[]`, `Vec4 g_MainVisibleCellList[]`, `CdlFILE g_CdFileCache[64]`,
`TeamLogoCanvas g_TeamLogoCanvas`. Of those 614:

- **262** have a size the promised type divides cleanly. These are a pure type
  fix. `g_MainVisibleCellList[1024]` is `Vec4[64]`, and 64 is the visible cell
  count the renderer already uses.
- **243** have a size the type does not fit, which is the boundary problem
  again: a scalar with a gap stuck to it. `g_TimeAttackPlateProgress` is
  declared `volatile s32` and defined as 7252 bytes.
- **169** promise a struct rather than a scalar.

**450 of the 1212 symbols are read by no `.c` file at all**, 107 KB of them.
Counting `extern` declarations in headers as uses hides this; they are not
uses. Some of the 450 are residue of the kind above, some are the six large
buffers (`g_LoadBuffer` alone is 1 MB), and some may be things that ought to
be read and are not. They are not all dead and must not be deleted wholesale.

## What makes the work safe

`tests/port/host_state_content_tests.c` folds the bytes of the 507 initialised
arrays, in a fixed order, and folds **only the bytes, never the names or the
sizes**. Cutting an array into two named halves, moving a definition to
another file, or giving it the type its header promises all leave the same
bytes in the same order, so the number must not move. Flipping a single byte
fails it.

That property is the whole point, and it is what `host_state_abi` cannot do:
the manifest pins names and sizes, not contents.

## Order of work, and why this order

1. **The content lock.** Done. Nothing after it is verifiable without it.
2. **Messages spelled as text.** Done, 67 arrays.
3. **String runs written out readably.** Done, 16 arrays, including the asset
   paths and the course names.
4. **Split the file by the subsystem that reads each symbol.** Done. Ownership
   was worked out by counting which `.c` files read each symbol, then adjusted
   where the count and the subject disagreed. The result is one
   `host_state_<area>.c` per subsystem: menu 262, race 138, track 102, save 64,
   car 60, audio 39, asset 25, render 24, cd 23, pad 18, and 471 in
   `host_state_unread.c`. `host_state.c` keeps the 17 that belong to nobody:
   the frame counter, the scene, the mirror, the random seed.

   The counting put the CD driver's state in a bucket with the boot loop's
   clock; the drive is its own subsystem and got its own file. Fifteen symbols
   went somewhere other than where the count pointed. Ten of those are cases
   where a port shim reads a thing it does not own, and the terrain cells, the
   visible cell lists, the course objects and the scenery variants belong to
   the track no matter who copies them across. The other five are near ties
   settled by what the symbol is: the tachometer needle's quad with the code
   that draws the needle, the controller scene's second angle with its first,
   the mono output flag with the audio it configures.
5. **Give the 262 clean cases the type their header promises.** Byte identical,
   and it turns tables of hexadecimal into tables of values.
6. **The 169 struct cases**, same idea, one struct at a time.
7. **The 243 with residue attached.** Each is a real object plus a gap. Cut the
   gap off and name it for what it is, or establish that nothing reads it.
8. **The 471 nobody reads**, now gathered in `host_state_unread.c` with the
   three kinds they fall into written at the top. Decide per symbol: residue,
   buffer, or something that should have a reader.

Readability first would have meant prettifying data sitting in the wrong
symbol. Splitting the file first would have scattered the fused arrays across
files and required gluing them back together. Hence locks, then text, then the
split, then types.
