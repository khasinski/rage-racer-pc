# Track texture regression

The playable class-5 Vainqure race reproduced incorrect road, cliff and
scenery colors. The fault involved two independent changes to texture data.

The course installer uploaded the deferred texture page before saving the
active page to the shadow buffer. Both pages share VRAM at 576,256 (448 by
256 words), so that order destroyed the first page. The installer now saves
it before the deferred upload, and publishes the shadow/cursor/generation
only after all uploads succeed.

The native terrain importer also applied the environment palette offset to
all six primitive modes. Retail modes 0/1 follow the environment; modes 2..5
have a fixed palette, including the encoded odd-mode offset. The importer
now retains that distinction in each material key and only offsets the
environment-controlled materials. Otherwise two faces with the same base
atlas and CLUT could be incorrectly deduplicated.

Verification on macOS:

- The image upload test runs the real installer and image decoder against
  an in-memory GPU, checking every word of both distinct texture pages.
  The preceding installer fails this regression test.
- The native terrain material test parses all six stream modes and checks
  both environment settings, both page variants and material identity.
- Seven focused loader, palette, material and track identity tests passed.
- Actual game captures compare the original reproduction at the class-5
  Mythical Coast start and the Over Pass City coastal section. A classic
  renderer capture supplies an independent palette reference.

Evidence is under `build/track-texture-evidence`:
[start comparison](../build/track-texture-evidence/start-comparison.png),
[coastal comparison](../build/track-texture-evidence/comparison.png),
`tests.log`, `negative-control.log`, and `transition.log`.

The fix uses the normal C loading/import path. No cache deletion, separate
asset command, or fallback renderer is needed by players.
