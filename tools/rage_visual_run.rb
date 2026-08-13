#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "json"
require "optparse"
require "open3"
require "pathname"
require "rbconfig"
require "shellwords"
require "time"

root = Pathname(__dir__).parent.expand_path
options = {
  profile: "road", timer_min: 800, timer_max: 900, capture_stride: 1,
  psx_capture_stride: nil, native_capture_stride: nil,
  save_psx_states: false,
  checkpoint_stride: nil,
  psx_cache: nil,
  psx_frames: 500, native_frames: 2400, jobs: 8, top: nil,
  psx_input: "0-1000:CROSS",
  native_input: "400:START,500:START,650:CROSS,950:CROSS,1100:CROSS," \
                "1200:CROSS,1469-3000:CROSS",
  psx_state_input: nil, native_state_input: nil,
  sync_random: nil, psx_sync_random: nil, native_sync_random: nil,
  sync_random_each_frame: false,
  match_args: [], budgets: [], draw_page: false, alignment_only: false,
  compare_only: false, dry_run: false, strict_race: false,
  bios: Pathname(Dir.home) / "Downloads/SCPH1001.BIN",
  cue: root / "disc/PAL/Rage Racer (Europe).cue",
  native: root / "build-host/rage-racer-smoke"
}

route_presets = {
  "grand-prix" => {
    native_input: "400:START,500:START,650:CROSS,950:CROSS,1100:CROSS," \
                  "1200:CROSS,1469-3000:CROSS",
    psx_input: "0-1000:CROSS"
  },
  "time-attack" => {
    native_input: "400:START,500:START,610:DOWN,650:CROSS,950:CROSS," \
                  "1100:CROSS,1200:CROSS,1264-10000:CROSS",
    psx_input: "0-10000:CROSS"
  }
}.freeze

