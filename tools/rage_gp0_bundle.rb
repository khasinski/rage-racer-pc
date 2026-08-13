#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "json"
require "open3"
require "optparse"
require "pathname"
require "tmpdir"
require "csv"

options = { keep_temp: false, ot_trace: false, texel_trace: false,
            resync_window: nil }
OptionParser.new do |parser|
  parser.banner = "usage: rage_gp0_bundle.rb --bundle DIR [options]"
  parser.on("--bundle DIR", "visual comparison frame bundle") { |v| options[:bundle] = v }
  parser.on("--keep-temp", "retain replay capture directories") { options[:keep_temp] = true }
  parser.on("--trace-only", "reuse existing GP0/VRAM artifacts and run only --pixel tracing") do
    options[:trace_only] = true
  end
  parser.on("--raster-only", "reuse existing GP0/VRAM artifacts and rerun only isolated raster comparison") do
    options[:raster_only] = true
  end
  parser.on("--pixel X,Y", "rerun the selected frame with packet coverage tracing") do |value|
    abort "invalid --pixel #{value.inspect}" unless value.match?(/\A\d+,\d+\z/)
    options[:pixel] = value
  end
  parser.on("--texel", "include actual PSX raster texel/CLUT tracing (requires --pixel)") do
    options[:texel_trace] = true
  end
  parser.on("--ot", "include ordering-table node tracing") { options[:ot_trace] = true }
  parser.on("--dump-psx-packet N", Integer,
            "dump retail VRAM immediately before GP0 packet N") do |value|
    options[:dump_psx_packet] = value
  end
  parser.on("--resync-window N", Integer, "find the next matching packet after a divergence") do |value|
    options[:resync_window] = value
  end
end.parse!
abort "--bundle is required" unless options[:bundle]
abort "--texel requires --pixel" if options[:texel_trace] && !options[:pixel]
abort "--trace-only requires --pixel" if options[:trace_only] && !options[:pixel]
abort "--raster-only cannot be combined with --pixel or --trace-only" if
  options[:raster_only] && (options[:pixel] || options[:trace_only])

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
  basename = File.basename(path)
  match = basename.match(/-f(\d+)(?:-d\d+)?-s\d+\.ppm\z/) ||
          basename.match(/\Async-(\d+)-t\d+-s\d+\.ppm\z/)
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
native_scene = Integer(native_row.fetch("scene"), 10)
native_timer = Integer(native_row.fetch("timer"), 10)
# PsyZ tags commands with the presentation interval in which their completed
# framebuffer is captured.  Display and draw-page manifests therefore both
# map to the filename's native frame; subtracting one selects stale geometry.
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
    "RAGE_GPU_GP0_TRACE" => "1",
    "RAGE_GPU_VRAM_TRANSFER_TRACE" => "1",
    "RAGE_GPU_GP0_TRACE_SCENE" => native_scene.to_s,
    "RAGE_GPU_GP0_TRACE_TIMER" => native_timer.to_s
  )
  env["RAGE_GPU_OT_TRACE"] = "1" if options[:ot_trace]
  if side == "psx"
    # A nearby checkpoint may resume with the target timer already visible at
    # the first VBlank. The original capture's stop condition would then exit
    # before that frame submits GP0 commands. Replay length is already bounded
    # by argv, so never inherit the terminal timer stop here.
    env.delete("RAGE_EMU_STOP_SCENE")
    env.delete("RAGE_EMU_STOP_SCENE_TIMER")
    env["RAGE_GPU_GP0_TRACE_VRAM"] = (bundle / "gp0-pre.vram").to_s
    env["RAGE_GPU_GP0_TRACE_VRAM_POST"] = (bundle / "gp0-post.vram").to_s
    if options[:dump_psx_packet]
      env["RAGE_GPU_GP0_TRACE_DUMP_PACKET"] = options[:dump_psx_packet].to_s
      env["RAGE_GPU_GP0_TRACE_DUMP_VRAM"] =
        (bundle / format("gp0-before-%04d.vram", options[:dump_psx_packet])).to_s
    end
  else
    env["RAGE_GPU_GP0_TRACE_VRAM"] = (bundle / "gp0-native-post.vram").to_s
  end
  argv = metadata.fetch("argv").dup
  if side == "psx"
    output_index = argv.index("bin/rage-frame-capture") + 3
    argv[output_index] = (replay_root / "psx-capture").to_s
    if psx_replay_frames && psx_replay_state
      env["RAGE_EMU_LOAD_STATE"] = psx_replay_state
      argv[output_index + 1] = (psx_replay_frames + 2).to_s
      argv[output_index + 2] = (psx_replay_frames + 2).to_s
    else
      # The visual filename's frame is relative to the same checkpoint used by
      # this command.  The original capture often ran thousands of VBlanks
      # beyond it; repeating that tail for every GP0/pixel pass only wastes
      # emulator time and can leave a long-running child after an interrupted
      # diagnostic.  Two extra VBlanks retain the existing boundary tolerance.
      argv[output_index + 1] = (frame + 2).to_s
    end
  else
    env["RAGE_PORT_SMOKE_CAPTURE_DIR"] = (replay_root / "native-capture").to_s
    env["RAGE_PORT_SMOKE_FRAMES"] = (frame + 2).to_s
  end
  FileUtils.mkdir_p(env[side == "psx" ? "RAGE_EMU_CAPTURE_DIR" : "RAGE_PORT_SMOKE_CAPTURE_DIR"].to_s) unless side == "psx"
  [env, argv, metadata.fetch("cwd"), bundle / "gp0-#{side}.log"]
