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
             "--sync-random", "12@700=0x12345678",
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
  abort "random synchronization was not applied symmetrically" unless
    metadata.dig("psx", "env", "RAGE_EMU_SYNC_RANDOM") == "12@700=0x12345678" &&
    metadata.dig("native", "env", "RAGE_PORT_SYNC_RANDOM") == "12@700=0x12345678"
  compare = metadata.dig("comparisons", "mirror-road", "argv")
  abort "diagnostic preset is missing" unless compare.each_cons(2).include?(["--preset", "mirror-road"])
  abort "front-buffer comparison did not refine presentation phase" unless
    compare.each_cons(2).include?(["--visual-refine", "3"])
  abort "repeated matcher arguments were not retained" unless
    compare.each_cons(2).include?(["--max-position-distance", "8"])
end

Dir.mktmpdir("rage-visual-run-asymmetric-sync-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--psx-sync-random", "12@200=1:0:0",
             "--native-sync-random", "12@199=1:0:0", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "asymmetric state-tap boundaries were not retained" unless
    metadata.dig("psx", "env", "RAGE_EMU_SYNC_RANDOM") == "12@200=1:0:0" &&
    metadata.dig("native", "env", "RAGE_PORT_SYNC_RANDOM") == "12@199=1:0:0"
end

Dir.mktmpdir("rage-visual-run-frame-rng-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--sync-random", "12@700=1:0:0",
             "--sync-random-each-frame", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "per-frame RNG reset was not applied symmetrically" unless
    metadata.dig("psx", "env", "RAGE_EMU_SYNC_RANDOM_EACH_FRAME") == "1" &&
    metadata.dig("native", "env", "RAGE_PORT_SYNC_RANDOM_EACH_FRAME") == "1"
end

Dir.mktmpdir("rage-visual-run-psx-cache-") do |output|
  Dir.mktmpdir("rage-visual-run-reference-") do |cache|
    File.write(File.join(cache, "capture-manifest.csv"), "filename,frame,scene,timer\n")
    command = [RbConfig.ruby, tool, "--psx-cache", cache, "--output", output,
               "--dry-run"]
    stdout, stderr, status = Open3.capture3(*command)
    abort stdout + stderr unless status.success?
    metadata = JSON.parse(File.read(File.join(output, "run.json")))
    abort "cached PSX capture path was not retained" unless
      metadata.fetch("psx_cache") == File.expand_path(cache)
  end
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
  compare = metadata.dig("comparisons", "road", "argv")
  abort "draw-page comparison must retain the exact packet phase" unless
    compare.each_cons(2).include?(["--visual-refine", "0"])
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

Dir.mktmpdir("rage-visual-run-time-attack-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--route", "time-attack", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "time-attack route did not select the menu entry" unless
    metadata.dig("native", "env", "RAGE_PORT_INPUT_SCRIPT").include?("610:DOWN")
  abort "time-attack route did not preload engine RPM from race entry" unless
    metadata.dig("native", "env", "RAGE_PORT_INPUT_SCRIPT").include?("1264-10000:CROSS")
  abort "time-attack route did not retain continuous acceleration" unless
    metadata.dig("psx", "env", "RAGE_EMU_INPUT_SCRIPT") == "0-10000:CROSS"
end

Dir.mktmpdir("rage-visual-run-alignment-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--alignment-only", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "alignment-only mode was not recorded or forwarded" unless
    metadata.fetch("alignment_only") &&
    metadata.dig("comparisons", "road", "argv").include?("--alignment-only")
end

Dir.mktmpdir("rage-visual-run-compare-only-") do |output|
  command = [RbConfig.ruby, tool, "--output", output, "--compare-only"]
  stdout, stderr, status = Open3.capture3(*command)
  abort "compare-only unexpectedly accepted missing captures" if status.success?
  expected = "--compare-only requires existing #{File.join(output, "psx", "capture-manifest.csv")}"
  abort stdout + stderr unless (stdout + stderr).include?(expected)
  metadata = JSON.parse(File.read(File.join(output, "compare-run.json")))
  abort "compare-only mode was not recorded" unless metadata.fetch("compare_only")
  abort "compare-only should not invent a checkpoint" if
    metadata.dig("psx", "env").key?("RAGE_EMU_LOAD_STATE")
  abort "compare-only overwrote capture provenance" if File.exist?(File.join(output, "run.json"))
end

puts "visual run supports cached comparisons and records explicit capture diagnostics"
