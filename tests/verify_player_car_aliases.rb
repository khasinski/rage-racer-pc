#!/usr/bin/env ruby
# frozen_string_literal: true

root = ARGV.fetch(0)
aliases = File.read(File.join(root, "include/game/player_car_aliases.h"))
car_header = File.read(File.join(root, "include/game/car.h"))
host_state = File.read(File.join(root, "src/port/host_state.c"))
native_state = File.read(File.join(root, "src/port/native_game_state.c"))
symbol_map = File.read(File.join(root, "configs/PAL/sym.bss.main.txt"))
headers = Dir[File.join(root, "include/**/*.h")].map { |path| File.read(path) }.join("\n")

base = 0x8009E6D4
expected = {
  "g_PlayerSegmentWeight" => 0x38,
  "g_PlayerField3C" => 0x3C,
  "g_PlayerSteerAngle" => 0x44,
  "g_PlayerCarWheelAngle" => 0x48,
  "g_PlayerRenderRotation" => 0x50,
  "g_PlayerRenderY" => 0x60,
  "g_PlayerFacingBackwards" => 0xB8,
  "g_PlayerVelocity" => 0xC4,
  "g_PlayerTransmission" => 0x130,
  "g_PlayerTargetRpm" => 0x134,
  "g_PlayerThrottle" => 0x15C,
  "g_RacePosition" => 0x160,
  "g_HudLapHighlightRow" => 0x162
}

expected.each do |name, offset|
  address = base + offset
  abort format("retail map moved %s away from +0x%X", name, offset) unless
    symbol_map.match?(/^#{Regexp.escape(name)} = 0x#{address.to_s(16).upcase};/i)

  abort "host state still allocates independent #{name} storage" if
    host_state.match?(/^unsigned char #{Regexp.escape(name)}\[/)
  abort "a header still declares independent #{name} storage" if
    headers.match?(/extern\s+[^;\n]*\b#{Regexp.escape(name)}(?:\[|\s*;)/)

  next if name == "g_PlayerThrottle"

  abort format("%s is not aliased at +0x%X", name, offset) unless
    aliases.match?(/#define\s+#{Regexp.escape(name)}\b[\s\\\n]*.*?\+\s*0x#{offset.to_s(16).upcase}\b/im)
end

abort "player alias storage accessor is missing" unless
  native_state.match?(/void \*GetPlayerCarStorage\(void\).*?return &g_PlayerCar;/m)
abort "player layout assertions are missing" unless
  car_header.scan(/must retain its retail alias offset/).length == 12

player_update = File.read(File.join(root, "src/main/PAL/main/car/update_player_car.c"))
abort "throttle must be read from the drivetrain field" unless
  player_update.include?("p->acceleratorInput.value")

puts "Player-car interior symbols preserve their retail aliases"
