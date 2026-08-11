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

options = { pixel: nil, hotspots: 8, hotspot_radius: 12, region: nil,
            clear_region: nil }
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
  parser.on("--hotspots N", Integer,
            "Find N separated pixels with the largest RGB error (default: 8)") do |value|
    options[:hotspots] = value
  end
  parser.on("--hotspot-radius N", Integer,
            "Minimum distance between reported hotspots (default: 12)") do |value|
    options[:hotspot_radius] = value
  end
  parser.on("--region X,Y,W,H", "Limit automatic hotspots to a screen region") do |value|
    options[:region] = value.split(",", 4).map { |part| Integer(part) }
    abort "--region requires X,Y,W,H" unless options[:region].length == 4
  end
  parser.on("--clear-region X,Y,W,H",
            "Count native-only dark-blue clear pixels in a road region") do |value|
    options[:clear_region] = value.split(",", 4).map { |part| Integer(part) }
    abort "--clear-region requires X,Y,W,H" unless options[:clear_region].length == 4
  end
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

def rgb_pixels(path, width, height)
  ppm = run!("magick", path, "-alpha", "off", "-colorspace", "sRGB",
             "-depth", "8", "ppm:-")
  ppm.force_encoding(Encoding::BINARY)
  match = ppm.match(/\AP6\s+(?:#[^\n]*\s+)*([0-9]+)\s+([0-9]+)\s+255\s/mn)
  abort "cannot decode ImageMagick PPM output for #{path}" unless match
  actual_size = [Integer(match[1]), Integer(match[2])]
  abort "decoded size differs for #{path}: #{actual_size.join('x')}" unless
    actual_size == [width, height]
  payload = ppm.byteslice(match.end(0)..)
  abort "short RGB payload for #{path}" unless payload&.bytesize == width * height * 3
  payload.bytes.each_slice(3).map(&:freeze)
end

def find_hotspots(psx_pixels, native_pixels, width, height, count, radius, region)
  left, top, region_width, region_height = region || [0, 0, width, height]
  abort "hotspot region is outside the image" if left.negative? || top.negative? ||
    region_width <= 0 || region_height <= 0 || left + region_width > width ||
    top + region_height > height
  indices = (top...(top + region_height)).flat_map do |y|
    ((y * width + left)...(y * width + left + region_width)).to_a
  end
  errors = indices.map do |index|
    a = psx_pixels[index]
    b = native_pixels[index]
    score = (a[0] - b[0])**2 + (a[1] - b[1])**2 + (a[2] - b[2])**2
    [score, index]
  end
  radius_squared = radius * radius
  selected = []
  errors.sort_by! { |score, _index| -score }
  errors.each do |score, index|
    break if selected.length >= count
    break if score.zero?
    x = index % width
    y = index / width
    next if selected.any? { |item| (item[:x] - x)**2 + (item[:y] - y)**2 < radius_squared }
    selected << {
      x: x, y: y, squared_error: score,
      psx_rgb: psx_pixels[index], native_rgb: native_pixels[index]
    }
  end
  selected
end

def native_only_clear_pixels(psx_pixels, native_pixels, width, height, region)
  return { count: 0, samples: [] } unless region
  left, top, region_width, region_height = region
  abort "clear region is outside the image" if left.negative? || top.negative? ||
    region_width <= 0 || region_height <= 0 || left + region_width > width ||
    top + region_height > height
  samples = []
  count = 0
  (top...(top + region_height)).each do |y|
    (left...(left + region_width)).each do |x|
      index = y * width + x
      psx = psx_pixels[index]
      native = native_pixels[index]
      psx_clear = psx[0] < 8 && psx[1] < 8 && psx[2] > 35
      native_clear = native[0] < 8 && native[1] < 8 && native[2] > 35
      next unless native_clear && !psx_clear
      count += 1
      samples << { x: x, y: y, psx_rgb: psx, native_rgb: native } if samples.length < 32
    end
  end
  { count: count, samples: samples }
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

psx_pixels = rgb_pixels(psx_png.to_s, *size)
native_pixels = rgb_pixels(native_png.to_s, *size)
hotspots = find_hotspots(psx_pixels, native_pixels, *size,
                         options[:hotspots], options[:hotspot_radius],
                         options[:region])
clear_pixels = native_only_clear_pixels(psx_pixels, native_pixels, *size,
                                        options[:clear_region])
unless hotspots.empty?
  draws = hotspots.map do |spot|
    x = spot[:x]
    y = spot[:y]
    "circle #{x},#{y} #{x + 4},#{y}"
  end
  [psx_png, native_png].each do |source|
    destination = output / "#{source.basename('.png')}-hotspots.png"
    command = ["magick", source.to_s, "-stroke", "#ff00ff",
               "-strokewidth", "1", "-fill", "none"]
    draws.each { |draw| command.concat(["-draw", draw]) }
    run!(*command, destination.to_s)
  end
end

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
  hotspot_region: options[:region],
  hotspots: hotspots,
  native_only_clear: clear_pixels.merge(region: options[:clear_region]),
  artifacts: %w[psx.png native.png difference.png heatmap.png side-by-side.png
                psx-hotspots.png native-hotspots.png packet-trace.txt]
}
File.write(output / "report.json", JSON.pretty_generate(report) + "\n")
puts "visual comparison: #{output} (RMSE=#{rmse || 'unavailable'})"
