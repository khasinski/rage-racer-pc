#!/usr/bin/env ruby
# frozen_string_literal: true

# Pair timer-named emulator/native captures and build per-frame visual bundles.
# The emulator sequence is expensive and can remain cached while the native
# directory is replaced after each renderer change.

require "fileutils"
require "json"
require "open3"
require "optparse"
require "pathname"
require "rbconfig"

options = { region: nil, hotspots: 8, radius: 12 }
OptionParser.new do |parser|
  parser.banner = "usage: rage_visual_batch.rb --psx-dir DIR --native-dir DIR --output DIR [options]"
  parser.on("--psx-dir DIR") { |value| options[:psx_dir] = value }
  parser.on("--native-dir DIR") { |value| options[:native_dir] = value }
  parser.on("--output DIR") { |value| options[:output] = value }
  parser.on("--region X,Y,W,H") { |value| options[:region] = value }
  parser.on("--hotspots N", Integer) { |value| options[:hotspots] = value }
  parser.on("--hotspot-radius N", Integer) { |value| options[:radius] = value }
end.parse!

abort "--psx-dir, --native-dir and --output are required" unless
  options.values_at(:psx_dir, :native_dir, :output).all?

psx_dir = Pathname(options[:psx_dir]).expand_path
native_dir = Pathname(options[:native_dir]).expand_path
output = Pathname(options[:output]).expand_path
FileUtils.mkdir_p(output)

psx_frames = psx_dir.glob("timer-*-s*.ppm").to_h { |path| [path.basename.to_s, path] }
native_frames = native_dir.glob("timer-*-s*.ppm").to_h { |path| [path.basename.to_s, path] }
names = (psx_frames.keys & native_frames.keys).sort
abort "no matching timer-*-s*.ppm captures" if names.empty?

compare = Pathname(__dir__) / "rage_visual_compare.rb"
rows = names.map do |name|
  frame_output = output / name.delete_suffix(".ppm")
  command = [RbConfig.ruby, compare.to_s,
             "--psx", psx_frames.fetch(name).to_s,
             "--native", native_frames.fetch(name).to_s,
             "--output", frame_output.to_s,
             "--hotspots", options[:hotspots].to_s,
             "--hotspot-radius", options[:radius].to_s]
  command.concat(["--region", options[:region]]) if options[:region]
  stdout, stderr, status = Open3.capture3(*command)
  abort "comparison failed for #{name}:\n#{stdout}#{stderr}" unless status.success?
  report = JSON.parse(File.read(frame_output / "report.json"))
  {
    frame: name,
    normalized_rmse: report.fetch("normalized_rmse"),
    worst_hotspot: report.fetch("hotspots").first,
    bundle: frame_output.to_s
  }
end

summary = {
  psx_directory: psx_dir.to_s,
  native_directory: native_dir.to_s,
  matched_frames: rows.length,
  frames: rows
}
File.write(output / "summary.json", JSON.pretty_generate(summary) + "\n")
rows.sort_by { |row| -row[:normalized_rmse].to_f }.each do |row|
  hotspot = row[:worst_hotspot]
  location = hotspot ? " hotspot=#{hotspot['x']},#{hotspot['y']}" : ""
  puts format("%s RMSE=%.6f%s", row[:frame], row[:normalized_rmse], location)
end
