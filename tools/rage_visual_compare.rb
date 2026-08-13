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
            clear_region: nil, black_region: nil, needle_region: nil,
            artifact_radius: 2, surface_error: 96 }
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
  parser.on("--black-region X,Y,W,H",
            "Count native-only near-black pixels in a render region") do |value|
    options[:black_region] = value.split(",", 4).map { |part| Integer(part) }
    abort "--black-region requires X,Y,W,H" unless options[:black_region].length == 4
  end
  parser.on("--needle-region X,Y,W,H",
            "Compare the red tachometer-needle silhouette") do |value|
    options[:needle_region] = value.split(",", 4).map { |part| Integer(part) }
    abort "--needle-region requires X,Y,W,H" unless
      options[:needle_region].length == 4
  end
  parser.on("--artifact-radius N", Integer,
            "Ignore clear/black matches within N reference pixels (default: 2)") do |value|
    abort "--artifact-radius must be non-negative" if value.negative?
    options[:artifact_radius] = value
  end
  parser.on("--surface-error N", Integer,
            "Minimum RGB distance for dark surface divergence (default: 96)") do |value|
    abort "--surface-error must be positive" unless value.positive?
    options[:surface_error] = value
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

def canonical_rgb555_pixels(pixels)
  pixels.map do |pixel|
    pixel.map do |channel|
      value5 = channel >> 3
      (value5 * 255 + 15) / 31
    end.freeze
  end
end

def write_rgb_pixels(path, pixels, width, height)
  payload = pixels.flatten.pack("C*")
  ppm = "P6\n#{width} #{height}\n255\n".b + payload
  IO.popen(["magick", "ppm:-", path.to_s], "wb") { |io| io.write(ppm) }
  abort "cannot write normalized RGB555 image #{path}" unless $?.success?
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

def reference_near?(pixels, width, height, x, y, radius)
  left = [0, x - radius].max
  right = [width - 1, x + radius].min
  top = [0, y - radius].max
  bottom = [height - 1, y + radius].min
  (top..bottom).any? do |sample_y|
    (left..right).any? do |sample_x|
      yield pixels[sample_y * width + sample_x]
    end
  end
end

def largest_four_connected_component(matches)
  remaining = matches.keys.to_h { |point| [point, true] }
  largest = []
  until remaining.empty?
    seed = remaining.each_key.first
    remaining.delete(seed)
    queue = [seed]
    component = []
    until queue.empty?
      point = queue.pop
      component << point
      x, y = point
      [[x - 1, y], [x + 1, y], [x, y - 1], [x, y + 1]].each do |neighbor|
        next unless remaining.delete(neighbor)
        queue << neighbor
      end
    end
    largest = component if component.length > largest.length
  end
  largest
end

def missing_area_report(matches, interior, sample_order = nil)
  interior_matches = interior.to_h { |sample| [[sample[:x], sample[:y]], sample] }
  largest = largest_four_connected_component(interior_matches)
  samples = sample_order ? interior.sort_by(&sample_order) : interior
  { count: interior.length, raw_count: matches.length,
    largest_component: largest.length,
    largest_component_bounds: if largest.empty?
                                nil
                              else
                                xs = largest.map(&:first)
                                ys = largest.map(&:last)
                                [xs.min, ys.min, xs.max - xs.min + 1,
                                 ys.max - ys.min + 1]
                              end,
    samples: samples.first(32) }
end

def native_only_clear_pixels(psx_pixels, native_pixels, width, height, region,
                             radius)
  return { count: 0, raw_count: 0, largest_component: 0,
           largest_component_bounds: nil, samples: [] } unless region
  left, top, region_width, region_height = region
  abort "clear region is outside the image" if left.negative? || top.negative? ||
    region_width <= 0 || region_height <= 0 || left + region_width > width ||
    top + region_height > height
  matches = {}
  (top...(top + region_height)).each do |y|
    (left...(left + region_width)).each do |x|
      index = y * width + x
      psx = psx_pixels[index]
      native = native_pixels[index]
      native_clear = native[0] < 8 && native[1] < 8 && native[2] > 35
      next unless native_clear
      next if reference_near?(psx_pixels, width, height, x, y, radius) do |sample|
        sample[0] < 8 && sample[1] < 8 && sample[2] > 35
      end
      matches[[x, y]] = { x: x, y: y, psx_rgb: psx, native_rgb: native }
    end
  end
  # A sub-pixel camera or front-buffer phase shift exposes the clear colour as
  # a one-pixel contour along an otherwise present polygon. Missing geometry
  # has an area: retain pixels that participate in both a horizontal and a
  # vertical run, which rejects diagonal/edge contours without eroding the
  # interior of a real triangular hole.
  interior = matches.values.select do |sample|
    x = sample[:x]
    y = sample[:y]
    horizontal = matches.key?([x - 1, y]) || matches.key?([x + 1, y])
    vertical = matches.key?([x, y - 1]) || matches.key?([x, y + 1])
    horizontal && vertical
  end
  missing_area_report(matches, interior)
