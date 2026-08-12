#!/usr/bin/env ruby
# frozen_string_literal: true

# Pair timer-named emulator/native captures and build per-frame visual bundles.
# The emulator sequence is expensive and can remain cached while the native
# directory is replaced after each renderer change.

require "fileutils"
require "json"
require "open3"
require "optparse"
require "pathname"
require "rbconfig"
require "etc"

REGION_PRESETS = {
  "road" => "0,55,250,150",
  "mirror" => "84,16,152,40",
  "mirror-road" => "84,40,152,16",
  "mirror-frame" => "80,12,160,48",
  "rank" => "0,0,84,64",
  "record" => "236,0,84,64",
  "tacho" => "225,145,95,95",
  "time" => "0,188,125,52",
  "hud" => "0,176,320,64"
}.freeze

options = { region: nil, hotspots: 8, radius: 12, match: "timer",
            max_position_distance: 64.0, max_view_distance: 32.0,
            max_speed_delta: 16, max_angle_delta: 32,
            max_lateral_delta: 32, max_rival_distance: Float::INFINITY,
            visual_refine: 0,
            clear_region: nil, black_region: nil, needle_region: nil,
            artifact_radius: 2, rank: "rmse",
            jobs: [Etc.nprocessors, 8].min, top: nil }
OptionParser.new do |parser|
  parser.banner = "usage: rage_visual_batch.rb --psx-dir DIR --native-dir DIR --output DIR [options]"
  parser.on("--psx-dir DIR") { |value| options[:psx_dir] = value }
  parser.on("--native-dir DIR") { |value| options[:native_dir] = value }
  parser.on("--output DIR") { |value| options[:output] = value }
  parser.on("--region X,Y,W,H") { |value| options[:region] = value }
  parser.on("--preset NAME", REGION_PRESETS.keys,
            "named region: #{REGION_PRESETS.keys.join(', ')}") do |value|
    options[:region] = REGION_PRESETS.fetch(value)
  end
  parser.on("--clear-region X,Y,W,H") { |value| options[:clear_region] = value }
  parser.on("--black-region X,Y,W,H") { |value| options[:black_region] = value }
  parser.on("--needle-region X,Y,W,H") { |value| options[:needle_region] = value }
  parser.on("--artifact-radius N", Integer) do |value|
    options[:artifact_radius] = value
  end
  parser.on("--hotspots N", Integer) { |value| options[:hotspots] = value }
  parser.on("--hotspot-radius N", Integer) { |value| options[:radius] = value }
  parser.on("--match MODE", %w[timer position],
            "pair by identical timer filename or nearest runtime position") do |value|
    options[:match] = value
  end
  parser.on("--max-position-distance N", Float,
            "skip position matches farther apart than N world units") do |value|
    options[:max_position_distance] = value
  end
  parser.on("--max-view-distance N", Float,
            "skip camera positions farther apart than N world units") do |value|
    options[:max_view_distance] = value
  end
  parser.on("--max-speed-delta N", Integer,
            "skip states whose car speeds differ by more than N") do |value|
    options[:max_speed_delta] = value
  end
  parser.on("--max-angle-delta N", Integer,
            "skip states whose body pitch/roll/yaw differ by more than N") do |value|
    options[:max_angle_delta] = value
  end
  parser.on("--max-lateral-delta N", Integer,
            "skip track lateral offsets farther apart than N") do |value|
    options[:max_lateral_delta] = value
  end
  parser.on("--max-rival-distance N", Float,
            "skip matches whose four rivals exceed this aggregate X/Z distance") do |value|
    options[:max_rival_distance] = value
  end
  parser.on("--visual-refine N", Integer,
            "choose the lowest-RMSE native image within N timer ticks") do |value|
    options[:visual_refine] = value
  end
  parser.on("--rank METRIC", %w[rmse clear black needle],
            "rank output by RMSE, clear, black, or needle mismatch") do |value|
    options[:rank] = value
  end
  parser.on("--jobs N", Integer, "parallel frame comparisons") do |value|
    options[:jobs] = value
  end
  parser.on("--top N", Integer, "print only the N worst frames") do |value|
    options[:top] = value
  end
end.parse!

abort "--psx-dir, --native-dir and --output are required" unless
  options.values_at(:psx_dir, :native_dir, :output).all?
abort "--jobs must be positive" unless options[:jobs].positive?
abort "--top must be positive" if options[:top] && !options[:top].positive?

psx_dir = Pathname(options[:psx_dir]).expand_path
native_dir = Pathname(options[:native_dir]).expand_path
output = Pathname(options[:output]).expand_path
FileUtils.mkdir_p(output)

