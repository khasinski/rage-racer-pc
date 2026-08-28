#!/usr/bin/env python3
"""Keep simulation and modern scene code outside the PSY-Z boundary."""

from pathlib import Path
import sys


def main() -> int:
    source = Path(sys.argv[1])
    cmake = (source / "CMakeLists.txt").read_text(encoding="utf-8")
    sim_block = cmake.split("add_library(rage-sim STATIC", 1)[1].split(")", 1)[0]
    if "psyz" in sim_block.lower():
        raise AssertionError("rage-sim directly names PSY-Z")

    # Legacy PS1 assumptions are deliberately local to the retail game, its
    # state mirror, and the small platform adapter.  A global relaxation would
    # silently weaken new renderer and mod code as it is added.
    if "add_compile_options(-fno-strict-aliasing -fwrapv)" in cmake:
        raise AssertionError("legacy compatibility flags are global")
    required_compat_targets = (
        "rage-game-full",
        "rage-host-state",
        "rage-port-legacy",
    )
    for target in required_compat_targets:
        needle = (
            f"target_compile_options({target} PRIVATE "
            "${RAGE_COMPAT_COMPILE_OPTIONS})"
        )
        if needle not in cmake:
            raise AssertionError(f"legacy compatibility flags missing from {target}")

    forbidden = ("<psyz/", '"psyz/', "<psyq/", '"psyq/')
    roots = [source / "src/render"]
    files = list((source / "src/render").glob("*.[ch]"))
    files += [source / path.strip().replace("${CMAKE_CURRENT_SOURCE_DIR}/", "")
              for path in cmake.split("set(RAGE_SIM_SOURCES", 1)[1]
                               .split(")", 1)[0].splitlines()
              if path.strip()]
    for path in files:
        text = path.read_text(encoding="utf-8")
        if any(token in text for token in forbidden):
            raise AssertionError(f"renderer-neutral code imports PSY-Z: {path}")
    if not roots[0].is_dir():
        raise AssertionError("Render World source directory is missing")
    classic_geometry = source / "src/port/classic/native_geometry.c"
    if not classic_geometry.is_file():
        raise AssertionError("classic GTE geometry is outside its renderer module")
    modern_sources = (source / "src/port/modern").glob("*.[ch]")
    if any("classic/" in path.read_text(encoding="utf-8")
           for path in modern_sources):
        raise AssertionError("modern renderer imports the classic renderer")
    modern = (source / "src/port/modern/modern_renderer.c").read_text(
        encoding="utf-8")
    renderer_config = (source / "src/port/port_config.h").read_text(
        encoding="utf-8")
    if renderer_config.count("RAGE_RENDERER_") != 2:
        raise AssertionError("the public runtime must expose exactly two renderers")
    if ("RAGE_RENDERER_CLASSIC = 0" not in renderer_config or
            "RAGE_RENDERER_MODERN = 1" not in renderer_config):
        raise AssertionError("classic/modern renderer identities changed")
    forbidden_modern_3d = (
        "ModernNativeGpuCanReplaceWorld",
        "ModernRenderLegacySelection",
        "MODERN_PIPE_3D",
        "ModernBuildFaceVertices",
        "RageCaptureFace",
    )
    for token in forbidden_modern_3d:
        if token in modern:
            raise AssertionError(
                f"modern renderer still contains captured PS1 3D path: {token}")
    if "ModernBuildOverlayFrame" not in modern:
        raise AssertionError("modern renderer has no explicit 2D overlay boundary")
    if "legacy 3D fallback is disabled" not in modern:
        raise AssertionError("incomplete native worlds are not diagnosed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
