#!/usr/bin/env ruby
# frozen_string_literal: true

# Build a stable visual-debug bundle from one emulator capture and one native
# capture. The expensive emulator frame is intentionally an input artifact so
# it can be cached and compared against many fast native iterations.

require "fileutils"
require "json"
require "open3"
require "optparse"
require "pathname"

options = { pixel: nil }
OptionParser.new do |parser|
  parser.banner = "usage: rage_visual_compare.rb --psx IMAGE --native IMAGE --output DIR [options]"
  parser.on("--psx PATH", "Ruby PSX emulator PPM/PNG") { |value| options[:psx] = value }
  parser.on("--native PATH", "Native port PPM/PNG") { |value| options[:native] = value }
  parser.on("--output DIR", "Comparison bundle directory") { |value| options[:output] = value }
  parser.on("--pixel X,Y", "Problem pixel to mark and extract from traces") do |value|
    options[:pixel] = value.split(",", 2).map { |part| Integer(part) }
  end
  parser.on("--psx-log PATH", "PSX gpu-cover log") { |value| options[:psx_log] = value }
  parser.on("--native-log PATH", "Native gpu-cover log") { |value| options[:native_log] = value }
end.parse!

abort "--psx, --native and --output are required" unless
  options.values_at(:psx, :native, :output).all?

def run!(*command)
  command = command.map(&:to_s)
  stdout, stderr, status = Open3.capture3(*command)
  return stdout if status.success?

  abort "command failed: #{command.join(' ')}\n#{stdout}#{stderr}"
end

def dimensions(path)
  run!("magick", "identify", "-format", "%w %h", path).split.map(&:to_i)
end

def normalize(source, destination, expected_size = nil)
  width, height = dimensions(source)
  if expected_size && [width, height] != expected_size
    abort "capture dimensions differ: #{source} is #{width}x#{height}, expected #{expected_size.join('x')}"
  end
  run!("magick", source, "-alpha", "off", "-colorspace", "sRGB", destination)
  [width, height]
end

def trace_lines(path, frame, pixel)
  return [] unless path

  File.readlines(path, chomp: true).grep(/^gpu-cover /).select do |line|
    (!frame || line.include?("frame=#{frame} ")) &&
      (!pixel || line.include?("pixel=#{pixel.join(',')} "))
  end
end

output = Pathname(options[:output]).expand_path
FileUtils.mkdir_p(output)
psx_png = output / "psx.png"
native_png = output / "native.png"
size = normalize(options[:psx], psx_png.to_s)
normalize(options[:native], native_png.to_s, size)

run!("magick", psx_png.to_s, native_png.to_s, "-compose", "difference",
     "-composite", (output / "difference.png").to_s)
run!("magick", output / "difference.png", "-colorspace", "gray",
     "-auto-level", "-fill", "#ff3000", "-tint", "100",
     (output / "heatmap.png").to_s)
run!("magick", psx_png.to_s, native_png.to_s, output / "heatmap.png", "+append",
     (output / "side-by-side.png").to_s)

# ImageMagick writes metrics to stderr and exits 1 when images differ. Run it
# separately because that exit status is a comparison result, not a tool error.
_stdout, metric_stderr, metric_status = Open3.capture3(
  "magick", "compare", "-metric", "RMSE", psx_png.to_s, native_png.to_s, "null:"
)
abort "ImageMagick comparison failed: #{metric_stderr}" unless [0, 1].include?(metric_status.exitstatus)
rmse = metric_stderr[/\(([0-9.eE+-]+)\)/, 1]&.to_f

if options[:pixel]
  x, y = options[:pixel]
  marker = "circle #{x},#{y} #{x + 4},#{y}"
  [psx_png, native_png].each do |image|
    marked = output / "#{image.basename('.png')}-marked.png"
    run!("magick", image.to_s, "-stroke", "#ff00ff", "-strokewidth", "1",
         "-fill", "none", "-draw", marker, marked.to_s)
  end
end

trace_report = []
{ psx: options[:psx_log], native: options[:native_log] }.each do |label, log|
  lines = trace_lines(log, nil, options[:pixel])
  trace_report << "#{label}:" << (lines.empty? ? "  (no matching packets)" : lines.map { |line| "  #{line}" })
end
File.write(output / "packet-trace.txt", trace_report.flatten.join("\n") + "\n")

report = {
  psx_source: File.expand_path(options[:psx]),
  native_source: File.expand_path(options[:native]),
  dimensions: size,
  normalized_rmse: rmse,
  pixel: options[:pixel],
  artifacts: %w[psx.png native.png difference.png heatmap.png side-by-side.png packet-trace.txt]
}
File.write(output / "report.json", JSON.pretty_generate(report) + "\n")
puts "visual comparison: #{output} (RMSE=#{rmse || 'unavailable'})"
