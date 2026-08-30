#!/usr/bin/env ruby
# frozen_string_literal: true

source_dir = File.expand_path(ARGV.fetch(0))
menu_header = File.read(File.join(source_dir, "include/game/menu.h"))
internal_header = File.read(File.join(source_dir, "include/game/menu_scripts_internal.h"))
native_state = File.read(File.join(source_dir, "src/port/native_game_state.c"))
declarations = menu_header + internal_header

scripts = Dir[File.join(source_dir, "src/main/PAL/main/**/*.c")].flat_map do |path|
  File.read(path).scan(/RunTimedDrawScript\(\s*&?(g_[A-Za-z0-9_]+)/).flatten
end.uniq.sort

unsafe = scripts.reject do |name|
  short_name = name.delete_prefix("g_")
  native_array = native_state.match?(/TimedDrawCommand\s+#{Regexp.escape(name)}\s*\[/)
  native_alias = menu_header.include?("#define #{name} g_Native#{short_name}")
  pointer_variable = declarations.match?(/extern\s+(?:u8|void)\s*\*\s*#{Regexp.escape(name)}\s*;/)
  native_array || native_alias || pointer_variable
end

abort "serialized PS1 UI scripts used as native commands: #{unsafe.join(', ')}" unless unsafe.empty?

puts "All directly executed UI scripts use native pointer-sized commands"
