# 0.6.9-alpha — release candidate

Not published. Manual platform testing is still required.

- Ray tracing is disabled by default, including when its setting is absent.
  It remains available as an experimental option in the editor.
  Existing configurations explicitly enabling it retain that preference.
- The unified editor supports car parameters, mesh import/export and material
  editing. Source-only builds use original disc models and support mesh mods;
  optional authored replacements are not required.
- The packaged native MCP server supports live editor edits, validation,
  undo/redo and publishing an atomic active mod build. Preview cancellation
  no longer cancels unrelated editor operations.
- Material controls are shared by car and track studios. Preview cancellation
  and refresh races have regression coverage.

The track studio is not a complete replacement track authoring pipeline.
This candidate does not establish that the outstanding AMD ray tracing or
Windows second-race rendering reports are resolved.
- Custom replay now records and restores rivals as a race, rather than using
  the player-only Time Attack replay format; this has unit and menu-to-replay
  regression coverage in both renderers.
