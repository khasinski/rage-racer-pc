#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
Dir.mktmpdir("rage-visual-run-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--profile", "mirror-road",
             "--timer-min", "850", "--timer-max", "870",
             "--psx-input", "0-400:CROSS", "--native-input", "1-2:CROSS",
             "--psx-state-input", "12@700+200:CROSS",
             "--native-state-input", "12@700+200:CROSS",
             "--match-arg=--max-position-distance", "--match-arg=8", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "visual runner profile was not retained" unless metadata.fetch("profile") == "mirror-road"
  abort "PSX capture is not draw-page deterministic" unless
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_DRAW_PAGE") == "1"
  abort "native capture is not draw-page deterministic" unless
    metadata.dig("native", "env", "RAGE_PORT_CAPTURE_DRAW_PAGE") == "1"
  abort "state-relative input was not applied symmetrically" unless
    metadata.dig("psx", "env", "RAGE_EMU_STATE_INPUT_SCRIPT") == "12@700+200:CROSS" &&
    metadata.dig("native", "env", "RAGE_PORT_STATE_INPUT_SCRIPT") == "12@700+200:CROSS"
  compare = metadata.dig("comparisons", "mirror-road", "argv")
  abort "diagnostic preset is missing" unless compare.each_cons(2).include?(["--preset", "mirror-road"])
  abort "repeated matcher arguments were not retained" unless
    compare.each_cons(2).include?(["--max-position-distance", "8"])
end


Dir.mktmpdir("rage-visual-run-all-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--profile", "all", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "all profile did not schedule every diagnostic" unless
    metadata.fetch("comparisons").keys == %w[road mirror-road tacho hud]
end

puts "visual run records reproducible parallel draw-page commands"
