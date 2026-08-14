#!/usr/bin/env ruby
# frozen_string_literal: true

require "csv"
require "optparse"

options = { timer_origin: nil }
OptionParser.new do |parser|
  parser.banner = "usage: rage_input_replay.rb MANIFEST [--timer-origin FRAME]"
  parser.on("--timer-origin FRAME", Integer,
            "map scene timer zero to this smoke frame") do |value|
    options[:timer_origin] = value
  end
end.parse!

manifest = ARGV.shift or abort "missing capture manifest"
rows = CSV.read(manifest, headers: true)
abort "manifest has no input samples" unless rows.headers.include?("pad_held")

buttons = {
  0x0001 => "L2", 0x0002 => "R2", 0x0004 => "L1", 0x0008 => "R1",
  0x0010 => "TRIANGLE", 0x0020 => "CIRCLE", 0x0040 => "CROSS",
  0x0080 => "SQUARE", 0x0100 => "SELECT", 0x0200 => "L3",
  0x0400 => "R3", 0x0800 => "START", 0x1000 => "UP",
  0x2000 => "RIGHT", 0x4000 => "DOWN", 0x8000 => "LEFT"
}.freeze

samples = rows.map do |row|
  source = options[:timer_origin] ? row.fetch("timer") : row.fetch("frame")
  frame = Integer(source) + (options[:timer_origin] || 0)
  [frame, Integer(row.fetch("pad_held"))]
end

runs = []
samples.each do |frame, mask|
  if !runs.empty? && runs[-1][1] + 1 == frame && runs[-1][2] == mask
    runs[-1][1] = frame
  else
    runs << [frame, frame, mask]
  end
end

events = runs.each_with_object([]) do |(first, last, mask), result|
  next if mask.zero?
  unknown = mask & ~buttons.keys.reduce(0, :|)
  abort format("unknown pad mask %#x at frame %d", unknown, first) unless unknown.zero?
  names = buttons.each_with_object([]) do |(bit, name), selected|
    selected << name if (mask & bit) != 0
  end
  range = first == last ? first.to_s : "#{first}-#{last}"
  result << "#{range}:#{names.join('+')}"
end

puts events.join(",")
