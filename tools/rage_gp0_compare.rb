#!/usr/bin/env ruby
# frozen_string_literal: true

require "optparse"

options = { raw: false }
OptionParser.new do |parser|
  parser.banner = "usage: rage_gp0_compare.rb --psx LOG --native LOG [options]"
  parser.on("--psx PATH", "Retail emulator GP0 trace") { |v| options[:psx] = v }
  parser.on("--native PATH", "Native PsyZ GP0 trace") { |v| options[:native] = v }
  parser.on("--skip-words N", Integer,
            "Ignore this many leading words on each side") { |v| options[:skip] = v }
  parser.on("--psx-frame N", Integer, "Compare only this retail frame") { |v| options[:psx_frame] = v }
  parser.on("--native-frame N", Integer, "Compare only this native frame") { |v| options[:native_frame] = v }
  parser.on("--raw", "Compare ignored/padding bits too") { options[:raw] = true }
end.parse!
abort "both --psx and --native are required" unless options[:psx] && options[:native]

Packet = Struct.new(:source, :line, :metadata, :words, keyword_init: true)

read_packets = lambda do |path, prefix, wanted_frame|
  packets = []
  File.foreach(path).with_index(1) do |line, number|
    next unless line.start_with?(prefix)
    frame = line[/\bframe=(\d+)/, 1]&.to_i
    next if wanted_frame && frame != wanted_frame
    words_text = line[/\bwords=([0-9a-fA-F,]+)/, 1]
    next unless words_text
    packets << Packet.new(
      source: path, line: number, metadata: line.strip.sub(/\s+words=.*/, ""),
      words: words_text.split(",").map { |word| Integer(word, 16) }
    )
  end
  packets
end

psx_packets = read_packets.call(options[:psx], "gp0-command ", options[:psx_frame])
native_packets = read_packets.call(options[:native], "gp0-packet ", options[:native_frame])
abort "no gp0-command records in #{options[:psx]}" if psx_packets.empty?
abort "no gp0-packet records in #{options[:native]}" if native_packets.empty?

normalize_draw_page = lambda do |packets|
  draw_top = packets.flat_map(&:words).find { |word| (word >> 24) == 0xe3 }
  draw_y = draw_top ? ((draw_top >> 10) & 0x1ff) : 0
  return if draw_y.zero? || options[:raw]
  packets.each do |packet|
    command = (packet.words.first >> 24) & 0xff
    if command == 0xe3 || command == 0xe4
      packet.words.map! do |word|
        case word >> 24
        when 0xe3, 0xe4
          y = ((word >> 10) & 0x1ff) - draw_y
          (word & ~0x0007_fc00) | ((y & 0x1ff) << 10)
        when 0xe5
          y = ((word >> 11) & 0x7ff) - draw_y
          (word & ~0x003f_f800) | ((y & 0x7ff) << 11)
        when 0x02
          # The following word is the fill origin and is handled below after
          # packet grouping; keep the opcode unchanged here.
          word
        else
          word
        end
      end
      packet.words.each_index do |index|
        next unless (packet.words[index] >> 24) == 0x02 && index + 1 < packet.words.length
        origin = packet.words[index + 1]
        y = ((origin >> 16) & 0xffff) - draw_y
        packet.words[index + 1] = (origin & 0xffff) | ((y & 0xffff) << 16)
      end
      next
    end
    if command == 0xe5
      word = packet.words[0]
      y = ((word >> 11) & 0x7ff) - draw_y
      packet.words[0] = (word & ~0x003f_f800) | ((y & 0x7ff) << 11)
      next
    end
    if command == 0x02 && packet.words.length >= 2
      word = packet.words[1]
      y = ((word >> 16) & 0xffff) - draw_y
      packet.words[1] = (word & 0xffff) | ((y & 0xffff) << 16)
      next
    end
    # Primitive XY is already expressed relative to E5 on both sides.  Only
    # physical VRAM commands above need page normalization.
  end
end

normalize_draw_page.call(psx_packets)
normalize_draw_page.call(native_packets)

flatten = lambda do |packets|
  packets.flat_map.with_index do |packet, packet_index|
    command = (packet.words.first >> 24) & 0xff
    ignored_color_words = case command & 0xfc
                          when 0x30 then [2, 4]       # POLY_G3
                          when 0x34 then [3, 6]       # POLY_GT3
                          when 0x38 then [2, 4, 6]    # POLY_G4
                          when 0x3c then [3, 6, 9]    # POLY_GT4
                          when 0x50 then [2]          # LINE_G2
                          else []
                          end
    ignored_high_half_words = case command & 0xfc
                              when 0x24 then [6]       # POLY_FT3 uv2/pad
                              when 0x2c then [6, 8]    # POLY_FT4 uv2/3 pad
                              when 0x34 then [8]       # POLY_GT3 uv2/pad
                              when 0x3c then [8, 11]   # POLY_GT4 uv2/3 pad
                              else []
                              end
    packet.words.map.with_index do |word, word_index|
      normalized = word
      unless options[:raw]
        normalized &= 0x00ff_ffff if ignored_color_words.include?(word_index)
        normalized &= 0x0000_ffff if ignored_high_half_words.include?(word_index)
      end
      [normalized, packet_index, word_index, word]
    end
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
  word, packet_index, word_index, raw_word = words[index]
  packet = packets[packet_index]
  raw_suffix = raw_word == word ? "" : format(" raw=%08x", raw_word)
  puts format("%s: word=%08x%s packet_word=%d %s", label, word, raw_suffix,
              word_index, packet.metadata)
  puts "#{label}: packet_words=#{packet.words.map { |v| format('%08x', v) }.join(',')}"
end

describe_payload_variants = lambda do |label, packets, words, index|
  next if index >= words.length
  _, packet_index, word_index, = words[index]
  next unless word_index == 0
  packet = packets[packet_index]
  command = (packet.words.first >> 24) & 0xff
  next unless command.between?(0x20, 0x3f) && packet.words.length > 1
  # Colour is word zero.  XY/UV/tpage/CLUT identify duplicate geometry much
  # more reliably when diagnosing a colour mismatch; ignored UV padding is
  # normalized by flatten above, so compare the canonical payload words.
  signature = flatten.call([packet]).drop(1).map(&:first)
  variants = packets.each_index.map do |candidate_index|
    candidate = packets[candidate_index]
    next unless candidate.words.length == packet.words.length
    next unless ((candidate.words.first >> 24) & 0xff) == command
    candidate_flat = flatten.call([candidate])
    next unless candidate_flat.drop(1).map(&:first) == signature
    [candidate_index, candidate.words.first]
  end.compact
  colors = variants.map(&:last).uniq
  return unless variants.length > 1 || colors.length > 1
  variant_text = variants.map do |candidate_index, color|
    format("packet=%d word0=%08x", candidate_index, color)
  end.join("; ")
  puts "#{label}: same-payload variants=#{variant_text}"
end

puts "first GP0 difference at word=#{difference}"
describe.call("psx", psx_words, psx_packets, difference)
describe.call("native", native_words, native_packets, difference)
describe_payload_variants.call("psx", psx_packets, psx_words, difference)
describe_payload_variants.call("native", native_packets, native_words, difference)
exit 1
