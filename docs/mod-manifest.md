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

`[meshes]` maps semantic car-part IDs to relative `meshes/*.rmesh` paths.
Mesh, texture and material lookups all use `ModManifestResolve`: channels are
independent, exact variant precedes the base key, and the last assignment wins
within a key. A mesh-only lookup does not select texture/material entries.
Invalid schema, parser errors and excessive counts in any asset table reject
the whole resolution, including mesh lookups and dependency planning.

The launcher composes enabled packages into one runtime directory after the
compiled selection check below and its existing resource-conflict checks.
JSON package metadata and TOML retain distinct identity namespaces; they now
share one dependency graph rather than silently dropping TOML requirements.

Version 1 preserves existing behavior: repeated asset keys use the last value,
and unknown fields/sections are ignored. This is not full TOML duplicate-key
semantics. Requirement declarations are validated, but dependency resolution,
multiple active mods, conflict resolution and source identity remain incomplete.

The parser owns no external resources. Its output stores copies of all values;
lookup results borrow this output until it is parsed again or reset. Failure
clears all content and retains only `error` and `errorLine`.

## Native material editing

The launcher delegates edits to
`rage-mod-cli --set-material INPUT OUTPUT KEY PROPERTIES`. An empty INPUT
argument explicitly creates a manifest for a raw-only mod; a missing nonempty
input is an error. OUTPUT must be a new staging file, never the installed file.
The C tool validates the source and edited document with the runtime parser,
preserves the original bytes (including identity, schema, requirements,
comments and unknown extensions), and appends the overriding material entry.
Normal manifest size, line and table limits apply; reaching a limit rejects
the edit without modifying the source. The launcher retains responsibility
for transactional installation and rollback after persistence failure.

CLI inspection exposes `schemaVersion` and TOML `requires` alongside the
asset tables. These fields must not be confused with the launcher's JSON
package metadata. Material editing preserves them; package composition
validates them by rereading installed manifests, not just cached UI metadata.

## Package selection boundary

`ModSelectionBuildOrder` is the common C graph engine. Runtime manifest
ordering and `rage-mod-cli --check-selection` use it. The launcher calls the
CLI before enablement, removal, version edits and composition; successful
composition follows the returned dependency-first indices. Explicit resource
conflict choices still determine the winning provider, independently of order.

Each package contributes its JSON package ID (or installation ID), version,
region and JSON requirements, plus the ID and requirements parsed from its
installed TOML. JSON requirements match package IDs and optional exact versions;
TOML requirements match TOML IDs. Both require the same region. A string in
one namespace never implicitly supplies the other. Missing or multiple matching
providers and cycles (including mixed JSON/TOML cycles) fail before an output
directory is created. Unrelated legacy packs may share a generic TOML ID;
referencing that ID is rejected if more than one active provider matches.

The CLI accepts up to 128 packages and 48 combined requirements per package
(JSON ingress currently allows 32; TOML allows 16). Its argument protocol is
`--mod PACKAGE VERSION REGION MANIFEST_PATH`, followed by any number of
`--requires PACKAGE VERSION` arguments for that package. An empty manifest
path explicitly denotes a raw-only package; a missing nonempty path fails.
An empty dependency version accepts any version. Output is a JSON array of
zero-based package indices. No files or runtime state are mutated by validation.

Remaining work includes compiled JSON metadata ingestion and resource-conflict
selection, UI display of TOML requirements, source fingerprints and a runtime
provider stack. Validation and copying are not yet one immutable snapshot of
the source files; this is not a concurrent-edit/hot-reload contract.

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
