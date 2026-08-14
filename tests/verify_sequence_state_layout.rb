#!/usr/bin/env ruby
# frozen_string_literal: true

root = ARGV.fetch(0)
audio_header = File.read(File.join(root, "include/game/audio_state_internal.h"))
host_state = File.read(File.join(root, "src/port/host_state.c"))
native_state = File.read(File.join(root, "src/port/native_game_state.c"))
runtime = File.read(File.join(root, "src/main/PAL/main/audio/sound_runtime.c"))

abort "sequence table accessor is missing" unless
  audio_header.include?("void *GetSndTableArea(void)")
abort "host state still fixes the sequence table to the 32-bit retail size" if
  host_state.match?(/^unsigned char g_SndTableArea\[/)
abort "native state lacks two complete sequence states" unless
  native_state.match?(/^SeqStruct g_SndTableArea\[2\];/)
abort "native sequence table accessor does not return the typed storage" unless
  native_state.match?(/void \*GetSndTableArea\(void\).*?return g_SndTableArea;/m)
abort "sound runtime does not pass the typed storage to retail libsnd" unless
  runtime.scan(/SsSetTableSize\(\(u8 \*\)GetSndTableArea\(\), 2, 1\)/).length == 2

puts "Sequence state storage follows the native pointer width"