def manifest_rows(directory)
  path = directory / "capture-manifest.csv"
  abort "missing capture manifest: #{path}" unless path.file?
  lines = File.readlines(path, chomp: true)
  header = lines.shift&.split(",")&.map(&:to_sym)
  abort "empty capture manifest: #{path}" unless header
  lines.map do |line|
    values = line.split(",", -1)
    row = header.zip(values).to_h
    # Manifests may carry opaque diagnostic payloads (for example complete
    # car structs encoded as hex). Matching only consumes scalar state fields;
    # keep every other column verbatim so extending capture diagnostics cannot
    # break the visual pipeline.
    numeric_fields = %i[
      frame scene timer x z speed progress lap body_yaw body_pitch body_roll
      track_lateral model_yaw mirror_y view_x view_y view_z view_angle_x
      view_angle_y view_angle_z environment_mode4 scratch_env_mode4
      random_seed anim_timer rival0_x rival0_z rival1_x rival1_z rival2_x
      rival2_z rival3_x rival3_z rival0_speed rival0_progress rival0_yaw
      rival0_lateral rival0_collision rival0_active rival1_speed
      rival1_progress rival1_yaw rival1_lateral rival1_collision rival1_active
      rival2_speed rival2_progress rival2_yaw rival2_lateral rival2_collision
      rival2_active rival3_speed rival3_progress rival3_yaw rival3_lateral
      rival3_collision rival3_active
    ]
    numeric_fields.each do |key|
      row[key] = Integer(row[key]) if row.key?(key) && !row[key].empty?
    end
    image = directory / row[:filename]
    image.file? ? row.merge(path: image) : nil
  end.compact
end

def image_rmse(psx, native, region)
  command = ["magick", "compare", "-metric", "RMSE"]
  if region
    x, y, width, height = region.split(",", 4).map { |part| Integer(part) }
    command.concat(["-extract", "#{width}x#{height}+#{x}+#{y}"])
  end
  command.concat([psx.to_s, native.to_s, "null:"])
  _stdout, metric, status = Open3.capture3(*command)
  abort "visual alignment failed for #{native}: #{metric}" unless
    [0, 1].include?(status.exitstatus)
  metric[/\(([0-9.eE+-]+)\)/, 1]&.to_f || Float::INFINITY
end

