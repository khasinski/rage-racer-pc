# Rage Racer PC 0.6.14-alpha

This alpha adds an optional HD texture downloader, a refreshed main menu with
mouse support, and smoother first-lap loading of HD assets.

## Changes

- Open **Escape → Mods → HD texture pack → Open downloader** to download and
  install the optional **Faithful Refresh 0.1.0-preview.1** pack. The dedicated
  screen shows progress and download size; Cancel is available during transfer.
  Installed textures can be enabled/disabled, verified or installed from a local
  `.rrhd` archive.
- The 54.8 MB HD preview uses lossless WebP with exactly preserved RGBA pixels,
  down from a 164.1 MB PNG archive. Downloads are verified by SHA-256 and per-file
  integrity checks before activation. A failed/cancelled download keeps the
  previous pack. Each HD version has an immutable URL.
- The preview refreshes selected road, cliff, sky, car, HUD and menu textures.
  Esperanza has a newly authored body atlas and more matte paint. Rank digits
  fit their HUD cells. Original textures remain on surfaces outside the preview.
- Texture banks, mipmaps and resident geometry are prepared before their first
  presentation, reducing first-use stalls at the opening corners. Shared GPU
  textures and cached HUD lookups reduce duplicate uploads and per-frame work.
- The main menu uses native text and supports mouse and keyboard navigation.
  Extra Grand Prix is hidden while locked. Route previews, transitions and
  viewport sizing have been corrected.
- Optional random time of day is available under Mods for GP / Extra GP races.
- Includes the current upstream fixes for the 60 Hz race-tick switch, race-start
  state and scaled sky-atlas reads.

## Install and play

Extract the archive for your platform and open the game. On first launch, choose
only your legally obtained Rage Racer CUE or Track 01 BIN; native renderer assets
are generated automatically. The optional HD pack is downloaded from the in-game
Mods page. No separate asset-conversion command or image-codec installation is
required. The editor is a separate optional download.

HD preview: [0.1.0-preview.1](https://rage-hd.polanka.ovh/hd-PAL-v0.1.0-preview.1.rrhd)
(54,816,675 bytes, SHA-256
`39c207c17d802f1f52a30fc24d76a1789fa0731ef355ba46cd98ad2bc9cf98ca`).

## Preview limits

The HD pack currently targets PAL game data and selected artwork rather than
all cars or every texture on every route. Original textures remain where no
replacement is supplied. This is an alpha game release and an optional HD art
preview; it does not certify that all previously reported visual glitches are
resolved.
