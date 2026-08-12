#!/usr/bin/env ruby
# frozen_string_literal: true

# Align two Rage Random15 traces by their unique 32-bit LCG states. This makes
# an inherited frontend-call offset explicit before renderer comparisons.

abort "usage: rage_random_align.rb PSX_LOG NATIVE_LOG" unless ARGV.length == 2

def random_rows(path)
  File.readlines(path).map do |line|
    match = line.match(/random index=(\d+).*?frame=(\d+).*?seed=([0-9a-fA-F]{8})/)
    next unless match
    { index: Integer(match[1]), frame: Integer(match[2]),
      seed: Integer(match[3], 16), line: line.strip }
  end.compact
end

psx = random_rows(ARGV[0])
native = random_rows(ARGV[1])
abort "no Random15 records in #{ARGV[0]}" if psx.empty?
abort "no Random15 records in #{ARGV[1]}" if native.empty?

native_by_seed = native.group_by { |row| row[:seed] }
matches = psx.map do |row|
  candidates = native_by_seed[row[:seed]]
  next unless candidates&.length == 1
  [row, candidates.first]
end.compact
abort "no unique common Random15 state" if matches.empty?

offset_counts = matches.group_by { |a, b| a[:index] - b[:index] }
offset, aligned = offset_counts.max_by { |_value, rows| rows.length }
first_psx, first_native = aligned.first
puts "index_offset_psx_minus_native=#{offset} common_states=#{aligned.length}"
puts "first_psx=#{first_psx[:line]}"
puts "first_native=#{first_native[:line]}"
if aligned.length < 16
  warn "warning: only #{aligned.length} common states; extend both traces " \
       "past the same gameplay interval before treating the offset as stable"
end

bad = aligned.find { |a, b| a[:index] - b[:index] != offset }
abort "Random15 alignment is not a constant offset" if bad