parser = OptionParser.new do |cli|
  cli.banner = "usage: rage_visual_run.rb --output DIR (--checkpoint STATE | --compare-only) [options]"
  cli.on("--checkpoint PATH", "PSX save state used as the deterministic start") { |v| options[:checkpoint] = v }
  cli.on("--psx-cache DIR", "reuse an existing PSX capture and run only native") do |v|
    options[:psx_cache] = Pathname(v)
  end
  cli.on("--output DIR", "create psx/, native/, compare/ and run.json here") { |v| options[:output] = v }
  cli.on("--profile NAME", %w[all road mirror mirror-road tacho hud]) { |v| options[:profile] = v }
  cli.on("--route NAME", route_presets.keys,
         "input preset used unless an explicit per-runtime script is supplied") do |v|
    options[:route] = v
  end
  cli.on("--timer-min N", Integer) { |v| options[:timer_min] = v }
  cli.on("--timer-max N", Integer) { |v| options[:timer_max] = v }
  cli.on("--capture-stride N", Integer) { |v| options[:capture_stride] = v }
  cli.on("--psx-capture-stride N", Integer,
         "PSX VBlank sampling stride (defaults to 2 for timer coverage)") do |v|
    options[:psx_capture_stride] = v
  end
  cli.on("--native-capture-stride N", Integer,
         "native game-timer sampling stride (defaults to --capture-stride)") do |v|
    options[:native_capture_stride] = v
  end
  cli.on("--psx-frames N", Integer) { |v| options[:psx_frames] = v }
  cli.on("--native-frames N", Integer) { |v| options[:native_frames] = v }
  cli.on("--psx-input SCRIPT") do |v|
    options[:psx_input] = v
    options[:psx_input_explicit] = true
  end
  cli.on("--native-input SCRIPT") do |v|
    options[:native_input] = v
    options[:native_input_explicit] = true
  end
  cli.on("--psx-state-input SCRIPT") { |v| options[:psx_state_input] = v }
  cli.on("--native-state-input SCRIPT") { |v| options[:native_state_input] = v }
  cli.on("--sync-random STATE=SEED",
         "set Random15 seed and optional scenery variants: scene@timer=seed[:v1:v2]") do |v|
    options[:sync_random] = v
  end
  cli.on("--psx-sync-random STATE=SEED", "override the PSX state-tap boundary") do |v|
    options[:psx_sync_random] = v
  end
  cli.on("--native-sync-random STATE=SEED", "override the native state-tap boundary") do |v|
    options[:native_sync_random] = v
  end
  cli.on("--sync-random-each-frame",
         "reset the selected RNG/scenery state before every later scene handler") do
    options[:sync_random_each_frame] = true
  end
  cli.on("--bios PATH") { |v| options[:bios] = Pathname(v) }
  cli.on("--cue PATH") { |v| options[:cue] = Pathname(v) }
  cli.on("--native PATH") { |v| options[:native] = Pathname(v) }
  cli.on("--jobs N", Integer) { |v| options[:jobs] = v }
  cli.on("--top N", Integer) { |v| options[:top] = v }
  cli.on("--match-arg ARG", "append one argument to rage_visual_batch.rb; repeatable") do |v|
    options[:match_args] << v
  end
  cli.on("--budget SPEC", "assert PROFILE.clear/black/needle/rmse/matched_min=VALUE") do |v|
    options[:budgets] << v
  end
  cli.on("--draw-page", "capture the active VRAM drawing page (packet/VRAM diagnosis only)") do
    options[:draw_page] = true
  end
  cli.on("--strict-race",
         "exact final-DMA audit of road, mirror, tachometer and HUD") do
    options[:strict_race] = true
  end
  cli.on("--save-psx-states", "save a PSX state beside every captured reference frame") do
    options[:save_psx_states] = true
  end
  cli.on("--checkpoint-stride N", Integer,
         "save a PSX replay checkpoint every N game-timer ticks") do |v|
    options[:checkpoint_stride] = v
  end
  cli.on("--alignment-only", "capture and report eligible state pairs without image bundles") do
    options[:alignment_only] = true
  end
  cli.on("--compare-only", "reuse existing psx/native captures; do not run either game") do
    options[:compare_only] = true
  end
  cli.on("--dry-run", "write/print the reproducible commands without executing") { options[:dry_run] = true }
end
parser.parse!

if options[:strict_race]
  options[:profile] = "all"
  options[:draw_page] = true
  options[:psx_capture_stride] ||= 1
  options[:match_args].concat(%w[
    --require-main-visible-list --require-mirror-visible-list
    --max-projection-delta 0 --max-mirror-projection-delta 0
    --max-timer-delta 0 --max-anim-timer-delta 0
  ])
  options[:budgets].concat(%w[
    road.black=0 road.clear=0 road.matched_min=1
    mirror-road.black=0 mirror-road.clear=0 mirror-road.matched_min=1
    tacho.needle=0 tacho.matched_min=1 hud.black=0 hud.matched_min=1
  ])
end

if options[:route]
  preset = route_presets.fetch(options[:route])
  options[:psx_input] = preset.fetch(:psx_input) unless options[:psx_input_explicit]
  options[:native_input] = preset.fetch(:native_input) unless options[:native_input_explicit]
end

abort parser.to_s unless options[:output] &&
  (options[:checkpoint] || options[:compare_only] || options[:psx_cache])
abort "timer range is invalid" if options[:timer_min] > options[:timer_max]
abort "capture stride must be positive" unless options[:capture_stride].positive?
options[:psx_capture_stride] ||= 2
options[:native_capture_stride] ||= options[:capture_stride]
abort "PSX capture stride must be positive" unless options[:psx_capture_stride].positive?
abort "native capture stride must be positive" unless options[:native_capture_stride].positive?
abort "checkpoint stride must be positive" if options[:checkpoint_stride] &&
  !options[:checkpoint_stride].positive?

