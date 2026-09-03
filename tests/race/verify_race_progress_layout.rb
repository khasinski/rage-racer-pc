#!/usr/bin/env ruby
# frozen_string_literal: true

root = ARGV.fetch(0)
race_header = File.read(File.join(root, "include/game/race.h"))
# Retail state is one segment split across a file per owning subsystem, so
# read it back as the one thing it is.
host_state = Dir[File.join(root, "src/port/host_state*.c")]
  .sort.map { |path| File.read(path) }.join("\n")
native_state = File.read(File.join(root, "src/port/native_game_state.c"))
menu_mode = File.read(File.join(root, "src/main/PAL/main/menu/menu_mode.c"))
course_select = File.read(File.join(root,
                                    "src/main/PAL/main/menu/course_select_screen.c"))
car_select = File.read(File.join(root,
                                 "src/main/PAL/main/menu/car_select_screen.c"))

progress_names = %w[g_GrandPrixSave g_ExtraGrandPrixSave g_TimeAttackSave]
progress_names.each do |name|
  abort "host state still truncates #{name}" if
    host_state.match?(/^unsigned char #{name}\[/)
  abort "native state does not allocate a complete #{name}" unless
    native_state.match?(/^GameRaceProgress #{name};/)
end

abort "race progress size is not pinned to 20 bytes" unless
  race_header.include?("sizeof(GameRaceProgress) == 0x14")
abort "race progress series word is not pinned to +0x10" unless
  race_header.include?("__builtin_offsetof(GameRaceProgress, money) == 0x10")
abort "Extra GP max class is still detached from its progress object" unless
  race_header.include?("#define g_ExtraGrandPrixSaveMaxClass (g_ExtraGrandPrixSave.maxClassReached)")

abort "Time Attack menu no longer restores its series from progress" unless
  menu_mode.include?("g_GrandPrixSeries = (u16)g_RaceProgress->timeAttackSeries")
# Both screens that start a race put the series in the money slot when there
# is no Grand Prix running. They used to write it twice each, once per exit;
# each now has one conditional that says which of the two the slot carries.
[["course select", course_select], ["car select", car_select]].each do |name, source|
  abort "#{name} no longer stores the Time Attack series in progress" unless
    source.include?("g_RaceProgress->money = g_PlayerMoney") &&
      source.include?("g_RaceProgress->timeAttackSeries = g_GrandPrixSeries")
end

puts "Race progress objects retain course, car, class, unlock and series state"