end

def native_only_black_pixels(psx_pixels, native_pixels, width, height, region,
                             radius)
  return { count: 0, raw_count: 0, largest_component: 0,
           largest_component_bounds: nil, samples: [] } unless region
  left, top, region_width, region_height = region
  abort "black region is outside the image" if left.negative? || top.negative? ||
    region_width <= 0 || region_height <= 0 || left + region_width > width ||
    top + region_height > height
  matches = {}
  (top...(top + region_height)).each do |y|
    (left...(left + region_width)).each do |x|
      index = y * width + x
      psx = psx_pixels[index]
      native = native_pixels[index]
      # Missing PS1 texels are true black. A looser near-black threshold also
      # classifies legitimate dark-green barrier stripes (for example 0,8,0)
      # as holes whenever a small camera delta shifts their high-contrast
      # pattern. Keep this detector specific and use the separate clear-pixel
      # detector for uncovered framebuffer areas.
      next unless native.max <= 2 && psx.max >= 32
      next if reference_near?(psx_pixels, width, height, x, y, radius) do |sample|
        sample.max <= 2
      end
      score = 3.times.sum { |channel| (psx[channel] - native[channel])**2 }
      matches[[x, y]] = { x: x, y: y, squared_error: score,
                          psx_rgb: psx, native_rgb: native }
    end
  end
  interior = matches.values.select do |sample|
    x = sample[:x]
    y = sample[:y]
    horizontal = matches.key?([x - 1, y]) || matches.key?([x + 1, y])
    vertical = matches.key?([x, y - 1]) || matches.key?([x, y + 1])
    horizontal && vertical
  end
  missing_area_report(matches, interior,
                      ->(sample) { -sample[:squared_error] })
end

def surface_divergence_pixels(psx_pixels, native_pixels, width, height,
                              region, radius, threshold)
  return { count: 0, raw_count: 0, largest_component: 0,
           largest_component_bounds: nil, samples: [] } unless region
  left, top, region_width, region_height = region
  threshold_squared = threshold * threshold
  matches = {}
  (top...(top + region_height)).each do |y|
    (left...(left + region_width)).each do |x|
      index = y * width + x
      psx = psx_pixels[index]
      native = native_pixels[index]
      score = 3.times.sum { |channel| (psx[channel] - native[channel])**2 }
      next if score < threshold_squared
      psx_luma = (psx[0] * 54 + psx[1] * 183 + psx[2] * 19) >> 8
      native_luma = (native[0] * 54 + native[1] * 183 + native[2] * 19) >> 8
      # Target missing/darkened surfaces, not arbitrary palette or texture
      # phase changes. A black triangle is a strong luminance deficit.
      next unless native_luma + 48 <= psx_luma
      # A small camera delta moves high-contrast texture edges coherently.
      # Retain a mismatch only when the native colour is absent from the
      # nearby reference neighbourhood, within PS1 5-bit colour precision.
      next if reference_near?(psx_pixels, width, height, x, y, radius) do |sample|
        3.times.all? { |channel| (sample[channel] - native[channel]).abs <= 8 }
      end
      matches[[x, y]] = { x: x, y: y, squared_error: score,
                          psx_rgb: psx, native_rgb: native }
    end
  end
  interior = matches.values.select do |sample|
    x = sample[:x]
    y = sample[:y]
    (matches.key?([x - 1, y]) || matches.key?([x + 1, y])) &&
      (matches.key?([x, y - 1]) || matches.key?([x, y + 1]))
  end
  missing_area_report(matches, interior,
                      ->(sample) { -sample[:squared_error] })
end

