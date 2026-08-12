#!/usr/bin/env ruby
# frozen_string_literal: true

require "optparse"

options = { clip_tolerance: 0, depth_tolerance: 0 }
OptionParser.new do |parser|
  parser.banner = "usage: rage_geometry_trace_compare.rb PSX_LOG NATIVE_LOG [options]"
  parser.on("--clip-tolerance N", Integer) { |value| options[:clip_tolerance] = value }
  parser.on("--depth-tolerance N", Integer) { |value| options[:depth_tolerance] = value }
end.parse!
abort "two trace logs are required" unless ARGV.length == 2

def parse(path)
  File.foreach(path).map do |line|
    next unless line.start_with?("terrain-decision ")
    fields = line.chomp.scan(/(\w+)=([^ ]+)/).to_h
    clip = fields.fetch("clip").split(",").map { |value| value == "na" ? nil : Integer(value) }
    {
      timer: Integer(fields.fetch("timer")), index: Integer(fields.fetch("index")),
      mirror: Integer(fields.fetch("mirror")), clut: fields.fetch("clut"),
      tpage: fields.fetch("tpage"), clip: clip,
      raw: fields["raw"] == "na" ? nil : Integer(fields.fetch("raw")),
      depth: fields["depth"] == "na" ? nil : Integer(fields.fetch("depth")),
      result: fields.fetch("result")
    }
  end.compact
end

psx = parse(ARGV[0])
native = parse(ARGV[1])
count = [psx.length, native.length].min
first = (0...count).find do |index|
  left = psx[index]
  right = native[index]
  structural = %i[mirror clut tpage result].any? { |field| left[field] != right[field] }
  depth = left[:depth] && right[:depth] &&
    (left[:depth] - right[:depth]).abs > options[:depth_tolerance]
  clip = left[:clip].zip(right[:clip]).any? do |a, b|
    a && b && (a - b).abs > options[:clip_tolerance]
  end
  structural || depth || clip
end

if first
  warn "first geometry divergence at record #{first}"
  warn "psx:    #{psx[first]}"
  warn "native: #{native[first]}"
  exit 1
end

if psx.length != native.length
  warn "trace length differs after #{count} matching records: psx=#{psx.length} native=#{native.length}"
  exit 1
end

puts "geometry trace: #{count} records match"
