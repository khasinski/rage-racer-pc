#!/usr/bin/env ruby
# frozen_string_literal: true

root = ARGV.fetch(0)
car_header = File.read(File.join(root, "include/game/car.h"))
menu_header = File.read(File.join(root, "include/game/menu.h"))
host_state = File.read(File.join(root, "src/port/host_state.c"))

abort "transmission is not aliased to the player drivetrain" unless
  car_header.include?("(u8 *)(void *)&g_PlayerCar + 0x130")
abort "retail +0x130 transmission offset is not asserted" unless
  car_header.include?("__builtin_offsetof(GameCarDrive, manual) == 0x130")
abort "menu still declares independent transmission storage" if
  menu_header.match?(/extern\s+s16\s+g_PlayerTransmission/)
abort "host state still allocates independent transmission storage" if
  host_state.match?(/^unsigned char g_PlayerTransmission\[/)

puts "Player transmission aliases the drivetrain manual flag"
