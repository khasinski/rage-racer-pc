#!/usr/bin/env ruby
# frozen_string_literal: true

abort "usage: #{$PROGRAM_NAME} BEFORE.VRAM AFTER.VRAM [X,Y]" unless (2..3).cover?(ARGV.length)

before = File.binread(ARGV[0]).unpack("v*")
after = File.binread(ARGV[1]).unpack("v*")
abort "VRAM inputs must both be 1024x512 RGB5551" unless before.length == 1024 * 512 && after.length == before.length

changes = before.each_index.select { |index| before[index] != after[index] }
if changes.empty?
  puts "changed=0"
  exit
end

xs = changes.map { |index| index % 1024 }
ys = changes.map { |index| index / 1024 }
puts "changed=#{changes.length} bounds=#{xs.min},#{ys.min},#{xs.max},#{ys.max}"

if ARGV[2]
  x, y = ARGV[2].split(",", 2).map { |value| Integer(value, 0) }
  abort "pixel outside VRAM" unless x.between?(0, 1023) && y.between?(0, 511)
  index = y * 1024 + x
  puts format("pixel=%d,%d before=%04x after=%04x changed=%d",
              x, y, before[index], after[index], before[index] == after[index] ? 0 : 1)
end
