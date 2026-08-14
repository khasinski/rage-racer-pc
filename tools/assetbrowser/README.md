# Rage Racer asset browser

Extracts every asset out of a Rage Racer disc and serves a web page that lists,
previews and 3D-renders them.

**Game data must never enter this repository.** The extractor writes to
`tools/decomp-wip/assets/` by default, which `.gitignore` already covers as a
whole (`tools/decomp-wip/`). The HTML page ships as source only; it loads a
manifest you generate locally.

## Usage

```sh
# extract everything, then serve the browser
python3 tools/assetbrowser/extract.py "/path/to/Rage Racer (Europe) (Track 01).bin" --serve

# faster passes while poking at one thing
python3 tools/assetbrowser/extract.py "<disc>" --only CAR_00,BIG1 --no-vram --no-raw
python3 tools/assetbrowser/extract.py "<disc>" -o /somewhere/else
```

The disc image is always an argument; nothing is hardcoded. Track 01 of a
MODE2/2352 rip is expected, but a cooked 2048-byte-per-sector `.iso` also works.

Flags: `-o/--out`, `--only` (indices or name substrings), `--serve [PORT]`,
`--exe` (re-derive the asset name table from a real `SCES_006.50`),
`--no-raw`, `--no-vram`, `--no-audio`, `--no-fmv`.

Output: `manifest.json`, `index.html`, `models/*.json`, `textures/*.png`,
`images/*_vram.png`, `audio/*.wav`, `fmv/*.mp4`, `raw/*.bin`. A full run takes a
few minutes and produces roughly 400 MB.

The 11 `RAGE.STR` movies are indexed here but decoded by **ffmpeg**, which is
the one external dependency and an optional one: without it on the `PATH` (or
with `--no-fmv`) the movies are still listed, just not playable. Everything else
is pure Python.

## Where the formats come from

Every layout here is read out of the decompilation, not sniffed from headers.
The file comments name the exact source. In short:

| thing | source |
|---|---|
| archive TOC | `LoadDiscArchiveIndex` |
| asset names | `g_AssetPaths` |
| model bank | `RegisterModelBank` / `SelectModelBank` in `asset/model_banks.c` |
| model faces | `SubmitModel` / `SubmitModelFaces` + `jtbl_8007DA14` |
| course objects | `RegisterCourseModels` / `SubmitCourseModel` + `jtbl_8007DA54` |
| terrain cells | `InstallTerrainCellData` / `SubmitTerrainCellFaces` + `jtbl_8007D9F4` |
| cell placement | `BuildVisibleCells` in `track/visible_cells.c` |
| texture window | the GP0 `0xE2` word each subdividing emitter puts in its packet |
| images | `UploadImageAsset` / `UploadImageBlock` in `asset/image_upload.c` |
| car texture page | `g_CarImageRect` = `{704, 0, 64, 256}` |
| pack sub-blocks | `LoadRaceAssets`, `LoadCarSelectAssets`, `LoadSelectBgmAssets` |
| VAB / VAG | `StartAudioSlotLoad` hands them to `SsVabOpenHeadSticky` |
| sample rate | tone `center`/`shift` vs the note `audio/audio.c` keys (0x3C) |
| FMV index | the STR frame headers in `RAGE.STR`; it has no table of contents |

`models.py` carries the byte-level tables; read its module docstring first.

## Coordinates

The three conventions worth knowing before reading a rendered frame:

* **Handedness.** PS1 camera space is (+X right, +Y down, +Z away); WebGL is
  (+X right, +Y up, +Z toward the viewer). The conversion is `(x, -y, -z)`.
  Negating only Y — which is what "PS1 +Y is down" seems to ask for — has
  determinant −1, so it renders every model as its own mirror image no matter
  where the camera is put. The quad winding flips with it: the PS1 order
  `v0 v1 v2 / v1 v3 v2` is clockwise once converted, so the renderer emits the
  reverse to keep face normals pointing out of the solid.
* **Two unit scales.** The 32×32 terrain grid steps 2048 units per cell in the
  units the camera and the track points use, but `BuildVisibleCells` shifts the
  cell translation left by 2 before it reaches the GTE, so cells sit **8192**
  apart in the units the vertex pool is stored in.
* **A VAG has no sample rate.** The SPU resamples, so a sample's rate is a
  property of the note it is keyed at. Key-on banks are always keyed at 0x3C by
  `audio/audio.c`; the sequenced bank has no single note and previews at its
  tones' centres. See `audio.py`'s docstring.
* **The texture window is not optional.** Terrain and course quads emitted by
  the subdividing paths carry a GP0 `0xE2` command in the four bytes their
  stride adds; the packet sets it, draws the quad and resets it. It makes UVs
  wrap inside a power-of-two tile, so a raw UV lookup reads a different texel
  for 95% of the corners on BIG1.

## Confidence

The parsers assert rather than assume: a model bank must tile its own extent, a
terrain cell's face list must end at exactly the byte the next cell's begins, no
face may index past its vertex pool. Those checks pass on all 135 assets of the
retail PAL disc, so a silent misparse would have to be self-consistent 135 times
over. Note what that buys and what it does not: they are checks on the *file*
layout. They say nothing about where a cell is placed in the world, and a build
in which every cell parsed perfectly still piled all 198 of them into a sixth of
the track's footprint.

What is *not* decoded, and is shown as raw bytes: `RES.DAT`, the SEQ sequence
blocks inside `SELBGM.BIN` and the car packs, and the track camera / environment
/ waypoint / event sub-blocks of a `.2ND` pack.
