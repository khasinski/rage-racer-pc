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

## Import staging

`ModFileClassify` owns the package-relative file policy in C: raw archive
indices 000..134, supported texture/mesh extensions, and the three metadata
filenames. It rejects absolute paths, traversal/empty components, backslashes,
controls and unsupported data types. Import, export and composition send their
inventories to `rage-mod-cli --check-files-stdin` before using them. The command
accepts a JSON array of paths (8 MiB, 10000 entries), validates the whole list,
and performs no filesystem writes. JavaScript still traverses directories,
rejects symlinks and enforces directory depth and package-size limits; the
compiled classifier is not a directory walker or a filesystem sandbox.

`ModFileDisposition` determines copy roles from the compiled file class and
semantic/legacy reference flags. Unreferenced textures and raw assets are global
overrides; referenced backing files stay provider-local. Metadata and unreferenced
meshes have no runtime copy role. A texture may supply both semantic and legacy
references without becoming a global filename override. Invalid reference kinds
are rejected. Composition uses these C results, so unrelated package metadata
or unused meshes cannot overwrite one another in the generated runtime folder.
Original library files and exports remain intact.

The CLI `--file-dispositions-stdin` accepts `[path,referenceBits]` pairs and
returns one role bitmask per file: global=1, semantic=2, legacy=4, omitted=0.
Only semantic/legacy bits are valid inputs; the tool validates the full batch
before output. Semantic/legacy reference discovery still lives in the launcher
and needs compiled migration; copy-role selection is not a complete claim graph.

Mod import inventories the selected directory, then uses
`rage-mod-cli --copy-snapshot-stdin` to copy the listed files into a new private
installation directory. Metadata, TOML, mesh references and legacy texture
indexes are validated against that copy, never against the original directory
after copying. Only successful validation publishes the profile entry. Failures,
cancellation and failed persistence remove the uncommitted private directory.

`ModFileSnapshotCopy` is the compiled streaming copier: exclusive destination
creation, 128 MiB per file, 1 GiB per batch, and byte comparison against a second
read of the same source handle. It removes its own failed output but never
overwrites an existing destination. The launcher owns whole-batch rollback.
The command accepts a bounded JSON array of `[source,target]` pairs over stdin
(8 MiB request, 10000 files); parent directories must already exist.

This freezes the bytes subsequently validated and installed. It is not an
adversarial filesystem sandbox or an instantaneous multi-file filesystem
snapshot: inventory and path/symlink checks still live in the launcher, and
external writers are not locked out.

Composition now freezes the selected profile descriptors and provider decisions,
copies installed packages into a private `mod-sources-*` tree with the same C
copier, and re-inspects those copies using the import validators. The dependency
graph, resource claims, conflict decisions and output read these staged files,
not cached UI manifests or live package directories. The original archive marker
is also staged when needed. Staging is removed after success or failure; only
the independently copied `active-mods-*` result survives a successful operation.
Refreshing descriptors for composition does not mutate the UI/profile cache.
Externally edited packages may therefore require reimport before their new
conflicts can be resolved in the UI. Editable JSON profile metadata remains
authoritative for package names/versions/requirements; TOML is read from staging.

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

### JSON package ingestion

Imports validate `rage-mod.json` through `rage-mod-cli --metadata PATH` and
`ModPackageParseJSON` before creating an installed mod. The C module owns the
format-1 policy and decoded fields: name, region, optional identity/details and
up to 32 requirements. It uses vendored yyjson 0.12.0 in strict JSON/UTF-8 mode;
its MIT license is included in launcher resources. The command returns the
original validated JSON; JavaScript only decodes the IPC response during import.

The 16 KiB file limit and UTF-16-unit field limits retain the launcher contract,
including Unicode and supplementary characters. Unknown root extensions are
accepted, unknown dependency fields rejected. Duplicate known fields, malformed
UTF-8 and unpaired surrogate escapes are rejected explicitly. A missing file
is an error, not an empty metadata object. The C output owns its strings and
failure clears it. CLI inspection does not modify the file. JSON profile edits
and export serialization still use launcher code; this is not yet a complete
migration of persistent profile handling or source snapshot ownership.

### Dependency graph

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

Remaining work includes compiled profile mutation/export and resource-claim
discovery, UI display of TOML requirements, source fingerprints and a runtime
provider stack. Staged files isolate validation from subsequent external edits,
but capture is not a point-in-time filesystem transaction or a hot-reload contract.

## Resource provider selection

`ModProviderResolve` is the compiled final-choice policy. A single candidate
needs no decision. Multiple candidates require an explicit winner and the
exact provider set recorded when the user chose it. Set ordering is irrelevant;
added/removed/replaced providers invalidate the decision. Empty or duplicate
IDs, excessive counts and winners outside the set fail without a fallback.
The API supports up to 128 candidates and returns an index into that list;
on failure it returns no index. It performs no I/O.

The launcher still discovers resource claims and renders its synchronous
conflict summary in JavaScript. That summary is advisory: composition uses
`rage-mod-cli --resolve-providers-stdin` before creating the output directory,
then selects the returned winners per resource. Changing the UI summary cannot
bypass this gate. Unrelated resources from losing packages remain selected.

The transport is a bounded NUL-delimited UTF-8 token stream (8 MiB, 262144
tokens), avoiding command-line length limits for large packs. Each group is
`--resource KEY`, repeated `--candidate ID`, optional `--choice ID`, and repeated
`--previous ID`. The CLI returns a JSON array of candidate indices only on
successful exit; any nonzero exit rejects the entire batch, including partial
stdout. `--resolve-providers` also accepts the same tokens as argv for small
diagnostic requests. Neither mode writes files. This does not yet replace
JavaScript resource discovery/copying or implement a runtime provider stack.

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
