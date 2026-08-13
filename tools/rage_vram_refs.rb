#!/usr/bin/env ruby
# frozen_string_literal: true

require "optparse"

options = {}
OptionParser.new do |parser|
  parser.banner = "usage: rage_vram_refs.rb --log GP0.LOG --frame N --retail VRAM --native VRAM"
  parser.on("--log PATH") { |value| options[:log] = value }
  parser.on("--frame N", Integer) { |value| options[:frame] = value }
  parser.on("--retail PATH") { |value| options[:retail] = value }
  parser.on("--native PATH") { |value| options[:native] = value }
end.parse!
%i[log frame retail native].each { |key| abort "--#{key} is required" unless options[key] }

load_vram = lambda do |path|
  words = File.binread(path).unpack("v*")
  abort "#{path} is not a 1024x512 VRAM dump" unless words.length == 1024 * 512
  words
end
retail = load_vram.call(options[:retail])
native = load_vram.call(options[:native])

tpages = {}
cluts = {}
current_tpage = nil
File.foreach(options[:log]) do |line|
  next unless line.start_with?("gp0-packet ") && line[/\bframe=(\d+)/, 1]&.to_i == options[:frame]
  encoded = line[/\bwords=([0-9a-fA-F,]+)/, 1]
  next unless encoded
  words = encoded.split(",").map { |word| Integer(word, 16) }
  code = words[0] >> 24
  if code == 0xe1
    current_tpage = words[0] & 0xffff
    next
  end
  textured_polygon = (code & 0x24) == 0x24 && (code & 0xe0).between?(0x20, 0x60)
  if textured_polygon
    cluts[words[2] >> 16] = true if words.length > 2
    tpage_index = (code & 0x10).zero? ? 4 : 5
    tpages[words[tpage_index] >> 16] = true if words.length > tpage_index
  elsif (code & 0xe0) == 0x60 && (code & 4) == 4
    cluts[words[2] >> 16] = true if words.length > 2
    tpages[current_tpage] = true if current_tpage
  end
end

compare_region = lambda do |x, y, width, height|
  exact = rgb = bit15 = 0
  height.times do |dy|
    width.times do |dx|
      a = retail[(y + dy) * 1024 + x + dx]
      b = native[(y + dy) * 1024 + x + dx]
      next if a == b
      exact += 1
      if (a & 0x7fff) == (b & 0x7fff)
        bit15 += 1
      else
        rgb += 1
      end
    end
  end
  [exact, rgb, bit15]
end

puts "referenced_tpages=#{tpages.length} referenced_cluts=#{cluts.length}"
tpages.keys.sort.each do |tpage|
  depth = (tpage >> 7) & 3
  width = [64, 128, 256, 256][depth]
  x = (tpage & 0xf) * 64
  y = ((tpage >> 4) & 1) * 256
  exact, rgb, bit15 = compare_region.call(x, y, width, 256)
  puts format("tpage=%04x depth=%d rect=%d,%d,%d,256 exact=%d rgb15=%d bit15=%d",
              tpage, depth, x, y, width, exact, rgb, bit15)
end
cluts.keys.sort.each do |clut|
  x = (clut & 0x3f) * 16
  y = (clut >> 6) & 0x1ff
  # The referenced tpage determines whether 16 or 256 entries are consumed.
  width = tpages.keys.any? { |tpage| ((tpage >> 7) & 3) == 1 } ? 256 : 16
  exact, rgb, bit15 = compare_region.call(x, y, width, 1)
  puts format("clut=%04x rect=%d,%d,%d,1 exact=%d rgb15=%d bit15=%d",
              clut, x, y, width, exact, rgb, bit15)
end
