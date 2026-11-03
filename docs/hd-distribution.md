# HD pack distribution

Escape → Mods provides **HD textures**, **HD texture pack → Open downloader**,
local file installation and file verification. The separate downloader screen
shows the transfer size, percentage, progress bar and installation/verification
phase. **Cancel** is available there during downloading, with keyboard and mouse
support; it is absent from the ordinary Mods settings. Completed or failed tasks
return through **Back to Mods**. Installation applies the new pack without a game
restart. Checking the same archive verifies it without downloading it again;
an intentionally disabled pack stays disabled.

The default origin is `https://rage-hd.polanka.ovh`, also used if `[hd] server`
is omitted. An explicit `http://127.0.0.1:3000` override supports local development.
There are no background network requests: the player opens the downloader.
Online downloads are desktop-only; local pack installation also remains available.

## Current preview

The optimized `0.1.0-preview.1` pack contains the same RGBA pixels as refresh7.236.
328 referenced source image files become 321 unique images. Image payload falls
from 163,555,078 to 54,073,984 bytes; the complete archive is 54,816,675 bytes.
This is an optimization of the current preview, not completion of the 0.1 art scope.
The source PNG authoring folder is retained.

WebP is encoded losslessly with exact transparent RGB. Every encoded image is
decoded and compared across all four channels before packaging. If an original
file is smaller, it is retained. Identical RGBA images with identical dimensions
share one file; all semantic material bindings and properties are preserved.
The game supports mixed PNG/static WebP packs, with bounds checks before decoded
allocation. Animated WebP is rejected. GPU texture sizes and mipmaps remain the
same; this reduces disk/download size rather than GPU memory.

The decoder is statically linked libwebp 1.6.0, with a pinned source checksum.
The encoder is only linked into the authoring tool. Desktop HTTPS uses libcurl:
the macOS system library, or a static build using Windows Schannel/Linux OpenSSL.
Players do not install image codecs or run import commands for an HD pack.
Dependency notices accompany release documentation.

## Author and publish

Build `rage-hd-optimize`, `rage-hd-publish` and `rage-hd-server` with the normal
compiled C toolchain. No Python helper is used.

```sh
build/release/rage-hd-optimize SOURCE_PACK NEW_PACK 0.1.0-preview.1 NEW_ARCHIVE.rrhd
build/release/rage-hd-publish NEW_PACK RELEASE_DIRECTORY http://127.0.0.1:3000
build/release/rage-hd-server RELEASE_DIRECTORY 3000
```

The optimizer requires new output paths and preserves the source. Publication
creates and validates a complete archive, refuses an existing version URL, and
updates the small catalog after the immutable release is ready. Use one publishing
process at a time. Repeat publishing to a separate release directory with the
public HTTPS origin when preparing deployment; the archive bytes are unchanged.

The deployed origin is **https://rage-hd.polanka.ovh** (CapRover app `rage-hd`).
Release files live at `/home/ubuntu/projects/rage-hd-server/releases` on ragnarok.
Version `0.1.0-preview.1` is published at
`https://rage-hd.polanka.ovh/hd-PAL-v0.1.0-preview.1.rrhd`, SHA-256
`39c207c17d802f1f52a30fc24d76a1789fa0731ef355ba46cd98ad2bc9cf98ca`.
Keep this URL immutable; publish the next change under a new version and update
only `catalog-PAL.json`. The existing `rageracer` and `pe` apps are unchanged.

The origin serves:

- `/api/mods/hd/PAL/latest` (and the corresponding NTSC-U/NTSC-J paths when published)
- `/hd-PAL-v0.1.0-preview.1.rrhd` (versioned, immutable)
- `/healthz`

The catalog contains `version`, `region`, `format: "rrhd-webp-v1"`, `url`,
`sha256` and exact `size`. Only PAL artwork is published in this preview.
Archives support HEAD, byte ranges and ETags. Catalogs are not cached as immutable.
Interrupted client downloads currently restart; partial downloads are removed.
The client accepts HTTPS origins, with explicit loopback HTTP for local tests,
does not follow redirects and only downloads the exact same-origin release path.

The archive's size and SHA-256 are checked before installation. Installation
validates every record, image, manifest reference and region in a staging folder.
The active pointer changes only after validation succeeds. Failed/cancelled
downloads preserve the installed pack. Old versions are retained locally.

`rage-hd-fetch SERVER_ORIGIN STORE REGION` runs the same downloader as the menu
for administration and integration checks; players use the menu.

## Server deployment

The C origin runs on Linux/macOS. The standalone `server/CMakeLists.txt` requires
SDL3 development files. `docker build server` builds the supplied container;
mount the release directory read-only at `/releases` and put it behind the
deployment's HTTPS reverse proxy. The container listens on port 3000 as an
unprivileged user. The ordinary executable binds loopback unless `HD_BIND` is set.
The origin stores no analytics or player data. The Docker image has been built and checked on Linux, then deployed behind
CapRover with managed TLS and forced HTTPS. A fresh store was downloaded and
verified through the game's downloader against the public origin. Windows/Linux
client release validation remains separate from these checks.

## Validation

Compiled tests cover exact WebP pixels/alpha, dimension and truncation limits,
HTTP catalog/HEAD/ranges, download/install, checksum failures, cancellation,
already-current releases, region mismatch, immutable publishing, preservation of
the previous pack, and menu action mapping. A hidden/silent GPU run with the
optimized full pack produced the same captured-frame SHA-256 as the PNG pack:
`6b75958db45536262abcdc8bf2c2f6bf3677325f7cf6048f287e78260840447f`.

The design follows the local Parasite Eve client and pe-mod-server workflow.
Codec references: [WebP API](https://developers.google.com/speed/webp/docs/api)
and [libcurl protocol restrictions](https://curl.se/libcurl/c/CURLOPT_PROTOCOLS_STR.html).
