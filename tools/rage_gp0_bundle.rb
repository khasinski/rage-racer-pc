#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "json"
require "open3"
require "optparse"
require "pathname"
require "tmpdir"
require "csv"

options = { keep_temp: false }
OptionParser.new do |parser|
  parser.banner = "usage: rage_gp0_bundle.rb --bundle DIR [options]"
  parser.on("--bundle DIR", "visual comparison frame bundle") { |v| options[:bundle] = v }
  parser.on("--keep-temp", "retain replay capture directories") { options[:keep_temp] = true }
end.parse!
abort "--bundle is required" unless options[:bundle]

bundle = Pathname(options[:bundle]).expand_path
report_path = bundle / "report.json"
abort "missing #{report_path}" unless report_path.file?
report = JSON.parse(File.read(report_path))
summary_path = bundle.parent / "summary.json"
abort "missing #{summary_path}" unless summary_path.file?
summary_frame = JSON.parse(File.read(summary_path)).fetch("frames").find do |frame|
  frame.fetch("bundle") == bundle.to_s || frame.fetch("frame") == bundle.basename.to_s
end
abort "bundle is absent from #{summary_path}" unless summary_frame

parse_frame = lambda do |path|
  match = File.basename(path).match(/-f(\d+)-s\d+\.ppm\z/)
  abort "cannot parse frame from #{path}" unless match
  Integer(match[1], 10)
end
psx_frame = parse_frame.call(report.fetch("psx_source"))
native_frame = parse_frame.call(report.fetch("native_source"))
native_manifest = CSV.read(
  Pathname(report.fetch("native_source")).dirname / "capture-manifest.csv",
  headers: true
)
native_row = native_manifest.find do |row|
  row["filename"] == File.basename(report.fetch("native_source"))
end
abort "native capture is absent from its manifest" unless native_row
native_frame -= 1 if native_row["capture_surface"] == "display"
psx_replay_frames = summary_frame["psx_replay_frames"]
psx_replay_state = summary_frame["psx_replay_pre_state"]
if psx_replay_frames && psx_replay_state
  # Checkpoints may straddle the VBlank/DMA boundary.  Use the replay distance
  # only as a target; after replay we select the nearest frame that actually
  # submitted GP0 commands.
  psx_frame = Integer(psx_replay_frames)
end

run_path = bundle.ascend.map { |dir| dir / "run.json" }.find(&:file?)
abort "no parent run.json for #{bundle}" unless run_path
run = JSON.parse(File.read(run_path))
root = Pathname(__dir__).parent.expand_path

replay_root = options[:keep_temp] ? bundle / "gp0-replay" : Pathname(Dir.mktmpdir("rage-gp0-replay-"))
FileUtils.mkdir_p(replay_root)

prepare = lambda do |side, frame|
  metadata = run.fetch(side)
  env = metadata.fetch("env").merge(
    "RAGE_GPU_GP0_TRACE" => "1", "RAGE_GPU_GP0_TRACE_FRAME" => frame.to_s
  )
  argv = metadata.fetch("argv").dup
  if side == "psx"
    output_index = argv.index("bin/rage-frame-capture") + 3
    argv[output_index] = (replay_root / "psx-capture").to_s
    if psx_replay_frames && psx_replay_state
      env.delete("RAGE_GPU_GP0_TRACE_FRAME")
      env["RAGE_EMU_LOAD_STATE"] = psx_replay_state
      argv[output_index + 1] = (psx_replay_frames + 2).to_s
      argv[output_index + 2] = (psx_replay_frames + 2).to_s
    end
  else
    env["RAGE_PORT_SMOKE_CAPTURE_DIR"] = (replay_root / "native-capture").to_s
  end
  FileUtils.mkdir_p(env[side == "psx" ? "RAGE_EMU_CAPTURE_DIR" : "RAGE_PORT_SMOKE_CAPTURE_DIR"].to_s) unless side == "psx"
  [env, argv, metadata.fetch("cwd"), bundle / "gp0-#{side}.log"]
end

runs = { "psx" => psx_frame, "native" => native_frame }.map do |side, frame|
  env, argv, cwd, log_path = prepare.call(side, frame)
  log = File.open(log_path, "w")
  pid = Process.spawn(env, *argv, chdir: cwd, out: log, err: [:child, :out])
  [side, pid, log]
end
failures = runs.map do |side, pid, log|
  _, status = Process.wait2(pid)
  log.close
  "#{side} replay failed (#{log.path})" unless status.success?
end.compact
abort failures.join("\n") unless failures.empty?

if psx_replay_frames && psx_replay_state
  available_frames = File.foreach(bundle / "gp0-psx.log").map do |line|
    match = line.match(/\Agp0-command frame=(\d+)\b/)
    Integer(match[1], 10) if match
  end.compact.uniq
  abort "replayed PSX checkpoint submitted no GP0 commands" if available_frames.empty?
  psx_frame = available_frames.min_by { |frame| [(frame - psx_frame).abs, frame] }
end

compare = [RbConfig.ruby, (root / "tools/rage_gp0_compare.rb").to_s,
           "--psx", (bundle / "gp0-psx.log").to_s,
           "--native", (bundle / "gp0-native.log").to_s,
           "--psx-frame", psx_frame.to_s, "--native-frame", native_frame.to_s]
output, status = Open3.capture2e(*compare, chdir: root.to_s)
File.write(bundle / "gp0-diff.txt", output)
puts output
puts "GP0 logs written to #{bundle}"
FileUtils.remove_entry(replay_root) unless options[:keep_temp]
exit(status.success? ? 0 : 1)
