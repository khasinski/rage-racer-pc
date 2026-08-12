#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "optparse"

options = {}
OptionParser.new do |parser|
  parser.banner = "usage: rage_gpu_digest_compare.rb --alignment summary.json --psx-log FILE --native-log FILE --output FILE"
  parser.on("--alignment FILE") { |value| options[:alignment] = value }
  parser.on("--psx-log FILE") { |value| options[:psx_log] = value }
  parser.on("--native-log FILE") { |value| options[:native_log] = value }
  parser.on("--output FILE") { |value| options[:output] = value }
end.parse!
abort "all four options are required" unless
  options.values_at(:alignment, :psx_log, :native_log, :output).all?

parse_codes = lambda do |text|
  return {} if text.nil? || text.empty?
  text.split(",").to_h do |entry|
    code, count = entry.split(":", 2)
    [code, Integer(count, 10)]
  end
end

parse_log = lambda do |path|
  File.readlines(path, chomp: true).each_with_object({}) do |line, rows|
    match = line.match(/gpu-digest frame=(\d+) count=(\d+) hash=([0-9a-f]{16}) codes=(.*)\z/i)
    next unless match
    rows[Integer(match[1], 10)] = {
      count: Integer(match[2], 10), hash: match[3].downcase,
      codes: parse_codes.call(match[4])
    }
  end
end

frame_number = lambda do |filename|
  match = filename.match(/-f(\d+)-s\d+\.ppm\z/)
  abort "aligned filename has no relative frame: #{filename}" unless match
  Integer(match[1], 10)
end

alignment = JSON.parse(File.read(options[:alignment]))
psx = parse_log.call(options[:psx_log])
native = parse_log.call(options[:native_log])
rows = alignment.fetch("frames").map do |pair|
  psx_frame = frame_number.call(pair.fetch("psx_frame"))
  native_frame = frame_number.call(pair.fetch("native_frame"))
  left = psx[psx_frame]
  right = native[native_frame]
  codes = ((left&.fetch(:codes, {})&.keys || []) |
           (right&.fetch(:codes, {})&.keys || [])).sort.to_h do |code|
    [code, (right&.dig(:codes, code) || 0) - (left&.dig(:codes, code) || 0)]
  end.reject { |_code, delta| delta.zero? }
  {
    frame: pair.fetch("frame"), psx_frame: psx_frame, native_frame: native_frame,
    missing_psx_digest: left.nil?, missing_native_digest: right.nil?,
    count_delta: left && right ? right[:count] - left[:count] : nil,
    hash_equal: left && right ? left[:hash] == right[:hash] : nil,
    code_count_delta: codes
  }
end
result = {
  aligned_frames: rows.length,
  comparable_frames: rows.count { |row| !row[:missing_psx_digest] && !row[:missing_native_digest] },
  equal_frames: rows.count { |row| row[:hash_equal] },
  frames: rows
}
File.write(options[:output], JSON.pretty_generate(result) + "\n")
rows.each do |row|
  puts "#{row[:frame]} count_delta=#{row[:count_delta] || '-'} " \
       "hash_equal=#{row[:hash_equal].nil? ? '-' : row[:hash_equal]} " \
       "codes=#{row[:code_count_delta].map { |code, delta| "#{code}:#{delta >= 0 ? '+' : ''}#{delta}" }.join(',')}"
end