output = Pathname(options[:output]).expand_path
psx_dir = options[:psx_cache] ? options[:psx_cache].expand_path : output / "psx"
native_dir = output / "native"
profiles = options[:profile] == "all" ? %w[road mirror-road tacho hud] : [options[:profile]]
compare_dirs = profiles.to_h { |profile| [profile, output / "compare" / profile] }
FileUtils.mkdir_p([native_dir, *compare_dirs.values])
FileUtils.mkdir_p(psx_dir) unless options[:psx_cache]
if options[:psx_cache] && !(psx_dir / "capture-manifest.csv").file?
  abort "--psx-cache requires #{psx_dir / 'capture-manifest.csv'}"
end

psx_env = {
  "RAGE_EMU_INPUT_SCRIPT" => options[:psx_input],
  "RAGE_EMU_TIMER_FILENAMES" => "1", "RAGE_EMU_CAPTURE_ALL_PHASES" => "1",
  "RAGE_EMU_CAPTURE_TIMER_MIN" => options[:timer_min].to_s,
  "RAGE_EMU_CAPTURE_TIMER_MAX" => options[:timer_max].to_s,
  "RAGE_EMU_CAPTURE_SCENE" => "12",
  "RAGE_EMU_STOP_SCENE" => "12",
  "RAGE_EMU_STOP_SCENE_TIMER" => options[:timer_max].to_s,
  "RUBYOPT" => "--yjit"
}
psx_env["RAGE_EMU_LOAD_STATE"] = Pathname(options[:checkpoint]).expand_path.to_s if options[:checkpoint]
psx_env["RAGE_EMU_CAPTURE_DRAW_PAGE"] = "1" if options[:draw_page]
psx_env["RAGE_EMU_CAPTURE_DMA_PHASES"] = "1" if options[:draw_page]
if options[:strict_race]
  psx_env["RAGE_EMU_CAPTURE_FINAL_DMA_ONLY"] = "1"
  psx_env["RAGE_EMU_CAPTURE_TIMER_STRIDE"] = options[:native_capture_stride].to_s
end
psx_env["RAGE_EMU_CAPTURE_VRAM_SIDECAR"] = "1"
psx_env["RAGE_EMU_SAVE_CAPTURE_STATES"] = "1" if options[:save_psx_states]
psx_env["RAGE_EMU_CHECKPOINT_TIMER_STRIDE"] = options[:checkpoint_stride].to_s if
  options[:checkpoint_stride]
psx_env["RAGE_EMU_STATE_INPUT_SCRIPT"] = options[:psx_state_input] if options[:psx_state_input]
psx_sync_random = options[:psx_sync_random] || options[:sync_random]
psx_env["RAGE_EMU_SYNC_RANDOM"] = psx_sync_random if psx_sync_random
psx_env["RAGE_EMU_SYNC_RANDOM_EACH_FRAME"] = "1" if options[:sync_random_each_frame]
psx_command = ["mise", "exec", "--", "bundle", "exec", "ruby",
               "bin/rage-frame-capture", options[:bios].expand_path.to_s,
               options[:cue].expand_path.to_s, psx_dir.to_s,
               options[:psx_frames].to_s, options[:psx_capture_stride].to_s]

