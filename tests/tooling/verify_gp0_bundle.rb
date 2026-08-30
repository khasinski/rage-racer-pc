#!/usr/bin/env ruby
# frozen_string_literal: true

source = File.read(ARGV.fetch(0))

abort "GP0 bundle lacks a pixel-only replay mode" unless
  source.include?("--trace-only") &&
  source.include?('execute_replays.call("") unless options[:trace_only] || options[:raster_only]')
abort "GP0 bundle lacks an artifact-only raster iteration mode" unless
  source.include?("--raster-only") &&
  source.include?('options[:trace_only] || options[:raster_only]')
required = [
  'report.fetch("psx_source")', 'report.fetch("native_source")',
  'summary_frame["psx_replay_frames"]', 'summary_frame["psx_replay_pre_state"]',
  '"RAGE_GPU_GP0_TRACE_SCENE"', '"RAGE_GPU_GP0_TRACE_TIMER"',
  '"RAGE_GPU_VRAM_TRANSFER_TRACE"',
  'native_draw_area', 'psx_draw_areas[frame] == native_draw_area',
  'psx_contexts[frame] == [native_scene, native_timer]',
  'matching_surface.empty? && available_frames.length > 1',
  'PSX replay has no scene=',
  'map to the filename\'s native frame',
  'dir / "run.json"',
  'bundle / "gp0-psx.log"', 'bundle / "gp0-native.log"',
  'bundle / "gp0-diff.txt"', 'tools/rage_gp0_compare.rb',
  '"--pixel X,Y"', '"RAGE_GPU_TRACE_PIXEL"', '"RAGE_GPU_TRACE_FRAME"',
  '"RAGE_GPU_TRACE_TEXEL"', '"RAGE_GPU_OT_TRACE"', '"--resync-window"',
  '"RAGE_GPU_GP0_TRACE_VRAM"', 'bundle / "gp0-pre.vram"',
  '"RAGE_GPU_GP0_TRACE_VRAM_POST"', 'bundle / "gp0-post.vram"',
  'bundle / "gp0-native-post.vram"', 'expected_vram_bytes = 1024 * 512 * 2',
  'tools/rage_vram_refs.rb', 'bundle / "gp0-vram-refs.txt"',
  'tools/rage_gp0_raster_compare.rb', 'bundle / "gp0-raster"',
  'bundle / "gp0-raster.txt"',
  '"--dump-psx-packet N"', '"RAGE_GPU_GP0_TRACE_DUMP_PACKET"',
  '"RAGE_GPU_GP0_TRACE_DUMP_VRAM"',
  'execute_replays.call("pixel"',
  'argv[output_index + 1] = (frame + 2).to_s',
  'env["RAGE_PORT_SMOKE_FRAMES"] = (frame + 2).to_s',
  'env.delete("RAGE_EMU_STOP_SCENE_TIMER")'
]
missing = required.reject { |text| source.include?(text) }
raise "bundle replay lost: #{missing.join(', ')}" unless missing.empty?
raise "PSX command-frame correction is limited to checkpoint replays" if
  source.match?(/if psx_replay_frames && psx_replay_state\s+native_draw_area/m)
raise "bundle cannot parse synchronized terminal PSX captures" unless
  source.include?('basename.match(/\\Async-(\\d+)-t\\d+-s\\d+\\.ppm\\z/)')
raise "bundle cannot parse final-DMA timer captures" unless
  source.include?('basename.match(/-f(\\d+)(?:-d\\d+)?-s\\d+\\.ppm\\z/)')
puts "GP0 bundle replay uses captured provenance and leaves a self-contained diff"
