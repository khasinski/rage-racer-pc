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

options = { region: nil, hotspots: 8, radius: 12, match: "timer",
            max_position_distance: 256.0, visual_refine: 0,
            clear_region: nil, rank: "rmse" }
OptionParser.new do |parser|
  parser.banner = "usage: rage_visual_batch.rb --psx-dir DIR --native-dir DIR --output DIR [options]"
  parser.on("--psx-dir DIR") { |value| options[:psx_dir] = value }
  parser.on("--native-dir DIR") { |value| options[:native_dir] = value }
  parser.on("--output DIR") { |value| options[:output] = value }
  parser.on("--region X,Y,W,H") { |value| options[:region] = value }
  parser.on("--clear-region X,Y,W,H") { |value| options[:clear_region] = value }
  parser.on("--hotspots N", Integer) { |value| options[:hotspots] = value }
  parser.on("--hotspot-radius N", Integer) { |value| options[:radius] = value }
  parser.on("--match MODE", %w[timer position],
            "pair by identical timer filename or nearest runtime position") do |value|
    options[:match] = value
  end
  parser.on("--max-position-distance N", Float,
            "skip position matches farther apart than N world units") do |value|
    options[:max_position_distance] = value
  end
  parser.on("--visual-refine N", Integer,
            "choose the lowest-RMSE native image within N timer ticks") do |value|
    options[:visual_refine] = value
  end
  parser.on("--rank METRIC", %w[rmse clear],
            "rank output by RMSE or native-only clear pixels") do |value|
    options[:rank] = value
  end
end.parse!

abort "--psx-dir, --native-dir and --output are required" unless
  options.values_at(:psx_dir, :native_dir, :output).all?

psx_dir = Pathname(options[:psx_dir]).expand_path
native_dir = Pathname(options[:native_dir]).expand_path
output = Pathname(options[:output]).expand_path
FileUtils.mkdir_p(output)

def manifest_rows(directory)
  path = directory / "capture-manifest.csv"
  abort "missing capture manifest: #{path}" unless path.file?
  lines = File.readlines(path, chomp: true)
  header = lines.shift&.split(",")&.map(&:to_sym)
  abort "empty capture manifest: #{path}" unless header
  lines.map do |line|
    values = line.split(",", -1)
    row = header.zip(values).to_h
    row.each { |key, value| row[key] = Integer(value) unless key == :filename }
    image = directory / row[:filename]
    image.file? ? row.merge(path: image) : nil
  end.compact
end

def image_rmse(psx, native, region)
  command = ["magick", "compare", "-metric", "RMSE"]
  if region
    x, y, width, height = region.split(",", 4).map { |part| Integer(part) }
    command.concat(["-extract", "#{width}x#{height}+#{x}+#{y}"])
  end
  command.concat([psx.to_s, native.to_s, "null:"])
  _stdout, metric, status = Open3.capture3(*command)
  abort "visual alignment failed for #{native}: #{metric}" unless
    [0, 1].include?(status.exitstatus)
  metric[/\(([0-9.eE+-]+)\)/, 1]&.to_f || Float::INFINITY
end

if options[:match] == "position"
  psx_states = manifest_rows(psx_dir)
  native_states = manifest_rows(native_dir)
  abort "capture manifest contains no usable frames" if psx_states.empty? || native_states.empty?
  pairs = psx_states.map do |psx|
    candidates = native_states.select do |native|
      native[:scene] == psx[:scene] && native[:lap] == psx[:lap]
    end
    abort "no native state candidate for #{psx[:filename]}" if candidates.empty?
    native = candidates.min_by do |candidate|
      dx = candidate[:x] - psx[:x]
      dz = candidate[:z] - psx[:z]
      dvx = candidate[:view_x] - psx[:view_x]
      dvy = candidate[:view_y] - psx[:view_y]
      dvz = candidate[:view_z] - psx[:view_z]
      speed = (candidate[:speed] - psx[:speed]).abs
      yaw = ((candidate[:body_yaw] - psx[:body_yaw] + 2048) % 4096 - 2048).abs
      # The VBlank checkpoint can observe the renderer during its mirror pass,
      # whose yaw is the main camera plus exactly 180 degrees.  Modulo 2048
      # compares the underlying view without turning that phase difference
      # into a false state mismatch.
      view_yaw = ((candidate[:view_angle_y] - psx[:view_angle_y] + 1024) % 2048 - 1024).abs
      Math.hypot(dx, dz) + Math.sqrt(dvx * dvx + dvy * dvy + dvz * dvz) * 2 +
        speed * 4 + yaw / 4.0 + view_yaw * 2 +
        (candidate[:timer] - psx[:timer]).abs
    end
    if options[:visual_refine] > 0
      nearby = candidates.select do |candidate|
        (candidate[:timer] - native[:timer]).abs <= options[:visual_refine]
      end
      native = nearby.min_by do |candidate|
        image_rmse(psx[:path], candidate[:path], options[:region])
      end
    end
    dx = native[:x] - psx[:x]
    dz = native[:z] - psx[:z]
    next if Math.hypot(dx, dz) > options[:max_position_distance]
    dvx = native[:view_x] - psx[:view_x]
    dvy = native[:view_y] - psx[:view_y]
    dvz = native[:view_z] - psx[:view_z]
    {
      psx: psx[:path], native: native[:path],
      label: "#{File.basename(psx[:filename], '.ppm')}__#{File.basename(native[:filename], '.ppm')}",
      state_delta: {
        x: dx, z: dz, distance: Math.hypot(dx, dz),
        view_distance: Math.sqrt(dvx * dvx + dvy * dvy + dvz * dvz),
        speed: native[:speed] - psx[:speed],
        timer: native[:timer] - psx[:timer]
      }
    }
  end.compact
  abort "no state-aligned frame pairs within the position limit" if pairs.empty?
