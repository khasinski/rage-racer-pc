#!/usr/bin/env ruby
# frozen_string_literal: true

root = ARGV.fetch(0)
race_header = File.read(File.join(root, "include/game/race.h"))
race_types = File.read(File.join(root, "include/game/race_types.h"))
host_state = File.read(File.join(root, "src/port/host_state.c"))
native_state = File.read(File.join(root, "src/port/native_game_state.c"))
menu_mode = File.read(File.join(root, "src/main/PAL/main/menu/menu_mode.c"))
course_select = File.read(File.join(root, "src/main/PAL/main/menu/course_select.c"))

progress_names = %w[g_GrandPrixSave g_ExtraGrandPrixSave g_TimeAttackSave]
progress_names.each do |name|
  abort "host state still truncates #{name}" if
    host_state.match?(/^unsigned char #{name}\[/)
  abort "native state does not allocate a complete #{name}" unless
    native_state.match?(/^GameRaceProgress #{name};/)
end

abort "race progress size is not pinned to 20 bytes" unless
  race_types.include?("sizeof(GameRaceProgress) == 0x14")
abort "race progress series word is not pinned to +0x10" unless
  race_types.include?("__builtin_offsetof(GameRaceProgress, money) == 0x10")
abort "Extra GP max class is still detached from its progress object" unless
  race_header.include?("#define g_ExtraGrandPrixSaveMaxClass (g_ExtraGrandPrixSave.maxClassReached)")

abort "Time Attack menu no longer restores its series from progress" unless
  menu_mode.include?("g_GrandPrixSeries = g_RaceProgress->money.half[0]")
abort "Time Attack exit no longer stores its series in progress" unless
  course_select.scan(/p->money\.value = g_GrandPrixSeries/).length >= 2

puts "Race progress objects retain course, car, class, unlock and series state"
