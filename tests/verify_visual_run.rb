#!/usr/bin/env ruby
# frozen_string_literal: true

require "json"
require "open3"
require "tmpdir"

tool = ARGV.fetch(0)
abort "default Grand Prix trajectory lost its verified acceleration boundary" unless
  File.read(tool).scan('1468-10000:CROSS').length == 2
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
  abort "mirror comparison did not require identical rival render state" unless
    compare.include?("--require-rival-render-state")
  abort "front-buffer comparison did not refine presentation phase" unless
    compare.each_cons(2).include?(["--visual-refine", "3"])
  abort "repeated matcher arguments were not retained" unless
    compare.each_cons(2).include?(["--max-position-distance", "8"])
  abort "logical capture stride did not account for the two PSX VBlanks per game tick" unless
    metadata.dig("capture_strides", "psx_vblank") == 2 &&
    metadata.dig("capture_strides", "native_timer") == 1
  abort "timer window did not stop both runtimes at its upper boundary" unless
    metadata.dig("psx", "env", "RAGE_EMU_STOP_SCENE") == "12" &&
    metadata.dig("psx", "env", "RAGE_EMU_STOP_SCENE_TIMER") == "870" &&
    metadata.dig("native", "env", "RAGE_PORT_SMOKE_STOP_SCENE") == "12" &&
    metadata.dig("native", "env", "RAGE_PORT_SMOKE_STOP_SCENE_TIMER") == "870"
  abort "capture window was not restricted to the race scene" unless
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_SCENE") == "12" &&
    metadata.dig("native", "env", "RAGE_PORT_SMOKE_CAPTURE_SCENE") == "12"
end

Dir.mktmpdir("rage-visual-run-sparse-stride-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--capture-stride", "10", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "sparse native sampling made the PSX timer oracle sparse" unless
    metadata.dig("capture_strides", "psx_vblank") == 2 &&
    metadata.dig("capture_strides", "native_timer") == 10
end

Dir.mktmpdir("rage-visual-run-explicit-strides-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--capture-stride", "7",
             "--psx-capture-stride", "3", "--native-capture-stride", "5",
             "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "explicit runtime capture strides were not retained" unless
    metadata.dig("capture_strides", "psx_vblank") == 3 &&
    metadata.dig("capture_strides", "native_timer") == 5
end

Dir.mktmpdir("rage-visual-run-checkpoints-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--checkpoint-stride", "100", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "sparse PSX checkpoint cadence was not retained" unless
    metadata.fetch("checkpoint_stride") == 100 &&
    metadata.dig("psx", "env", "RAGE_EMU_CHECKPOINT_TIMER_STRIDE") == "100"
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
    metadata.dig("native", "env", "RAGE_PORT_CAPTURE_DRAW_PAGE") == "1" &&
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_VRAM_SIDECAR") == "1" &&
    metadata.dig("native", "env", "RAGE_PORT_CAPTURE_VRAM_SIDECAR") == "1"
  abort "visual run did not isolate physical host input" unless
    metadata.dig("native", "env", "RAGE_PORT_DISABLE_HOST_INPUT") == "1"
  compare = metadata.dig("comparisons", "road", "argv")
  abort "draw-page comparison must retain the exact packet phase" unless
    compare.each_cons(2).include?(["--visual-refine", "0"])
end

Dir.mktmpdir("rage-visual-run-strict-race-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--strict-race", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "strict race audit did not select exact final-DMA captures" unless
    metadata.fetch("strict_race") && metadata.fetch("draw_page") &&
    metadata.fetch("profile") == "all" &&
    metadata.dig("capture_strides", "psx_vblank") == 1 &&
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_DMA_PHASES") == "1" &&
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_FINAL_DMA_ONLY") == "1" &&
    metadata.dig("psx", "env", "RAGE_EMU_CAPTURE_TIMER_STRIDE") == "1"
  road = metadata.dig("comparisons", "road", "argv")
  abort "strict race audit omitted visibility/projection gates" unless
    road.include?("--require-main-visible-cells") &&
    road.include?("--require-mirror-visible-cells") &&
    road.each_cons(2).include?(["--max-position-distance", "4"]) &&
    road.each_cons(2).include?(["--max-view-distance", "4"]) &&
    road.each_cons(2).include?(["--max-tacho-rpm-delta", "0"]) &&
    road.include?("--require-rivals-only-render-state") &&
    !road.include?("--require-rival-render-state") &&
    road.each_cons(2).include?(["--max-projection-delta", "0"])
  abort "strict race audit omitted artifact budgets" unless
    metadata.fetch("budgets").include?("road.black_component=8") &&
    metadata.fetch("budgets").include?("tacho.needle=0")
end

Dir.mktmpdir("rage-visual-run-visible-cells-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--visible-cells", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "visibility diagnostics were not enabled symmetrically" unless
    metadata.fetch("visible_cells") &&
    metadata.dig("psx", "env", "RAGE_EMU_VISIBLE_CELLS") == "1" &&
    metadata.dig("native", "env", "RAGE_PORT_SMOKE_VISIBLE_CELLS") == "1"
end


Dir.mktmpdir("rage-visual-run-all-") do |output|
  command = [RbConfig.ruby, tool, "--checkpoint", "/tmp/reference.psxstate",
             "--output", output, "--profile", "all", "--dry-run"]
  stdout, stderr, status = Open3.capture3(*command)
  abort stdout + stderr unless status.success?
  metadata = JSON.parse(File.read(File.join(output, "run.json")))
  abort "all profile did not schedule every diagnostic" unless
    metadata.fetch("comparisons").keys == %w[road mirror-road tacho hud]
  abort "road profiles must retain rival-state gating" unless
    %w[road mirror-road].all? do |profile|
      metadata.dig("comparisons", profile, "argv").include?("--require-rival-render-state")
    end
  abort "HUD profiles must not be discarded solely by unrelated rival state" unless
    %w[tacho hud].none? do |profile|
      metadata.dig("comparisons", profile, "argv").include?("--require-rival-render-state")
    end
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

source = File.read(tool)
abort "multi-profile comparison still aborts before independent profiles run" unless
  source.include?("comparison_failures << message") &&
  source.index('batch_commands.each do |profile, command|') <
    source.index('unless comparison_failures.empty?')
abort "budget evaluation still masks an earlier comparison failure" unless
  source.index('unless comparison_failures.empty?') <
    source.index('unless options[:budgets].empty?')