def normalized_region_rmse(psx_pixels, native_pixels, width, height, region)
  return nil unless region
  left, top, region_width, region_height = region
  abort "RMSE region is outside the image" if left.negative? || top.negative? ||
    region_width <= 0 || region_height <= 0 || left + region_width > width ||
    top + region_height > height
  squared_error = 0
  (top...(top + region_height)).each do |y|
    (left...(left + region_width)).each do |x|
      index = y * width + x
      3.times do |channel|
        delta = psx_pixels[index][channel] - native_pixels[index][channel]
        squared_error += delta * delta
      end
    end
  end
  Math.sqrt(squared_error.to_f / (region_width * region_height * 3)) / 255.0
end

def tachometer_needle_difference(psx_pixels, native_pixels, width, height,
                                 region)
  return { psx_pixels: 0, native_pixels: 0, mismatch_pixels: 0,
           intersection_pixels: 0, union_pixels: 0, iou: nil } unless region
  left, top, region_width, region_height = region
  abort "needle region is outside the image" if left.negative? || top.negative? ||
    region_width <= 0 || region_height <= 0 || left + region_width > width ||
    top + region_height > height
  is_needle = lambda do |pixel|
    pixel[0] > 100 && pixel[0] > pixel[1] + 60 &&
      pixel[0] > pixel[2] + 60
  end
  psx_count = 0
  native_count = 0
  mismatch = 0
  intersection = 0
  union = 0
  (top...(top + region_height)).each do |y|
    (left...(left + region_width)).each do |x|
      index = y * width + x
      psx = is_needle.call(psx_pixels[index])
      native = is_needle.call(native_pixels[index])
      psx_count += 1 if psx
      native_count += 1 if native
      mismatch += 1 if psx != native
      intersection += 1 if psx && native
      union += 1 if psx || native
    end
  end
  { psx_pixels: psx_count, native_pixels: native_count,
    mismatch_pixels: mismatch, intersection_pixels: intersection,
    union_pixels: union, iou: union.zero? ? nil : intersection.to_f / union }
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

psx_pixels = canonical_rgb555_pixels(rgb_pixels(psx_png.to_s, *size))
native_pixels = canonical_rgb555_pixels(rgb_pixels(native_png.to_s, *size))
write_rgb_pixels(psx_png, psx_pixels, *size)
write_rgb_pixels(native_png, native_pixels, *size)

run!("magick", psx_png.to_s, native_png.to_s, "-compose", "difference",
     "-composite", (output / "difference.png").to_s)
run!("magick", output / "difference.png", "-colorspace", "gray",
     "-auto-level", "-fill", "#ff3000", "-tint", "100",
     (output / "heatmap.png").to_s)
run!("magick", psx_png.to_s, native_png.to_s, output / "heatmap.png", "+append",
     (output / "side-by-side.png").to_s)

rmse = normalized_region_rmse(psx_pixels, native_pixels, *size,
                              [0, 0, size[0], size[1]])
hotspots = find_hotspots(psx_pixels, native_pixels, *size,
                         options[:hotspots], options[:hotspot_radius],
                         options[:region])
clear_pixels = native_only_clear_pixels(psx_pixels, native_pixels, *size,
                                        options[:clear_region],
                                        options[:artifact_radius])
black_pixels = native_only_black_pixels(psx_pixels, native_pixels, *size,
                                        options[:black_region],
                                        options[:artifact_radius])
surface_divergence = surface_divergence_pixels(
  psx_pixels, native_pixels, *size, options[:region],
  options[:artifact_radius], options[:surface_error]
)
needle_difference = tachometer_needle_difference(
  psx_pixels, native_pixels, *size, options[:needle_region]
)
region_rmse = normalized_region_rmse(psx_pixels, native_pixels, *size,
                                    options[:region])
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
  normalized_region_rmse: region_rmse,
  pixel: options[:pixel],
  hotspot_region: options[:region],
  hotspots: hotspots,
  native_only_clear: clear_pixels.merge(region: options[:clear_region]),
  native_only_black: black_pixels.merge(region: options[:black_region]),
  surface_divergence: surface_divergence.merge(
    region: options[:region], threshold: options[:surface_error]
  ),
  tachometer_needle: needle_difference.merge(region: options[:needle_region]),
  artifact_radius: options[:artifact_radius],
  artifacts: %w[psx.png native.png difference.png heatmap.png side-by-side.png
                psx-hotspots.png native-hotspots.png packet-trace.txt]
}
File.write(output / "report.json", JSON.pretty_generate(report) + "\n")
puts "visual comparison: #{output} (RMSE=#{rmse || 'unavailable'})"
