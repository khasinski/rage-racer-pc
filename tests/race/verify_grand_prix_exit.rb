#!/usr/bin/env ruby
# frozen_string_literal: true

require "open3"
require "timeout"

executable = File.expand_path(ARGV.fetch(0))
source_dir = File.expand_path(ARGV.fetch(1))
menu_header = File.read(File.join(source_dir, "include/game/menu.h"))
native_state = File.read(File.join(source_dir, "src/port/native_game_state.c"))

abort "save-prompt banner is not decoded as a native UI script" unless
  menu_header.include?("RAGE_NATIVE_UI_SCRIPT(CourseSelectSavePromptBanner, 2);") &&
  native_state.include?("NATIVE_UI_SCRIPT(g_NativeCourseSelectSavePromptBanner, 0x800827fcu)")
environment = {
  "SDL_AUDIODRIVER" => "dummy",
  "RAGE_PORT_SMOKE_FRAMES" => "2200",
  "RAGE_PORT_DISABLE_HOST_INPUT" => "1",
  "RAGE_PORT_INPUT_SCRIPT" =>
    "400:START,500:START,650:CROSS,1015:CROSS," \
    "1100:DOWN,1106:DOWN,1112:CROSS,1180:CROSS"
}

output = nil
status = nil
Timeout.timeout(70) do
  output, status = Open3.capture2e(environment, executable, chdir: source_dir)
end
abort output unless status.success?
abort "Grand Prix exit did not leave course selection" unless
  output.match?(/smoke state frame=.* scene=(?:2|4|24) /)
abort "Grand Prix exit reported a memory error" if
  output.include?("AddressSanitizer") || output.include?("runtime error")

puts "End Grand Prix left course selection without overrunning its UI script"
