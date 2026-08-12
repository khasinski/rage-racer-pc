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
  abort "visual comparison must default to the presented front buffer" if
    metadata.dig("psx", "env").key?("RAGE_EMU_CAPTURE_DRAW_PAGE") ||
    metadata.dig("native", "env").key?("RAGE_PORT_CAPTURE_DRAW_PAGE") ||
    metadata.fetch("draw_page")
  abort "state-relative input was not applied symmetrically" unless
    metadata.dig("psx", "env", "RAGE_EMU_STATE_INPUT_SCRIPT") == "12@700+200:CROSS" &&
    metadata.dig("native", "env", "RAGE_PORT_STATE_INPUT_SCRIPT") == "12@700+200:CROSS"
  compare = metadata.dig("comparisons", "mirror-road", "argv")
  abort "diagnostic preset is missing" unless compare.each_cons(2).include?(["--preset", "mirror-road"])
  abort "repeated matcher arguments were not retained" unless
    compare.each_cons(2).include?(["--max-position-distance", "8"])
end

Dir.mktmpdir("rage-visual-run-draw-page-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--draw-page", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "explicit draw-page mode was not applied symmetrically" unless
    metadata.fetch("draw_page") &&
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_DRAW_PAGE") == "1" &&
    metadata.dig("native", "env", "RAGE_PORT_CAPTURE_DRAW_PAGE") == "1"
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

puts "visual run defaults to front buffers and records explicit draw-page diagnostics"
