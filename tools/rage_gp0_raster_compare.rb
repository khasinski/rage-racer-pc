#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "digest"
require "json"
require "open3"
require "optparse"
require "pathname"
require "rbconfig"
require "shellwords"

root = Pathname(__dir__).parent.expand_path
reproduction_argv = [$PROGRAM_NAME, *ARGV]
options = { native: root / "build-host/rage-gp0-replay" }
OptionParser.new do |cli|
  cli.banner = "usage: rage_gp0_raster_compare.rb --vram RAW --log GP0.LOG --frame N --output DIR"
  cli.on("--vram PATH") { |v| options[:vram] = Pathname(v).expand_path }
  cli.on("--log PATH") { |v| options[:log] = Pathname(v).expand_path }
  cli.on("--frame N", Integer) { |v| options[:frame] = v }
  cli.on("--output DIR") { |v| options[:output] = Pathname(v).expand_path }
  cli.on("--native PATH") { |v| options[:native] = Pathname(v).expand_path }
  cli.on("--last-packet N", Integer) { |v| options[:last_packet] = v }
  cli.on("--only-packet N", Integer) { |v| options[:only_packet] = v }
end.parse!

required = %i[vram log frame output]
abort "missing #{required.reject { |key| options.key?(key) }.join(', ')}" unless
  required.all? { |key| options.key?(key) }
abort "invalid VRAM snapshot (expected 1048576 bytes): #{options[:vram]}" unless
  options[:vram].file? && options[:vram].size == 1024 * 512 * 2
abort "missing GP0 log: #{options[:log]}" unless options[:log].file?
abort "missing native replayer: #{options[:native]}" unless options[:native].file?
abort "--only-packet requires --last-packet" if options[:only_packet] && !options[:last_packet]

output = options[:output]
FileUtils.mkdir_p(output)
native_ppm = output / "native.ppm"
psx_ppm = output / "psx.ppm"

native_command = [options[:native].to_s, options[:vram].to_s,
                  options[:log].to_s, options[:frame].to_s, native_ppm.to_s]
native_command << options[:last_packet].to_s if options[:last_packet]
native_command << options[:only_packet].to_s if options[:only_packet]
stdout, stderr, status = Open3.capture3(*native_command)
File.write(output / "native.log", stdout + stderr)
abort "native replay failed; see #{output / 'native.log'}" unless status.success?

psx_script = <<~'RUBY'
  require "psx"
  vram_path, log_path, frame_text, ppm_path, last_text, only_text = ARGV
  frame = Integer(frame_text, 10)
  last_packet = last_text && Integer(last_text, 10)
  only_packet = only_text && Integer(only_text, 10)
  gpu = PSX::GPU.new
  gpu.vram.replace(File.binread(vram_path).unpack("v*"))
  page_y = 0
  prefix = "frame=#{frame} "
  File.foreach(log_path) do |line|
    next unless line.include?(prefix)
    words_text = line[/\bwords=([0-9a-fA-F,]+)/, 1]
    next unless words_text
    packet = line[/\bpacket=(\d+)/, 1]&.to_i
    words = words_text.split(",").map { |word| Integer(word, 16) }
    draw_area = words.find { |word| word >> 24 == 0xe3 }
    page_y = ((draw_area >> 10) & 0x3ff) / 240 * 240 if draw_area
    break if last_packet && packet && packet > last_packet
    code = words[0] >> 24
    next if only_packet && packet != only_packet && !(0xe1..0xe6).cover?(code)
    words.each { |word| gpu.gp0(word) }
  end
  rgb = String.new(capacity: 320 * 240 * 3, encoding: Encoding::BINARY)
  240.times do |y|
    row = (page_y + y) * 1024
    320.times do |x|
      pixel = gpu.vram[row + x]
      rgb << ((pixel & 31) << 3) << (((pixel >> 5) & 31) << 3) << (((pixel >> 10) & 31) << 3)
    end
  end
  File.binwrite(ppm_path, "P6\n320 240\n255\n" + rgb)
RUBY
psx_command = ["mise", "exec", "--", "bundle", "exec", "ruby",
               "-Ilib", "-e", psx_script, options[:vram].to_s,
               options[:log].to_s, options[:frame].to_s, psx_ppm.to_s]
psx_command << options[:last_packet].to_s if options[:last_packet]
psx_command << options[:only_packet].to_s if options[:only_packet]
stdout, stderr, status = Open3.capture3(*psx_command, chdir: root / "tools/psx-ruby")
File.write(output / "psx.log", stdout + stderr)
abort "PSX replay failed; see #{output / 'psx.log'}" unless status.success?

payload = ->(path) do
  bytes = File.binread(path)
  header = bytes.index("\n255\n")
  abort "invalid PPM: #{path}" unless header
  bytes.byteslice(header + 5, 320 * 240 * 3)
