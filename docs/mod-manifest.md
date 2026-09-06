# Semantic mod manifest

The game reads `mod.toml` from the configured mod directory. The shared C
parser is `src/render/mod_manifest.*`; tools should use it instead of creating
a second parser. This is a small TOML subset, not a general TOML implementation.

```toml
[mod]
schema_version = 1
id = "example-hd"

[textures]
"track.big1.terrain.material.3" = "textures/tunnel.png"
```

`schema_version` is an integer describing the file contract, not the release
number of an individual mod. Current support is version 1. Existing manifests
without the field retain version-1 behavior. Unsupported versions and duplicate
version declarations are rejected. An invalid or unreadable manifest disables
all overrides from that directory, including `raw/asset_NNN.bin` and legacy PNG
patches. The game logs the error and uses original content. An absent manifest
is allowed for legacy raw-only packs. Manifest input is limited to 2 MiB.

`mod_assets.c` owns the validated manifest and a copy of the configured path.
Both legacy loading and the modern renderer use that same selection. It is
initialized once per asset session; changing configuration or files on disk
does not hot-reload the manifest. Presentation/device restarts borrow the same
immutable manifest. Full `ModernAssetsShutdown` drops renderer views and then
calls idempotent `ModAssetsShutdown`; the next access reads a new selection.
Direct callers must retire every borrowed view before shutdown. This does not
remove asset bytes already installed into game state and is not a mid-race
hot-reload mechanism or a complete game-session reset.

Texture keys use lowercase semantic identifiers. Paths must be relative, use
forward slashes, and contain no empty, `.` or `..` components, drive prefix or
trailing slash. This is lexical validation, not a sandbox against symlinks.
Quoted values support escaped quotes and backslashes; paths themselves cannot
contain backslashes. Material overrides use the existing `[materials]` table
and validated `RenderMaterialParseProperties` string format.

Version 1 preserves existing behavior: repeated asset keys use the last value,
and unknown fields/sections are ignored. This is not full TOML duplicate-key
semantics. Requirement declarations are validated, but dependency resolution,
multiple active mods, conflict resolution and source identity remain incomplete.

The parser owns no external resources. Its output stores copies of all values;
lookup results borrow this output until it is parsed again or reset. Failure
clears all content and retains only `error` and `errorLine`.

## Required mods

The optional `[mod]` field `requires = ["base-pack", "shared-textures"]`
declares up to 16 unique mod IDs. The current parser accepts a single-line
array of quoted lowercase identifiers (letters, digits, dots and hyphens).
An omitted field or empty array declares no dependencies.

Currently the game selects only one mod directory. A nonempty requirement
list therefore disables that mod with an unmet-dependency diagnostic, before
raw assets or semantic overrides are installed. It does not search the disk,
download dependencies or partially activate the mod. The shared C
`ModManifestBuildOrder` accepts up to 16 already-parsed manifests and produces
dependency-first indices using stable depth-first traversal. Independent roots
are visited in input order; dependencies are visited in declaration order.
It rejects missing requirements, duplicate IDs and cycles without publishing a
partial order. Multiple selected manifests need nonempty IDs; a sole unnamed
legacy manifest remains valid. The loader uses this validator for its current
single selection. Multi-directory discovery/activation and asset-conflict
handling are not implemented yet.
