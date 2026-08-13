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

tpages = Hash.new { |hash, key| hash[key] = {} }
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
    gouraud = (code & 0x10) != 0
    quad = (code & 8) != 0
    uv_indices = gouraud ? [2, 5, 8, 11] : [2, 4, 6, 8]
    uv_indices.pop unless quad
    next unless uv_indices.all? { |index| words[index] }
    tpage = words[uv_indices[1]] >> 16
    clut = words[uv_indices[0]] >> 16
    depth = (tpage >> 7) & 3
    uvs = uv_indices.map { |index| [words[index] & 0xff, (words[index] >> 8) & 0xff] }
    u_min, u_max = uvs.map(&:first).minmax
    v_min, v_max = uvs.map(&:last).minmax
    # A conservative UV bounding box includes every texel a non-wrapping
    # affine triangle can sample. It may include a few words outside the
    # triangle, but unlike vertex-only sampling cannot miss its interior.
    (v_min..v_max).each do |v|
      ((u_min >> [2, 1, 0, 0][depth])..(u_max >> [2, 1, 0, 0][depth])).each do |u|
        tpages[tpage][[u, v]] = true
      end
    end
    cluts[clut] = [cluts.fetch(clut, 0), depth == 1 ? 256 : 16].max
  elsif (code & 0xe0) == 0x60 && (code & 4) == 4
    next unless words[2] && current_tpage
    depth = (current_tpage >> 7) & 3
    size_mode = (code >> 3) & 3
    width, height = case size_mode
                    when 0 then [words[3] & 0x3ff, (words[3] >> 16) & 0x1ff]
                    when 1 then [1, 1]
                    when 2 then [8, 8]
                    else [16, 16]
                    end
    u0 = words[2] & 0xff
    v0 = (words[2] >> 8) & 0xff
    height.times do |dv|
      width.times do |du|
        tpages[current_tpage][[((u0 + du) & 0xff) >> [2, 1, 0, 0][depth],
                               (v0 + dv) & 0xff]] = true
      end
    end
    clut = words[2] >> 16
    cluts[clut] = [cluts.fetch(clut, 0), depth == 1 ? 256 : 16].max
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
  x = (tpage & 0xf) * 64
  y = ((tpage >> 4) & 1) * 256
  samples = tpages[tpage].keys
  exact = rgb = bit15 = 0
  samples.each do |word_x, word_y|
    a = retail[(y + word_y) * 1024 + x + word_x]
    b = native[(y + word_y) * 1024 + x + word_x]
    next if a == b
    exact += 1
    (a & 0x7fff) == (b & 0x7fff) ? bit15 += 1 : rgb += 1
  end
  puts format("tpage=%04x depth=%d referenced_words=%d exact=%d rgb15=%d bit15=%d",
              tpage, depth, samples.length, exact, rgb, bit15)
end
cluts.keys.sort.each do |clut|
  x = (clut & 0x3f) * 16
  y = (clut >> 6) & 0x1ff
  width = cluts.fetch(clut)
  exact, rgb, bit15 = compare_region.call(x, y, width, 1)
  puts format("clut=%04x rect=%d,%d,%d,1 exact=%d rgb15=%d bit15=%d",
              clut, x, y, width, exact, rgb, bit15)
end
