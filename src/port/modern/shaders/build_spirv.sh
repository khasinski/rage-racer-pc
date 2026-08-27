#!/usr/bin/env sh
set -eu

shader_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
glslang=${GLSLANG_VALIDATOR:-glslangValidator}
spirv_val=${SPIRV_VAL:-spirv-val}

build_shader() {
    source_name=$1
    symbol_name=$2
    stage=$3
    output="$shader_dir/${symbol_name}_spv.h"
    temporary=$(mktemp "${TMPDIR:-/tmp}/rage-${symbol_name}.XXXXXX.spv")
    trap 'rm -f "$temporary"' EXIT HUP INT TERM

    "$glslang" -V --target-env vulkan1.0 -S "$stage" \
        "$shader_dir/$source_name.glsl" -o "$temporary"
    if command -v "$spirv_val" >/dev/null 2>&1; then
        "$spirv_val" --target-env vulkan1.0 "$temporary"
    fi
    xxd -i -n "${symbol_name}_spv" "$temporary" > "$output"
    rm -f "$temporary"
    trap - EXIT HUP INT TERM
}

build_shader modern.vert modern_vert vert
build_shader modern.frag modern_frag frag
build_shader post.vert post_vert vert
build_shader post.frag post_frag frag
build_shader native.vert native_vert vert
build_shader native_sky.vert native_sky_vert vert
build_shader native_sky.frag native_sky_frag frag
build_shader native_shadow.vert native_shadow_vert vert
build_shader native_shadow.frag native_shadow_frag frag
build_shader native_shadow_masked.frag native_shadow_masked_frag frag
build_shader native_texture.frag native_texture_frag frag
build_shader native_color.frag native_color_frag frag