else
  psx_frames = psx_dir.glob("timer-*-s*.ppm").to_h { |path| [path.basename.to_s, path] }
  native_frames = native_dir.glob("timer-*-s*.ppm").to_h { |path| [path.basename.to_s, path] }
  names = (psx_frames.keys & native_frames.keys).sort
  abort "no matching timer-*-s*.ppm captures" if names.empty?
  pairs = names.map do |name|
    { psx: psx_frames.fetch(name), native: native_frames.fetch(name),
      label: name.delete_suffix(".ppm"), state_delta: nil }
  end
end

compare = Pathname(__dir__) / "rage_visual_compare.rb"
rows = pairs.map do |pair|
  frame_output = output / pair[:label]
  command = [RbConfig.ruby, compare.to_s,
             "--psx", pair[:psx].to_s,
             "--native", pair[:native].to_s,
             "--output", frame_output.to_s,
             "--hotspots", options[:hotspots].to_s,
             "--hotspot-radius", options[:radius].to_s]
  command.concat(["--region", options[:region]]) if options[:region]
  command.concat(["--clear-region", options[:clear_region]]) if options[:clear_region]
  stdout, stderr, status = Open3.capture3(*command)
  abort "comparison failed for #{pair[:label]}:\n#{stdout}#{stderr}" unless status.success?
  report = JSON.parse(File.read(frame_output / "report.json"))
  {
    frame: pair[:label],
    psx_frame: pair[:psx].basename.to_s,
    native_frame: pair[:native].basename.to_s,
    state_delta: pair[:state_delta],
    normalized_rmse: report.fetch("normalized_rmse"),
    normalized_region_rmse: report["normalized_region_rmse"],
    native_only_clear: report.fetch("native_only_clear").fetch("count"),
    worst_hotspot: report.fetch("hotspots").first,
    bundle: frame_output.to_s
  }
end

summary = {
  psx_directory: psx_dir.to_s,
  native_directory: native_dir.to_s,
  match: options[:match],
  rank: options[:rank],
  matched_frames: rows.length,
  frames: rows
}
File.write(output / "summary.json", JSON.pretty_generate(summary) + "\n")
rank_rmse = lambda do |row|
  row[:normalized_region_rmse] || row[:normalized_rmse]
end
ranked = if options[:rank] == "clear"
           rows.sort_by do |row|
             [-row[:native_only_clear], -rank_rmse.call(row).to_f]
           end
         else
           rows.sort_by { |row| -rank_rmse.call(row).to_f }
         end
ranked.each do |row|
  hotspot = row[:worst_hotspot]
  location = hotspot ? " hotspot=#{hotspot['x']},#{hotspot['y']}" : ""
  delta = row[:state_delta]
  alignment = delta ? format(" distance=%.1f view_distance=%.1f " \
                             "speed_delta=%d timer_delta=%d",
                             delta[:distance], delta[:view_distance],
                             delta[:speed], delta[:timer]) : ""
  clear = options[:clear_region] ?
    " native_only_clear=#{row[:native_only_clear]}" : ""
  metric = rank_rmse.call(row)
  metric_name = row[:normalized_region_rmse] ? "region_RMSE" : "RMSE"
  puts format("%s %s=%.6f%s%s%s", row[:frame], metric_name, metric,
              location, clear, alignment)
end