if options[:match] == "position"
  psx_states = manifest_rows(psx_dir)
  native_states = manifest_rows(native_dir)
  abort "capture manifest contains no usable frames" if psx_states.empty? || native_states.empty?
  pairs = psx_states.map do |psx|
    candidates = native_states.select do |native|
      native[:scene] == psx[:scene] && native[:lap] == psx[:lap]
    end
    abort "no native state candidate for #{psx[:filename]}" if candidates.empty?
    native_state = candidates.min_by do |candidate|
      dx = candidate[:x] - psx[:x]
      dz = candidate[:z] - psx[:z]
      dvx = candidate[:view_x] - psx[:view_x]
      dvy = candidate[:view_y] - psx[:view_y]
      dvz = candidate[:view_z] - psx[:view_z]
      speed = (candidate[:speed] - psx[:speed]).abs
      yaw = ((candidate[:body_yaw] - psx[:body_yaw] + 2048) % 4096 - 2048).abs
      pitch = if candidate.key?(:body_pitch) && psx.key?(:body_pitch)
                ((candidate[:body_pitch] - psx[:body_pitch] + 2048) % 4096 - 2048).abs
              else
                0
              end
      roll = if candidate.key?(:body_roll) && psx.key?(:body_roll)
               ((candidate[:body_roll] - psx[:body_roll] + 2048) % 4096 - 2048).abs
             else
               0
             end
      lateral = if candidate.key?(:track_lateral) && psx.key?(:track_lateral)
                  (candidate[:track_lateral] - psx[:track_lateral]).abs
                else
                  0
                end
      seed_penalty = if candidate.key?(:random_seed) && psx.key?(:random_seed) &&
                        candidate[:random_seed] != psx[:random_seed]
                       512
                     else
                       0
                     end
      phase_penalty = if candidate.key?(:anim_timer) && psx.key?(:anim_timer) &&
                         (candidate[:anim_timer] - psx[:anim_timer]) % 128 != 0
                        1024
                      else
                        0
                      end
      # The VBlank checkpoint can observe the renderer during its mirror pass,
      # whose yaw is the main camera plus exactly 180 degrees.  Modulo 2048
      # compares the underlying view without turning that phase difference
      # into a false state mismatch.
      view_yaw = ((candidate[:view_angle_y] - psx[:view_angle_y] + 1024) % 2048 - 1024).abs
      rival_distance = (0...4).sum do |index|
        x_key = :"rival#{index}_x"
        z_key = :"rival#{index}_z"
        next 0 unless candidate.key?(x_key) && psx.key?(x_key)
        Math.hypot(candidate[x_key] - psx[x_key],
                   candidate[z_key] - psx[z_key])
      end
      Math.hypot(dx, dz) + Math.sqrt(dvx * dvx + dvy * dvy + dvz * dvz) * 2 +
        speed * 4 + yaw / 4.0 + pitch / 2.0 + roll / 2.0 +
        lateral / 4.0 + view_yaw * 2 + rival_distance * 2 +
        seed_penalty + phase_penalty +
        (candidate[:timer] - psx[:timer]).abs
    end
    native = native_state
    if options[:visual_refine] > 0
      nearby = candidates.select do |candidate|
        (candidate[:timer] - native_state[:timer]).abs <= options[:visual_refine]
      end
      refined = nearby.min_by do |candidate|
        image_rmse(psx[:path], candidate[:path], options[:region])
      end
      native = refined unless refined.nil?
    end
    # The manifest describes the state sampled at VBlank, but its screenshot
    # can still show the preceding front buffer. Keep state validation tied to
    # the simulation match while visual refinement selects that displayed
    # image independently (including its preceding animation phase).
    dx = native_state[:x] - psx[:x]
    dz = native_state[:z] - psx[:z]
    dvx = native_state[:view_x] - psx[:view_x]
    dvy = native_state[:view_y] - psx[:view_y]
    dvz = native_state[:view_z] - psx[:view_z]
    position_distance = Math.hypot(dx, dz)
    view_distance = Math.sqrt(dvx * dvx + dvy * dvy + dvz * dvz)
    speed_delta = native_state[:speed] - psx[:speed]
    angle_delta = %i[body_yaw body_pitch body_roll].map do |key|
      next unless native_state.key?(key) && psx.key?(key)
      ((native_state[key] - psx[key] + 2048) % 4096 - 2048).abs
    end.compact.max || 0
    lateral_delta = if native_state.key?(:track_lateral) && psx.key?(:track_lateral)
                      (native_state[:track_lateral] - psx[:track_lateral]).abs
                    else
                      0
                    end
    rival_distance = (0...4).sum do |index|
      x_key = :"rival#{index}_x"
      z_key = :"rival#{index}_z"
      next 0 unless native_state.key?(x_key) && psx.key?(x_key)
      Math.hypot(native_state[x_key] - psx[x_key],
                 native_state[z_key] - psx[z_key])
    end
    next if position_distance > options[:max_position_distance] ||
            view_distance > options[:max_view_distance] ||
            speed_delta.abs > options[:max_speed_delta] ||
            angle_delta > options[:max_angle_delta] ||
            lateral_delta > options[:max_lateral_delta] ||
            rival_distance > options[:max_rival_distance]
    next if native_state.key?(:anim_timer) && psx.key?(:anim_timer) &&
            (native_state[:anim_timer] - psx[:anim_timer]) % 128 != 0
    {
      psx: psx[:path], native: native[:path],
      label: "#{File.basename(psx[:filename], '.ppm')}__#{File.basename(native[:filename], '.ppm')}",
      state_delta: {
        x: dx, z: dz, distance: position_distance,
        view_distance: view_distance,
        speed: speed_delta,
        angle: angle_delta,
        timer: native_state[:timer] - psx[:timer],
        display_timer: native[:timer] - psx[:timer],
        body_pitch: native_state.key?(:body_pitch) && psx.key?(:body_pitch) ?
          native_state[:body_pitch] - psx[:body_pitch] : nil,
        body_roll: native_state.key?(:body_roll) && psx.key?(:body_roll) ?
          native_state[:body_roll] - psx[:body_roll] : nil,
        track_lateral: native_state.key?(:track_lateral) && psx.key?(:track_lateral) ?
          native_state[:track_lateral] - psx[:track_lateral] : nil,
        rival_distance: rival_distance,
        random_seed_equal: native_state.key?(:random_seed) && psx.key?(:random_seed) ?
          native_state[:random_seed] == psx[:random_seed] : nil,
        anim_timer: native_state.key?(:anim_timer) && psx.key?(:anim_timer) ?
          native_state[:anim_timer] - psx[:anim_timer] : nil
      }
    }
  end.compact
  abort "no state-aligned frame pairs within the position limit" if pairs.empty?
else
  psx_frames = psx_dir.glob("timer-*-s*.ppm").to_h { |path| [path.basename.to_s, path] }
  native_frames = native_dir.glob("timer-*-s*.ppm").to_h { |path| [path.basename.to_s, path] }
  names = (psx_frames.keys & native_frames.keys).sort
  abort "no matching timer-*-s*.ppm captures" if names.empty?
  pairs = names.map do |name|
    { psx: psx_frames.fetch(name), native: native_frames.fetch(name),
      label: name.delete_suffix(".ppm"), state_delta: nil }
  end
