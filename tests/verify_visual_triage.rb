#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "fileutils"
require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
abort "triage rejects a complete canonical report after expected GP0 stream divergence" unless
  File.read(tool).include?("status.success? || canonical_path.file?")
Dir.mktmpdir("rage-visual-triage-") do |run|
  profile = File.join(run, "compare", "mirror-road")
  bundle = File.join(profile, "candidate")
  renderer_bundle = File.join(profile, "renderer-candidate")
  raster = File.join(bundle, "gp0-raster")
  renderer_raster = File.join(renderer_bundle, "gp0-raster")
  FileUtils.mkdir_p([raster, renderer_raster])
  File.write(File.join(profile, "summary.json"), JSON.generate(
    "frames" => [{ "frame" => "timer-1", "bundle" => bundle,
                   "native_only_black_component" => 83,
                   "surface_divergence_component" => 30 },
                 { "frame" => "timer-2", "bundle" => renderer_bundle,
                   "native_only_black_component" => 12,
                   "surface_divergence_component" => 0 }]
  ))
  File.write(File.join(raster, "report.json"), JSON.generate(
    "regions" => { "mirror_road" => { "significant_pixels" => 3,
                                        "largest_component" => 1 } }
  ))
  File.write(File.join(renderer_raster, "report.json"), JSON.generate(
    "regions" => { "mirror_road" => { "significant_pixels" => 10,
                                        "largest_component" => 9 } }
  ))
  stdout, stderr, status = Open3.capture3(
    RbConfig.ruby, tool, "--run", run, "--profile", "mirror-road",
    "--top", "2", "--reuse-only"
  )
  abort stdout + stderr unless status.success?
  result = JSON.parse(File.read(File.join(run, "triage.json"))).fetch("results").fetch(0)
  abort "near-pose mirror artifact was not classified" unless
    result.fetch("visual_component") == 83 && result.fetch("canonical_component") == 1 &&
    result.fetch("classification") == "pose_or_game_packets"
  renderer = JSON.parse(File.read(File.join(run, "triage.json"))).fetch("results").fetch(1)
  abort "canonical renderer regression was not classified" unless
    renderer.fetch("canonical_component") == 9 &&
    renderer.fetch("classification") == "renderer"
end

puts "visual triage separates near-state image artifacts from canonical renderer differences"