native_env = {
  "SDL_AUDIODRIVER" => "dummy", "RAGE_PORT_SMOKE_FRAMES" => options[:native_frames].to_s,
  "RAGE_PORT_INPUT_SCRIPT" => options[:native_input], "RAGE_PORT_SMOKE_STOP_SCENE" => "12",
  "RAGE_PORT_SMOKE_CAPTURE_DIR" => native_dir.to_s,
  "RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE" => options[:native_capture_stride].to_s,
  "RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN" => options[:timer_min].to_s,
  "RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX" => options[:timer_max].to_s,
  "RAGE_PORT_SMOKE_CAPTURE_SCENE" => "12",
  "RAGE_PORT_SMOKE_STOP_SCENE_TIMER" => options[:timer_max].to_s,
  "RAGE_PORT_SMOKE_CAPTURE_ALL_PHASES" => "1"
}
native_env["RAGE_PORT_CAPTURE_DRAW_PAGE"] = "1" if options[:draw_page]
native_env["RAGE_PORT_CAPTURE_VRAM_SIDECAR"] = "1"
native_env["RAGE_PORT_STATE_INPUT_SCRIPT"] = options[:native_state_input] if options[:native_state_input]
native_sync_random = options[:native_sync_random] || options[:sync_random]
native_env["RAGE_PORT_SYNC_RANDOM"] = native_sync_random if native_sync_random
native_env["RAGE_PORT_SYNC_RANDOM_EACH_FRAME"] = "1" if options[:sync_random_each_frame]
native_command = [options[:native].expand_path.to_s]

batch_commands = profiles.to_h do |profile|
  command = [RbConfig.ruby, (root / "tools/rage_visual_batch.rb").to_s,
             "--psx-dir", psx_dir.to_s, "--native-dir", native_dir.to_s,
             "--output", compare_dirs.fetch(profile).to_s, "--preset", profile,
             "--match", "position", "--visual-refine", options[:draw_page] ? "0" : "3",
             "--require-tacho-flash",
             "--max-display-timer-delta", options[:draw_page] ? "0" : "1",
             "--jobs", options[:jobs].to_s, *options[:match_args]]
  command << "--require-rival-render-state" if %w[road mirror-road].include?(profile)
  command << "--alignment-only" if options[:alignment_only]
  command.concat(["--top", options[:top].to_s]) if options[:top]
  [profile, command]
end

serialize = lambda do |env, command, cwd|
  { cwd: cwd.to_s, env: env, argv: command,
    shell: ([*env.map { |k, v| "#{k}=#{Shellwords.escape(v)}" },
             *command.map { |v| Shellwords.escape(v) }]).join(" ") }
end
metadata = {
  created_at: Time.now.iso8601, profile: options[:profile],
  budgets: options[:budgets],
  alignment_only: options[:alignment_only],
  compare_only: options[:compare_only],
  psx_cache: options[:psx_cache]&.expand_path&.to_s,
  timer_range: [options[:timer_min], options[:timer_max]],
  capture_strides: {
    psx_vblank: options[:psx_capture_stride],
    native_timer: options[:native_capture_stride]
  },
  draw_page: options[:draw_page], strict_race: options[:strict_race],
  psx: serialize.call(psx_env, psx_command, root / "tools/psx-ruby"),
  save_psx_states: options[:save_psx_states],
  checkpoint_stride: options[:checkpoint_stride],
  native: serialize.call(native_env, native_command, root),
  comparisons: batch_commands.transform_values { |command| serialize.call({}, command, root) }
}
metadata_path = output / (options[:compare_only] ? "compare-run.json" : "run.json")
File.write(metadata_path, JSON.pretty_generate(metadata) + "\n")

if options[:dry_run]
  puts JSON.pretty_generate(metadata)
  exit
end

run_capture = lambda do |name, env, command, cwd, log_path|
  log = File.open(log_path, "w")
  pid = Process.spawn(env, *command, chdir: cwd.to_s, out: log, err: [:child, :out])
  [name, pid, log]
end
unless options[:compare_only]
  captures = []
  captures << run_capture.call(
    "psx", psx_env, psx_command, root / "tools/psx-ruby", psx_dir / "run.log"
  ) unless options[:psx_cache]
  captures << run_capture.call(
    "native", native_env, native_command, root, native_dir / "run.log"
  )
  failures = captures.map do |name, pid, log|
    _waited, status = Process.wait2(pid)
    log.close
    status.success? ? nil : "#{name} capture failed with #{status.exitstatus} (#{log.path})"
  end.compact
  abort failures.join("\n") unless failures.empty?
