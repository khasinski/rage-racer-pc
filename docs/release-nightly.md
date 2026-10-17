# Rage Racer PC — nightly

Nightly pre-release of the **Rage Racer PC** port and the standalone **Rage Racer Editor**.

## Downloads

- **Standalone Game (`Rage-Racer-nightly-*`)**: Double-click to launch. On first run, select your legally obtained Rage Racer `CUE` or Track 01 `BIN` image; required native assets are generated automatically and the game starts in the modern renderer.
- **Standalone Editor (`Rage-Racer-Editor-nightly-*`)**: Unified desktop application bundling the native game, asset tools, **Car Studio**, **Track Studio**, **Save Editor**, and **Mod Manager**.

## Highlights

- **Standalone Rage Racer Editor**:
  - **Car Studio**: Interactive 3D car preview, body/wheel/glass mesh import & export (`.obj` / `.glb`), wheel well and hub alignment adjustments, performance & handling tuning (`cars.toml`), and material editing.
  - **Track Studio**: 2D and 3D course viewport with **Drive View** (driver's-eye camera with per-section track texture pages, road banking, elevation profile editing, and depth-biased surface rendering), drivable route deformation, terrain unit brush, AI racing line editing, and course scenery placement.
  - **Save Editor & Mod Manager**: Create and edit memory card saves across PAL, NTSC-U, and NTSC-J regions, manage mod packages and conflict priorities, and build active mods directly into the game profile.
  - **Native Studio MCP Server**: Bundled `rage-studio-mcp` for live editor automation, validation, undo/redo, and atomic mod builds.
- **Renderer & Replay Improvements**:
  - Hybrid ray tracing is now opt-in (`ray_tracing = off` by default in `rage-port.ini`, configurable in the editor).
  - Custom Race replays now record and restore full rival grids across both classic and modern renderers.
  - Source-only builds support original disc car geometry and mesh mods without requiring embedded authored car assets.