end
psx = payload.call(psx_ppm)
native = payload.call(native_ppm)
mismatches = []
significant = []
significant_mask = Array.new(320 * 240, false)
sum_sq = 0
(320 * 240).times do |pixel|
  offset = pixel * 3
  # PsyZ expands RGB555 channels to 255 while the emulator exposes the native
  # 5-bit value shifted by three (248 maximum). Compare in PS1 colour space.
  delta = 3.times.map do |channel|
    (native.getbyte(offset + channel) >> 3) - (psx.getbyte(offset + channel) >> 3)
  end
  next if delta.all?(&:zero?)
  sum_sq += delta.sum { |value| value * value }
  item = [pixel % 320, pixel / 320,
          psx.byteslice(offset, 3).bytes, native.byteslice(offset, 3).bytes]
  mismatches << item
  if delta.any? { |value| value.abs > 1 }
    significant << item
    significant_mask[pixel] = true
  end
end

components = []
visited = Array.new(significant_mask.length, false)
significant_mask.each_index do |start|
  next unless significant_mask[start] && !visited[start]
  queue = [start]
  visited[start] = true
  pixels = []
  until queue.empty?
    pixel = queue.pop
    pixels << pixel
    x = pixel % 320
    y = pixel / 320
    neighbours = []
    neighbours << pixel - 1 if x.positive?
    neighbours << pixel + 1 if x < 319
    neighbours << pixel - 320 if y.positive?
    neighbours << pixel + 320 if y < 239
    neighbours.each do |other|
      next unless significant_mask[other] && !visited[other]
      visited[other] = true
      queue << other
    end
  end
  xs = pixels.map { |pixel| pixel % 320 }
  ys = pixels.map { |pixel| pixel / 320 }
  components << { pixels: pixels.length, bbox: [xs.min, ys.min, xs.max, ys.max],
                  sample: [pixels[0] % 320, pixels[0] / 320] }
end
components.sort_by! { |component| -component[:pixels] }

region_rects = {
  road: [0, 55, 250, 150],
  mirror_road: [84, 40, 152, 16],
  hud: [0, 176, 320, 64],
  tachometer: [225, 145, 95, 95],
  tachometer_needle: [250, 160, 45, 42]
}
region_profiles = region_rects.transform_values do |(left, top, width, height)|
  region_sum_sq = 0
  region_mismatches = 0
  region_significant = 0
  region_mask = Array.new(width * height, false)
  height.times do |dy|
    width.times do |dx|
      pixel = (top + dy) * 320 + left + dx
      offset = pixel * 3
      delta = 3.times.map do |channel|
        (native.getbyte(offset + channel) >> 3) -
          (psx.getbyte(offset + channel) >> 3)
      end
      next if delta.all?(&:zero?)
      region_mismatches += 1
      region_sum_sq += delta.sum { |value| value * value }
      if delta.any? { |value| value.abs > 1 }
        region_significant += 1
        region_mask[dy * width + dx] = true
      end
    end
  end

  local_visited = Array.new(region_mask.length, false)
  largest_component = 0
  region_mask.each_index do |start|
    next unless region_mask[start] && !local_visited[start]
    queue = [start]
    local_visited[start] = true
    size = 0
    until queue.empty?
      pixel = queue.pop
      size += 1
      x = pixel % width
      y = pixel / width
      neighbours = []
      neighbours << pixel - 1 if x.positive?
      neighbours << pixel + 1 if x + 1 < width
      neighbours << pixel - width if y.positive?
      neighbours << pixel + width if y + 1 < height
      neighbours.each do |other|
        next unless region_mask[other] && !local_visited[other]
        local_visited[other] = true
        queue << other
      end
    end
    largest_component = size if size > largest_component
  end
  {
    rect: [left, top, width, height],
    rmse_rgb5: Math.sqrt(region_sum_sq.to_f / (width * height * 3)),
    mismatched_pixels: region_mismatches,
    significant_pixels: region_significant,
    largest_component: largest_component
  }
end

diff_rgb = String.new(capacity: 320 * 240 * 3, encoding: Encoding::BINARY)
significant_mask.each do |different|
  diff_rgb << (different ? "\xff\x00\xff" : "\x00\x00\x00")
end
File.binwrite(output / "diff.ppm", "P6\n320 240\n255\n" + diff_rgb)
report = {
  frame: options[:frame], last_packet: options[:last_packet],
  only_packet: options[:only_packet], mismatched_pixels: mismatches.length,
  significant_pixels: significant.length,
  rmse_rgb5: Math.sqrt(sum_sq.to_f / (320 * 240 * 3)),
  components: components.first(64), examples: significant.first(32),
  regions: region_profiles,
  inputs: {
    vram: options[:vram].to_s, vram_sha256: Digest::SHA256.file(options[:vram]).hexdigest,
    log: options[:log].to_s, log_sha256: Digest::SHA256.file(options[:log]).hexdigest
  },
  command: reproduction_argv.shelljoin,
  psx: psx_ppm.to_s, native: native_ppm.to_s, diff: (output / "diff.ppm").to_s
}
File.write(output / "report.json", JSON.pretty_generate(report) + "\n")
puts JSON.generate(report.slice(:mismatched_pixels, :significant_pixels,
                                :rmse_rgb5, :regions, :components, :examples))