end

compare = Pathname(__dir__) / "rage_visual_compare.rb"
work = Queue.new
pairs.each_with_index { |pair, index| work << [index, pair] }
rows = Array.new(pairs.length)
errors = Queue.new
[options[:jobs], pairs.length].min.times.map do
  Thread.new do
    loop do
      index, pair = work.pop(true)
  frame_output = output / pair[:label]
  command = [RbConfig.ruby, compare.to_s,
             "--psx", pair[:psx].to_s,
             "--native", pair[:native].to_s,
             "--output", frame_output.to_s,
             "--hotspots", options[:hotspots].to_s,
             "--hotspot-radius", options[:radius].to_s,
             "--artifact-radius", options[:artifact_radius].to_s]
  command.concat(["--region", options[:region]]) if options[:region]
  command.concat(["--clear-region", options[:clear_region]]) if options[:clear_region]
  command.concat(["--black-region", options[:black_region]]) if options[:black_region]
  command.concat(["--needle-region", options[:needle_region]]) if options[:needle_region]
  stdout, stderr, status = Open3.capture3(*command)
      unless status.success?
        errors << "comparison failed for #{pair[:label]}:\n#{stdout}#{stderr}"
        next
      end
  report = JSON.parse(File.read(frame_output / "report.json"))
      rows[index] = {
    frame: pair[:label],
    psx_frame: pair[:psx].basename.to_s,
    native_frame: pair[:native].basename.to_s,
    state_delta: pair[:state_delta],
    normalized_rmse: report.fetch("normalized_rmse"),
    normalized_region_rmse: report["normalized_region_rmse"],
    native_only_clear: report.fetch("native_only_clear").fetch("count"),
    native_only_black: report.fetch("native_only_black").fetch("count"),
    tachometer_needle: report.fetch("tachometer_needle"),
    worst_hotspot: report.fetch("hotspots").first,
    bundle: frame_output.to_s
      }
    rescue ThreadError
      break
    rescue StandardError => error
      errors << "comparison worker failed: #{error.full_message}"
    end
  end
end.each(&:join)
abort errors.pop unless errors.empty?

summary = {
  psx_directory: psx_dir.to_s,
  native_directory: native_dir.to_s,
  match: options[:match],
  rank: options[:rank],
  matched_frames: rows.length,
  frames: rows
}
File.write(output / "summary.json", JSON.pretty_generate(summary) + "\n")
rank_rmse = lambda do |row|
  row[:normalized_region_rmse] || row[:normalized_rmse]
end
ranked = if options[:rank] == "clear"
           rows.sort_by do |row|
             [-row[:native_only_clear], -rank_rmse.call(row).to_f]
           end
         elsif options[:rank] == "black"
           rows.sort_by do |row|
             [-row[:native_only_black], -rank_rmse.call(row).to_f]
           end
         elsif options[:rank] == "needle"
           rows.sort_by do |row|
             [-row[:tachometer_needle].fetch("mismatch_pixels"),
              -rank_rmse.call(row).to_f]
           end
         else
           rows.sort_by { |row| -rank_rmse.call(row).to_f }
         end
(options[:top] ? ranked.first(options[:top]) : ranked).each do |row|
  hotspot = row[:worst_hotspot]
  location = hotspot ? " hotspot=#{hotspot['x']},#{hotspot['y']}" : ""
  delta = row[:state_delta]
  alignment = delta ? format(" distance=%.1f view_distance=%.1f " \
                             "rival_distance=%.1f speed_delta=%d " \
                             "timer_delta=%d display_timer_delta=%d",
                             delta[:distance], delta[:view_distance],
                             delta[:rival_distance],
                             delta[:speed], delta[:timer],
                             delta[:display_timer]) : ""
  clear = options[:clear_region] ?
    " native_only_clear=#{row[:native_only_clear]}" : ""
  black = options[:black_region] ?
    " native_only_black=#{row[:native_only_black]}" : ""
  needle = options[:needle_region] ?
    format(" needle_mismatch=%d needle_iou=%s",
           row[:tachometer_needle].fetch("mismatch_pixels"),
           row[:tachometer_needle]["iou"]&.then { |value| "%.3f" % value } || "n/a") : ""
  metric = rank_rmse.call(row)
  metric_name = row[:normalized_region_rmse] ? "region_RMSE" : "RMSE"
  puts format("%s %s=%.6f%s%s%s%s%s", row[:frame], metric_name, metric,
              location, clear, black, needle, alignment)
end
