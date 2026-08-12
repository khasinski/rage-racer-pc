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
  psx_frames: 500, native_frames: 2400, jobs: 8, top: nil,
  psx_input: "0-1000:CROSS",
  native_input: "400:START,500:START,650:CROSS,950:CROSS,1100:CROSS," \
                "1200:CROSS,1469-3000:CROSS",
  match_args: [], dry_run: false,
  bios: Pathname(Dir.home) / "Downloads/SCPH1001.BIN",
  cue: root / "disc/PAL/Rage Racer (Europe).cue",
  native: root / "build-host/rage-racer-smoke"
}

parser = OptionParser.new do |cli|
  cli.banner = "usage: rage_visual_run.rb --checkpoint STATE --output DIR [options]"
  cli.on("--checkpoint PATH", "PSX save state used as the deterministic start") { |v| options[:checkpoint] = v }
  cli.on("--output DIR", "create psx/, native/, compare/ and run.json here") { |v| options[:output] = v }
  cli.on("--profile NAME", %w[all road mirror mirror-road tacho hud]) { |v| options[:profile] = v }
  cli.on("--timer-min N", Integer) { |v| options[:timer_min] = v }
  cli.on("--timer-max N", Integer) { |v| options[:timer_max] = v }
  cli.on("--capture-stride N", Integer) { |v| options[:capture_stride] = v }
  cli.on("--psx-frames N", Integer) { |v| options[:psx_frames] = v }
  cli.on("--native-frames N", Integer) { |v| options[:native_frames] = v }
  cli.on("--psx-input SCRIPT") { |v| options[:psx_input] = v }
  cli.on("--native-input SCRIPT") { |v| options[:native_input] = v }
  cli.on("--bios PATH") { |v| options[:bios] = Pathname(v) }
  cli.on("--cue PATH") { |v| options[:cue] = Pathname(v) }
  cli.on("--native PATH") { |v| options[:native] = Pathname(v) }
  cli.on("--jobs N", Integer) { |v| options[:jobs] = v }
  cli.on("--top N", Integer) { |v| options[:top] = v }
  cli.on("--match-arg ARG", "append one argument to rage_visual_batch.rb; repeatable") do |v|
    options[:match_args] << v
  end
  cli.on("--dry-run", "write/print the reproducible commands without executing") { options[:dry_run] = true }
end
parser.parse!

abort parser.to_s unless options[:checkpoint] && options[:output]
abort "timer range is invalid" if options[:timer_min] > options[:timer_max]
abort "capture stride must be positive" unless options[:capture_stride].positive?

output = Pathname(options[:output]).expand_path
psx_dir = output / "psx"
native_dir = output / "native"
profiles = options[:profile] == "all" ? %w[road mirror-road tacho hud] : [options[:profile]]
compare_dirs = profiles.to_h { |profile| [profile, output / "compare" / profile] }
FileUtils.mkdir_p([psx_dir, native_dir, *compare_dirs.values])

psx_env = {
  "RAGE_EMU_LOAD_STATE" => Pathname(options[:checkpoint]).expand_path.to_s,
  "RAGE_EMU_INPUT_SCRIPT" => options[:psx_input],
  "RAGE_EMU_TIMER_FILENAMES" => "1", "RAGE_EMU_CAPTURE_ALL_PHASES" => "1",
  "RAGE_EMU_CAPTURE_DRAW_PAGE" => "1",
  "RAGE_EMU_CAPTURE_TIMER_MIN" => options[:timer_min].to_s,
  "RAGE_EMU_CAPTURE_TIMER_MAX" => options[:timer_max].to_s,
  "RUBYOPT" => "--yjit"
}
psx_command = ["mise", "exec", "--", "bundle", "exec", "ruby",
               "bin/rage-frame-capture", options[:bios].expand_path.to_s,
               options[:cue].expand_path.to_s, psx_dir.to_s,
               options[:psx_frames].to_s, options[:capture_stride].to_s]