else
  [psx_dir, native_dir].each do |directory|
    manifest = directory / "capture-manifest.csv"
    abort "--compare-only requires existing #{manifest}" unless manifest.file?
  end
end

comparison_results = {}
comparison_failures = []
batch_commands.each do |profile, command|
  stdout, stderr, status = Open3.capture3(*command, chdir: root.to_s)
  unless status.success?
    message = "#{profile} comparison failed:\n#{stdout}#{stderr}"
    comparison_failures << message
    comparison_results[profile] = {
      matched_frames: 0, failed: true, error: (stdout + stderr).strip
    }
    next
  end
  print stdout
  warn stderr unless stderr.empty?
  summary = JSON.parse(File.read(compare_dirs.fetch(profile) / "summary.json"))
  frames = summary.fetch("frames")
  if options[:alignment_only]
    comparison_results[profile] = {
      matched_frames: summary.fetch("matched_frames"),
      rejected_frames: summary.fetch("rejected_frames"),
      frames: frames
    }
    next
  end
  worst_clear = frames.max_by { |frame| frame.fetch("native_only_clear") }
  worst_black = frames.max_by { |frame| frame.fetch("native_only_black") }
  worst_clear_component = frames.max_by do |frame|
    frame.fetch("native_only_clear_component", 0)
  end
  worst_black_component = frames.max_by do |frame|
    frame.fetch("native_only_black_component", 0)
  end
  worst_needle = frames.max_by do |frame|
    frame.dig("tachometer_needle", "mismatch_pixels") || 0
  end
  comparison_results[profile] = {
    matched_frames: summary.fetch("matched_frames"),
    coverage: {
      timer: frames.map { |frame| frame.dig("state", "timer") }.compact.minmax,
      progress: frames.map { |frame| frame.dig("state", "progress") }.compact.minmax,
      courses: frames.map { |frame| frame.dig("state", "course_index") }.compact.uniq.sort,
      laps: frames.map { |frame| frame.dig("state", "lap") }.compact.uniq.sort
    },
    maximum_clear: frames.map { |frame| frame.fetch("native_only_clear") }.max || 0,
    maximum_black: frames.map { |frame| frame.fetch("native_only_black") }.max || 0,
    maximum_clear_component: frames.map { |frame| frame.fetch("native_only_clear_component", 0) }.max || 0,
    maximum_black_component: frames.map { |frame| frame.fetch("native_only_black_component", 0) }.max || 0,
    maximum_needle_mismatch: frames.map { |frame| frame.dig("tachometer_needle", "mismatch_pixels") || 0 }.max || 0,
    worst_region_rmse: frames.map { |frame| frame["normalized_region_rmse"] || frame["normalized_rmse"] }.max,
    maximum_clear_frame: worst_clear && { frame: worst_clear["frame"], state_delta: worst_clear["state_delta"] },
    maximum_black_frame: worst_black && { frame: worst_black["frame"], state_delta: worst_black["state_delta"] },
    maximum_clear_component_frame: worst_clear_component &&
      { frame: worst_clear_component["frame"], state_delta: worst_clear_component["state_delta"] },
    maximum_black_component_frame: worst_black_component &&
      { frame: worst_black_component["frame"], state_delta: worst_black_component["state_delta"] },
    maximum_needle_frame: worst_needle && { frame: worst_needle["frame"], state_delta: worst_needle["state_delta"],
                                           tacho: worst_needle["tachometer_needle"] }
  }
end
File.write(output / "summary.json", JSON.pretty_generate(comparison_results) + "\n")
unless comparison_failures.empty?
  warn comparison_failures.join("\n")
  exit 1
end
unless options[:budgets].empty?
  budget_command = [RbConfig.ruby, (root / "tools/rage_visual_budget.rb").to_s,
                    "--summary", (output / "summary.json").to_s,
                    *options[:budgets].flat_map { |budget| ["--budget", budget] }]
  system(*budget_command, chdir: root.to_s)
  exit 1 unless $?.success?
end