end

execute_replays = lambda do |suffix, frame_filters = nil|
  runs = { "psx" => psx_frame, "native" => native_frame }.map do |side, frame|
    env, argv, cwd, default_log_path = prepare.call(side, frame)
    log_path = suffix.empty? ? default_log_path : bundle / "#{suffix}-#{side}.log"
    if frame_filters
      env["RAGE_GPU_TRACE_PIXEL"] = options.fetch(:pixel)
      env["RAGE_GPU_TRACE_FRAME"] = frame_filters.fetch(side).to_s
      env["RAGE_GPU_TRACE_TEXEL"] = "1" if options[:texel_trace]
    end
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
end

execute_replays.call("") unless options[:trace_only] || options[:raster_only]

expected_vram_bytes = 1024 * 512 * 2
{
  "retail pre-frame VRAM" => bundle / "gp0-pre.vram",
  "retail post-frame VRAM" => bundle / "gp0-post.vram",
  "native post-frame VRAM" => bundle / "gp0-native-post.vram"
}.each do |label, path|
  abort "missing #{label}: #{path}" unless path.file?
  abort "invalid #{label} size #{path.size}, expected #{expected_vram_bytes}: #{path}" unless
    path.size == expected_vram_bytes
end

native_frames = File.foreach(bundle / "gp0-native.log").map do |line|
  next unless line.start_with?("gp0-packet ") &&
              line[/\bscene=(-?\d+)/, 1].to_i == native_scene &&
              line[/\btimer=(-?\d+)/, 1].to_i == native_timer
  line[/\bframe=(\d+)/, 1]&.to_i
end.compact.uniq
abort "native replay submitted no GP0 commands for scene=#{native_scene} timer=#{native_timer}" if native_frames.empty?
native_frame = native_frames.min_by { |frame| [(frame - native_frame).abs, frame] }

native_draw_area = File.foreach(bundle / "gp0-native.log").map do |line|
    next unless line.start_with?("gp0-packet ") &&
                line[/\bframe=(\d+)/, 1].to_i == native_frame &&
                line[/\bscene=(-?\d+)/, 1].to_i == native_scene &&
                line[/\btimer=(-?\d+)/, 1].to_i == native_timer
    words = line[/\bwords=([0-9a-fA-F,]+)/, 1]
    next unless words
    words.split(",").map { |word| Integer(word, 16) }.find { |word| (word >> 24) == 0xe3 }