native_env = {
  "SDL_AUDIODRIVER" => "dummy", "RAGE_PORT_SMOKE_FRAMES" => options[:native_frames].to_s,
  "RAGE_PORT_INPUT_SCRIPT" => options[:native_input], "RAGE_PORT_SMOKE_STOP_SCENE" => "12",
  "RAGE_PORT_CAPTURE_DRAW_PAGE" => "1", "RAGE_PORT_SMOKE_CAPTURE_DIR" => native_dir.to_s,
  "RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE" => options[:capture_stride].to_s,
  "RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN" => options[:timer_min].to_s,
  "RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX" => options[:timer_max].to_s,
  "RAGE_PORT_SMOKE_CAPTURE_ALL_PHASES" => "1"
}
native_command = [options[:native].expand_path.to_s]

batch_commands = profiles.to_h do |profile|
  command = [RbConfig.ruby, (root / "tools/rage_visual_batch.rb").to_s,
             "--psx-dir", psx_dir.to_s, "--native-dir", native_dir.to_s,
             "--output", compare_dirs.fetch(profile).to_s, "--preset", profile,
             "--match", "position", "--visual-refine", "0",
             "--jobs", options[:jobs].to_s, *options[:match_args]]
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
  timer_range: [options[:timer_min], options[:timer_max]],
  draw_page: true, psx: serialize.call(psx_env, psx_command, root / "tools/psx-ruby"),
  native: serialize.call(native_env, native_command, root),
  comparisons: batch_commands.transform_values { |command| serialize.call({}, command, root) }
}
File.write(output / "run.json", JSON.pretty_generate(metadata) + "\n")

if options[:dry_run]
  puts JSON.pretty_generate(metadata)
  exit
end

run_capture = lambda do |name, env, command, cwd, log_path|
  log = File.open(log_path, "w")
  pid = Process.spawn(env, *command, chdir: cwd.to_s, out: log, err: [:child, :out])
  [name, pid, log]
end
captures = [
  run_capture.call("psx", psx_env, psx_command, root / "tools/psx-ruby", psx_dir / "run.log"),
  run_capture.call("native", native_env, native_command, root, native_dir / "run.log")
]
failures = captures.map do |name, pid, log|
  _waited, status = Process.wait2(pid)
  log.close
  status.success? ? nil : "#{name} capture failed with #{status.exitstatus} (#{log.path})"
end.compact
abort failures.join("\n") unless failures.empty?

comparison_results = {}
batch_commands.each do |profile, command|
  stdout, stderr, status = Open3.capture3(*command, chdir: root.to_s)
  abort "#{profile} comparison failed:\n#{stdout}#{stderr}" unless status.success?
  print stdout
  warn stderr unless stderr.empty?
  summary = JSON.parse(File.read(compare_dirs.fetch(profile) / "summary.json"))
  frames = summary.fetch("frames")
  worst_clear = frames.max_by { |frame| frame.fetch("native_only_clear") }
  worst_black = frames.max_by { |frame| frame.fetch("native_only_black") }
  worst_needle = frames.max_by do |frame|
    frame.dig("tachometer_needle", "mismatch_pixels") || 0
  end
  comparison_results[profile] = {
    matched_frames: summary.fetch("matched_frames"),
    maximum_clear: frames.map { |frame| frame.fetch("native_only_clear") }.max || 0,
    maximum_black: frames.map { |frame| frame.fetch("native_only_black") }.max || 0,
    maximum_needle_mismatch: frames.map { |frame| frame.dig("tachometer_needle", "mismatch_pixels") || 0 }.max || 0,
    worst_region_rmse: frames.map { |frame| frame["normalized_region_rmse"] || frame["normalized_rmse"] }.max,
    maximum_clear_frame: worst_clear && { frame: worst_clear["frame"], state_delta: worst_clear["state_delta"] },
    maximum_black_frame: worst_black && { frame: worst_black["frame"], state_delta: worst_black["state_delta"] },
    maximum_needle_frame: worst_needle && { frame: worst_needle["frame"], state_delta: worst_needle["state_delta"],
                                           tacho: worst_needle["tachometer_needle"] }
  }
end
File.write(output / "summary.json", JSON.pretty_generate(comparison_results) + "\n")
