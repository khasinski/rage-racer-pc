#!/usr/bin/env ruby
# frozen_string_literal: true

require "optparse"

options = {}
OptionParser.new do |parser|
  parser.banner = "usage: rage_gp0_compare.rb --psx LOG --native LOG [options]"
  parser.on("--psx PATH", "Retail emulator GP0 trace") { |v| options[:psx] = v }
  parser.on("--native PATH", "Native PsyZ GP0 trace") { |v| options[:native] = v }
  parser.on("--skip-words N", Integer,
            "Ignore this many leading words on each side") { |v| options[:skip] = v }
end.parse!
abort "both --psx and --native are required" unless options[:psx] && options[:native]

Packet = Struct.new(:source, :line, :metadata, :words, keyword_init: true)

read_packets = lambda do |path, prefix|
  packets = []
  File.foreach(path).with_index(1) do |line, number|
    next unless line.start_with?(prefix)
    words_text = line[/\bwords=([0-9a-fA-F,]+)/, 1]
    next unless words_text
    packets << Packet.new(
      source: path, line: number, metadata: line.strip.sub(/\s+words=.*/, ""),
      words: words_text.split(",").map { |word| Integer(word, 16) }
    )
  end
  packets
end

psx_packets = read_packets.call(options[:psx], "gp0-command ")
native_packets = read_packets.call(options[:native], "gp0-packet ")
abort "no gp0-command records in #{options[:psx]}" if psx_packets.empty?
abort "no gp0-packet records in #{options[:native]}" if native_packets.empty?

flatten = lambda do |packets|
  packets.flat_map.with_index do |packet, packet_index|
    packet.words.map.with_index { |word, word_index| [word, packet_index, word_index] }
  end
end

psx_words = flatten.call(psx_packets)
native_words = flatten.call(native_packets)
skip = options.fetch(:skip, 0)
limit = [psx_words.length, native_words.length].min
difference = (skip...limit).find { |index| psx_words[index][0] != native_words[index][0] }
difference ||= limit if psx_words.length != native_words.length

if difference.nil?
  puts "gp0 streams equal: words=#{limit} psx_packets=#{psx_packets.length} " \
       "native_packets=#{native_packets.length}"
  exit 0
end

describe = lambda do |label, words, packets, index|
  if index >= words.length
    puts "#{label}: end-of-stream words=#{words.length}"
    next
  end
  word, packet_index, word_index = words[index]
  packet = packets[packet_index]
  puts format("%s: word=%08x packet_word=%d %s", label, word, word_index,
              packet.metadata)
  puts "#{label}: packet_words=#{packet.words.map { |v| format('%08x', v) }.join(',')}"
end

puts "first GP0 difference at word=#{difference}"
describe.call("psx", psx_words, psx_packets, difference)
describe.call("native", native_words, native_packets, difference)
exit 1