end.compact.first
psx_draw_areas = {}
psx_contexts = {}
available_frames = File.foreach(bundle / "gp0-psx.log").map do |line|
    match = line.match(/\Agp0-command frame=(\d+)\b/)
    next unless match
    frame = Integer(match[1], 10)
    scene = line[/\bscene=(-?\d+)/, 1]
    timer = line[/\btimer=(-?\d+)/, 1]
    psx_contexts[frame] ||= [scene.to_i, timer.to_i] if scene && timer
    words = line[/\bwords=([0-9a-fA-F,]+)/, 1]
    if words
      draw_area = words.split(",").map { |word| Integer(word, 16) }.find { |word| (word >> 24) == 0xe3 }
      psx_draw_areas[frame] ||= draw_area if draw_area
    end
    frame
end.compact.uniq
abort "replayed PSX run submitted no GP0 commands" if available_frames.empty?
matching_surface = native_draw_area && available_frames.select do |frame|
  psx_draw_areas[frame] == native_draw_area &&
    psx_contexts[frame] == [native_scene, native_timer]
end
if native_draw_area && matching_surface.empty? && available_frames.length > 1
  abort "PSX replay has no scene=#{native_scene} timer=#{native_timer} frame " \
        "on native draw page #{format('%08x', native_draw_area)}"
end
candidates = matching_surface.empty? ? available_frames : matching_surface
psx_frame = candidates.min_by { |frame| [(frame - psx_frame).abs, frame] }

compare = [RbConfig.ruby, (root / "tools/rage_gp0_compare.rb").to_s,
           "--psx", (bundle / "gp0-psx.log").to_s,
           "--native", (bundle / "gp0-native.log").to_s,
           "--psx-frame", psx_frame.to_s, "--native-frame", native_frame.to_s]
compare.concat(["--resync-window", options[:resync_window].to_s]) if options[:resync_window]
if options[:trace_only] || options[:raster_only]
  status = Struct.new(:success?).new(true)
else
  output, status = Open3.capture2e(*compare, chdir: root.to_s)
  File.write(bundle / "gp0-diff.txt", output)
  puts output
end
vram_compare = [RbConfig.ruby, (root / "tools/rage_vram_refs.rb").to_s,
                "--log", (bundle / "gp0-native.log").to_s,
                "--frame", native_frame.to_s,
                "--retail", (bundle / "gp0-post.vram").to_s,
                "--native", (bundle / "gp0-native-post.vram").to_s]
unless options[:trace_only] || options[:raster_only]
  vram_output, vram_status = Open3.capture2e(*vram_compare, chdir: root.to_s)
  abort vram_output unless vram_status.success?
  File.write(bundle / "gp0-vram-refs.txt", vram_output)
  puts vram_output
end
unless options[:trace_only]
  raster_dir = bundle / "gp0-raster"
  raster_compare = [
    RbConfig.ruby, (root / "tools/rage_gp0_raster_compare.rb").to_s,
    "--vram", (bundle / "gp0-pre.vram").to_s,
    "--log", (bundle / "gp0-native.log").to_s,
    "--frame", native_frame.to_s,
    "--output", raster_dir.to_s
  ]
  raster_output, raster_status = Open3.capture2e(*raster_compare, chdir: root.to_s)
  File.write(bundle / "gp0-raster.txt", raster_output)
  abort "isolated GP0 raster comparison failed:\n#{raster_output}" unless
    raster_status.success?
  puts raster_output
end
if options[:pixel]
  execute_replays.call("pixel", { "psx" => psx_frame, "native" => native_frame })
  puts "Pixel trace written to #{bundle / 'pixel-psx.log'} and #{bundle / 'pixel-native.log'}"
end
puts "GP0 logs written to #{bundle}"
FileUtils.remove_entry(replay_root) unless options[:keep_temp]
exit(status.success? ? 0 : 1)
