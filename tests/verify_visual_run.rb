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
             "--match-arg=--max-position-distance", "--match-arg=8", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "visual runner profile was not retained" unless metadata.fetch("profile") == "mirror-road"
  abort "PSX capture is not draw-page deterministic" unless
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_DRAW_PAGE") == "1"
  abort "native capture is not draw-page deterministic" unless
    metadata.dig("native", "env", "RAGE_PORT_CAPTURE_DRAW_PAGE") == "1"
  compare = metadata.dig("compare", "argv")
  abort "diagnostic preset is missing" unless compare.each_cons(2).include?(["--preset", "mirror-road"])
  abort "repeated matcher arguments were not retained" unless
    compare.each_cons(2).include?(["--max-position-distance", "8"])
end

puts "visual run records reproducible parallel draw-page commands"
