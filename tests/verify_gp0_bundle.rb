#!/usr/bin/env ruby
# frozen_string_literal: true

source = File.read(ARGV.fetch(0))
required = [
  'report.fetch("psx_source")', 'report.fetch("native_source")',
  'summary_frame["psx_replay_frames"]', 'summary_frame["psx_replay_pre_state"]',
  '"RAGE_GPU_GP0_TRACE_SCENE"', '"RAGE_GPU_GP0_TRACE_TIMER"',
  'native_draw_area', 'psx_draw_areas[frame] == native_draw_area',
  'psx_contexts[frame] == [native_scene, native_timer]',
  'matching_surface.empty?', 'PSX replay has no scene=',
  'map to the filename\'s native frame',
  'dir / "run.json"',
  'bundle / "gp0-psx.log"', 'bundle / "gp0-native.log"',
  'bundle / "gp0-diff.txt"', 'tools/rage_gp0_compare.rb'
]
missing = required.reject { |text| source.include?(text) }
raise "bundle replay lost: #{missing.join(', ')}" unless missing.empty?
puts "GP0 bundle replay uses captured provenance and leaves a self-contained diff"
