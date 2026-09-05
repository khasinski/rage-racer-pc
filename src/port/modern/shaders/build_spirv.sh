#!/usr/bin/env sh
#
# Every shader has one source, the GLSL beside this script. Vulkan takes the
# SPIR-V compiled from it; Metal takes MSL translated from that same SPIR-V.
# Both are generated here and checked in, so a build needs no shader compiler,
# and neither can drift from the other or from the GLSL they came from.
#
set -eu

shader_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
glslang=${GLSLANG_VALIDATOR:-glslangValidator}
spirv_val=${SPIRV_VAL:-spirv-val}
spirv_cross=${SPIRV_CROSS:-spirv-cross}

build_shader() {
    source_name=$1
    symbol_name=$2
    stage=$3
    entry_point=$4
    spirv_output="$shader_dir/${symbol_name}_spv.h"
    msl_output="$shader_dir/${symbol_name}_msl.h"
    temporary=$(mktemp "${TMPDIR:-/tmp}/rage-${symbol_name}.XXXXXX.spv")
    metal_temporary="$temporary.metal"
    trap 'rm -f "$temporary" "$metal_temporary"' EXIT HUP INT TERM

    "$glslang" -V --target-env vulkan1.0 -S "$stage" \
        "$shader_dir/$source_name.glsl" -o "$temporary"
    if command -v "$spirv_val" >/dev/null 2>&1; then
        "$spirv_val" --target-env vulkan1.0 "$temporary"
    fi
    xxd -i -n "${symbol_name}_spv" "$temporary" > "$spirv_output"

    #
    # The binding numbers are carried across rather than reassigned, so a
    # texture the GLSL puts at binding 0 is [[texture(0)]] in Metal, which is
    # what the renderer asks SDL for. The entry point is renamed because SDL
    # is given one name for both formats.
    #
    "$spirv_cross" --msl --msl-version 20000 --msl-decoration-binding \
        --rename-entry-point main "$entry_point" "$stage" \
        "$temporary" > "$metal_temporary"
    # SDL is handed the source as a C string, so terminate it.
    printf '\0' >> "$metal_temporary"
    xxd -i -n "${symbol_name}_msl" "$metal_temporary" > "$msl_output"

    rm -f "$temporary" "$metal_temporary"
    trap - EXIT HUP INT TERM
}

if [ "${1:-}" = "composite" ]; then
    build_shader composite.frag composite_frag frag fs_composite
    exit 0
fi

build_shader modern.vert modern_vert vert vs_main
build_shader modern.frag modern_frag frag fs_main
build_shader post.vert post_vert vert vs_post
build_shader post.frag post_frag frag fs_post
build_shader composite.frag composite_frag frag fs_composite
build_shader native.vert native_vert vert vs_native
build_shader native_sky.vert native_sky_vert vert vs_native_sky
build_shader native_sky.frag native_sky_frag frag fs_native_sky
build_shader native_shadow.vert native_shadow_vert vert vs_shadow
build_shader native_shadow.frag native_shadow_frag frag fs_shadow
build_shader native_shadow_masked.frag native_shadow_masked_frag frag fs_shadow_masked
build_shader native_texture.frag native_texture_frag frag fs_native
build_shader native_color.frag native_color_frag frag fs_native_color
