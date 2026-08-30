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
4. **Split the file by the subsystem that reads each symbol.** In progress.
   Ownership was worked out by counting which `.c` files read each symbol:
   menu 257, race 137, track 92, save 63, car 61, audio 38, port 27, other 24,
   asset 23, render 23, pad 17, unread 450.
5. **Give the 262 clean cases the type their header promises.** Byte identical,
   and it turns tables of hexadecimal into tables of values.
6. **The 169 struct cases**, same idea, one struct at a time.
7. **The 243 with residue attached.** Each is a real object plus a gap. Cut the
   gap off and name it for what it is, or establish that nothing reads it.
8. **The 450 nobody reads.** Decide per symbol: residue, buffer, or something
   that should have a reader.

Readability first would have meant prettifying data sitting in the wrong
symbol. Splitting the file first would have scattered the fused arrays across
files and required gluing them back together. Hence locks, then text, then the
split, then types.
