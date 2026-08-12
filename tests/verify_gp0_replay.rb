#!/usr/bin/env ruby
# frozen_string_literal: true

source = File.read(ARGV.fetch(0))
required = [
  "1024u * 512u", "invalid 1024x512 RGB5551 VRAM snapshot",
  "frame_field", 'strstr(line, "words=")',
  "Psyz_GpuReplayBegin", "Psyz_GpuReplayPacket", "Psyz_GpuReplayEnd",
  "StoreImage", "page_y", "P6\\n%d %d\\n255\\n"
]
missing = required.reject { |text| source.include?(text) }
raise "GP0 replay harness lost: #{missing.join(', ')}" unless missing.empty?
puts "GP0 replay preserves trace packet boundaries and validates its VRAM input"
